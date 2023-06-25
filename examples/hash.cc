#include <parallel_hashmap/phmap_utils.h> // minimal header providing phmap::HashState()
#include <string>
#include <utility>
#include <tuple>
#include <vector>
#include <array>
#if PHMAP_HAVE_STD_STRING_VIEW
    #include <string_view>
#endif
#include <iostream>

using std::string;
using std::tuple;
using std::pair;

using groupid_t = std::array<uint16_t, 4>;

namespace std
{
    template<> struct hash<groupid_t>
    {
#if PHMAP_HAVE_STD_STRING_VIEW
        std::size_t operator()(groupid_t const &g) const
        {
            const std::string_view bv{reinterpret_cast<const char*>(g.data()), sizeof(g)};
            return std::hash<std::string_view>()(bv);
        }
#else
        std::size_t operator()(groupid_t const &g) const
        {
            return phmap::Hash<decltype(std::tuple_cat(g))>()(std::tuple_cat(g));
        }
#endif
    };
}

int main()
{
    std::vector<groupid_t> groups = {
        {17, 75, 82, 66},
        {22, 88, 54, 42},
        {11, 55, 77, 99} };

    for (const auto &g : groups)
        std::cout << std::hash<groupid_t>()(g) << '\n';
    
    return 0;
}
