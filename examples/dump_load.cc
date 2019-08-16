#include <iostream>
#include <string>
#include <parallel_hashmap/phmap.h>

using phmap::flat_hash_map;
using phmap::flat_hash_set;
using phmap::parallel_flat_hash_map;

void dump_load_string_string() {
    flat_hash_map<std::string, std::string> mp1;
    phmap::BinaryOutputArchive ar_out("./dump.data");
    // Add a new entry
    mp1["key-1"] = "value-1";
    mp1["key-2"] = "value-2";

    // Iterate and print keys and values 
    for (const auto& n : mp1)
        std::cout << n.first << "'s value is: " << n.second << "\n";
 
    mp1.dump(ar_out);
    flat_hash_map<std::string, std::string> mp2;

    phmap::BinaryInputArchive ar_in("./dump.data");

    mp2.load(ar_in);
    // Iterate and print keys and values g|++
    for (const auto& n : mp2) 
        std::cout << n.first << "'s value is: " << n.second << "\n";

    std::remove("./dump.data");
}

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

    std::remove("./dump.data");
}

void dump_load_string_uint32() {
    flat_hash_map<std::string, uint32_t> mp1;
    phmap::BinaryOutputArchive ar_out("./dump.data");
    // Add a new entry
    mp1["key-1"] = 99;
    mp1["key-2"] = 299;

    // Iterate and print keys and values 
    for (const auto& n : mp1)
        std::cout << n.first << "'s value is: " << n.second << "\n";
 
    mp1.dump(ar_out);
    flat_hash_map<std::string, uint32_t> mp2;
    phmap::BinaryInputArchive ar_in("./dump.data");

    mp2.load(ar_in);
    // Iterate and print keys and values g|++
    for (const auto& n : mp2) 
        std::cout << n.first << "'s value is: " << n.second << "\n";

    std::remove("./dump.data");
}

void dump_load_uint32_string() {
    flat_hash_map<uint32_t, std::string> mp1;
    phmap::BinaryOutputArchive ar_out("./dump.data");
 
    // Add a new entry
    mp1[100] = "hello";
    mp1[299] = "world";

    // Iterate and print keys and values 
    for (const auto& n : mp1)
        std::cout << n.first << "'s value is: " << n.second << "\n";
 
    mp1.dump(ar_out);
    flat_hash_map<uint32_t, std::string> mp2;
    phmap::BinaryInputArchive ar_in("./dump.data");

    mp2.load(ar_in);
    // Iterate and print keys and values g|++
    for (const auto& n : mp2) 
        std::cout << n.first << "'s value is: " << n.second << "\n";

    std::remove("./dump.data");
}

void dump_load_string() {
    flat_hash_set<std::string> st1;
    phmap::BinaryOutputArchive ar_out("./dump.data");

    // Add a new entry
    st1.insert("hello");
    st1.insert("world");

    // Iterate and print
    for (const auto& n : st1)
        std::cout << "value: " << n << "\n";
 
    st1.dump(ar_out);
    flat_hash_set<std::string> st2;
    phmap::BinaryInputArchive ar_in("./dump.data");

    st2.load(ar_in);
    // Iterate and print keys and values g|++
    for (const auto& n : st2)
        std::cout << "value: " << n << "\n";

    std::remove("./dump.data");
}

void dump_load_uint64() {
    flat_hash_set<uint64_t> st1;
    phmap::BinaryOutputArchive ar_out("./dump.data");

    // Add a new entry
    st1.insert(878);
    st1.insert(1424);

    // Iterate and print
    for (const auto& n : st1)
        std::cout << "value: " << n << "\n";
 
    st1.dump(ar_out);
    flat_hash_set<uint64_t> st2;
    phmap::BinaryInputArchive ar_in("./dump.data");

    st2.load(ar_in);
    // Iterate and print keys and values g|++
    for (const auto& n : st2)
        std::cout << "value: " << n << "\n";

    std::remove("./dump.data");
}

void dump_load_parallel_flat_hash_map() {
    parallel_flat_hash_map<uint64_t, uint32_t> mp1;
    phmap::OutputArchiveWrapper<phmap::BinaryOutputArchive> w_out("./");

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
 
    mp1.dump(w_out);
    parallel_flat_hash_map<uint64_t, uint32_t> mp2;
    phmap::InputArchiveWrapper<phmap::BinaryInputArchive> w_in("./");

    mp2.load(w_in);
    for (const auto& n : mp2)
        std::cout << "key: " << n.first << ", value: " << n.second << "\n";
}

int main()
{    
    dump_load_string_string();
    dump_load_uint64_uint32();
    dump_load_string_uint32();
    dump_load_uint32_string();
    dump_load_string();
    dump_load_uint64();
    dump_load_parallel_flat_hash_map();
    return 0;
}