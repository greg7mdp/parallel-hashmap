#include <iostream>
#include <fstream>
#include <sstream>
#include <parallel_hashmap/phmap.h>
#include <parallel_hashmap/btree.h>
#include <thread>
#include <array>
#include <vector>
#include <algorithm>
#include <cstdlib>


/*
 * count the number of occurrences of each word in a large text file using multiple threads
 */

int main() {
    // download Jane Austin "Pride and Prejudice"
    // ------------------------------------------
    if (system("curl https://www.gutenberg.org/files/1342/1342-0.txt -o 1342-0.txt") != 0) {
        std::cout << "Error: could not retrieve test file https://www.gutenberg.org/files/1342/1342-0.txt\n";
        return 1;
    }
    
    const std::string filename = "1342-0.txt";
    
    constexpr int num_threads = 4;
    std::vector<std::thread> threads;
    std::array<std::vector<std::string>, num_threads> lines_array;

    {
        // populate 4 vectors with lines from the book
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: could not open file " << filename << std::endl;
            return 1;
        }
        int line_idx = 0;
        std::string line;
        while (std::getline(file, line)) {
            lines_array[line_idx % num_threads].push_back(std::move(line));
            ++line_idx;
        }
    }

    using Map = phmap::parallel_flat_hash_map_m<std::string, int>; // parallel_flat_hash_map_m has default internal mutex
    Map word_counts;

    // run 4 threads, each thread processing lines from one of the vectors
    // -------------------------------------------------------------------
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            [&word_counts](std::vector<std::string>&& lines) {
                for (auto& line : lines) {
                    std::replace_if(line.begin(), line.end(), [](char c) -> bool { return !std::isalnum(c); }, ' ');
                    std::istringstream iss(line);
                    std::string word;
                    while (iss >> word) {
                        // use lazy_emplace to modify the map while the mutex is locked
                        word_counts.lazy_emplace_l(word,
                                                   [&](Map::value_type& p) { ++p.second; },  // called only when key was already present
                                                   [&](const Map::constructor& ctor) // construct value_type in place when key not present
                                                   { ctor(std::move(word), 1); } );
                    }
                }
            },
            std::move(lines_array[i]));
    }
    
    for (auto& thread : threads)
        thread.join();

    // print one word used at each frequency
    // -------------------------------------
    phmap::btree_map<int, std::string> result;
    for (const auto& pair : word_counts)
        result[pair.second] = pair.first;

    for (const auto& p : result) 
        std::cout << p.first << ": " << p.second << std::endl;

    return 0;
}
