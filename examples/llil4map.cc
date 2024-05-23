// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// llil4map.cc (new chunking variant)
// https://www.perlmonks.com/?node_id=11149643
// A phmap::parallel_flat_hash_map demonstration.
//    By Mario Roy, March 31, 2024
//    Based on llil3m.cpp  https://perlmonks.com/?node_id=11149482
//    Original challenge   https://perlmonks.com/?node_id=11147822
//         and summary     https://perlmonks.com/?node_id=11150293
//    Other demonstrations https://perlmonks.com/?node_id=11149907
//
// Further changes after original version in this repository by Gregory Popovitch
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OpenMP Little Book - https://nanxiao.gitbooks.io/openmp-little-book
//
// Obtain the parallel hashmap library (required dependency):
//    git clone --depth=1 https://github.com/greg7mdp/parallel-hashmap
//
// Compile on Linux (clang++ or g++):
//    clang++ -o llil4map -std=c++20 -fopenmp -Wall -O3 llil4map.cc -I./parallel-hashmap
//
// On macOS, use g++-12 from https://brew.sh (installation: brew install gcc@12).
// The g++ command also works with mingw C++ compiler (https://sourceforge.net/projects/mingw-w64)
// that comes bundled with Strawberry Perl (C:\Strawberry\c\bin\g++.exe).
//
// Obtain gen-llil.pl and gen-long-llil.pl from https://perlmonks.com/?node_id=11148681
//    perl gen-llil.pl big1.txt 200 3 1
//    perl gen-llil.pl big2.txt 200 3 1
//    perl gen-llil.pl big3.txt 200 3 1
//    perl gen-long-llil.pl long1.txt 600
//    perl gen-long-llil.pl long2.txt 600
//    perl gen-long-llil.pl long3.txt 600
//
// To make random input, obtain shuffle.pl from https://perlmonks.com/?node_id=11149800
//    perl shuffle.pl big1.txt >tmp && mv tmp big1.txt
//    perl shuffle.pl big2.txt >tmp && mv tmp big2.txt
//    perl shuffle.pl big3.txt >tmp && mv tmp big3.txt
//
// Example run:  llil4map big1.txt big2.txt big3.txt >out.txt
// NUM_THREADS=3 llil4map ...
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <compare>
#include <chrono>

#include <string>
#include <string_view>

#include <array>
#include <vector>

#include <thread>
#include <execution>
#include <atomic>

#include <iostream>
#include <iomanip>
#include <fstream>

#include <parallel_hashmap/phmap.h>

static_assert(sizeof(size_t) == sizeof(int64_t), "size_t too small, need a 64-bit compile");

// Specify 0/1 to use boost's parallel sorting algorithm; faster than __gnu_parallel::sort.
// https://www.boost.org/doc/libs/1_81_0/libs/sort/doc/html/sort/parallel.html
// This requires the boost header files: e.g. devpkg-boost bundle on Clear Linux.
// Note: Another option is downloading and unpacking Boost locally.
// (no need to build it because the bits we use are header file only)
#define USE_BOOST_PARALLEL_SORT 1

#if USE_BOOST_PARALLEL_SORT
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wshadow"

    #include <boost/sort/sort.hpp>

    #pragma clang diagnostic pop
#endif

#ifdef _OPENMP
    #include <omp.h>
#endif


class spinlock_mutex {
   // https://rigtorp.se/spinlock/
   // https://vorbrodt.blog/2019/02/12/fast-mutex/
public:
   void lock() noexcept {
      for (;;) {
         if (!lock_.exchange(true, std::memory_order_acquire))
            break;
         while (lock_.load(std::memory_order_relaxed))
            __builtin_ia32_pause();
      }
   }
   void unlock() noexcept {
      lock_.store(false, std::memory_order_release);
   }
private:
   alignas(4 * sizeof(std::max_align_t)) std::atomic_bool lock_ = false;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef uint32_t int_type;

// ---------------------------------------------------------------------------------------------
// Stores a string + a count
// For strings up to 11 bytes, total space used is 16 bytes (no wasted space in set)
// For larger strings, uses 16 bytes + strlen(string) + 1
//
// invariants
//    if extra[3], str is a valid string pointer
//    if !extra[3], the 12 bytes starting at (const char *)(&str) store a null_terminated string
// ---------------------------------------------------------------------------------------------
struct string_cnt {
   char *    str;
   char      extra[4];
   uint32_t  cnt;

