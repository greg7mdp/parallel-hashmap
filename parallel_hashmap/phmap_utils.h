#if !defined(phmap_utils_h_guard_)
#define phmap_utils_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
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
//
// Includes work from abseil-cpp (https://github.com/abseil/abseil-cpp)
// with modifications.
// 
// Copyright 2018 The Abseil Authors.
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

//#include <cstddef>
#include <functional>
#define PHMAP_HASH_CLASS std::hash

namespace phmap
{

template <class T>  T phmap_min(T a, T b) { return a < b  ? a : b; }
template <class T>  T phmap_max(T a, T b) { return a >= b ? a : b; }

template <class T> 
using Allocator = typename std::allocator<T>;

template <class T>
struct EqualTo
{
    inline size_t operator()(const T& a, const T& b) const
    {
        return std::equal_to<T>()(a, b);
    }
};

template <class T>
struct Hash
{
    inline size_t operator()(const T& __v) const
    {
        return PHMAP_HASH_CLASS<T>()(__v);
    }
};

template <class T>
struct Hash<T *>
{
    static size_t phmap_log2 (size_t val) noexcept
    {
        size_t res = 0;
        while (val > 1)
        {
            val >>= 1;
            res++;
        }
        return res;
    }

    inline size_t operator()(const T *__v) const noexcept
    {
        static const size_t shift = 3; // phmap_log2(1 + sizeof(T)); // T might be incomplete!
        const uintptr_t i = (const uintptr_t)__v;
        return static_cast<size_t>(i >> shift);
    }
};

// from http://burtleburtle.net/bob/hash/integer.html
// fast and efficient for power of two table sizes where we always
// consider the last bits.
// ---------------------------------------------------------------
inline size_t phmap_mix_32(uint32_t a)
{
    a = a ^ (a >> 4);
    a = (a ^ 0xdeadbeef) + (a << 5);
    a = a ^ (a >> 11);
    return static_cast<size_t>(a);
}

// More thorough scrambling as described in
// https://gist.github.com/badboy/6267743
// ----------------------------------------
inline size_t phmap_mix_64(uint64_t a)
{
    a = (~a) + (a << 21); // a = (a << 21) - a - 1;
    a = a ^ (a >> 24);
    a = (a + (a << 3)) + (a << 8); // a * 265
    a = a ^ (a >> 14);
    a = (a + (a << 2)) + (a << 4); // a * 21
    a = a ^ (a >> 28);
    a = a + (a << 31);
    return static_cast<size_t>(a);
}

template<class ArgumentType, class ResultType>
struct phmap_unary_function
{
    typedef ArgumentType argument_type;
    typedef ResultType result_type;
};

template <>
struct Hash<bool> : public phmap_unary_function<bool, size_t>
{
    inline size_t operator()(bool __v) const noexcept
    { return static_cast<size_t>(__v); }
};

template <>
struct Hash<char> : public phmap_unary_function<char, size_t>
{
    inline size_t operator()(char __v) const noexcept
    { return static_cast<size_t>(__v); }
};

template <>
struct Hash<signed char> : public phmap_unary_function<signed char, size_t>
{
    inline size_t operator()(signed char __v) const noexcept
    { return static_cast<size_t>(__v); }
};

template <>
struct Hash<unsigned char> : public phmap_unary_function<unsigned char, size_t>
{
    inline size_t operator()(unsigned char __v) const noexcept
    { return static_cast<size_t>(__v); }
};

template <>
struct Hash<wchar_t> : public phmap_unary_function<wchar_t, size_t>
{
    inline size_t operator()(wchar_t __v) const noexcept
    { return static_cast<size_t>(__v); }
};

template <>
struct Hash<int16_t> : public phmap_unary_function<int16_t, size_t>
{
    inline size_t operator()(int16_t __v) const noexcept
    { return phmap_mix_32(static_cast<uint32_t>(__v)); }
};

template <>
struct Hash<uint16_t> : public phmap_unary_function<uint16_t, size_t>
{
    inline size_t operator()(uint16_t __v) const noexcept
    { return phmap_mix_32(static_cast<uint32_t>(__v)); }
};

template <>
struct Hash<int32_t> : public phmap_unary_function<int32_t, size_t>
{
    inline size_t operator()(int32_t __v) const noexcept
    { return phmap_mix_32(static_cast<uint32_t>(__v)); }
};

template <>
struct Hash<uint32_t> : public phmap_unary_function<uint32_t, size_t>
{
    inline size_t operator()(uint32_t __v) const noexcept
    { return phmap_mix_32(static_cast<uint32_t>(__v)); }
};

template <>
struct Hash<int64_t> : public phmap_unary_function<int64_t, size_t>
{
    inline size_t operator()(int64_t __v) const noexcept
    { return phmap_mix_64(static_cast<uint64_t>(__v)); }
};

template <>
struct Hash<uint64_t> : public phmap_unary_function<uint64_t, size_t>
{
    inline size_t operator()(uint64_t __v) const noexcept
    { return phmap_mix_64(static_cast<uint64_t>(__v)); }
};

template <>
struct Hash<float> : public phmap_unary_function<float, size_t>
{
    inline size_t operator()(float __v) const noexcept
    {
        // -0.0 and 0.0 should return same hash
        uint32_t *as_int = reinterpret_cast<uint32_t *>(&__v);
        return (__v == 0) ? static_cast<size_t>(0) : phmap_mix_32(*as_int);
    }
};

template <>
struct Hash<double> : public phmap_unary_function<double, size_t>
{
    inline size_t operator()(double __v) const noexcept
    {
        // -0.0 and 0.0 should return same hash
        uint64_t *as_int = reinterpret_cast<uint64_t *>(&__v);
        return (__v == 0) ? static_cast<size_t>(0) : phmap_mix_64(*as_int);
    }
};

template <class T, int sz> struct Combiner
{
    inline void operator()(T& seed, T value);
};

template <class T> struct Combiner<T, 4>
{
    inline void  operator()(T& seed, T value)
    {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

template <class T> struct Combiner<T, 8>
{
    inline void  operator()(T& seed, T value)
    {
        seed ^= value + T(0xc6a4a7935bd1e995) + (seed << 6) + (seed >> 2);
    }
};

template <class T>
inline void hash_combine(std::size_t& seed, T const& v)
{
    phmap::Hash<T> hasher;
    Combiner<std::size_t, sizeof(std::size_t)> combiner;

    combiner(seed, hasher(v));
}

}


#endif // phmap_utils_h_guard_
