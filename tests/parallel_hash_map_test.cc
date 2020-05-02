#ifndef THIS_HASH_MAP
    #define THIS_HASH_MAP  parallel_flat_hash_map
    #define THIS_TEST_NAME ParallelFlatHashMap
#endif

namespace phmap {
namespace container_internal {
namespace {

TEST(THIS_TEST_NAME, ThreadSafeContains) {
    // We can't test mutable keys, or non-copyable keys with ThisMap.
    // Test that the nodes have the proper API.
    ThisMap<int, int> m = { {1, 7}, {2, 9} };
    auto val = 0;
    EXPECT_TRUE(m.contains(1, val));
    EXPECT_EQ(val, 7);

    EXPECT_FALSE(m.contains(3, val));

#if __cplusplus > 199711L
    auto func = [&val](int& v) { val = v; };
    EXPECT_TRUE(m.if_contains(2, func));
    EXPECT_EQ(val, 9);

    EXPECT_FALSE(m.if_contains(3, func));
#endif

}

}  // namespace
}  // namespace container_internal
}  // namespace phmap
