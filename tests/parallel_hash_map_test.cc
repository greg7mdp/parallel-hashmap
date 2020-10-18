#ifndef THIS_HASH_MAP
    #define THIS_HASH_MAP  parallel_flat_hash_map
    #define THIS_TEST_NAME ParallelFlatHashMap
#endif

#include "flat_hash_map_test.cc"

namespace phmap {
namespace priv {
namespace {

TEST(THIS_TEST_NAME, ThreadSafeContains) {
    // We can't test mutable keys, or non-copyable keys with ThisMap.
    // Test that the nodes have the proper API.
    using Map = ThisMap<int, int>;

    Map m = { {1, 7}, {2, 9} };
    const Map& const_m(m);
    
    auto val = 0; 
    auto get_value = [&val](const int& v) { val = v; };
    EXPECT_TRUE(const_m.if_contains(2, get_value));
    EXPECT_EQ(val, 9);

    EXPECT_FALSE(m.if_contains(3, get_value));

    auto set_value = [](int& v) { v = 11; };
    EXPECT_TRUE(m.modify_if(2, set_value));
    EXPECT_EQ(m[2], 11);

    EXPECT_FALSE(m.modify_if(3, set_value));

    // overwrite an existing value
    m.try_emplace_l(2, [](int& v) { v = 5; });
    EXPECT_EQ(m[2], 5);

    // insert a value that is not already present. Will be default initialised to 0 and lambda not called
    m.try_emplace_l(3, [](int& v) { assert(v == 0); /* should not be called when value constructed */ v = 6; });
    EXPECT_EQ(m[3], 0);
    
    // insert a value that is not already present, provide argument to value-construct it
    m.try_emplace_l(4, [](int& ) { assert(0); /* should not be called when value constructed */ }, 999);
    EXPECT_EQ(m[4], 999);

    // insert a value that is not already present.
    m.lazy_emplace_l(5, 
                     [](int& v) { assert(0); /* should not be called when value constructed */ v = 6; },
                     [](const Map::constructor& ctor) { ctor(5, 13); });

    EXPECT_EQ(m[5], 13);

    // change a value that is present
    m.lazy_emplace_l(5, 
                     [](int& v) { v = 6; },
                     [](const Map::constructor& ctor) { assert(0); /* should not be called when value exists */ctor(5, 13); });
    EXPECT_EQ(m[5], 6);
}

}  // namespace
}  // namespace priv
}  // namespace phmap
