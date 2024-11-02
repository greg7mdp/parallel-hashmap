#include <boost/interprocess/managed_mapped_file.hpp>
#include <scoped_allocator>
#include <parallel_hashmap/phmap.h>
#include <vector>
#include <cstdint>
#include <iostream>

using mmap_file_t = boost::interprocess::managed_mapped_file;

template <typename T>
using bi_alloc_t = boost::interprocess::allocator<T, boost::interprocess::managed_mapped_file::segment_manager>;

template <typename T>
using scoped_alloc_t = std::scoped_allocator_adaptor<T>;

void simple_map() {
   struct LatpLon {
      int32_t latp;
      int32_t lon;
   };

   using nodestore_pair_t = std::pair<const uint64_t, LatpLon>;
   using map_t            = phmap::flat_hash_map<const uint64_t, LatpLon, std::hash<uint64_t>, std::equal_to<uint64_t>,
                                                 bi_alloc_t<nodestore_pair_t>>;

   auto mmap_file =
      boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, "map_iv.dat", 1000000);
   map_t* map = mmap_file.find_or_construct<map_t>("node_store")(mmap_file.get_segment_manager());

   for (unsigned int i = 0; i < 1000; ++i) {
      LatpLon p = {10, 10};
      map->emplace(i, p);
   }

   std::cout << map->at(10).latp << " " << map->at(10).lon << std::endl;
}

void scoped_map() {
   using way_t           = std::vector<uint64_t, bi_alloc_t<uint64_t>>;
   using waystore_pair_t = std::pair<const uint64_t, way_t>;
   using map_t           = phmap::flat_hash_map<const uint64_t, way_t, std::hash<uint64_t>, std::equal_to<uint64_t>,
                                                std::scoped_allocator_adaptor<bi_alloc_t<waystore_pair_t>>>;

   auto mmap_file =
      boost::interprocess::managed_mapped_file(boost::interprocess::open_or_create, "map_iv.dat", 1000000);
   map_t* map = mmap_file.find_or_construct<map_t>("ways_store")(mmap_file.get_segment_manager());

   for (unsigned int i = 0; i < 1000; ++i) {
      std::vector<uint64_t> init = {1, 2, 3, 4};
      map->emplace(std::piecewise_construct, std::forward_as_tuple(i), std::forward_as_tuple(init.begin(), init.end()));
   }

   std::cout << map->at(10).size() << std::endl;
   for (auto const& i : map->at(10))
      std::cout << i << " ";
   std::cout << std::endl;
}

int main()
{
    simple_map();
    scoped_map();
    return 0;
}