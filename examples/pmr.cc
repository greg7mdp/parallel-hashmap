#if __has_include(<experimental/memory_resource>)
#include <experimental/memory_resource>
    namespace std
    {
        namespace pmr = experimental::pmr;
    }
#elif __has_include(<memory_resource>)
    #include <memory_resource>
#elif
    #error <memory_resource> is missing 
#endif

#include <parallel_hashmap/phmap.h>

struct MyStruct
{
    template<typename Key, typename Value>
    using ParallelFlatHashMap = phmap::parallel_flat_hash_map<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                                              std::pmr::polymorphic_allocator<std::pair<const Key, Value>>>;

    ParallelFlatHashMap<uint32_t, uint32_t> hashMap;

    // No compile errors
    MyStruct() 
    {
    }

    // Compile errors
    MyStruct(std::pmr::memory_resource* memoryResource = std::pmr::get_default_resource()) 
        : hashMap(memoryResource)
    {
    }
};
