# The Parallel Hashmap  [![Build Status](https://travis-ci.org/greg7mdp/parallel-hashmap.svg?branch=master)](https://travis-ci.org/greg7mdp/parallel-hashmap)  [![Build Status](https://ci.appveyor.com/api/projects/status/86kc657lp4cja8ju?svg=true)](https://ci.appveyor.com/project/greg7mdp/parallel-hashmap)

## Overview

This repository aims to provide an set of excellent hash map implementations, with the following characteristics:

- **header only**: nothing to build, just copy the `parallel_hashmap` directory to your project and you are good to go.

- compiler with **C++11 support** required, **C++14 and C++17 APIs are provided**

- **Very efficient**, significantly faster than your compiler's unordered map/set or Boost's, or than [sparsepp](https://github.com/greg7mdp/sparsepp)

- **Memory friendly**: low memory usage, although a little higher than [sparsepp](https://github.com/greg7mdp/sparsepp)

- support **heterogeneous lookup**

- **Tested** on Windows (vs2015 & vs2017), linux (g++ 5, 6, 7, 8, clang++ 3.9, 4.0, 5.0) and MacOS (g++ and clang++) - click on travis [![Build Status](https://travis-ci.org/greg7mdp/parallel-hashmap.svg?branch=master)](https://travis-ci.org/greg7mdp/parallel-hashmap) and appveyor [![Build Status](https://ci.appveyor.com/api/projects/status/86kc657lp4cja8ju?svg=true)](https://ci.appveyor.com/project/greg7mdp/parallel-hashmap) icons for detailed test status.


## Very fast *and*  memory friendly

Click here [For a full writeup explaining the design and benefits of the Parallel Hashmap](https://greg7mdp.github.io/parallel-hashmap/).


> **IMPORTANT:** This repository borrows code from the [abseil-cpp](https://github.com/abseil/abseil-cpp) repository, with modifications, and may behave differently from the original. This repository is an independent work, with no guarantees implied or provided by the authors. Please go to [abseil-cpp](https://github.com/abseil/abseil-cpp) for the official Abseil libraries.

## Installation

Copy the parallel_hashmap directory to your project. Update your include path. That's all.

> cmake configuration files (CMakeLists.txt) are provided for building the tests and examples. Command for building and running the tests is: `mkdir build && cd build && cmake -DPHMAP_BUILD_TESTS=ON .. && cmake --build . && make test`

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

Key points:

- The `flat` hash maps may move the keys and values in memory. So if you keep a pointer to something inside a `flat` hash map, this pointer may become invalid when the map is mutated. The `node` hash maps don't, and should be used instead if this is a problem.

- The `flat` hash maps will use less memory, and usually be faster than the `node` hash maps, so use them if you can. A possible exception is when the values inserted in the hash map are large (say more than 100 bytes [*needs testing*]).

- The `parallel` hash maps are preferred when you have a few hash maps that will store a very large number of values. The `non-parallel` hash maps are preferred if you have a large number of hash maps, each storing a relatively small number of values.

- The benefits of the `parallel` hash maps are:  
   a. reduced peak memory usage (when resizing), and  
   b. multithreading support (and inherent internal parallelism)


## Memory usage




