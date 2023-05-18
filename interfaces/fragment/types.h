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

#ifndef INTERFACES_FRAGMENT_TYPES_H_
#define INTERFACES_FRAGMENT_TYPES_H_

#include "vineyard/graph/fragment/property_graph_types.h"

namespace gart {
using vid_t = uint64_t;
using fid_t = grape::fid_t;

using order_t = uint8_t;
using segid_t = uint64_t;
using timestamp_t = int64_t;
using label_id_t = vineyard::property_graph_types::LABEL_ID_TYPE;
using vertex_t = grape::Vertex<vid_t>;

enum PropertyType {
  INVALID = 0,
  BOOL = 1,
  CHAR = 2,
  SHORT = 3,
  INT = 4,
  LONG = 5,
  FLOAT = 6,
  DOUBLE = 7,
  STRING = 8,
  BYTES = 9,
  INT_LIST = 10,
  LONG_LIST = 11,
  FLOAT_LIST = 12,
  DOUBLE_LIST = 13,
  STRING_LIST = 14,
  DATE = 15,
  DATETIME = 16,
  LONGSTRING = 17,
  TEXT = 18
};

#define VERTEX_PER_SEG 4096
}  // namespace gart

#endif  // INTERFACES_FRAGMENT_TYPES_H_
