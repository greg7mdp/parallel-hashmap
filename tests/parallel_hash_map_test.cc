#ifndef THIS_HASH_MAP
    #define THIS_HASH_MAP  parallel_flat_hash_map
    #define THIS_TEST_NAME ParallelFlatHashMap
#endif

#include "flat_hash_map_test.cc"

namespace phmap {
namespace priv {
namespace {

TEST(THIS_TEST_NAME, Swap) {
    using Map = ThisMap<int, int>;
    using MapB = ThisMap_NullMutex<int, int>;
    
    Map t;
    EXPECT_TRUE(t.find(0) == t.end());
    auto res = t.emplace(0, 1);
    EXPECT_TRUE(res.second);
    EXPECT_EQ(1, t.size());
    MapB u;
    t.swap(u);
    EXPECT_EQ(0, t.size());
    EXPECT_EQ(1, u.size());
    EXPECT_TRUE(t.find(0) == t.end());
    EXPECT_TRUE(u[0] == 1);
}


TEST(THIS_TEST_NAME, IfContains) {
    // ----------------
    // test if_contains
    // ----------------

    using Map = ThisMap<int, int>;
    Map m = { {1, 7}, {2, 9} };
    const Map& const_m(m);
    
    auto val = 0; 
    auto get_value = [&val](const Map::value_type& v) { val = v.second; };
    EXPECT_TRUE(const_m.if_contains(2, get_value));
    EXPECT_EQ(val, 9);

    EXPECT_FALSE(m.if_contains(3, get_value));
}

TEST(THIS_TEST_NAME, ModifyIf) {
    // --------------
    // test modify_if
    // --------------
    using Map = ThisMap<int, int>;
    Map m = { {1, 7}, {2, 9} };

    auto set_value = [](Map::value_type& v) { v.second = 11; };
    EXPECT_TRUE(m.modify_if(2, set_value));
    EXPECT_EQ(m[2], 11);

    EXPECT_FALSE(m.modify_if(3, set_value)); // because m[3] does not exist
}

TEST(THIS_TEST_NAME, TryEmplaceL) {
    // ------------------
    // test try_emplace_l
    // ------------------
    using Map = ThisMap<int, int>;
    Map m = { {1, 7}, {2, 9} };

    // overwrite an existing value
    m.try_emplace_l(2, [](Map::value_type& v) { v.second = 5; });
    EXPECT_EQ(m[2], 5);

    // insert a value that is not already present. Will be default initialised to 0 and lambda not called
    m.try_emplace_l(3, 
                    [](Map::value_type& v) { v.second = 6; }, // called only when key was already present
                    1);                    // argument to construct new value is key not present
    EXPECT_EQ(m[3], 1);
    
    // insert a value that is not already present, provide argument to value-construct it
    m.try_emplace_l(4, 
                    [](Map::value_type& ) {}, // called only when key was already present
                    999);                     // argument to construct new value is key not present

    EXPECT_EQ(m[4], 999);
}

TEST(THIS_TEST_NAME, LazyEmplaceL) {
    // --------------------
    // test lazy_emplace_l
    // --------------------
    using Map = ThisMap<int, int>;
    Map m = { {1, 7}, {2, 9} };
 
    // insert a value that is not already present.
    // right now m[5] does not exist
    m.lazy_emplace_l(5, 
                     [](Map::value_type& v) { v.second = 6; },           // called only when key was already present
                     [](const Map::constructor& ctor) { ctor(5, 13); }); // construct value_type in place when key not present 
    EXPECT_EQ(m[5], 13);

    // change a value that is present. Currently m[5] == 13
    m.lazy_emplace_l(5, 
                     [](Map::value_type& v) { v.second = 6; },           // called only when key was already present
                     [](const Map::constructor& ctor) { ctor(5, 13); }); // construct value_type in place when key not present
    EXPECT_EQ(m[5], 6);
}

TEST(THIS_TEST_NAME, EraseIf) {
    // -------------
    // test erase_if
    // -------------
    using Map = ThisMap<int, int>;
    Map m = { {1, 7}, {2, 9}, {5, 6} };

    EXPECT_EQ(m.erase_if(9, [](Map::value_type& v) { assert(0); return v.second == 12; }), false); // m[9] not present - lambda not called
    EXPECT_EQ(m.erase_if(5, [](Map::value_type& v) { return v.second == 12; }), false);            // m[5] == 6, so erase not performed
    EXPECT_EQ(m[5], 6);
    EXPECT_EQ(m.erase_if(5, [](Map::value_type& v) { return v.second == 6; }), true);              // lambda returns true, so m[5] erased
    EXPECT_EQ(m[5], 0);
}

TEST(THIS_TEST_NAME, ForEach) {
    // -------------
    // test for_each
    // -------------
    using Map = ThisMap<int, int>;
    Map m = { {1, 7}, {2, 8}, {5, 11} };

    // increment all values by 1
    m.for_each_m([](Map::value_type &pair) {  ++pair.second; });

    int counter = 0;
    m.for_each([&counter](const Map::value_type &pair) {
            ++counter;
            EXPECT_EQ(pair.first + 7, pair.second);
        });
    EXPECT_EQ(counter, 3);
    
    counter = 0;
    for (size_t i=0; i<m.subcnt(); ++i) {
        m.with_submap(i, [&](const Map::EmbeddedSet& set) {
            for (auto& p : set) {
                ++counter;
                EXPECT_EQ(p.first + 7, p.second);
            }
        });
    }
    EXPECT_EQ(counter, 3);
}

TEST(THIS_TEST_NAME, EmplaceSingle) {
    // --------------------
    // test emplace_single
    // --------------------
    using Map = ThisMap<int, int>;
    Map m = { {1, 4}, {11, 4} };
 
    // emplace_single insert a value if not already present, else removes it
    for (int i=0; i<12; ++i)
        m.emplace_single(i, [i](const Map::constructor& ctor) { ctor(i, 4); });
    EXPECT_EQ(m.count(0), 1);
    EXPECT_EQ(m.count(1), 0);
    EXPECT_EQ(m.count(2), 1);
    EXPECT_EQ(m.count(11), 0);
}


}  // namespace
}  // namespace priv
}  // namespace phmap
