// ------------------------
// Windows specific example 
// curtesy of @kanonka
// ------------------------
#include <windows.h>
#include "parallel_hashmap/phmap.h"
#include <cstring>
#include <vector>
#include <ppl.h>

class srwlock {
    SRWLOCK _lock;

public:
    srwlock()     { InitializeSRWLock(&_lock); }
    void lock()   { AcquireSRWLockExclusive(&_lock); }
    void unlock() { ReleaseSRWLockExclusive(&_lock); }
};

using Map =  phmap::parallel_flat_hash_map<std::string, int, phmap::priv::hash_default_hash<std::string>,
                                           phmap::priv::hash_default_eq<std::string>,
                                           std::allocator<std::pair<const std::string, int>>, 8, srwlock>;

class Dict
{
    Map m_stringsMap;

public:
    int addParallel(std::string&& str, volatile long* curIdx)
    {
        int newIndex = -1;
        m_stringsMap.lazy_emplace_l(std::move(str),
                                    [&](Map::value_type& p) { newIndex = p.second; },  // called only when key was already present
                                    [&](const Map::constructor& ctor) // construct value_type in place when key not present
                                        { newIndex = InterlockedIncrement(curIdx); ctor(std::move(str), newIndex); }); 

        return newIndex;
    }
};

int main()
{
    size_t totalSize = 6000000;
    std::vector<int> values(totalSize);
    Dict dict;
    volatile long index = 0;
    concurrency::parallel_for(size_t(0), size_t(totalSize), 
        [&](size_t i) {
            std::string s = "ab_uu_" + std::to_string(i % 1000000);
            values[i] = dict.addParallel(std::move(s), &index);
        });

    return 0;
}
