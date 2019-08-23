#include <iostream>
#include <string>
#include <parallel_hashmap/phmap_dump.h>

using phmap::flat_hash_map;
using phmap::parallel_flat_hash_map;

void dump_load_uint64_uint32() {
    flat_hash_map<uint64_t, uint32_t> mp1;
    phmap::BinaryOutputArchive ar_out("./dump.data");
    // Add a new entry
    mp1[100] = 99;
    mp1[300] = 299;

    // Iterate and print keys and values 
    for (const auto& n : mp1)
        std::cout << n.first << "'s value is: " << n.second << "\n";
 
    mp1.dump(ar_out);
    flat_hash_map<uint64_t, uint32_t> mp2;
    phmap::BinaryInputArchive ar_in("./dump.data");
    mp2.load(ar_in);
    // Iterate and print keys and values g|++
    for (const auto& n : mp2)
        std::cout << n.first << "'s value is: " << n.second << "\n";
}

void dump_load_parallel_flat_hash_map() {
    parallel_flat_hash_map<uint64_t, uint32_t> mp1;
    phmap::BinaryOutputArchive ar_out("./dump.data");

    // Add a new entry
    mp1[100] = 99;
    mp1[300] = 299;
    mp1[101] = 992;
    mp1[1300] = 2991;
    mp1[1130] = 299;
    mp1[2130] = 1299;
    // Iterate and print
    for (const auto& n : mp1)
        std::cout << "key: " << n.first << ", value: " << n.second << "\n";
 
    mp1.dump(ar_out);
    parallel_flat_hash_map<uint64_t, uint32_t> mp2;
    phmap::BinaryInputArchive ar_in("./dump.data");

    mp2.load(ar_in);
    for (const auto& n : mp2)
        std::cout << "key: " << n.first << ", value: " << n.second << "\n";
}

#if defined(__linux__)
void mmap_load_uint64_uint32() {
    using MapType = flat_hash_map<uint64_t, uint32_t,
            phmap::container_internal::hash_default_hash<uint64_t>,
            phmap::container_internal::hash_default_eq<uint64_t>,
            phmap::MmapAllocator<
                        phmap::container_internal::Pair<const uint64_t, uint32_t>>>;
    MapType mp1;
    mp1.reserve(100);
    phmap::MmapOutputArchive ar_out("./dump.data");
    // Add a new entry
    mp1[100] = 99;
    mp1[300] = 299;

    // Iterate and print keys and values 
    for (const auto& n : mp1)
        std::cout << n.first << "'s value is: " << n.second << "\n";
 
    mp1.mmap_dump(ar_out);
    MapType mp2;
    
    phmap::MmapInputArchive ar_in("./dump.data");    
    mp2.mmap_load(ar_in);
    mp2[849242] = 141;
    mp2[11] = 1111;
    // Iterate and print keys and values g|++
    for (const auto& n : mp2)
        std::cout << n.first << "'s value is: " << n.second << "\n";
}

void mmap_load_parallel_flat_hash_map() {
    using MapType = parallel_flat_hash_map<uint64_t, uint32_t,
            phmap::container_internal::hash_default_hash<uint64_t>,
            phmap::container_internal::hash_default_eq<uint64_t>,
            phmap::MmapAllocator<
                        phmap::container_internal::Pair<const uint64_t, uint32_t>>,
            4,
            phmap::NullMutex>;
    
    MapType mp1;
    phmap::MmapOutputArchive ar_out("./dump.data");

    // Add a new entry
    mp1[100] = 99;
    mp1[300] = 299;
    mp1[101] = 992;
    mp1[1300] = 2991;
    mp1[1130] = 299;
    mp1[2130] = 1299;
    // Iterate and print
    for (const auto& n : mp1)
        std::cout << "key: " << n.first << ", value: " << n.second << "\n";
 
    mp1.mmap_dump(ar_out);
    MapType mp2;
    phmap::MmapInputArchive ar_in("./dump.data");

    mp2.mmap_load(ar_in);
    std::cout << "[debug] map capacity: " << mp2.capacity() << ", size: " << mp2.size() << std::endl;
    for (size_t i = 0; i < 100; i ++) {
        mp2[6771 + i] = i;
    }
    std::cout << "[debug] map capacity: " << mp2.capacity() << ", size: " << mp2.size() << std::endl;
    for (const auto& n : mp2)
        std::cout << "key: " << n.first << ", value: " << n.second << "\n";
}
#endif

int main()
{
    dump_load_uint64_uint32();
    dump_load_parallel_flat_hash_map();

#if defined(__linux__)
    mmap_load_uint64_uint32();
    mmap_load_parallel_flat_hash_map();
#endif
    return 0;
}

