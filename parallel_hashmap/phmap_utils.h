#if !defined(phmap_utils_h_guard_)
#define phmap_utils_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
//
//       minimal header providing phmap::hash_combine
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

#if defined(__APPLE__)
    // forward declaration of std::hash does not work on mac. It is not really supposed 
    // to work, I know, but it is nice to reduce the amount of headers included.
    #include <functional>
#else
    #include <cstddef>  // for size_t

    namespace std
    {
        template<class Key> struct hash;
    } 
#endif

namespace phmap
{

template <class H, int sz> struct Combiner
{
    H operator()(H seed, size_t value);
};

template <class H> struct Combiner<H, 4>
{
    H operator()(H seed, size_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
};

template <class H> struct Combiner<H, 8>
{
    H operator()(H seed, size_t value)
    {
        return seed ^ (value + size_t(0xc6a4a7935bd1e995) + (seed << 6) + (seed >> 2));
    }
};


// -----------------------------------------------------------------------------
template <typename H>
class HashStateBase {
public:
    template <typename T, typename... Ts>
    static H combine(H state, const T& value, const Ts&... values);

    static H combine(H state) { return state; }
};

template <typename H>
template <typename T, typename... Ts>
H HashStateBase<H>::combine(H seed, const T& v, const Ts&... vs)
{
    return HashStateBase<H>::combine(Combiner<H, sizeof(H)>()(
                                         seed, std::hash<T>()(v)),  vs...);
}

using HashState = HashStateBase<size_t>;

}  // namespace phmap


#endif // phmap_utils_h_guard_
