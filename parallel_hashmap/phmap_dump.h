#if !defined(phmap_dump_h_guard_)
#define phmap_dump_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
//
//       providing dump/load/mmap_load
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include "phmap.h"
namespace phmap
{

namespace type_traits_internal {

#if defined(__GLIBCXX__) && __GLIBCXX__ < 20150801
    template<typename T> struct IsTriviallyCopyable : public std::integral_constant<bool, __has_trivial_copy(T)> {};
#else
    template<typename T> struct IsTriviallyCopyable : public std::is_trivially_copyable<T> {};
#endif

template <class T1, class T2>
struct IsTriviallyCopyable<std::pair<T1, T2>> {
    static constexpr bool value = IsTriviallyCopyable<T1>::value && IsTriviallyCopyable<T2>::value;
};
}

namespace container_internal {

//// raw_hash_set
template <class Policy, class Hash, class Eq, class Alloc>
template<typename OutputArchive>
bool raw_hash_set<Policy, Hash, Eq, Alloc>::dump(OutputArchive& ar) {
    static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                    "value_type should be dumpable");

    typename OutputArchive::Guard guard(&ar);
    if (!ar.dump(size_)) {
        std::cerr << "Failed to dump size_" << std::endl;
        return false;
    }
    if (size_ == 0) {
        return true;
    }
    if (!ar.dump(capacity_)) {
        std::cerr << "Failed to dump capacity_" << std::endl;
        return false;
    }
    if (!ar.dump(reinterpret_cast<char*>(ctrl_),
        sizeof(ctrl_t) * (capacity_ + Group::kWidth + 1))) {

        std::cerr << "Failed to dump ctrl_" << std::endl;
        return false;
    }
    if (!ar.dump(reinterpret_cast<char*>(slots_),
                    sizeof(slot_type) * capacity_)) {
        std::cerr << "Failed to dump slot_" << std::endl;
        return false;
    }
    return true;
}

template <class Policy, class Hash, class Eq, class Alloc>
template<typename InputArchive>
bool raw_hash_set<Policy, Hash, Eq, Alloc>::load(InputArchive& ar) {
    static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                    "value_type should be dumpable");
    
    typename InputArchive::Guard guard(&ar);
    if (!ar.load(&size_)) {
        std::cerr << "Failed to load size_" << std::endl;
        return false;
    }
    if (size_ == 0) {
        return true;
    }
    if (!ar.load(&capacity_)) {
        std::cerr << "Failed to load capacity_" << std::endl;
        return false;
    }

    // allocate memory for ctrl_ and slots_
    initialize_slots();
    if (!ar.load(reinterpret_cast<char*>(ctrl_),
        sizeof(ctrl_t) * (capacity_ + Group::kWidth + 1))) {
        std::cerr << "Failed to load ctrl" << std::endl;
        return false;
    }
    if (!ar.load(reinterpret_cast<char*>(slots_),
                    sizeof(slot_type) * capacity_)) {
        std::cerr << "Failed to load slot" << std::endl;
        return false;
    }
    return true;
}

////// parallel_hash_set
template <size_t N,
          template <class, class, class, class> class RefSet,
          class Mtx_,
          class Policy, class Hash, class Eq, class Alloc>
template<typename OutputArchive>
bool parallel_hash_set<N, RefSet, Mtx_, Policy, Hash, Eq, Alloc>::dump(OutputArchive& ar) {
    static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                    "value_type should be dumpable");

    typename OutputArchive::Guard guard(&ar);
    if (! ar.dump(subcnt())) {
        std::cerr << "Failed to dump meta!" << std::endl;
        return false;
    }
    for (size_t i = 0; i < sets_.size(); ++i) {
        auto& inner = sets_[i];
        typename Lockable::UniqueLock m(const_cast<Inner&>(inner));
        if (!inner.set_.dump(ar)) {
            std::cerr << "Failed to dump submap " << i << std::endl;
            return false;
        }
    }
    return true;
}

