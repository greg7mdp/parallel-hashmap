# The Parallel Hashmap  [![Build Status](https://travis-ci.org/greg7mdp/parallel-hashmap.svg?branch=master)](https://travis-ci.org/greg7mdp/parallel-hashmap)  [![Build Status](https://ci.appveyor.com/api/projects/status/86kc657lp4cja8ju?svg=true)](https://ci.appveyor.com/project/greg7mdp/parallel-hashmap)

## Overview

This repository aims to provide an set of excellent hash map implementations, with the following characteristics:

- **Header only**: nothing to build, just copy the `parallel_hashmap` directory to your project and you are good to go.

- **drop-in replacement** for std::unordered_map and std::unordered_set

- Compiler with **C++11 support** required, **C++14 and C++17 APIs are provided (such as `try_emplace`)**

- **Very efficient**, significantly faster than your compiler's unordered map/set or Boost's, or than [sparsepp](https://github.com/greg7mdp/sparsepp)

- **Memory friendly**: low memory usage, although a little higher than [sparsepp](https://github.com/greg7mdp/sparsepp)

- Supports **heterogeneous lookup**

- Easy to **forward declare**: just include `phmap_fwd_decl.h` in your header files to forward declare Parallel Hashmap containers. This header is only 111 lines including comments.

- **Tested** on Windows (vs2015 & vs2017), linux (g++ 5, 6, 7, 8, clang++ 3.9, 4.0, 5.0) and MacOS (g++ and clang++) - click on travis and appveyor icons above for detailed test status.


## Fast *and*  memory friendly

Click here [For a full writeup explaining the design and benefits of the Parallel Hashmap](https://greg7mdp.github.io/parallel-hashmap/).

The hashmaps provided here are built upon those open sourced by Google in the Abseil library. They use closed hashing, where values are stored directly into a memory array, avoiding memory indirections. By using parallel SSE2 instructions, these hashmaps are able to look up items by checking 16 slots in parallel,  allowing the implementation to remain fast even when the table is filled up to 87.5% capacity.

> **IMPORTANT:** This repository borrows code from the [abseil-cpp](https://github.com/abseil/abseil-cpp) repository, with modifications, and may behave differently from the original. This repository is an independent work, with no guarantees implied or provided by the authors. Please visit [abseil-cpp](https://github.com/abseil/abseil-cpp) for the official Abseil libraries.

## Installation

Copy the parallel_hashmap directory to your project. Update your include path. That's all.

> A cmake configuration files (CMakeLists.txt) is provided for building the tests and examples. Command for building and running the tests is: `mkdir build && cd build && cmake -DPHMAP_BUILD_TESTS=ON .. && cmake --build . && make test`

## Example

```
#include <iostream>
#include <string>
#include <parallel_hashmap/phmap.h>

using phmap::flat_hash_map;
 
int main()
{
    // Create an unordered_map of three strings (that map to strings)
    flat_hash_map<std::string, std::string> email = 
    {
        { "tom",  "tom@gmail.com"},
        { "jeff", "jk@gmail.com"},
        { "jim",  "jimg@microsoft.com"}
    };
 
    // Iterate and print keys and values 
    for (const auto& n : email) 
        std::cout << n.first << "'s email is: " << n.second << "\n";
 
    // Add a new entry
    email["bill"] = "bg@whatever.com";
 
    // and print it
    std::cout << "bill's email is: " << email["bill"] << "\n";
 
    return 0;
}
```

## Various hash maps and their pros and cons

The header `parallel_hashmap/phmap.h` provides the implementation for the following eight hash tables:
- phmap::flat_hash_set
- phmap::flat_hash_map
- phmap::node_hash_set
- phmap::node_hash_map
- phmap::parallel_flat_hash_set
- phmap::parallel_flat_hash_map
- phmap::parallel_node_hash_set
- phmap::parallel_node_hash_map

The full types with template parameters can be found in the [parallel_hashmap/phmap_fwd_decl.h](https://raw.githubusercontent.com/greg7mdp/parallel-hashmap/master/parallel_hashmap/phmap_fwd_decl.h) header, which is useful for forward declaring the Parallel Hashmaps when necessary.

**Key decision points:**

- The `flat` hash maps may move the keys and values in memory. So if you keep a pointer to something inside a `flat` hash map, this pointer may become invalid when the map is mutated. The `node` hash maps don't, and should be used instead if this is a problem.

- The `flat` hash maps will use less memory, and usually be faster than the `node` hash maps, so use them if you can. A possible exception is when the values inserted in the hash map are large (say more than 100 bytes [*needs testing*]).

- The `parallel` hash maps are preferred when you have a few hash maps that will store a very large number of values. The `non-parallel` hash maps are preferred if you have a large number of hash maps, each storing a relatively small number of values.

- The benefits of the `parallel` hash maps are:  
   a. reduced peak memory usage (when resizing), and  
   b. multithreading support (and inherent internal parallelism)

## Changes to Abseil's hashmaps

- The default hash framework is std::hash, not absl::Hash
- The `erase(iterator)` and `erase(const_iterator)` both return an iterator to the element following the removed element, as the do the std::unordered_map. A non-standard `void _erase(iterator)` is provided in case the return value is not needed.
- No new types, such as `absl::string_view`, are provided. `std::string_view` is supported by phmap tables when built with a stdlib providing this type.


## Memory usage

|  type                 |    memory usage   | additional *peak* memory usage when resizing  |
|-----------------------|-------------------|-----------------------------------------------|
| flat tables           | ![flat_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/flat_mem_usage.gif?raw=true) | ![flat_peak_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/flat_peak.gif?raw=true) | 
| node tables           | ![node_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/node_mem_usage.gif?raw=true) | ![node_peak_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/node_peak.gif?raw=true) | 
| parallel flat tables  | ![flat_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/flat_mem_usage.gif?raw=true) | ![parallel_flat_peak](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/parallel_flat_peak.gif?raw=true) | 
| parallel node tables  | ![node_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/node_mem_usage.gif?raw=true) | ![parallel_node_peak](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/parallel_node_peak.gif?raw=true) | 


- *size()* is the number of values in the container, as returned by the size() method
- *load_factor()* is the ratio: `size() / bucket_count()`. It varies between 0.4375 (just after the resize) to 0.875 (just before the resize). The size of the bucket array doubles at each resize.
- the value 9 comes from `sizeof(void *) + 1`, as the *node* hash maps store one pointer plus one byte of metadata for each entry in the bucket array.
- flat tables store the values, plus one byte of metadata per value), directly into the bucket array, hence the `sizeof(C::value_type) + 1`.
- the additional peak memory usage (when resizing) corresponds the the old bucket array (half the size of the new one, hence the 0.5), which contains the values to be copied to the new bucket array, and which is freed when the values have been copied.
- the *parallel* hashmaps, when created with a template parameter N=4, create 16 submaps. When the hash values are well distributed, and in single threaded mode, only one of these 16 submaps resizes at any given time, hence the factor `0.03` roughly equal to `0.5 / 16`

## Iterator invalidation

The rules are the same as for std::unordered_map, and are valid for all the phmap containers:


|    Operations	                            | Invalidated                |
|-------------------------------------------|----------------------------|
| All read only operations, swap, std::swap | Never                      |
| clear, rehash, reserve, operator=         | Always                     |
| insert, emplace, emplace_hint, operator[] | Only if rehash triggered   |
| erase                                     | Only to the element erased |

## Integer keys, and other hash function considerations.

1. For basic integer types, phmap provides a default hash function which does some mixing of the bits of the keys (see [Integer Hashing](http://burtleburtle.net/bob/hash/integer.html)). This prevents a pathological case where inserted keys are sequential (1, 2, 3, 4, ...), and the lookup on non-present keys becomes very slow. 

   Of course, the user of phmap may provide its own hash function,  as shown below:
   
   ```c++
   #include <parallel_hashmap/phmap.h>
   
   struct Hash64 {
       size_t operator()(uint64_t k) const { return (k ^ 14695981039346656037ULL) * 1099511628211ULL; }
   };
   
   struct Hash32 {
       size_t operator()(uint32_t k) const { return (k ^ 2166136261U)  * 16777619UL; }
   };
   
   int main() 
   {
       phmap::flat_hash_map<uint64_t, double, Hash64> map;
       ...
   }
   
   ```

2. When the user provides its own hash function, for example when inserting custom classes into a hash map, sometimes the resulting hash keys have similar low order bits and cause many collisions, decreasing the efficiency of the hash map. To address this use case, phmap provides an optional 'mixing' of the hash key (see [Integer Hash Function](https://gist.github.com/badboy/6267743) which can be enabled by defining the proprocessor macro: PHMAP_MIX_HASH. 

## Example 2 - providing a hash function for a user-defined class

In order to use a flat_hash_set or flat_hash_map, a hash function should be provided. Even though a the hash function can be provided via the HashFcn template parameter, we recommend injecting a specialization of `std::hash` for the class into the "std" namespace. For example:

```c++
#include <iostream>
#include <functional>
#include <string>
#include <parallel_hashmap/phmap.h>

using std::string;

struct Person 
{
    bool operator==(const Person &o) const 
    { return _first == o._first && _last == o._last; }

    string _first;
    string _last;
};

namespace std
{
    // inject specialization of std::hash for Person into namespace std
    // ----------------------------------------------------------------
    template<> 
    struct hash<Person>
    {
        std::size_t operator()(Person const &p) const
        {
            std::size_t seed = 0;
            phmap::hash_combine(seed, p._first);
            phmap::hash_combine(seed, p._last);
            return seed;
        }
    };
}
 
int main()
{
    // As we have defined a specialization of std::hash() for Person, 
    // we can now create flat_hash_set or flat_hash_map of Persons
    // ----------------------------------------------------------------
    phmap::flat_hash_set<Person> persons = { { "John", "Galt" }, 
                                             { "Jane", "Doe" } };
    for (auto& p: persons)
        std::cout << p._first << ' ' << p._last << '\n';
}
```

The `std::hash` specialization for `Person` combines the hash values for both first and last name using the convenient phmap::hash_combine function, and returns the combined hash value. 

phmap::hash_combine is provided by the header `parallel_hashmap/phmap.h`. However, class definitions often appear in header files, and it is desirable to limit the size of headers included in such header files, so we provide the very small header `parallel_hashmap/phmap_utils.h` for that purpose:

```c++
#include <string>
#include <parallel_hashmap/phmap_utils.h>

using std::string;
 
struct Person 
{
    bool operator==(const Person &o) const 
    { 
        return _first == o._first && _last == o._last && _age == o._age; 
    }

    string _first;
    string _last;
    int    _age;
};

namespace std
{
    // inject specialization of std::hash for Person into namespace std
    // ----------------------------------------------------------------
    template<> 
    struct hash<Person>
    {
        std::size_t operator()(Person const &p) const
        {
            std::size_t seed = 0;
            phmap::hash_combine(seed, p._first);
            phmap::hash_combine(seed, p._last);
            phmap::hash_combine(seed, p._age);
            return seed;
        }
    };
}
```


## Thread safety

Parallel Hashmap containers follow the thread safety rules of the Standard C++ library. In Particular:

- A single phmap hash table is thread safe for reading from multiple threads. For example, given a hash table A, it is safe to read A from thread 1 and from thread 2 simultaneously.

- If a single hash table is being written to by one thread, then all reads and writes to that hash table on the same or other threads must be protected. For example, given a hash table A, if thread 1 is writing to A, then thread 2 must be prevented from reading from or writing to A. 

- It is safe to read and write to one instance of a type even if another thread is reading or writing to a different instance of the same type. For example, given hash tables A and B of the same type, it is safe if A is being written in thread 1 and B is being read in thread 2.

- The *parallel* tables can be made fully thread-safe, by providing a synchronization type (for example [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex)) as the last template argument. Because locking is performed at the *submap* level, a high level of concurrency can still be achieved.
