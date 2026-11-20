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

TEST(DumpLoad, MalformedArchiveIsRejected) {
    auto load = [](size_t size, size_t capacity, bool sentinel, size_t growth_left) {
        std::string buf;
        auto put = [&](size_t v) { buf.append(reinterpret_cast<const char*>(&v), sizeof(v)); };
        put(s_version);
        put(size);
        put(capacity);
        std::string ctrl(capacity + Group::kWidth + 1, static_cast<char>(kEmpty));
        ctrl[0] = 0x01; // one full slot
        if (sentinel)
            ctrl[capacity] = static_cast<char>(kSentinel);
        buf += ctrl;
        buf.append(sizeof(uint32_t) * capacity, '\0');
        put(growth_left);

        std::stringstream ss(buf);
        phmap::BinaryInputArchive ar_in(ss);
        phmap::flat_hash_set<uint32_t> st;
        bool ok = st.phmap_load(ar_in);
        EXPECT_EQ(st.size(), ok ? size : 0u);
        size_t n = 0;
        for (auto v : st) { (void)v; ++n; }
        EXPECT_EQ(n, st.size());
        st.insert(1u);
        return ok;
    };

    const size_t growth = CapacityToGrowth(7) - 1;
    EXPECT_TRUE(load(1, 7, true, growth));
    EXPECT_FALSE(load(1, 0, true, 0));        // size without storage
    EXPECT_FALSE(load(1, 6, true, growth));   // capacity not 2^n - 1
    EXPECT_FALSE(load(8, 7, true, 0));        // size above max load
    EXPECT_FALSE(load(1, 7, false, growth));  // missing sentinel
    EXPECT_FALSE(load(2, 7, true, growth));   // size disagrees with ctrl bytes
    EXPECT_FALSE(load(1, 7, true, ~size_t{}));  // growth_left out of range
}

}
}
}

