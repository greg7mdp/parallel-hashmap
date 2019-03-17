#if !defined(phmap_bits_h_guard_)
#define phmap_bits_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Includes work from abseil-cpp (https://github.com/abseil/abseil-cpp)
// with modifications.
// 
// Copyright 2018 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------

// The following guarantees declaration of the byte swap functions
#ifdef _MSC_VER
    #include <stdlib.h>  // NOLINT(build/include)
#elif defined(__APPLE__)
    // Mac OS X / Darwin features
    #include <libkern/OSByteOrder.h>
#elif defined(__FreeBSD__)
    #include <sys/endian.h>
#elif defined(__GLIBC__)
    #include <byteswap.h>  // IWYU pragma: export
#endif

#include <string.h>
#include <cstdint>
#include "phmap_config.h"

// -----------------------------------------------------------------------------
// unaligned APIs
// -----------------------------------------------------------------------------
// Portable handling of unaligned loads, stores, and copies.
// On some platforms, like ARM, the copy functions can be more efficient
// then a load and a store.
// -----------------------------------------------------------------------------

#if defined(ADDRESS_SANITIZER) || defined(THREAD_SANITIZER) ||\
    defined(MEMORY_SANITIZER)
#include <stdint.h>

extern "C" {
    uint16_t __sanitizer_unaligned_load16(const void *p);
    uint32_t __sanitizer_unaligned_load32(const void *p);
    uint64_t __sanitizer_unaligned_load64(const void *p);
    void __sanitizer_unaligned_store16(void *p, uint16_t v);
    void __sanitizer_unaligned_store32(void *p, uint32_t v);
    void __sanitizer_unaligned_store64(void *p, uint64_t v);
}  // extern "C"

namespace phmap {
namespace bits {

inline uint16_t UnalignedLoad16(const void *p) {
  return __sanitizer_unaligned_load16(p);
}

inline uint32_t UnalignedLoad32(const void *p) {
  return __sanitizer_unaligned_load32(p);
}

inline uint64_t UnalignedLoad64(const void *p) {
  return __sanitizer_unaligned_load64(p);
}

inline void UnalignedStore16(void *p, uint16_t v) {
  __sanitizer_unaligned_store16(p, v);
}

inline void UnalignedStore32(void *p, uint32_t v) {
  __sanitizer_unaligned_store32(p, v);
}

inline void UnalignedStore64(void *p, uint64_t v) {
  __sanitizer_unaligned_store64(p, v);
}

}  // namespace bits
}  // namespace phmap

#define PHMAP_INTERNAL_UNALIGNED_LOAD16(_p) (phmap::bits::UnalignedLoad16(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD32(_p) (phmap::bits::UnalignedLoad32(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD64(_p) (phmap::bits::UnalignedLoad64(_p))

#define PHMAP_INTERNAL_UNALIGNED_STORE16(_p, _val) (phmap::bits::UnalignedStore16(_p, _val))
#define PHMAP_INTERNAL_UNALIGNED_STORE32(_p, _val) (phmap::bits::UnalignedStore32(_p, _val))
#define PHMAP_INTERNAL_UNALIGNED_STORE64(_p, _val) (phmap::bits::UnalignedStore64(_p, _val))

#elif defined(UNDEFINED_BEHAVIOR_SANITIZER)

