#include <iostream>
#include <bitset>
#include <cinttypes>
#include "parallel_hashmap/phmap.h"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/bitset.hpp"
#include "cereal/archives/binary.hpp"
#include <fstream>
#include <random>
#include <chrono>
#include <functional>
#include <cstdio>

using phmap::flat_hash_map;
using namespace std;
template <typename T> using milliseconds = std::chrono::duration<T, std::milli>;

void showtime(const char *name, std::function<void ()> doit)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    doit();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto elapsed = milliseconds<double>(t2 - t1).count();
    printf("%s: %.3fms\n", name, (int)elapsed / 1000.0f);
}
    
int main()
{
    using MapType = phmap::flat_hash_map<bitset<42>, int>;
    MapType table;
    const int num_items = 100000000;

    // Iterate and add keys and values 
    // -------------------------------
    showtime("build hash", [&table, num_items]() {
            random_device dev;
            mt19937 rng(dev());
            uniform_int_distribution<mt19937::result_type> dist6(0,1);
            table.reserve(num_items);
            for (int i=0; i < num_items; ++i) 
            {
                bitset<42> bs;
                for (int j=0; j<42; ++j) 
                    bs[j] = dist6(rng);
                table[bs] = i;
            }
        });

    // cerealize and save data
    // -----------------------
    showtime("serialize", [&table]() {
            ofstream os("out.cereal", ios::binary);
            cereal::BinaryOutputArchive archive(os);
            archive(table.size());
            archive(table);
        });

    MapType().swap(table); // make sure table is newly created

    // deserialize
    // -----------
    showtime("deserialize", [&table]() {
            ifstream is("out.cereal", ios::binary);
            cereal::BinaryInputArchive archive_in(is);
            size_t table_size;

            archive_in(table_size);
            table.reserve(table_size);
            archive_in(table);             // deserialize from file out.cereal into variable
        });

    
    printf("table size: %zu\n", table.size());

    return 0;
}
