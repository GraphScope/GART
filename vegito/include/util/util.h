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

#ifndef VEGITO_INCLUDE_UTIL_UTIL_H_
#define VEGITO_INCLUDE_UTIL_UTIL_H_

#include <cstdint>
#include <vector>

#define ALWAYS_INLINE __attribute__((always_inline))
inline ALWAYS_INLINE uint64_t rdtsc(void) {
  uint32_t hi, lo;
  __asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

namespace gart {
namespace util {

// Round "a" according to "b"
template <class Num>
inline ALWAYS_INLINE Num Round(Num a, Num b) {
  return (a + b - a % b);
}

inline bool CAS(uint32_t* ptr, uint32_t oldval, uint32_t newval) {
  return __sync_bool_compare_and_swap(ptr, oldval, newval);
}

inline bool CAS(volatile uint64_t* ptr, uint64_t oldval, uint64_t newval) {
  return __sync_bool_compare_and_swap(ptr, oldval, newval);
}

// fetch and add
inline uint64_t FAA(uint64_t* ptr, uint64_t add_val) {
  return __sync_fetch_and_add(ptr, add_val);
}

inline uint64_t FAA(volatile uint64_t* ptr, uint64_t add_val) {
  return __sync_fetch_and_add(ptr, add_val);
}

inline void lock32(uint32_t* lock_ptr) {
  while (!CAS(lock_ptr, 0, 1)) {}
}

inline void unlock32(uint32_t* lock_ptr) { *lock_ptr = 0; }

// insert a value into a vector at a given index (expand the vector if needed)
// return true if the vector is expanded
template <class T>
inline bool insert_vec(std::vector<T>& vec, int idx, const T& val,
                       const T& default_val) {
  bool expand = false;
  if (unlikely(idx >= vec.size())) {
    vec.resize(idx + 1, default_val);
    expand = true;
  }
  vec[idx] = val;
  return expand;
}

}  // namespace util
}  // namespace gart

#endif  // VEGITO_INCLUDE_UTIL_UTIL_H_
