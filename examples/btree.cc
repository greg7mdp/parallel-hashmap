#include <iostream>
#include <string>
#include <parallel_hashmap/btree.h>

using phmap::btree_map;
using phmap::btree_set;

int main()
{ 
    btree_map<std::string, int> persons = 
        { { "John", 35 },
          { "Jane", 32 },
          { "Joe",  30 },
        };

    for (auto& p: persons)
        std::cout << p.first <<  " (" << p.second << ")" << '\n';

    // Create a btree_set of three floats (that map to strings)
    using X = std::tuple<float, std::string>;
    btree_set<X> email;
 
    // Iterate and print keys and values 
    for (int i=0; i<10; ++i)
        email.insert(X((float)i, "aha"));
    
    for (auto& e: email)
        std::cout << std::get<0>(e) << ", " << std::get<1>(e) << '\n';
            return 0;
}
