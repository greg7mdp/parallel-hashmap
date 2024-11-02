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
#include <bit>

#include <utility>
#include <iterator>
#include <algorithm>
#include <filesystem>

#if 1
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wshadow"

    #include <boost/sort/sort.hpp>

    #pragma clang diagnostic pop
#endif

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/lockfree/queue.hpp>

static_assert(sizeof(size_t) == sizeof(int64_t), "size_t too small, need a 64-bit compiler");

// ------------------------------------------------------------------------------------------
template <class F>
struct show_time {
   using high_resolution_clock = std::chrono::high_resolution_clock;
   using time_point            = std::chrono::high_resolution_clock::time_point;
   using milliseconds          = std::chrono::milliseconds;

   show_time(std::string_view message, F&& f) {
      auto start = high_resolution_clock::now();
      std::forward<F>(f)();
      auto elasped = double(std::chrono::duration_cast<milliseconds>(high_resolution_clock::now() - start).count()) / 1000;
      std::cerr << message << std::setw(8) << elasped << " secs\n";
   }
};



// ------------------------------------------------------------------------------------------
struct stats_t {
   using high_resolution_clock = std::chrono::high_resolution_clock;
   using time_point            = std::chrono::high_resolution_clock::time_point;
   using milliseconds          = std::chrono::milliseconds;

   struct time_pairs {
      double elasped() const { return double(std::chrono::duration_cast<milliseconds>(_stop - _start).count()) / 1000; }
      void start()           { _start = high_resolution_clock::now(); }
      void stop()            { _stop  = high_resolution_clock::now(); }
      //template <class F>
      //void show_time(std::string_view message, F&& f) {}

      time_point _start, _stop;
   };

   time_pairs get_props;
   time_pairs map_to_vec;
   time_pairs sort;
   time_pairs write_stdout;
   time_pairs total;
};

// ---------------------------------------------------------------------------------------------
// Stores a string + a count
// For strings up to N-1 bytes, total space used is N + 4 bytes
// For larger strings, uses N + 4  bytes + strlen(string) + 1
//
// invariants
//    if  extra[mark_idx], str is a valid string pointer
//    if !extra[mark_idx], the N bytes starting at (const char *)(&str) store a null_terminated string
// ---------------------------------------------------------------------------------------------
template<size_t N>
struct string_cnt_t {
   using uint_t = uint32_t;

   static_assert(N >= 12);
   static constexpr size_t buffsz   = N;
   static constexpr size_t extra_sz = N - sizeof(char*);
   static constexpr size_t mark_idx = extra_sz - 1;

   char *   str;
   char     extra[extra_sz];
   uint_t   cnt;

   string_cnt_t() : str{nullptr}, extra{0}, cnt{0} {}

   string_cnt_t(std::string_view s, uint_t c = 0) : str(nullptr), cnt(c) { set(s); }

   ~string_cnt_t() { free(); }

   string_cnt_t(const string_cnt_t& o) {
      set(o.get());
   }

   string_cnt_t(string_cnt_t&& o) noexcept {
      if (o.extra[mark_idx]) {
         str = o.str;
         o.str = nullptr;
         extra[mark_idx] = 1;
      } else {
         std::strcpy((char *)(&str), o.get());
         extra[mark_idx] = 0;
      }
      cnt = o.cnt;
   }

   string_cnt_t& operator=(const string_cnt_t& o) {
      free();
      set(o.get());
      cnt = o.cnt;
      return *this;
   }

   string_cnt_t& operator=(string_cnt_t&& o) noexcept {
      free();
      new (this) string_cnt_t(std::move(o));
      return *this;
   }

   std::strong_ordering operator<=>(const string_cnt_t& o) const { return std::strcmp(get(), o.get()) <=> 0; }
   bool operator==(const string_cnt_t& o) const { return std::strcmp(get(), o.get()) == 0; }

   std::size_t hash() const {
      auto s = get();
      std::string_view sv {s};
      return std::hash<std::string_view>()(sv);
   }

