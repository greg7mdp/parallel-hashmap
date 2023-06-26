#include <iostream>
#include <string>
#include <array>
#include <cstdint>
#include <limits>
#include <random>
#include <utility>
#define PHMAP_ALLOCATOR_NOTHROW 1
#include <parallel_hashmap/phmap.h>

// this is probably the fastest high quality 64bit random number generator that exists.
// Implements Small Fast Counting v4 RNG from PractRand.
class sfc64 {
public:
    using result_type = uint64_t;

    // no copy ctors so we don't accidentally get the same random again
    sfc64(sfc64 const&) = delete;
    sfc64& operator=(sfc64 const&) = delete;

    sfc64(sfc64&&) = default;
    sfc64& operator=(sfc64&&) = default;

    sfc64(std::array<uint64_t, 4> const& _state)
        : m_a(_state[0])
        , m_b(_state[1])
        , m_c(_state[2])
        , m_counter(_state[3]) {}

    static constexpr uint64_t(min)() {
        return (std::numeric_limits<uint64_t>::min)();
    }
    static constexpr uint64_t(max)() {
        return (std::numeric_limits<uint64_t>::max)();
    }

    sfc64()
        : sfc64(UINT64_C(0x853c49e6748fea9b)) {}

    sfc64(uint64_t _seed)
        : m_a(_seed)
        , m_b(_seed)
        , m_c(_seed)
        , m_counter(1) {
        for (int i = 0; i < 12; ++i) {
            operator()();
        }
    }

    void seed() {
        *this = sfc64{std::random_device{}()};
    }

    uint64_t operator()() noexcept {
        auto const tmp = m_a + m_b + m_counter++;
        m_a = m_b ^ (m_b >> right_shift);
        m_b = m_c + (m_c << left_shift);
        m_c = rotl(m_c, rotation) + tmp;
        return tmp;
    }

    std::array<uint64_t, 4> state() const {
        return {{m_a, m_b, m_c, m_counter}};
    }

    void state(std::array<uint64_t, 4> const& s) {
        m_a = s[0];
        m_b = s[1];
        m_c = s[2];
        m_counter = s[3];
    }

private:
    template <typename T>
    T rotl(T const x, int k) {
        return (x << k) | (x >> (8 * sizeof(T) - k));
    }

    static constexpr int rotation = 24;
    static constexpr int right_shift = 11;
    static constexpr int left_shift = 3;
    uint64_t m_a;
    uint64_t m_b;
    uint64_t m_c;
    uint64_t m_counter;
};

static inline std::string to_str(uint64_t x) {
    std::string res(4, '1');
    x = (x >> 48) ^ (x >> 32) ^ (x >> 16) ^ x; // combine 64 bits > 16 lsb
    for (size_t i=0; i<4; ++i) {
        res[i] = 'a' + (x & 0xF);
        x >>= 4;
    }
    return res;
}

int main()
{
    using Map = phmap::flat_hash_map<std::string, uint32_t>;
    Map map;
    map.reserve((size_t)(65536 * 1.1)); // we will create a maximun of 65536 different strings

    sfc64 rng(123);
    constexpr size_t const n = 50000000;
    for (size_t i = 0; i < n; ++i) {
        auto s = to_str(rng());
        map[s]++;
        map[s]++;
        map[s]++;
        map[s]++;
        map[s]++; 
        map[s]++;
        map[s]++;
        map[s]++;
        map[s]++;
        map[s]++;
    }

    uint64_t cnt = 0;
    for (const auto& s : map) {
        if (++cnt == 6) break;
        std::cout << s.first << ": " << s.second << '\n';
    }
    
    return 0;
}
