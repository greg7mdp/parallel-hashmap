# The Parallel Hashmap  [![Build Status](https://travis-ci.org/greg7mdp/parallel-hashmap.svg?branch=master)](https://travis-ci.org/greg7mdp/parallel-hashmap)  [![Build Status](https://ci.appveyor.com/api/projects/status/86kc657lp4cja8ju?svg=true)](https://ci.appveyor.com/project/greg7mdp/parallel-hashmap)

## Overview

This repository aims to provide an set of excellent hash map implementations, with the following characteristics:

- **Header only**: nothing to build, just copy the `parallel_hashmap` directory to your project and you are good to go.

- Compiler with **C++11 support** required, **C++14 and C++17 APIs are provided**

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

**Key points:**

- The `flat` hash maps may move the keys and values in memory. So if you keep a pointer to something inside a `flat` hash map, this pointer may become invalid when the map is mutated. The `node` hash maps don't, and should be used instead if this is a problem.

- The `flat` hash maps will use less memory, and usually be faster than the `node` hash maps, so use them if you can. A possible exception is when the values inserted in the hash map are large (say more than 100 bytes [*needs testing*]).

- The `parallel` hash maps are preferred when you have a few hash maps that will store a very large number of values. The `non-parallel` hash maps are preferred if you have a large number of hash maps, each storing a relatively small number of values.

- The benefits of the `parallel` hash maps are:  
   a. reduced peak memory usage (when resizing), and  
   b. multithreading support (and inherent internal parallelism)


## Memory usage

|  type                 |    memory usage   | additional *peak* memory usage when resizing  |
|-----------------------|-------------------|-----------------------------------------------|
| flat tables           | ![flat_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/flat_mem_usage.gif?raw=true) | ![flat_peak_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/flat_peak.gif?raw=true) | 
| node tables           | ![node_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/node_mem_usage.gif?raw=true) | ![node_peak_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/node_peak.gif?raw=true) | 
| parallel flat tables  | ![flat_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/flat_mem_usage.gif?raw=true) | ![parallel_flat_peak](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/parallel_flat_peak.gif?raw=true) | 
| parallel node tables  | ![node_mem_usage](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/node_mem_usage.gif?raw=true) | ![parallel_node_peak](https://github.com/greg7mdp/parallel-hashmap/blob/master/html/img/parallel_node_peak.gif?raw=true) | 


- *size()* is the number of values in the container, as returned by the size() method
- *load_factor()* is the ratio: `size() / bucket_count()`. It varies between 0.4375 (just after the resize) to 0.875 (just before the resize). 
- the value 9 comes from `sizeof(void *) + 1`, as the *node* hash maps store one pointer plus one byte of metadata for each entry in the bucket array.
- the additional peak memory usage (when resizing) corresponds the the old bucket array (half the size of the new one, hence the 0.5), which contains the values to be copied to the new bucket array, and which is freed when the values have been copied.
- the *parallel* hashmaps, when created with a template parameter N=4, create 16 submaps. When the hash values are well distributed, and in single threaded mode, only one of these 16 submaps resizes at any given time, hence the factor `0.03` roughly equal to `0.5 / 16`



