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

#ifndef VEGITO_INCLUDE_UTIL_SPINLOCK_H_
#define VEGITO_INCLUDE_UTIL_SPINLOCK_H_

#include "atomic.h"  // NOLINT(build/include_subdir)

/* The counter should be initialized to be 0. */
class SpinLock {
 public:
  // 0: free, 1: busy
  // occupy an exclusive cache line
  volatile uint8_t padding1[32];
  volatile uint16_t lock;
  volatile uint8_t padding2[32];

 public:
  SpinLock() { lock = 0; }

  inline void Lock() {
    while (1) {
      uint16_t* p = const_cast<uint16_t*>(
          reinterpret_cast<const volatile uint16_t*>(&lock));
      if (!xchg16(p, 1))
        return;

      while (lock)
        cpu_relax();
    }
  }

  inline void Unlock() {
    barrier();
    lock = 0;
  }

  inline uint16_t Trylock() {
    uint16_t* p = const_cast<uint16_t*>(
        reinterpret_cast<const volatile uint16_t*>(&lock));
    return xchg16(p, 1);
  }

  inline uint16_t IsLocked() { return lock; }
};

#endif  // VEGITO_INCLUDE_UTIL_SPINLOCK_H_
