/** Copyright 2020-2023 Alibaba Group Holding Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VEGITO_INCLUDE_UTIL_BITSET_H_
#define VEGITO_INCLUDE_UTIL_BITSET_H_

#include <cstdint>

#define BYTE_SIZE(n) (((n) + 7ul) >> 3)

#define BYTE_INDEX(i) ((i) >> 3)
#define BIT_OFFSET(i) ((i) &0x7)

inline const bool get_bit(uint8_t* data, uint64_t idx) {
  return data[BYTE_INDEX(idx)] & (((uint8_t) 1) << BIT_OFFSET(idx));
}

inline void set_bit(uint8_t* data, uint64_t idx) {
  __sync_fetch_and_or(data + BYTE_INDEX(idx), ((uint8_t) 1) << BIT_OFFSET(idx));
}

inline void reset_bit(uint8_t* data, uint64_t idx) {
  __sync_fetch_and_and(data + BYTE_INDEX(idx),
                       ~(((uint8_t) 1) << BIT_OFFSET(idx)));
}

#endif  // VEGITO_INCLUDE_UTIL_BITSET_H_
