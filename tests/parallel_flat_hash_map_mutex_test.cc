#define THIS_HASH_MAP  parallel_flat_hash_map
#define THIS_EXTRA_TPL_PARAMS , 4, std::mutex
#define THIS_TEST_NAME ParallelFlatHashMap

#include <mutex>

#include "flat_hash_map_test.cc"
