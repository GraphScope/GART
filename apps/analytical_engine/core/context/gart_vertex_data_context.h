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

#ifndef ANALYTICAL_ENGINE_CORE_CONTEXT_GART_VERTEX_DATA_CONTEXT_H_
#define ANALYTICAL_ENGINE_CORE_CONTEXT_GART_VERTEX_DATA_CONTEXT_H_

#include <mpi.h>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "grape/app/context_base.h"
#include "grape/app/vertex_data_context.h"
#include "grape/serialization/in_archive.h"
#include "grape/utils/vertex_array.h"
#include "grape/worker/comm_spec.h"
#include "vineyard/basic/ds/arrow_utils.h"
#include "vineyard/basic/ds/dataframe.h"
#include "vineyard/client/client.h"
#include "vineyard/client/ds/i_object.h"
#include "vineyard/common/util/uuid.h"

#include "core/config.h"
#include "core/context/context_protocols.h"
#include "core/context/i_context.h"
#include "core/context/selector.h"
#include "core/context/tensor_dataframe_builder.h"
#include "core/error.h"
#include "core/server/rpc_utils.h"
#include "core/utils/mpi_utils.h"
#include "core/utils/transform_utils.h"
#include "graphscope/proto/types.pb.h"

#define CONTEXT_TYPE_VERTEX_DATA "vertex_data"
#define CONTEXT_TYPE_LABELED_VERTEX_DATA "labeled_vertex_data"
#define CONTEXT_TTPE_DYNAMIC_VERTEX_DATA "dynamic_vertex_data"

namespace gs {
/**
 * @brief VertexDataContext for labeled fragment
 *
 * @tparam FRAG_T The fragment class (Labeled fragment only)
 * @tparam DATA_T The Data type hold by context
 * @tparam Enable
 */
template <typename FRAG_T>
class GartLabeledVertexDataContext : public grape::ContextBase {
 public:
  using fragment_t = FRAG_T;

  explicit GartLabeledVertexDataContext(const fragment_t& fragment)
      : fragment_(fragment) {}

  const fragment_t& fragment() { return fragment_; }

 private:
  const fragment_t& fragment_;
};

}  // namespace gs

#endif  // ANALYTICAL_ENGINE_CORE_CONTEXT_GART_VERTEX_DATA_CONTEXT_H_
