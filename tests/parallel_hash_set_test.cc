#ifndef THIS_HASH_SET
    #define THIS_HASH_SET  parallel_flat_hash_set
    #define THIS_TEST_NAME ParallelFlatHashSet
#endif

#include "flat_hash_set_test.cc"

namespace phmap {
namespace priv {
namespace {

struct Entry
{
    Entry(int k, int v=0) : key(k), value(v) {}

    bool operator==(const Entry &o) const
    { 
        return key == o.key; // not checking value
    }

    // Demonstrates how to provide the hash function as a friend member function of the class
    // This can be used as an alternative to providing a std::hash<Person> specialization
    // --------------------------------------------------------------------------------------
    friend size_t hash_value(const Entry &p) 
    {
        return phmap::HashState().combine(0, p.key); // not checking value
    }

    int key;
    int value;
};

TEST(THIS_TEST_NAME, IfContains) {
    // ----------------
    // test if_contains
    // ----------------
    using Set = phmap::THIS_HASH_SET<Entry>;
    Set m = { {1, 7}, {2, 9} };
    const Set& const_m(m);
    
    auto val = 0; 
    auto get_value = [&val](const Set::value_type& v) { val = v.value; };
    EXPECT_TRUE(const_m.if_contains(Entry{2}, get_value));
    EXPECT_EQ(val, 9);

    EXPECT_FALSE(m.if_contains(Entry{3}, get_value));
}

TEST(THIS_TEST_NAME, ModifyIf) {
    // --------------
    // test modify_if
    // --------------
    using Set = phmap::THIS_HASH_SET<Entry>;
    Set m = { {1, 7}, {2, 9} };

    auto set_value = [](Set::value_type& v) { v.value = 11; };
    EXPECT_TRUE(m.modify_if(Entry{2}, set_value));

    auto val = 0; 
    auto get_value = [&val](const Set::value_type& v) { val = v.value; };
    EXPECT_TRUE(m.if_contains(Entry{2}, get_value));
    EXPECT_EQ(val, 11);

    EXPECT_FALSE(m.modify_if(Entry{3}, set_value)); // because m[3] does not exist
}

TEST(THIS_TEST_NAME, LazyEmplaceL) {
    // --------------------
    // test lazy_emplace_l
    // --------------------
    using Set = phmap::THIS_HASH_SET<Entry>;
    Set m = { {1, 7}, {2, 9} };
 
    // insert a value that is not already present.
    // right now m[5] does not exist
    m.lazy_emplace_l(Entry{5}, 
                     [](Set::value_type& v) { v.value = 6; },            // called only when key was already present
                     [](const Set::constructor& ctor) { ctor(5, 13); }); // construct value_type in place when key not present 
    EXPECT_EQ(m.find(Entry{5})->value, 13);

    // change a value that is present. 
    m.lazy_emplace_l(Entry{5}, 
                     [](Set::value_type& v) { v.value = 6; },            // called only when key was already present
                     [](const Set::constructor& ctor) { ctor(5, 13); }); // construct value_type in place when key not present
    EXPECT_EQ(m.find(Entry{5})->value, 6);
}

TEST(THIS_TEST_NAME, EraseIf) {
    // -------------
    // test erase_if
    // -------------
    using Set = phmap::THIS_HASH_SET<Entry>;
    Set m = { {1, 7}, {2, 9}, {5, 6} };

    EXPECT_EQ(m.erase_if(Entry{9}, [](Set::value_type& v) { assert(0); return v.value == 12; }), false); // m[9] not present - lambda not called
    EXPECT_EQ(m.erase_if(Entry{5}, [](Set::value_type& v) { return v.value == 12; }), false);            // m[5] == 6, so erase not performed
    EXPECT_EQ(m.find(Entry{5})->value, 6);
    EXPECT_EQ(m.erase_if(Entry{5}, [](Set::value_type& v) { return v.value == 6; }), true);              // lambda returns true, so m[5] erased
    EXPECT_EQ(m.find(Entry{5}), m.end());
}

TEST(THIS_TEST_NAME, ForEach) {
    // -------------
    // test for_each
    // -------------
    using Set = phmap::THIS_HASH_SET<Entry>;
    Set m = { {1, 7}, {2, 8}, {5, 11} };

    int counter = 0;
    m.for_each([&counter](const Set::value_type &v) {
            ++counter;
            EXPECT_EQ(v.key + 6, v.value);
        });
    EXPECT_EQ(counter, 3);
}

TEST(THIS_TEST_NAME, EmplaceSingle) {
    using Set = phmap::THIS_HASH_SET<int>;

    // --------------------
    // test emplace_single
    // --------------------
    Set m = { 1, 11 };
 
    // emplace_single insert a value if not already present, else removes it
    for (int i=0; i<12; ++i)
        m.emplace_single(i, [i](const Set::constructor& ctor) { ctor(i); });
    EXPECT_EQ(m.count(0), 1);
    EXPECT_EQ(m.count(1), 0);
    EXPECT_EQ(m.count(2), 1);
    EXPECT_EQ(m.count(11), 0);
}

}  // namespace
}  // namespace priv
}  // namespace phmap
