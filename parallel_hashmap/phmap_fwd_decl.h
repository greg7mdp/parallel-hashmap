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
// ---------------------------------------------------------------------------

#include <memory>
#include <utility>

namespace phmap {

    template <class T> struct Hash;
    template <class T> struct EqualTo;
    template <class T> using Allocator      = typename std::allocator<T>;
    template<class T1, class T2> using Pair = typename std::pair<T1, T2>;

    class NullMutex;

    namespace container_internal {

        // The hash of an object of type T is computed by using phmap::Hash.
        template <class T, class E = void>
        struct HashEq 
        {
#if defined(PHMAP_USE_ABSL_HASHEQ)
            using Hash = absl::Hash<T>;
            using Eq   = phmap::EqualTo<T>;
#else
            using Hash = phmap::Hash<T>;
            using Eq   = phmap::EqualTo<T>;
#endif
        };

        template <class T>
        using hash_default_hash = typename container_internal::HashEq<T>::Hash;

        template <class T>
        using hash_default_eq = typename container_internal::HashEq<T>::Eq;

        // type alias for std::allocator so we can forward declare without including other headers
        template <class T>  
        using Allocator = typename phmap::Allocator<T>;

        // type alias for std::pair so we can forward declare without including other headers
        template<class T1, class T2> 
        using Pair = typename phmap::Pair<T1, T2>;

    }  // namespace container_internal

    template <class T, 
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = phmap::container_internal::Allocator<T>>  // alias for std::allocator
        class flat_hash_set;

    template <class K, class V,
              class Hash  = phmap::container_internal::hash_default_hash<K>,
              class Eq    = phmap::container_internal::hash_default_eq<K>,
              class Alloc = phmap::container_internal::Allocator<
                            phmap::container_internal::Pair<const K, V>>> // alias for std::allocator
        class flat_hash_map;
    
    template <class T, 
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = phmap::container_internal::Allocator<T>> // alias for std::allocator
        class node_hash_set;

    template <class Key, class Value,
              class Hash  = phmap::container_internal::hash_default_hash<Key>,
              class Eq    = phmap::container_internal::hash_default_eq<Key>,
              class Alloc = phmap::container_internal::Allocator<
                            phmap::container_internal::Pair<const Key, Value>>> // alias for std::allocator
        class node_hash_map;

    template <class T,
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = phmap::container_internal::Allocator<T>, // alias for std::allocator
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_flat_hash_set;

    template <class K, class V,
              class Hash  = phmap::container_internal::hash_default_hash<K>,
              class Eq    = phmap::container_internal::hash_default_eq<K>,
              class Alloc = phmap::container_internal::Allocator<
                            phmap::container_internal::Pair<const K, V>>, // alias for std::allocator
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_flat_hash_map;

    template <class T, 
              class Hash  = phmap::container_internal::hash_default_hash<T>,
              class Eq    = phmap::container_internal::hash_default_eq<T>,
              class Alloc = phmap::container_internal::Allocator<T>, // alias for std::allocator
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_node_hash_set;

    template <class Key, class Value,
              class Hash  = phmap::container_internal::hash_default_hash<Key>,
              class Eq    = phmap::container_internal::hash_default_eq<Key>,
              class Alloc = phmap::container_internal::Allocator<
                            phmap::container_internal::Pair<const Key, Value>>, // alias for std::allocator
              size_t N    = 4,                  // 2**N submaps
              class Mutex = phmap::NullMutex>   // use std::mutex to enable internal locks
        class parallel_node_hash_map;



}  // namespace phmap


#endif // phmap_fwd_decl_h_guard_
