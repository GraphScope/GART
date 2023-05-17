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

#ifndef RESEARCH_GART_SRC_GRAPH_TYPE_DEF_H_
#define RESEARCH_GART_SRC_GRAPH_TYPE_DEF_H_

#include "util/inline_str.h"

namespace gart {
namespace graph {
namespace ldbc {

typedef uint64_t ID;
typedef uint32_t Uint;
typedef inline_str_8<40> String;
typedef inline_str_8<256> LongString;  // TODO: unified string types, inc. text?
typedef inline_str_16<2000> Text;
typedef inline_str_fixed<10> Date;  // yyyy-mm-dd
typedef inline_str_fixed<28>
    DateTime;  // yyyy-mm-ddTHH:MM:ss.sss+0000
               // specificiation is yyyy-mm-ddTHH:MM:ss.sss+00:00

}  // namespace ldbc
}  // namespace graph
}  // namespace gart

#endif  // RESEARCH_GART_SRC_GRAPH_TYPE_DEF_H_
