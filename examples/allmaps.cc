// Silly program just to test the natvis file for Visual Studio
// ------------------------------------------------------------
#include <string>
#include "parallel_hashmap/phmap.h"

template<class Set, class F>
void test_set(F &f)
{
    Set s;
    typename Set::iterator it;
    for (int i=0; i<100; ++i)
        s.insert(f(i));

    it = s.begin();
    ++it;

	it = s.end();
    it = s.begin();
    while(it != s.end())
        ++it;
    it = s.begin();
}

int main(int, char **)
{
    using namespace std;

    auto make_int    = [](int i) { return i; };
    auto make_string = [](int i) { return std::to_string(i); };

    auto make_2int    = [](int i) { return std::make_pair(i, i); };
    auto make_2string = [](int i) { return std::make_pair(std::to_string(i), std::to_string(i)); };

    test_set<phmap::flat_hash_set<int>>(make_int);
    test_set<phmap::flat_hash_set<string>>(make_string);

    test_set<phmap::node_hash_set<int>>(make_int);
    test_set<phmap::node_hash_set<string>>(make_string);

    test_set<phmap::flat_hash_map<int, int>>(make_2int);
    test_set<phmap::flat_hash_map<string, string>>(make_2string);

    test_set<phmap::node_hash_map<int, int>>(make_2int);
    test_set<phmap::node_hash_map<string, string>>(make_2string);

    test_set<phmap::parallel_flat_hash_set<int>>(make_int);
    test_set<phmap::parallel_flat_hash_set<string>>(make_string);

    test_set<phmap::parallel_node_hash_set<int>>(make_int);
    test_set<phmap::parallel_node_hash_set<string>>(make_string);

    test_set<phmap::parallel_flat_hash_map<int, int>>(make_2int);
    test_set<phmap::parallel_flat_hash_map<string, string>>(make_2string);

    test_set<phmap::parallel_node_hash_map<int, int>>(make_2int);
    test_set<phmap::parallel_node_hash_map<string, string>>(make_2string);
}
