// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// llil4map.cc (new chunking variant)
// https://www.perlmonks.com/?node_id=11149643
// A phmap::parallel_flat_hash_map demonstration.
//    By Mario Roy, March 31, 2024
//    Based on llil3m.cpp  https://perlmonks.com/?node_id=11149482
//    Original challenge   https://perlmonks.com/?node_id=11147822
//         and summary     https://perlmonks.com/?node_id=11150293
//    Other demonstrations https://perlmonks.com/?node_id=11149907
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

// Specify 0/1 to use boost's parallel sorting algorithm; faster than __gnu_parallel::sort.
// https://www.boost.org/doc/libs/1_81_0/libs/sort/doc/html/sort/parallel.html
// This requires the boost header files: e.g. devpkg-boost bundle on Clear Linux.
// Note: Another option is downloading and unpacking Boost locally.
// (no need to build it because the bits we use are header file only)
#include <string_view>
#define USE_BOOST_PARALLEL_SORT 1

#include <chrono>
#include <thread>

#include <parallel_hashmap/phmap.h>

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <string>
#include <array>
#include <vector>

#include <utility>
#include <iterator>
#include <execution>
#include <algorithm>

#if USE_BOOST_PARALLEL_SORT > 0
#include <boost/sort/sort.hpp>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

static_assert(sizeof(size_t) == sizeof(int64_t), "size_t too small, need a 64-bit compile");

#include <atomic>
#include <cstddef>

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

// All words in big1.txt, big2.txt, big3.txt are <= 6 chars in length.
// big.txt  max word length is 6
// long.txt max word length is 208
//
// Based on rough benchmarking, the short fixed string hack below is only
// worth trying for MAX_STR_LEN_L up to about 30.
// See also https://backlinko.com/google-keyword-study
//
// To use (limited length) fixed length strings uncomment the next line.
#define MAX_STR_LEN_L (size_t) 12

#ifdef MAX_STR_LEN_L
struct str_type : std::array<char, MAX_STR_LEN_L> {
   bool operator==( const str_type& o ) const {
      return ::memcmp(this->data(), o.data(), MAX_STR_LEN_L) == 0;
   }
   bool operator<( const str_type& o ) const {
      return ::memcmp(this->data(), o.data(), MAX_STR_LEN_L) < 0;
   }
};
// inject specialization of std::hash for str_type into namespace std
namespace std {
   template<> struct hash<str_type> {
      std::size_t operator()( str_type const& v ) const noexcept {
         std::basic_string_view<char> bv {
            reinterpret_cast<const char*>(v.data()), v.size() * sizeof(char) };
         return std::hash<std::basic_string_view<char>>()(bv);
      }
   };
}
#else
using str_type = std::basic_string<char>;
#endif

using str_int_type     = std::pair<str_type, int_type>;
using vec_str_int_type = std::vector<str_int_type>;

// create the parallel_flat_hash_map with mutexes
using map_str_int_type = phmap::parallel_flat_hash_map<
   str_type, int_type,
   phmap::priv::hash_default_hash<str_type>,
   phmap::priv::hash_default_eq<str_type>,
   phmap::priv::Allocator<phmap::priv::Pair<const str_type, int_type>>,
   12,
   spinlock_mutex
>;

// Mimic the Perl get_properties subroutine ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// fast_atoll64
// https://stackoverflow.com/questions/16826422/
// c-most-efficient-way-to-convert-string-to-int-faster-than-atoi

inline int64_t fast_atoll64( const char* str )
{
   int64_t val  = 0;
   int     sign = 0;
   if (*str == '-') {
      sign = 1, ++str;
   }
   uint8_t digit;
   while ((digit = uint8_t(*str++ - '0')) <= 9)
      val = val * 10 + digit;
   return sign ? -val : val;
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
   const int          nthds,     // in    : the number of threads
   map_str_int_type&  map_ret)   // inout : the map to be updated
{
   int64_t num_lines = 0;
   std::ifstream fin(fname, std::ifstream::binary);
   if (!fin.is_open()) {
      std::cerr << "Error opening '" << fname << "' : " << strerror(errno) << '\n';
      return num_lines;
   }

   #pragma omp parallel reduction(+:num_lines)
   {
      std::string buf; buf.resize(CHUNK_SIZE + MAX_LINE_LEN + 1, '\0');
      char *first, *last, *found;
      size_t len, klen;
      int_type count;

      while (fin.good()) {
         len = 0;

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

         if (!len) break;
         buf[len] = '\0';
         first    = &buf[0];
         last     = &buf[len];

         // Process max Nthreads chunks concurrently.
         while (first < last) {
            char* beg_ptr{first};   first = find_char(first, last, '\n');
            char* end_ptr{first};   ++first, ++num_lines;

            if ((found = find_char(beg_ptr, end_ptr, '\t')) == end_ptr) continue;
            count = fast_atoll64(found + 1);

#ifdef MAX_STR_LEN_L
            str_type s {};  // {} initializes all elements of s to '\0'
            klen = std::min(MAX_STR_LEN_L, (size_t)(found - beg_ptr));
            ::memcpy(s.data(), beg_ptr, klen);
#else
            klen = found - beg_ptr;
            str_type s(beg_ptr, klen);
#endif
            // Use lazy_emplace to modify the map while the mutex is locked.
            map_ret.lazy_emplace_l(
               s,
               [&](map_str_int_type::value_type& p) {
                  // called only when key was already present
                  p.second += count;
               },
               [&](const map_str_int_type::constructor& ctor) {
                  // construct value_type in place when key not present
                  ctor(std::move(s), count);
               }
            );
         }
      }
   }

   fin.close();
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
   vec_str_int_type&  vec)       // in   : the vector to output
{
   size_t num_chunks = divide_up(vec.size(), CHUNK_SIZE);
#ifdef _OPENMP
   int nthds_out = std::min(nthds, 6);
#endif

   #pragma omp parallel for ordered schedule(static, 1) num_threads(nthds_out)
   for (size_t chunk_id = 1; chunk_id <= num_chunks; ++chunk_id) {
      std::string str(""); str.reserve(2048 * 1024);
      auto it  = vec.begin() + (chunk_id - 1) * CHUNK_SIZE;
      auto it2 = vec.begin() + std::min(vec.size(), chunk_id * CHUNK_SIZE);

      for (; it != it2; ++it) {
#ifdef MAX_STR_LEN_L
         str.append(it->first.data());
#else
         str.append(it->first.data(), it->first.size());
#endif
         str.append("\t", 1);
         str.append(std::to_string(it->second));
         str.append("\n", 1);
      }

      #pragma omp ordered
      std::cout << str << std::flush;
   }
}