namespace phmap {
namespace bits {

inline uint16_t UnalignedLoad16(const void *p) {
  uint16_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint32_t UnalignedLoad32(const void *p) {
  uint32_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint64_t UnalignedLoad64(const void *p) {
  uint64_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline void UnalignedStore16(void *p, uint16_t v) { memcpy(p, &v, sizeof v); }

inline void UnalignedStore32(void *p, uint32_t v) { memcpy(p, &v, sizeof v); }

inline void UnalignedStore64(void *p, uint64_t v) { memcpy(p, &v, sizeof v); }

}  // namespace bits
}  // namespace phmap

#define PHMAP_INTERNAL_UNALIGNED_LOAD16(_p) \
  (phmap::bits::UnalignedLoad16(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD32(_p) \
  (phmap::bits::UnalignedLoad32(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD64(_p) \
  (phmap::bits::UnalignedLoad64(_p))

#define PHMAP_INTERNAL_UNALIGNED_STORE16(_p, _val) \
  (phmap::bits::UnalignedStore16(_p, _val))
#define PHMAP_INTERNAL_UNALIGNED_STORE32(_p, _val) \
  (phmap::bits::UnalignedStore32(_p, _val))
#define PHMAP_INTERNAL_UNALIGNED_STORE64(_p, _val) \
  (phmap::bits::UnalignedStore64(_p, _val))

#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386) || \
    defined(_M_IX86) || defined(__ppc__) || defined(__PPC__) ||    \
    defined(__ppc64__) || defined(__PPC64__)

// x86 and x86-64 can perform unaligned loads/stores directly;
// modern PowerPC hardware can also do unaligned integer loads and stores;
// but note: the FPU still sends unaligned loads and stores to a trap handler!

#define PHMAP_INTERNAL_UNALIGNED_LOAD16(_p) \
  (*reinterpret_cast<const uint16_t *>(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD32(_p) \
  (*reinterpret_cast<const uint32_t *>(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD64(_p) \
  (*reinterpret_cast<const uint64_t *>(_p))

#define PHMAP_INTERNAL_UNALIGNED_STORE16(_p, _val) \
  (*reinterpret_cast<uint16_t *>(_p) = (_val))
#define PHMAP_INTERNAL_UNALIGNED_STORE32(_p, _val) \
  (*reinterpret_cast<uint32_t *>(_p) = (_val))
#define PHMAP_INTERNAL_UNALIGNED_STORE64(_p, _val) \
  (*reinterpret_cast<uint64_t *>(_p) = (_val))

#elif defined(__arm__) && \
      !defined(__ARM_ARCH_5__) && \
      !defined(__ARM_ARCH_5T__) && \
      !defined(__ARM_ARCH_5TE__) && \
      !defined(__ARM_ARCH_5TEJ__) && \
      !defined(__ARM_ARCH_6__) && \
      !defined(__ARM_ARCH_6J__) && \
      !defined(__ARM_ARCH_6K__) && \
      !defined(__ARM_ARCH_6Z__) && \
      !defined(__ARM_ARCH_6ZK__) && \
      !defined(__ARM_ARCH_6T2__)


namespace phmap {
namespace bits {

struct Unaligned16Struct 
{
    uint16_t value;
    uint8_t dummy;  // To make the size non-power-of-two.
} PHMAP_ATTRIBUTE_PACKED;

struct Unaligned32Struct 
{
    uint32_t value;
    uint8_t dummy;  // To make the size non-power-of-two.
} PHMAP_ATTRIBUTE_PACKED;

}  // namespace bits
}  // namespace phmap

#define PHMAP_INTERNAL_UNALIGNED_LOAD16(_p)                                  \
  ((reinterpret_cast<const ::phmap::bits::Unaligned16Struct *>(_p))->value)
#define PHMAP_INTERNAL_UNALIGNED_LOAD32(_p)                                  \
  ((reinterpret_cast<const ::phmap::bits::Unaligned32Struct *>(_p))->value)

#define PHMAP_INTERNAL_UNALIGNED_STORE16(_p, _val)                      \
  ((reinterpret_cast< ::phmap::bits::Unaligned16Struct *>(_p))->value = (_val))
#define PHMAP_INTERNAL_UNALIGNED_STORE32(_p, _val)                      \
  ((reinterpret_cast< ::phmap::bits::Unaligned32Struct *>(_p))->value = (_val))

namespace phmap {
namespace bits {

inline uint64_t UnalignedLoad64(const void *p) 
{
    uint64_t t;
    memcpy(&t, p, sizeof t);
    return t;
}

inline void UnalignedStore64(void *p, uint64_t v) { memcpy(p, &v, sizeof v); }

}  // namespace bits
}  // namespace phmap

#define PHMAP_INTERNAL_UNALIGNED_LOAD64(_p) \
  (phmap::bits::UnalignedLoad64(_p))
#define PHMAP_INTERNAL_UNALIGNED_STORE64(_p, _val) \
  (phmap::bits::UnalignedStore64(_p, _val))

#else

// PHMAP_INTERNAL_NEED_ALIGNED_LOADS is defined when the underlying platform
// doesn't support unaligned access.
#define PHMAP_INTERNAL_NEED_ALIGNED_LOADS

// These functions are provided for architectures that don't support
// unaligned loads and stores.

namespace phmap {
namespace bits {

inline uint16_t UnalignedLoad16(const void *p) {
  uint16_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint32_t UnalignedLoad32(const void *p) {
  uint32_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint64_t UnalignedLoad64(const void *p) {
  uint64_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline void UnalignedStore16(void *p, uint16_t v) { memcpy(p, &v, sizeof v); }

inline void UnalignedStore32(void *p, uint32_t v) { memcpy(p, &v, sizeof v); }

inline void UnalignedStore64(void *p, uint64_t v) { memcpy(p, &v, sizeof v); }

}  // namespace bits
}  // namespace phmap

#define PHMAP_INTERNAL_UNALIGNED_LOAD16(_p) (phmap::bits::UnalignedLoad16(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD32(_p) (phmap::bits::UnalignedLoad32(_p))
#define PHMAP_INTERNAL_UNALIGNED_LOAD64(_p) (phmap::bits::UnalignedLoad64(_p))

#define PHMAP_INTERNAL_UNALIGNED_STORE16(_p, _val) (phmap::bits::UnalignedStore16(_p, _val))
#define PHMAP_INTERNAL_UNALIGNED_STORE32(_p, _val) (phmap::bits::UnalignedStore32(_p, _val))
#define PHMAP_INTERNAL_UNALIGNED_STORE64(_p, _val) (phmap::bits::UnalignedStore64(_p, _val))

#endif

// -----------------------------------------------------------------------------
// File: optimization.h
// -----------------------------------------------------------------------------

#if defined(__pnacl__)
    #define PHMAP_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#elif defined(__clang__)
    // Clang will not tail call given inline volatile assembly.
    #define PHMAP_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(__GNUC__)
    // GCC will not tail call given inline volatile assembly.
    #define PHMAP_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(_MSC_VER)
    #include <intrin.h>
    // The __nop() intrinsic blocks the optimisation.
    #define PHMAP_BLOCK_TAIL_CALL_OPTIMIZATION() __nop()
#else
    #define PHMAP_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#endif

#if defined(__GNUC__)
    // Cache line alignment
    #if defined(__i386__) || defined(__x86_64__)
        #define PHMAP_CACHELINE_SIZE 64
    #elif defined(__powerpc64__)
        #define PHMAP_CACHELINE_SIZE 128
    #elif defined(__aarch64__)
        // We would need to read special register ctr_el0 to find out L1 dcache size.
        // This value is a good estimate based on a real aarch64 machine.
        #define PHMAP_CACHELINE_SIZE 64
    #elif defined(__arm__)
        // Cache line sizes for ARM: These values are not strictly correct since
        // cache line sizes depend on implementations, not architectures.  There
        // are even implementations with cache line sizes configurable at boot
        // time.
        #if defined(__ARM_ARCH_5T__)
            #define PHMAP_CACHELINE_SIZE 32
        #elif defined(__ARM_ARCH_7A__)
            #define PHMAP_CACHELINE_SIZE 64
        #endif
    #endif

    #ifndef PHMAP_CACHELINE_SIZE
        // A reasonable default guess.  Note that overestimates tend to waste more
        // space, while underestimates tend to waste more time.
        #define PHMAP_CACHELINE_SIZE 64
    #endif

    #define PHMAP_CACHELINE_ALIGNED __attribute__((aligned(PHMAP_CACHELINE_SIZE)))
#elif defined(_MSC_VER)
    #define PHMAP_CACHELINE_SIZE 64
    #define PHMAP_CACHELINE_ALIGNED __declspec(align(PHMAP_CACHELINE_SIZE))
#else
    #define PHMAP_CACHELINE_SIZE 64
    #define PHMAP_CACHELINE_ALIGNED
#endif


#if PHMAP_HAVE_BUILTIN(__builtin_expect) || \
    (defined(__GNUC__) && !defined(__clang__))
    #define PHMAP_PREDICT_FALSE(x) (__builtin_expect(x, 0))
    #define PHMAP_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#else
    #define PHMAP_PREDICT_FALSE(x) (x)
    #define PHMAP_PREDICT_TRUE(x) (x)
#endif


#endif // phmap_bits_h_guard_