   const char *get() const { return extra[mark_idx] ? str : (const char *)(&str); }

private:
   void free() { if (extra[mark_idx]) { delete [] str; str = nullptr; } }

   void set(std::string_view s) {
      static_assert(offsetof(string_cnt_t, cnt) == (intptr_t)buffsz);
      static_assert(sizeof(string_cnt_t) == N + sizeof(cnt));

      assert(!extra[mark_idx] || !str);
      if (s.empty())
         std::memset(&str, 0, buffsz);
      else {
         auto len = s.size();
         if (len >= buffsz) {
            str = new char[len+1];
            std::memcpy(str, s.data(), len);
            str[len] = 0;
            extra[mark_idx] = 1;
         } else {
            std::memcpy((char *)(&str), s.data(), len);
            ((char *)&str)[len] = 0;
            extra[mark_idx] = 0;
         }
      }
   }

   void set(const char *s) {
      set(std::string_view{s});
   }
};

using string_cnt = string_cnt_t<12>;

namespace std {
   template<> struct hash<string_cnt> {
      std::size_t operator()(const string_cnt& v) const noexcept { return v.hash(); };
   };
}

namespace bip = boost::interprocess;
namespace lf  = boost::lockfree;

// ------------------------------------------------------------------------------------------
// ideas
// -----
// do a first pass that
//
//   - gather string length counts (each thread has an array `size_t lengths[256]` which are
//     merged at the end)
//     => enables to decide N for string_count (12, 20, 28, 36, 44) - chosen size should
//        be enough for 95% of strings
//        or
//        use `arena` allocator which doesn't free anything?
//
//   - gather statistics of first two starting letters (each thread has an array
//     `size_t count[65536]` which aremerged at the end)
//     => enables to divide equally between threads, and each thread holds the correctly
//        sorted bunch
//
//   - after extracting the vector on each thread, we can do `std::sort` and then output
//     the results directly from the sorted vectors of each thread (no need to combine into
//     large vector.
//
//   - blocks sent to consumer strings should not contain string_cnt, but blocks of:
//     <str><null><cnt>
//           1b    2b
//
//     and the `string_cnt` used to do the find() in the map should be pointing to the string
//     in the block (no copy), so no string alloc for duplicate strings
// ------------------------------------------------------------------------------------------
template<size_t num_consumers>
class llil_t {
public:
   using uint_t  = string_cnt::uint_t;
   static_assert(std::bit_ceil(num_consumers) == num_consumers);

   using string_cnt_set_t = phmap::parallel_flat_hash_set<
      string_cnt,
      phmap::priv::hash_default_hash<string_cnt>,
      phmap::priv::hash_default_eq<string_cnt>,
      phmap::priv::Allocator<string_cnt>,
      std::countr_zero(std::bit_ceil(num_consumers))
      >;

   using string_cnt_vector_t = std::vector<string_cnt>;
   using string_cnt_vector_t2 = std::array<string_cnt, 2>;

   struct consumer {
      consumer() : queue(10000) {}

      lf::queue<string_cnt_vector_t*> queue;
      std::thread                     thread;
      std::atomic<bool>               done {false};
   };

   string_cnt_set_t                    set;
   std::atomic<size_t>                 num_lines = 0;
   size_t                              num_unique;
   std::array<consumer, num_consumers> consumers;

   ~llil_t() {
      show_stats();
   }

