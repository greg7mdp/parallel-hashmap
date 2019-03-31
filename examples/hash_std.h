#ifndef phmap_example_hash_std_
#define phmap_example_hash_std_

#include <parallel_hashmap/phmap_utils.h>

#include <string>
using std::string;

struct Person
{
    bool operator==(const Person &o) const
    { 
        return _first == o._first && _last == o._last; 
    }

    string _first;
    string _last;
};

namespace std
{
// inject specialization of std::hash for Person into namespace std
// ----------------------------------------------------------------
    template<>
    struct hash<Person>
    {
        std::size_t operator()(Person const &p) const
        {
            std::size_t seed = 0;
            phmap::hash_combine(seed, p._first);
            phmap::hash_combine(seed, p._last);
            return seed;
        }
    };
}

#endif // phmap_example_hash_std_