template <size_t N,
          template <class, class, class, class> class RefSet,
          class Mtx_,
          class Policy, class Hash, class Eq, class Alloc>
template<typename InputArchive>
bool parallel_hash_set<N, RefSet, Mtx_, Policy, Hash, Eq, Alloc>::load(InputArchive& ar) {
    static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                    "value_type should be dumpable");

    typename InputArchive::Guard guard(&ar);
    size_t submap_count = 0;
    if (!ar.load(&submap_count)) {
        std::cerr << "Failed to load submap count!" << std::endl;
        return false;
    }

    if (submap_count != subcnt()) {
        std::cerr << "submap count(" << submap_count << ") != N(" << N << ")" << std::endl;
        return false;
    }

    for (size_t i = 0; i < submap_count; ++i) {            
        auto& inner = sets_[i];
        typename Lockable::UniqueLock m(const_cast<Inner&>(inner));
        if (!inner.set_.load(ar)) {
            std::cerr << "Failed to load submap " << i << std::endl;
            return false;
        }
    }
    return true;
}
} // namesapce container_internal



// ArchiveOutput & ArchiveInput

#define CHECK_FILE(f) {                                 \
    if (!f.is_open()) {                                 \
        std::cerr << "File is not open!" << std::endl;  \
        return false;                                   \
    }                                                   \
}

template<typename Archive>
class ArchiveGuard {
public:
    ArchiveGuard(Archive* ar): ar_(ar) {
        if (ar_->guard_ == NULL) {
            ar_->guard_ = this;
        }
    };
    ~ArchiveGuard() {
        if (ar_ && ar_->guard_ == this) {
            ar_->finish();
        }
    }
private:
    Archive* ar_;
};

class BinaryOutputArchive {
public:
    using Guard = ArchiveGuard<BinaryOutputArchive>;

    BinaryOutputArchive(const std::string& file_path): offset_(0), guard_(NULL) {
        ofs_.open(file_path.c_str(), std::ios_base::binary);
    }

    virtual ~BinaryOutputArchive() {
        finish();
    }

    bool dump(char* p, size_t sz) {
        CHECK_FILE(ofs_);
        ofs_.write(p, sz);
        offset_ += sz;
        return true;
    }

    template<typename V>
    typename std::enable_if<type_traits_internal::IsTriviallyCopyable<V>::value, bool>::type
    dump(const V& v) {
        CHECK_FILE(ofs_);        
        ofs_.write(reinterpret_cast<char*>(const_cast<V*>(&v)), sizeof(V));
        offset_ += sizeof(V);
        return true;
    }

    void finish() {
        if (ofs_.is_open()) {
            ofs_.close();
            offset_ = 0;
        }
    }

private:
    friend class ArchiveGuard<BinaryOutputArchive>;
    std::ofstream ofs_;
    size_t offset_;
    Guard* guard_;
};


class BinaryInputArchive {
public:
    using Guard = ArchiveGuard<BinaryInputArchive>;

    BinaryInputArchive(const std::string& file_path): offset_(0), guard_(NULL) {
        ifs_.open(file_path.c_str(), std::ios_base::binary);
    }

    virtual ~BinaryInputArchive() {
        finish();
    }

    bool load(char* p, size_t sz) {
        CHECK_FILE(ifs_);
        ifs_.read(p, sz);
        offset_ += sz;
        return true;
    }

    template<typename V>
    typename std::enable_if<type_traits_internal::IsTriviallyCopyable<V>::value, bool>::type
    load(V* v) {
        CHECK_FILE(ifs_);
        ifs_.read(reinterpret_cast<char*>(v), sizeof(V));
        offset_ += sizeof(V);
        return true;
    }

    void finish() {
        if (ifs_.is_open()) {
            ifs_.close();
            offset_ = 0;
        }
    }
   
private:
    friend class ArchiveGuard<BinaryInputArchive>;
    std::ifstream ifs_;
    size_t offset_;
    Guard* guard_;
};

} // namespace phmap

#endif // phmap_dump_h_guard_