typedef std::chrono::high_resolution_clock high_resolution_clock;
typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::milliseconds milliseconds;

double elaspe_time(
   time_point cend,
   time_point cstart)
{
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
#ifdef MAX_STR_LEN_L
   std::cerr << "llil4map (fixed string length=" << MAX_STR_LEN_L << ") start\n";
#else
   std::cerr << "llil4map start\n";
#endif
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
#endif

   // Get the list of input files from the command line
   int    nfiles = argc - 1;
   char** fname  = &argv[1];

   // Store the properties into a vector
   vec_str_int_type propvec;
   int64_t num_lines = 0;

   {
      // Enclose the map inside a block, so GC releases the object
      // immediately after exiting the scope.
      map_str_int_type map;

      for (int i = 0; i < nfiles; ++i)
         num_lines += get_properties(fname[i], nthds, map);

      cend1 = high_resolution_clock::now();
      double ctaken1 = elaspe_time(cend1, cstart1);
      std::cerr << "get properties      " << std::setw(8) << ctaken1 << " secs\n";

      if (map.size() == 0) {
         std::cerr << "No work, exiting...\n";
         return 1;
      }

      cstart2 = high_resolution_clock::now();

      if (nthds == 1) {
         propvec.reserve(map.size());
         for (auto const& x : map)
            propvec.emplace_back(std::move(x.first), x.second);
      }
      else {
         propvec.resize(map.size());
         std::vector<vec_str_int_type::iterator> I; I.resize(map.subcnt());
         I[0] = propvec.begin();
         size_t prev_num_keys;

         for (size_t i = 0; i < map.subcnt(); ++i) {
            map.with_submap(i, [&](const map_str_int_type::EmbeddedSet& set) {
               if (i == 0) {
                  prev_num_keys = set.size();
               } else {
                  I[i] = I[i-1] + prev_num_keys;
                  prev_num_keys = set.size();
               }
            });
         }

         #pragma omp parallel for schedule(static, 1) num_threads(nthds_move)
         for (size_t i = 0; i < map.subcnt(); ++i) {
            map.with_submap_m(i, [&](map_str_int_type::EmbeddedSet& set) {
               auto it = I[i];
               for (auto& x : set)
                  *it++ = std::make_pair(std::move(const_cast<str_type&>(x.first)), x.second);

               // reset the set (no longer needed) to reclaim memory early
               set = map_str_int_type::EmbeddedSet();
            });
         }
      }

      // phmap's clear() retains capacity until out of scope
      // swap map with an empty temporary, which is immediately destroyed
      // map.clear();
      map_str_int_type().swap(map);

      cend2 = high_resolution_clock::now();
      double ctaken2 = elaspe_time(cend2, cstart2);
      std::cerr << "map to vector       " << std::setw(8) << ctaken2 << " secs\n";
   }

   cstart3 = high_resolution_clock::now();

   // Sort the vector by (count) in reverse order, (name) in lexical order
#if USE_BOOST_PARALLEL_SORT == 0
   std::sort(
      // Standard sort
      propvec.begin(), propvec.end(),
      [](const str_int_type& left, const str_int_type& right) {
         return left.second != right.second
            ? left.second > right.second
            : left.first  < right.first;
      }
   );
#else
   boost::sort::block_indirect_sort(
      // Parallel sort
      propvec.begin(), propvec.end(),
      [](const str_int_type& left, const str_int_type& right) {
         return left.second != right.second
            ? left.second > right.second
            : left.first  < right.first;
      },
      std::min(nthds, 32)
   );
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