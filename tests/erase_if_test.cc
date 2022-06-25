#include <vector>

#include "gtest/gtest.h"

#include "parallel_hashmap/phmap.h"

namespace phmap {
namespace priv {
namespace {

TEST(EraseIf, FlatHashSet_uint32) {
    phmap::flat_hash_set<uint32_t> st1 = { 3, 6, 7, 9 };
    auto num_erased = erase_if(st1, [](const uint32_t& v) { return v >= 7; });
    EXPECT_TRUE(num_erased == 2);

    phmap::flat_hash_set<uint32_t> st2 = { 0, 2, 3, 6 };
    num_erased = erase_if(st2, [](const uint32_t& v) { return v <= 2; });
    EXPECT_TRUE(num_erased == 2);

    EXPECT_TRUE(st1 == st2);
}

TEST(EraseIf, FlatHashMap_uint64_uint32) {
    using map = phmap::flat_hash_map<uint32_t, uint32_t>;
    map st1 = { {3, 0}, {6, 0}, {7, 0}, {9, 0} };
    auto num_erased = erase_if(st1, [](const map::value_type& v) { return v.first >= 7; });
    EXPECT_TRUE(num_erased == 2);

    map st2 = { {0, 0}, {2, 0}, {3, 0}, {6, 0} };
    num_erased = erase_if(st2, [](const map::value_type& v) { return v.first <= 2; });
    EXPECT_TRUE(num_erased == 2);

    EXPECT_TRUE(st1 == st2);
}

TEST(EraseIf, ParallelFlatHashMap_uint64_uint32) {
    using map = phmap::parallel_flat_hash_map<uint32_t, uint32_t>;
    map st1 = { {3, 0}, {6, 0}, {7, 0}, {9, 0} };
    auto num_erased = erase_if(st1, [](const map::value_type& v) { return v.first >= 7; });
    EXPECT_TRUE(num_erased == 2);

    map st2 = { {0, 0}, {2, 0}, {3, 0}, {6, 0} };
    num_erased = erase_if(st2, [](const map::value_type& v) { return v.first <= 2; });
    EXPECT_TRUE(num_erased == 2);

    EXPECT_TRUE(st1 == st2);
}

}
}
}

