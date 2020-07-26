#include <chrono>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <random>
#include <parallel_hashmap/phmap.h>

// -------------------------------------------------------------------
// -------------------------------------------------------------------
class Timer
{
public:
    Timer(std::string name) : _name(name), _start(std::chrono::high_resolution_clock::now()) {}

    ~Timer() 
    {
        std::chrono::duration<float> elapsed_seconds = std::chrono::high_resolution_clock::now() - _start;
        printf("%s: %.3fs\n", _name.c_str(), elapsed_seconds.count());
    }

private:
    std::string _name;
    std::chrono::high_resolution_clock::time_point _start;
};

// --------------------------------------------------------------------------
//  from: https://github.com/preshing/RandomSequence
// --------------------------------------------------------------------------
class RSU
{
private:
    uint32_t m_index;
    uint32_t m_intermediateOffset;

    static uint32_t permuteQPR(uint32_t x)
    {
        static const uint32_t prime = 4294967291u;
        if (x >= prime)
            return x;  // The 5 integers out of range are mapped to themselves.
        uint32_t residue = ((unsigned long long) x * x) % prime;
        return (x <= prime / 2) ? residue : prime - residue;
    }

public:
    RSU(uint32_t seedBase, uint32_t seedOffset)
    {
        m_index = permuteQPR(permuteQPR(seedBase) + 0x682f0161);
        m_intermediateOffset = permuteQPR(permuteQPR(seedOffset) + 0x46790905);
    }

    uint32_t next()
    {
        return permuteQPR((permuteQPR(m_index++) + m_intermediateOffset) ^ 0x5bf03635);
    }
};

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
template<class Set, size_t N>
void test(const char *name, std::function<void (std::vector<uint64_t> &)> perturb)
{
    Set s;

    unsigned int seed = 76687;
	RSU rsu(seed, seed + 1);

    for (uint32_t i=0; i<N; ++i)
        s.insert(rsu.next());

    std::vector<uint64_t> order(s.begin(), s.end());
    perturb(order);
    order.resize(N/4);

    Timer t(name);
    Set c(order.begin(), order.end());
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
int main()
{
    auto shuffle = [](std::vector<uint64_t> &order) { 
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(order.begin(), order.end(), g); 
    };

    auto noop = [](std::vector<uint64_t> &) {};

    test<phmap::flat_hash_set<uint64_t>, 10000000>("ordered", noop);

    test<phmap::flat_hash_set<uint64_t>, 10000000>("shuffled", shuffle);

    test<phmap::parallel_flat_hash_set<uint64_t>, 10000000>("parallel ordered", noop);

    test<phmap::parallel_flat_hash_set<uint64_t>, 10000000>("parallel shuffled", shuffle);

}
    
    
    
    
