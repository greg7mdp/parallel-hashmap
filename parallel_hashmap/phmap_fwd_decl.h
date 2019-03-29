#if !defined(phmap_fwd_decl_h_guard_)
#define phmap_fwd_decl_h_guard_

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

namespace std 
{
    template<class> class allocator;
    template<class T1, class T2> struct pair;
}

namespace phmap {

    template <class T> struct Hash;
    template <class T> struct EqualTo;

    class NullMutex;

    namespace container_internal {

        // The hash of an object of type T is computed by using phmap::Hash.
        template <class T, class E = void>
        struct HashEq 
        {
            using Hash = phmap::Hash<T>;
            using Eq   = phmap::EqualTo<T>;
        };


        template <class T>
        using hash_default_hash = typename container_internal::HashEq<T>::Hash;

        template <class T>
        using hash_default_eq = typename container_internal::HashEq<T>::Eq;

    }  // namespace container_internal

    template <class T, 
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = std::allocator<T>>
        class flat_hash_set;

    template <class K, class V,
              class Hash  = phmap::container_internal::hash_default_hash<K>,
              class Eq    = phmap::container_internal::hash_default_eq<K>,
              class Alloc = std::allocator<std::pair<const K, V>>>
        class flat_hash_map;
    
    template <class T, 
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = std::allocator<T>>
        class node_hash_set;

    template <class Key, class Value,
              class Hash  = phmap::container_internal::hash_default_hash<Key>,
              class Eq    = phmap::container_internal::hash_default_eq<Key>,
              class Alloc = std::allocator<std::pair<const Key, Value>>>
        class node_hash_map;

    template <class T,
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = std::allocator<T>,
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_flat_hash_set;

    template <class K, class V,
              class Hash  = phmap::container_internal::hash_default_hash<K>,
              class Eq    = phmap::container_internal::hash_default_eq<K>,
              class Alloc = std::allocator<std::pair<const K, V>>,
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_flat_hash_map;

    template <class T, 
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = std::allocator<T>,
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_node_hash_set;

    template <class Key, class Value,
              class Hash  = phmap::container_internal::hash_default_hash<Key>,
              class Eq    = phmap::container_internal::hash_default_eq<Key>,
              class Alloc = std::allocator<std::pair<const Key, Value>>,
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_node_hash_map;



}  // namespace phmap


#endif // phmap_fwd_decl_h_guard_