   static constexpr size_t buffsz = sizeof(str) + sizeof(extra);

   string_cnt() : str{nullptr}, extra{0,0,0,0}, cnt{0} {}

   string_cnt(const char *s, uint32_t c = 0) : str(nullptr), cnt(c) { set(s); }

   ~string_cnt() { free(); }

   string_cnt(const string_cnt& o) {
      set(o.get());
   }

   string_cnt(string_cnt&& o) noexcept {
      if (o.extra[3]) {
         str = o.str;
         o.str = nullptr;
         extra[3] = 1;
      } else {
         std::strcpy((char *)(&str), o.get());
         extra[3] = 0;
      }
      cnt = o.cnt;
   }

   string_cnt& operator=(const string_cnt& o) {
      free();
      set(o.get());
      cnt = o.cnt;
      return *this;
   }

   string_cnt& operator=(string_cnt&& o) noexcept {
      free();
      new (this) string_cnt(std::move(o));
      return *this;
   }

   std::strong_ordering operator<=>(const string_cnt& o) const { return std::strcmp(get(), o.get()) <=> 0; }
   bool operator==(const string_cnt& o) const { return std::strcmp(get(), o.get()) == 0; }

   std::size_t hash() const {
      auto s = get();
      std::string_view sv { s, strlen(s) };
      return std::hash<std::string_view>()(sv);
   }

   const char *get() const { return extra[3] ? str : (const char *)(&str); }

private:
   void free() { if (extra[3]) { delete [] str; str = nullptr; } }

