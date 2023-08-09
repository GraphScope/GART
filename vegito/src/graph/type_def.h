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

#ifndef VEGITO_SRC_GRAPH_TYPE_DEF_H_
#define VEGITO_SRC_GRAPH_TYPE_DEF_H_

#include "util/inline_str.h"

namespace gart {
namespace graph {

// data types from LDBC SNB

typedef uint64_t ID;
typedef uint32_t Uint;

typedef int Date;
typedef int64_t DateTime;
typedef int64_t Time;
typedef inline_str_fixed<22> TimeStamp;
// format for debezium: 2010-12-30T17:15:00.0Z

}  // namespace graph
}  // namespace gart

#endif  // VEGITO_SRC_GRAPH_TYPE_DEF_H_
