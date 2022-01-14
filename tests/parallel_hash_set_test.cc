#ifndef THIS_HASH_SET
    #define THIS_HASH_SET  parallel_flat_hash_set
    #define THIS_TEST_NAME ParallelFlatHashSet
#endif

#include "flat_hash_set_test.cc"

namespace phmap {
namespace priv {
namespace {

TEST(THIS_TEST_NAME, ThreadSafeContains) {
    // We can't test mutable keys, or non-copyable keys with ThisSet.
    // Test that the nodes have the proper API.
    using Set = phmap::THIS_HASH_SET<int>;

    {
        // --------------------
        // test emplace_single
        // --------------------
        Set m = { {1}, {11} };
 
        // emplace_single insert a value if not already present, else removes it
        for (int i=0; i<12; ++i)
            m.emplace_single(i, [i](const Set::constructor& ctor) { ctor(i); });
        EXPECT_EQ(m.count(0), 1);
        EXPECT_EQ(m.count(1), 0);
        EXPECT_EQ(m.count(2), 1);
        EXPECT_EQ(m.count(11), 0);
    }

}

}  // namespace
}  // namespace priv
}  // namespace phmap
