#include <cstdint>
#include <fstream>
#include <parallel_hashmap/phmap.h>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"

#include "parallel_hashmap/phmap_dump.h"

namespace phmap {
namespace priv {
namespace {

TEST(DumpLoad, FlatHashSet_uint32) {
    phmap::flat_hash_set<uint32_t> st1 = { 1991, 1202 };

    {
        phmap::BinaryOutputArchive ar_out("./dump.data");
        EXPECT_TRUE(st1.phmap_dump(ar_out));
    }

    phmap::flat_hash_set<uint32_t> st2;
    {
        phmap::BinaryInputArchive ar_in("./dump.data");
        EXPECT_TRUE(st2.phmap_load(ar_in));
    }
    EXPECT_TRUE(st1 == st2);

    {
        std::stringstream ss;
        phmap::BinaryOutputArchive ar_out(ss);
        EXPECT_TRUE(st1.phmap_dump(ar_out));
        phmap::flat_hash_set<uint32_t> st3;
        phmap::BinaryInputArchive ar_in(ss);
        EXPECT_TRUE(st3.phmap_load(ar_in));
        EXPECT_TRUE(st1 == st3);
    }
}

TEST(DumpLoad, FlatHashMap_uint64_uint32) {
    phmap::flat_hash_map<uint64_t, uint32_t> mp1 = {
        { 78731, 99}, {13141, 299}, {2651, 101} };

    {
        phmap::BinaryOutputArchive ar_out("./dump.data");
        EXPECT_TRUE(mp1.phmap_dump(ar_out));
    }

    phmap::flat_hash_map<uint64_t, uint32_t> mp2;
    {
        phmap::BinaryInputArchive ar_in("./dump.data");
        EXPECT_TRUE(mp2.phmap_load(ar_in));
    }

    {
        std::stringstream ss;
        phmap::BinaryOutputArchive ar_out(ss);
        EXPECT_TRUE(mp1.phmap_dump(ar_out));
        phmap::flat_hash_map<uint64_t, uint32_t> mp3;
        phmap::BinaryInputArchive ar_in(ss);
        EXPECT_TRUE(mp3.phmap_load(ar_in));
        EXPECT_TRUE(mp1 == mp3);
    }

    EXPECT_TRUE(mp1 == mp2);
}

TEST(DumpLoad, ParallelFlatHashMap_uint64_uint32) {
    phmap::parallel_flat_hash_map<uint64_t, uint32_t> mp1 = {
        {99, 299}, {992, 2991}, {299, 1299} };

    {
        phmap::BinaryOutputArchive ar_out("./dump.data");
        EXPECT_TRUE(mp1.phmap_dump(ar_out));
    }

    phmap::parallel_flat_hash_map<uint64_t, uint32_t> mp2;
    {
        phmap::BinaryInputArchive ar_in("./dump.data");
        EXPECT_TRUE(mp2.phmap_load(ar_in));
    }
    EXPECT_TRUE(mp1 == mp2);

    // test stringstream and dump/load in the middle of the stream
    {
        char hello[] = "Hello";
        std::stringstream ss;
        ss.write(hello, 5);
        phmap::BinaryOutputArchive ar_out(ss);
        EXPECT_TRUE(mp1.phmap_dump(ar_out));
        phmap::parallel_flat_hash_map<uint64_t, uint32_t> mp3;
        phmap::BinaryInputArchive ar_in(ss);
        char s[5];
        ss.read(s, 5);
        for (int i = 0; i < 5; ++i) {
            EXPECT_EQ(hello[i], s[i]);
        }
        EXPECT_TRUE(mp3.phmap_load(ar_in));
        EXPECT_TRUE(mp1 == mp3);
    }
}

}
}
}

