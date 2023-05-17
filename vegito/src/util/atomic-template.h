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

/* The following code may be included multiple times in a single file. */


#if DATA_BITS == 64
#define DATA_TYPE uint64_t
#define SUFFIX q
#elif DATA_BITS == 32
#define DATA_TYPE uint32_t
#define SUFFIX l
#elif DATA_BITS == 16
#define DATA_TYPE uint16_t
#define SUFFIX w
#elif DATA_BITS == 8
#define DATA_TYPE uint8_t
#define SUFFIX b
#else
#error unsupported data size
#endif

static __inline__ void glue(atomic_inc, DATA_BITS)(DATA_TYPE *p) {
  asm volatile(LOCK_PREFIX "inc" sstr(SUFFIX) " %0" : "+m"(*p) : : "cc");
}

static __inline__ void glue(atomic_dec, DATA_BITS)(DATA_TYPE *p) {
  asm volatile(LOCK_PREFIX "dec" sstr(SUFFIX) " %0" : "+m"(*p) : : "cc");
}

static __inline__ void glue(atomic_add, DATA_BITS)(DATA_TYPE *addr,
                                                   DATA_TYPE val) {
  asm volatile(LOCK_PREFIX "add" sstr(SUFFIX) " %1, %0"
               : "+m"(*addr)
               : "a"(val)
               : "cc");
}

/* Return previous value in addr. So if the return value is the same as oldval,
 * swap occured. */
static __inline__ DATA_TYPE glue(atomic_cmpxchg, DATA_BITS)(DATA_TYPE *addr,
                                                            DATA_TYPE oldval,
                                                            DATA_TYPE newval) {
  asm volatile(LOCK_PREFIX "cmpxchg" sstr(SUFFIX) " %2, %1"
               : "+a"(oldval), "+m"(*addr)
               : "q"(newval)
               : "cc");

  return oldval;
}

/* We don't need the lock prefix for xchg. */

static __inline__ DATA_TYPE glue(xchg, DATA_BITS)(DATA_TYPE *addr,
                                                  DATA_TYPE val) {
  asm volatile("xchg" sstr(SUFFIX) " %0,%1"
               : "=r"(val)
               : "m"(*addr), "0"(val)
               : "memory");

  return val;
}

static __inline__ void glue(atomic_and, DATA_BITS)(DATA_TYPE *addr,
                                                   DATA_TYPE mask) {
  asm volatile(LOCK_PREFIX "and" sstr(SUFFIX) " %1, %0"
               : "+m"(*addr)
               : "r"(mask)
               : "cc");
}

static __inline__ void glue(atomic_or, DATA_BITS)(DATA_TYPE *addr,
                                                  DATA_TYPE mask) {
  asm volatile(LOCK_PREFIX "or" sstr(SUFFIX) " %1, %0"
               : "+m"(*addr)
               : "r"(mask)
               : "cc");
}

/* Return the old value in addr. */
static __inline__ DATA_TYPE glue(atomic_fetch_and_add,
                                 DATA_BITS)(DATA_TYPE *addr, DATA_TYPE val) {
  asm volatile(LOCK_PREFIX "xadd" sstr(SUFFIX) " %0, %1"
               : "+a"(val), "+m"(*addr)
               :
               : "cc");

  return val;
}

#undef DATA_BITS
#undef DATA_TYPE
#undef SUFFIX
