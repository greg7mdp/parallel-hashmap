#include <iostream>
#include <string>
#define PHMAP_BIDIRECTIONAL 1
#include <parallel_hashmap/phmap.h>

using phmap::flat_hash_map;
using namespace std; 
 
int main()
{
    // Create an unordered_map of three strings (that map to strings)
    flat_hash_map<std::string, std::string> email;
 
    // Iterate and print keys and values 
    for (const auto& n : email) 
        std::cout << n.first << "'s email is: " << n.second << "\n";
 
    // Add a new entry
    email["bill"] = "bg@whatever.com";
 
    // and print it
    std::cout << "bill's email is: " << email["bill"] << "\n";

    phmap::flat_hash_set<int> v1 { 1, 2, 3, 4, 5 };

    for (auto x : v1)
        cout << (x) << " "; 

    auto i1 = v1.end();
    for ( ;i1 != v1.begin(); --i1) 
    { 
        if (i1 != v1.end()) 
            cout << (*i1) << " "; 
    } 
    if (i1 != v1.end())
        cout << (*i1); 
 
    return 0;
}