   void set(const char *s) {
      static_assert(buffsz == 12);
      static_assert(offsetof(string_cnt, cnt) == (intptr_t)buffsz);
      static_assert(sizeof(string_cnt) == 16);

      assert(!extra[3] || !str);
      if (!s)
         std::memset(&str, 0, buffsz);
      else {
         auto len = std::strlen(s);
         if (len >= buffsz) {
            str = new char[len+1];
            std::strcpy(str, s);
            extra[3] = 1;
         } else {
            std::strcpy((char *)(&str), s);
            extra[3] = 0;
         }
      }
   }
};

namespace std {
   template<> struct hash<string_cnt> {
      std::size_t operator()(const string_cnt& v) const noexcept { return v.hash(); };
   };
}

using string_cnt_vector_t = std::vector<string_cnt>;

// declare the type of the parallel_flat_hash_set with spinlock mutexes
using string_cnt_set_t = phmap::parallel_flat_hash_set<
   string_cnt,
   phmap::priv::hash_default_hash<string_cnt>,
   phmap::priv::hash_default_eq<string_cnt>,
   phmap::priv::Allocator<string_cnt>,
   12,
   spinlock_mutex
>;

// Mimic the Perl get_properties subroutine ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// convert positive number from string to uint32_t
inline uint32_t fast_atoll64(const char* str)
{
   uint32_t val  = 0;
   uint8_t digit;
   while ((digit = uint8_t(*str++ - '0')) <= 9)
      val = val * 10 + digit;
   return val;
}

// Helper function to find a character.
inline char* find_char(char* first, char* last, char c)
{
   while (first != last) {
      if (*first == c) break;
      ++first;
   }
   return first;
}

// Limit chunk size and line length.
inline constexpr size_t CHUNK_SIZE   = 32768;
inline constexpr size_t MAX_LINE_LEN = 255;

static int64_t get_properties(
   const char*        fname,     // in    : the input file name
   string_cnt_set_t&  set_ret)   // inout : the set to be updated
{
   int64_t num_lines = 0;
   std::ifstream fin(fname, std::ifstream::binary);
   if (!fin.is_open()) {
      std::cerr << "Error opening '" << fname << "' : " << strerror(errno) << '\n';
      return num_lines;
   }

   #pragma omp parallel reduction(+:num_lines)
   {
      std::string buf;
      buf.resize(CHUNK_SIZE + MAX_LINE_LEN + 1, '\0');

      while (fin.good()) {
         size_t len = 0;

         // Read the next chunk serially.
         #pragma omp critical
         {
            fin.read(&buf[0], CHUNK_SIZE);
            if ((len = fin.gcount()) > 0) {
               if (buf[len - 1] != '\n' && fin.getline(&buf[len], MAX_LINE_LEN)) {
                  // Getline discards the newline char and appends null char.
                  // Therefore, change '\0' to '\n'.
                  len += fin.gcount();
                  buf[len - 1] = '\n';
               }
            }
         }

         if (!len)
            break;

         buf[len]    = '\0';
         char *first = &buf[0];
         char *last  = &buf[len];

         // Process max Nthreads chunks concurrently.
         while (first < last) {
            char* beg_ptr{first};
            char* end_ptr{find_char(first, last, '\n')};
            char *found = find_char(beg_ptr, end_ptr, '\t');
            if (found == end_ptr)
               continue;

            assert(*found == '\t');
            *found = 0; // replace the tab with '\0'

            int_type count = fast_atoll64(found + 1);

            // Use lazy_emplace to modify the set while the mutex is locked.
            set_ret.lazy_emplace_l(
               beg_ptr,
               [&](string_cnt_set_t::value_type& p) {
                  p.cnt += count;       // called only when key was already present
               },
               [&](const string_cnt_set_t::constructor& ctor) {
                  ctor(beg_ptr, count); // construct value_type in place when key not present
               }
            );

            first = end_ptr + 1;
            ++num_lines;
         }
      }
   }

   fin.close();
   // std::cerr << "getprops done\n";
   return num_lines;
}

// Output subroutine ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

size_t divide_up(size_t dividend, size_t divisor)
{
   if (dividend % divisor)
      return (size_t)(dividend / divisor) + 1;
   else
      return (size_t)(dividend / divisor);
}

static void out_properties(
   const int          nthds,     // in   : the number of threads
   string_cnt_vector_t&  vec)       // in   : the vector to output
{
   size_t num_chunks = divide_up(vec.size(), CHUNK_SIZE);
   int nthds_out = 1;
#ifdef _OPENMP
   nthds_out = std::min(nthds, 6);
#endif

   #pragma omp parallel for ordered schedule(static, 1) num_threads(nthds_out)
   for (size_t chunk_id = 1; chunk_id <= num_chunks; ++chunk_id) {
      std::string str(""); str.reserve(2048 * 1024);
      auto it  = vec.begin() + (chunk_id - 1) * CHUNK_SIZE;
      auto it2 = vec.begin() + std::min(vec.size(), chunk_id * CHUNK_SIZE);

      for (; it != it2; ++it) {
         str.append(it->get());
         str.append("\t", 1);
         str.append(std::to_string(it->cnt));
         str.append("\n", 1);
      }

      #pragma omp ordered
      std::cout << str << std::flush;
   }
}

typedef std::chrono::high_resolution_clock high_resolution_clock;
typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::milliseconds milliseconds;

double elaspe_time(time_point cend, time_point cstart) {
   return double (
      std::chrono::duration_cast<milliseconds>(cend - cstart).count()
   ) * 1e-3;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char* argv[])
{
   if (argc < 2) {
      if (argc > 0)
         std::cerr << "usage: llil4map file1 file2 ... >out.txt\n";
      return 1;
   }

   std::cerr << std::setprecision(3) << std::setiosflags(std::ios::fixed);
   std::cerr << "llil4map start\n";
#ifdef _OPENMP
   std::cerr << "use OpenMP\n";
#else
   std::cerr << "don't use OpenMP\n";
#endif
#if USE_BOOST_PARALLEL_SORT == 0
   std::cerr << "don't use boost sort\n";
#else
   std::cerr << "use boost sort\n";
#endif
   time_point cstart1, cend1, cstart2, cend2, cstart3, cend3s, cend3;
   cstart1 = high_resolution_clock::now();

#ifdef _OPENMP
   // Determine the number of threads.
   const char* env_nthds = std::getenv("NUM_THREADS");
   int nthds = ( env_nthds && strlen(env_nthds) )
      ? ::atoi(env_nthds)
      : std::thread::hardware_concurrency();
   omp_set_dynamic(false);
   omp_set_num_threads(nthds);
   omp_set_max_active_levels(1);
   int nthds_move = std::min(nthds, 6);
#else
   int nthds = 1;
   int nthds_move = 1;
#endif

   // Get the list of input files from the command line
   int    nfiles = argc - 1;
   char** fname  = &argv[1];

   // Store the properties into a vector
   string_cnt_vector_t propvec;
   int64_t num_lines = 0;

   {
      // Enclose the set inside a block, so GC releases the object
      // immediately after exiting the scope.
      string_cnt_set_t set;

      for (int i = 0; i < nfiles; ++i)
         num_lines += get_properties(fname[i], set);

      cend1 = high_resolution_clock::now();
      double ctaken1 = elaspe_time(cend1, cstart1);
      std::cerr << "get properties      " << std::setw(8) << ctaken1 << " secs\n";

      if (set.size() == 0) {
         std::cerr << "No work, exiting...\n";
         return 1;
      }

      cstart2 = high_resolution_clock::now();

      if (nthds == 1) {
         propvec.reserve(set.size());
         for (auto& x : set)
            propvec.push_back(std::move(const_cast<string_cnt&>(x)));
         set.clear();
      }
      else {
         propvec.resize(set.size());
         std::vector<string_cnt_vector_t::iterator> I(set.subcnt());
         auto cur = propvec.begin();

         for (size_t i = 0; i < set.subcnt(); ++i) {
            set.with_submap(i, [&](const string_cnt_set_t::EmbeddedSet& set) {
               I[i] = cur;
               cur += set.size();
            });
         }

         #pragma omp parallel for schedule(static, 1) num_threads(nthds_move)
         for (size_t i = 0; i < set.subcnt(); ++i) {
            set.with_submap_m(i, [&](string_cnt_set_t::EmbeddedSet& set) {
               auto it = I[i];
               for (auto& x : set)
                  *it++ = std::move(const_cast<string_cnt&>(x)); // force move

               // reset the set (no longer needed) to reclaim memory early
               set = string_cnt_set_t::EmbeddedSet();
            });
         }
      }

      cend2 = high_resolution_clock::now();
      double ctaken2 = elaspe_time(cend2, cstart2);
      std::cerr << "map to vector       " << std::setw(8) << ctaken2 << " secs\n";
   }

   cstart3 = high_resolution_clock::now();

   // Sort the vector by (count) in reverse order, (name) in lexical order
   auto reverse_order = [](const string_cnt& left, const string_cnt& right) {
      return left.cnt != right.cnt ? left.cnt > right.cnt : left < right;
   };
#if USE_BOOST_PARALLEL_SORT == 0
   // Standard sort
   std::sort(propvec.begin(), propvec.end(), reverse_order);
#else
   // Parallel sort
   boost::sort::block_indirect_sort(propvec.begin(), propvec.end(), reverse_order, nthds);
#endif

   cend3s = high_resolution_clock::now();

   // Output the sorted vector
   out_properties(nthds, propvec);
   cend3 = high_resolution_clock::now();

   double ctaken   = elaspe_time(cend3, cstart1);
   double ctaken3s = elaspe_time(cend3s, cstart3);
   double ctaken3o = elaspe_time(cend3, cend3s);

   std::cerr << "vector stable sort  " << std::setw(8) << ctaken3s << " secs\n";
   std::cerr << "write stdout        " << std::setw(8) << ctaken3o << " secs\n";
   std::cerr << "total time          " << std::setw(8) << ctaken   << " secs\n";
   std::cerr << "    count lines     " << num_lines << "\n";
   std::cerr << "    count unique    " << propvec.size() << "\n";

   // Hack to see Private Bytes in Windows Task Manager
   // (uncomment next line so process doesn't exit too quickly)
   // std::this_thread::sleep_for(milliseconds(9000));

   return 0;
}