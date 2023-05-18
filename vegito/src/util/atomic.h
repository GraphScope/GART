/*
 * The code is a part of our project called VEGITO, which retrofits
 * high availability mechanism to tame hybrid transaction/analytical
 * processing.
 *
 * Copyright (c) 2021 Shanghai Jiao Tong University.
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://ipads.se.sjtu.edu.cn/projects/vegito
 *
 */

#ifndef VEGITO_SRC_UTIL_ATOMIC_H_
#define VEGITO_SRC_UTIL_ATOMIC_H_

#include <cstdint>

#define xglue(a, b) a##b
// If a/b is macro, it will expand first, then pass to xglue
#define glue(a, b) xglue(a, b)

#define xstr(s) #s
#define sstr(s) xstr(s)

#define __inline__ inline __attribute__((always_inline))

/* Compile read-write barrier */
#define barrier() asm volatile("" : : : "memory")

#define rmb() asm volatile("lfence" ::: "memory")
#define wmb() asm volatile("sfence" ::: "memory")
#define mb() asm volatile("mfence" ::: "memory")

/* Pause instruction to prevent excess processor bus usage */
#define cpu_relax() asm volatile("pause\n" : : : "memory")

#define CACHE_LINE_SIZE 64
inline void prefetch(const void* ptr) {
  typedef struct {
    char x[CACHE_LINE_SIZE];
  } cacheline_t;
  asm volatile("prefetcht0 %0" : : "m"(*(const cacheline_t*) ptr));
}

#define LOCK_PREFIX "lock; "

// Is this the correct way to detect 64 system?
#if (__LP64__ == 1)
/* Compares the value in rdx:rax to value in memp, if equal load rcx:rbx into
 * memp, else load value into rdx:rax.
 * Return 1 on success, 0 otherwise. */
static __inline__ uint8_t atomic_cmpxchg16b(uint64_t* memp, uint64_t old0,
                                            uint64_t old1, uint64_t new0,
                                            uint64_t new1) {
  uint8_t z;
  asm volatile(
      "lock; cmpxchg16b %3\n\t"
      "setz %2\n\t"
      : "+a"(old0), "+d"(old1), "=r"(z), "+m"(*memp)
      : "b"(new0), "c"(new1)
      : "memory", "cc");
  return z;
}
#endif

static __inline__ uint8_t atomic_cmpxchg8b(uint32_t* memp, uint32_t old0,
                                           uint32_t old1, uint32_t new0,
                                           uint32_t new1) {
  uint8_t z;
  asm volatile(
      "lock; cmpxchg8b %3\n\t"
      "setz %2\n\t"
      : "+a"(old0), "+d"(old1), "=r"(z), "+m"(*memp)
      : "b"(new0), "c"(new1)
      : "memory", "cc");
  return z;
}

#define DATA_BITS 8
#include "atomic-template.h"

#define DATA_BITS 16
#include "atomic-template.h"  // NOLINT(build/include)

#define DATA_BITS 32
#include "atomic-template.h"  // NOLINT(build/include)

#define DATA_BITS 64
#include "atomic-template.h"  // NOLINT(build/include)

#endif  // VEGITO_SRC_UTIL_ATOMIC_H_
