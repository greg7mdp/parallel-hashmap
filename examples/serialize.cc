#include <iostream>
#include <bitset>
#include <cinttypes>

#define USE_CEREAL 0

#if USE_CEREAL
    #include "cereal/types/unordered_map.hpp"
    #include "cereal/types/memory.hpp"
    #include "cereal/types/bitset.hpp"
    #include "cereal/archives/binary.hpp"
    #include <fstream>
#endif

#include "parallel_hashmap/phmap_dump.h"

#include <chrono>
#include <functional>
#include <cstdio>

using phmap::flat_hash_map;
using namespace std;
template <typename T> using milliseconds = std::chrono::duration<T, std::milli>;

// --------------------------------------------------------------------------
//  from: https://github.com/preshing/RandomSequence
// --------------------------------------------------------------------------
class RSU
{
private:
    unsigned int m_index;
    unsigned int m_intermediateOffset;

    static unsigned int permuteQPR(unsigned int x)
    {
        static const unsigned int prime = 4294967291u;
        if (x >= prime)
            return x;  // The 5 integers out of range are mapped to themselves.
        unsigned int residue = ((unsigned long long) x * x) % prime;
        return (x <= prime / 2) ? residue : prime - residue;
    }

public:
    RSU(unsigned int seedBase, unsigned int seedOffset)
    {
        m_index = permuteQPR(permuteQPR(seedBase) + 0x682f0161);
        m_intermediateOffset = permuteQPR(permuteQPR(seedOffset) + 0x46790905);
    }

    unsigned int next()
    {
        return permuteQPR((permuteQPR(m_index++) + m_intermediateOffset) ^ 0x5bf03635);
    }
};  

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void showtime(const char *name, const std::function<void ()>& doit)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    doit();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto elapsed = milliseconds<double>(t2 - t1).count();
    printf("%s: %.3fs\n", name, (int)elapsed / 1000.0f);
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
template <class MapType>
void testMapSerialization(const char *maptype, const char *fname)
{
    MapType table;
    const int num_items = 100000000;

    printf("Building test %s\n", maptype);

    // Iterate and add keys and values 
    // -------------------------------
    showtime("build time", [&table, num_items]() {
            unsigned int seed = 76687;
            RSU rsu(seed, seed + 1);

            table.reserve(num_items);
            for (int i=0; i < num_items; ++i) 
                table.insert(typename MapType::value_type(rsu.next(), i)); 
        });

    // cerealize and save data
    // -----------------------
    showtime("serialize", [&]() {
#if !USE_CEREAL
            phmap::BinaryOutputArchive ar_out(fname);
            table.phmap_dump(ar_out);
#else
            ofstream os(fname, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
            cereal::BinaryOutputArchive archive(os);
            archive(table.size());
            archive(table);
#endif
        });

    MapType table_in;

    // deserialize
    // -----------
    showtime("deserialize", [&]() {
#if !USE_CEREAL
            phmap::BinaryInputArchive ar_in(fname);
            table_in.phmap_load(ar_in);
#else
            ifstream is(fname, std::ofstream::in | std::ofstream::binary);
            cereal::BinaryInputArchive archive_in(is);
            size_t table_size;

            archive_in(table_size);
            table_in.reserve(table_size);
            archive_in(table_in);             // deserialize from file out.cereal into table_in
#endif
        });

    
    if (table == table_in)
        printf("All checks out, table size: %zu\n\n", table_in.size());
    else
        printf("FAILURE\n");
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
template <class SetType>
void testSetSerialization(const char *settype, const char *fname)
{
    SetType table;
    const int num_items = 100000000;

    printf("Building test %s\n", settype);

    // Iterate and add keys and values 
    // -------------------------------
    showtime("build time", [&]() {
            unsigned int seed = 76687;
            RSU rsu(seed, seed + 1);

            table.reserve(num_items);
            for (int i=0; i < num_items; ++i) 
                table.insert(typename SetType::value_type(rsu.next())); 
        });

    // cerealize and save data
    // -----------------------
    showtime("serialize", [&]() {
#if !USE_CEREAL
            phmap::BinaryOutputArchive ar_out(fname);
            table.phmap_dump(ar_out);
#else
            ofstream os(fname, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
            cereal::BinaryOutputArchive archive(os);
            archive(table.size());
            archive(table);
#endif
        });

    SetType table_in;

    // deserialize
    // -----------
    showtime("deserialize", [&]() {
#if !USE_CEREAL
            phmap::BinaryInputArchive ar_in(fname);
            table_in.phmap_load(ar_in);
#else
            ifstream is(fname, std::ofstream::in | std::ofstream::binary);
            cereal::BinaryInputArchive archive_in(is);
            size_t table_size;

            archive_in(table_size);
            table_in.reserve(table_size);
            archive_in(table_in);             // deserialize from file out.cereal into table_in
#endif
        });

    
    if (table == table_in)
        printf("All checks out, table size: %zu\n\n", table_in.size());
    else
        printf("FAILURE\n");
}



// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
int main()
{
    testSetSerialization<phmap::flat_hash_set<unsigned int>>("flat_hash_set", "dump1.bin");
#if 0
    testSetSerialization<phmap::parallel_flat_hash_set<unsigned int>>("parallel_flat_hash_set", "dump1.bin");

    testMapSerialization<phmap::flat_hash_map<unsigned int, int>>("flat_hash_map", "dump1.bin");
    testMapSerialization<phmap::parallel_flat_hash_map<unsigned int, int>>("parallel_flat_hash_map", "dump1.bin");
#endif

    return 0;
}
