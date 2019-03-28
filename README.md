[![Build Status](https://travis-ci.org/greg7mdp/parallel_hashmap.svg?branch=master)](https://travis-ci.org/greg7mdp/org/greg7mdp/parallel_hashmap)  
[![Build Status](https://ci.appveyor.com/api/projects/status/github/greg7mdp/parallel_hashmap)](https://ci.appveyor.com/api/projects/status/github/greg7mdp/parallel_hashmap)

# The Parallel Hashmap: very fast, very memory friendly

Click here [For a full writeup explaining the design and benefits of the Parallel Hashmap](https://greg7mdp.github.io/parallel-hashmap/).


> **IMPORTANT:** This repository borrowed a lot of code from the [abseil-cpp](https://github.com/abseil/abseil-cpp) repository, notably for the implementations of `phmap::flat_hash_map`, `phmap::flat_hash_set`, `phmap::node_hash_map` and `phmap::node_hash_set`. Please be aware that the code from [abseil-cpp](https://github.com/abseil/abseil-cpp) has been modified, and may behave differently than the original.  
This repository should be construed as an independent work, with no guarantees of any kind implied or provided by the authors. Please go to [abseil-cpp](https://github.com/abseil/abseil-cpp) for the official Abseil libraries.


# Work in progress - please do not use yet - should be ready before the end of March 2019

This repository aims to provide an set of excellent hash map implementations, with the following characteristics:

- **header only**: nothing to build, just copy the `parallel_hashmap` directory to your project and you are good to go.

- compiler with **C++11 support** required, **C++14 and C++17 APIs are provided**

- **Very efficient**, significantly faster than your compiler's unordered map/set or Boost's, or than [sparsepp](https://github.com/greg7mdp/sparsepp)

- **Memory friendly**: low memory usage, although a little higher than [sparsepp](https://github.com/greg7mdp/sparsepp)

- support heterogeneous lookup

- Not yet **Tested** ~~on Windows (vs2010-2015, g++), linux (g++, clang++) and MacOS (clang++)~~.

## Installation

Copy the parallel_hashmap directory to your project. That's all.

> cmake configuration files (CMakeLists.txt) are provided for building the tests and examples.

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