   void get_properties(const char* fname) {
      auto mapping = bip::file_mapping(fname, bip::read_only);
      auto rgn = bip::mapped_region(mapping, bip::read_only);
      char* first = (char *)rgn.get_address();
      char* last = first + rgn.get_size();

      size_t _num_lines = 0;

      std::array<string_cnt_vector_t, num_consumers> vecs;
      constexpr size_t batch_size = 2048;
      for (auto& v : vecs)
         v.reserve(batch_size);

      while (first < last) {
         char* beg_ptr{first};
         char* end_ptr{find_char(first, last, '\n')};
         char *found = find_char(beg_ptr, end_ptr, '\t');
         if (found == end_ptr)
            continue;

         assert(*found == '\t');
         std::string_view word{beg_ptr, static_cast<size_t>(found - beg_ptr)};
         uint_t count = fast_atoui(found + 1, end_ptr);

         size_t hashval = std::hash<std::string_view>()(word);
         auto subidx = set.subidx(hashval);
         auto& v = vecs[subidx];
         v.emplace_back(word, count);

         if (v.size() == batch_size) {
            // enqueue vector to consumer thread
            enqueue_vec(std::move(v), subidx);
            v.clear();
            v.reserve(batch_size);
         }

         first = end_ptr + 1;
         ++_num_lines;
      }

      for (size_t i=0; i<vecs.size(); ++i)
         if (!vecs[i].empty())
            enqueue_vec(std::move(vecs[i]), i);

      num_lines += _num_lines;
   }

   template <size_t num_producers>
   void get_properties(char** fname, int nfiles) {
      std::vector<std::thread> producers;                 // produce blocks of word/count to add
      lf::queue<const char*>   file_queue(4096);          // queue of files to process
      std::atomic<bool>        done_adding_files {false}; // true when all files to process are enqueued

      producers.reserve(num_producers);

      for (size_t i=0; i<num_producers; ++i) {
         producers.emplace_back([&]() {
            const char* value;
            while (!done_adding_files) {
               while (file_queue.pop(value))
                  get_properties(value);
            }
            while (file_queue.pop(value))
               get_properties(value);
         });
      }

      for (int i = 0; i < nfiles; ++i) {
         while (!file_queue.push(fname[i]))
            ;
      }
      done_adding_files = true;

      // producers are now enqueueing blocks to process on the consumer queues
      // start the consumers
      for (size_t i=0; i<num_consumers; ++i) {
         consumer& c = consumers[i];

         c.thread = std::thread([&, i=i]() {
            string_cnt_vector_t* v;
            while (!c.done) {
               while (c.queue.pop(v))
                  process_vec(i, v);
            }
            while (c.queue.pop(v))
               process_vec(i, v);
         });
      }

      for (size_t i=0; i<num_producers; ++i)
         producers[i].join();

      for (auto& c : consumers)
         c.done = true;

      for (auto& c : consumers)
         c.thread.join();

      num_unique = set.size();
   }

   void show_stats() {
      std::cerr << "\n\n";
      std::cerr << "    count lines     " << num_lines << "\n";
      std::cerr << "    num uniques     " << num_unique << "\n";
   }

private:
   void enqueue_vec(string_cnt_vector_t&& v, size_t subidx) {
      auto v_ptr = new string_cnt_vector_t(std::move(v));
      while (!consumers[subidx].queue.push(v_ptr))
         ;
   }

   void process_vec(size_t subidx, string_cnt_vector_t* v) {
#if 1
      set.with_submap_m(subidx, [&](auto& s) {
         for (auto& sc : *v) {
            auto it = s.find(sc);
            if (it == s.end())
               s.insert(std::move(sc));
            else
               const_cast<string_cnt&>(*it).cnt +=  sc.cnt;
         }
      });
#endif
      delete v;
   }

   uint_t fast_atoui(const char* first, char* last) {
      uint_t val  = 0;
      uint8_t digit;
      while (first < last && (digit = uint8_t(*first++ - '0')) <= 9)
         val = val * 10 + digit;
      return val;
   }

   char* find_char(char* first, char* last, char c) {
      while (first != last) {
         if (*first == c) break;
         ++first;
      }
      return first;
   }

};



int main(int argc, char* argv[])
{
   if (argc < 2) { std::cerr << "usage: llil4map file1 file2 ... >out.txt\n";  return 1; }

   llil_t<32> llil;

   show_time("get properties      ", [&]() { llil.get_properties<6>(&argv[1], argc - 1); });

   return 0;
}