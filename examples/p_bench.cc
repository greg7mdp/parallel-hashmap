// example graciously provided @samuelpmish
// ----------------------------------------
//
// Getting rid of the mutexes for read access
//
// This example demonstrated how to populate a parallel_flat_hash_map from multiple
// concurrent threads (The map is protected by internal mutexes), but then doing a
// swap to get rid of the mutexes (and all locking) for accessing the same hash_map
// in `read` only mode, again concurrently from multiple threads.
// --------------------------------------------------------------------------------
#include <random>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "parallel_hashmap/phmap.h"

///////////////////////////////////////////////////////////////////////////////

#include <chrono>

class timer {
    typedef std::chrono::high_resolution_clock::time_point time_point;
    typedef std::chrono::duration<double>                  duration_type;

public:
    void   start()   { then = std::chrono::high_resolution_clock::now(); }
    void   stop()    { now = std::chrono::high_resolution_clock::now(); }
    double elapsed() { return std::chrono::duration_cast<duration_type>(now - then).count(); }

private:
    time_point then, now;
};  
  
///////////////////////////////////////////////////////////////////////////////

#include <thread>

struct threadpool {

    std::vector< uint64_t > partition(uint64_t n) {
        uint64_t quotient = n / num_threads;
        uint64_t remainder = n % num_threads;

        std::vector< uint64_t > blocks(num_threads + 1);
        blocks[0] = 0;
        for (int i = 1; i < num_threads + 1; i++) {
            if (remainder > 0) {
                blocks[i] = blocks[i-1] + quotient + 1;
                remainder--;
            } else {
                blocks[i] = blocks[i-1] + quotient;
            }
        }
        return blocks;
    }

    threadpool(int n) : num_threads(n) {}

    template < typename lambda >
    void parallel_for(uint64_t n, const lambda & f) {

        std::vector< uint64_t > blocks = partition(n);

        for (int tid = 0; tid < num_threads; tid++) {
            threads.push_back(std::thread([&](uint64_t i0) {
                for (uint64_t i = blocks[i0]; i < blocks[i0+1]; i++) {
                    f(i);
                }
            }, tid));
        }

        for (int i = 0; i < num_threads; i++) {
            threads[i].join();
        }

        threads.clear();
    }

    int num_threads;
    std::vector< std::thread > threads;
};
 
///////////////////////////////////////////////////////////////////////////////
 
template < int n >
using pmap = phmap::parallel_flat_hash_map< 
                uint64_t, 
                uint64_t,
                std::hash<uint64_t>,
                std::equal_to<uint64_t>, 
                std::allocator<std::pair<const uint64_t, uint64_t>>, 
                n, 
                std::mutex >;

template < int n >
using pmap_nullmutex = phmap::parallel_flat_hash_map< 
                uint64_t, 
                uint64_t,
                std::hash<uint64_t>,
                std::equal_to<uint64_t>, 
                std::allocator<std::pair<const uint64_t, uint64_t>>, 
                n, 
                phmap::NullMutex >;



template < typename Map, typename Map_nomutex >
void renumber(const std::vector< uint64_t > & vertex_ids, 
              std::vector< std::array< uint64_t, 4 > > elements,
              int num_threads) {
    bool supports_parallel_insertion = 
        !std::is_same< Map, std::unordered_map<uint64_t, uint64_t> >::value;

    Map new_ids;
    std::atomic< uint64_t > new_id{ 0 };

    timer stopwatch;
    threadpool pool((supports_parallel_insertion) ? num_threads : 1);

    stopwatch.start();
    new_ids.reserve(vertex_ids.size() * 110 / 100);
    pool.parallel_for(vertex_ids.size(), [&](uint64_t i){
        auto id = new_id++;
        new_ids[vertex_ids[i]] = id;
    });
    stopwatch.stop();
    std::cout << stopwatch.elapsed() * 1000 << "ms ";

    pool.num_threads = num_threads;
    stopwatch.start();
    Map_nomutex new_ids_nc;
    new_ids_nc.swap(new_ids);
    pool.parallel_for(elements.size(), [&](uint64_t i) {
        auto & elem = elements[i];
        elem = {  new_ids_nc.at(elem[0]),
                  new_ids_nc.at(elem[1]),
                  new_ids_nc.at(elem[2]),
                  new_ids_nc.at(elem[3]) };
    });
    stopwatch.stop();
    std::cout << stopwatch.elapsed() * 1000 << "ms" << std::endl;
}

int main() {
    uint64_t nvertices = 5000000;
    uint64_t nelements = 25000000;

    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<uint64_t> vertex_id_dist(0, uint64_t(1) << 35);
    std::uniform_int_distribution<uint64_t> elem_id_dist(0, nvertices-1);

    std::cout << "generating dataset ." << std::flush;
    std::vector< uint64_t > vertex_ids(nvertices);
    for (uint64_t i = 0; i < nvertices; i++) {
        vertex_ids[i] = vertex_id_dist(gen);
    }

    std::cout << "." << std::flush;

    std::vector< std::array<uint64_t, 4> > elements(nelements);
    for (uint64_t i = 0; i < nelements; i++) {
        elements[i] = {
            vertex_ids[elem_id_dist(gen)], 
            vertex_ids[elem_id_dist(gen)], 
            vertex_ids[elem_id_dist(gen)], 
            vertex_ids[elem_id_dist(gen)]
        };
    }
    std::cout << " done" << std::endl;

    using stdmap = std::unordered_map<uint64_t, uint64_t>;
    std::cout << "std::unordered_map, 1 thread: ";
    renumber< stdmap, stdmap >(vertex_ids, elements, 1);

    std::cout << "std::unordered_map, 32 thread (single threaded insertion): ";
    renumber< stdmap, stdmap >(vertex_ids, elements, 32);

    std::cout << "pmap4, 1 thread: ";
    renumber< pmap<4>, pmap_nullmutex<4> >(vertex_ids, elements, 1);

    std::cout << "pmap4, 32 threads: ";
    renumber< pmap<4>, pmap_nullmutex<4> >(vertex_ids, elements, 32);

    std::cout << "pmap6, 1 thread: ";
    renumber< pmap<6>, pmap_nullmutex<6> >(vertex_ids, elements, 1);

    std::cout << "pmap6, 32 threads: ";
    renumber< pmap<6>, pmap_nullmutex<6> >(vertex_ids, elements, 32);
}