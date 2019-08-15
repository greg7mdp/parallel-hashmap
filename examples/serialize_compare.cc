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
    printf("%s: %.3fs\n", name, (int)elapsed / 1000.0f);
}

const size_t bigger_than_cachesize = 10 * 1024 * 1024;
long *p = new long[bigger_than_cachesize];
long *p1 = new long[bigger_than_cachesize];

int main(int argc, char* argv[])
{
    size_t num_items =  2000000;
    if (argc == 2) {
        num_items = atoi(argv[1]);
    }
    std::cout << "items size: " << num_items << std::endl;

    using MapType = phmap::flat_hash_map<uint64_t, int>;
    MapType table;

    std::vector<uint64_t> test_data_arr(num_items, 0);
    for (size_t i = 0; i < num_items; i++) {
        test_data_arr[i] = i;
    }

    std::srand(std::time(nullptr));
    auto generate_random = [](int n) -> size_t {
        return std::rand() % n;
    };

    auto shuffle = [&generate_random](std::vector<uint64_t>& arr) {
        size_t n = arr.size();
        size_t pos = n;
        while(pos > 0) {
            size_t r = generate_random(pos);
            swap(arr[r], arr[pos - 1]);
            pos --;
        }
    };

    // shuffle
    shuffle(test_data_arr);

    // Iterate and add keys and values
    // -------------------------------
    showtime("build hash", [&table, &test_data_arr, num_items]() {
            table.reserve(num_items);
            for (int i=0; i < num_items; ++i) {
                table[test_data_arr[i]] = rand();
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


    // "flush" cache.
    for(int i = 0; i < bigger_than_cachesize; i++) {
        p[i] = rand();
    }

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


    // dump data
    // -----------------------
    showtime("dump", [&table]() {
            table.dump("./out.dump");
        });

    // "flush" cache.
    for(int i = 0; i < bigger_than_cachesize; i++) {
        p1[i] = rand();
    }


    MapType().swap(table); // make sure table is newly created

    // load
    // -----------
    showtime("load", [&table]() {
            table.load("./out.dump");
        });

    printf("table size: %zu\n", table.size());
    return 0;
}
