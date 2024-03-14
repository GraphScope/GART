/** Copyright 2020-2023 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_SSSP_H_
#define APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_SSSP_H_

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "vineyard/common/util/json.h"

#include "core/app/app_base.h"
#include "core/context/gart_vertex_data_context.h"
#include "core/utils/gart_vertex_array.h"
#include "interfaces/fragment/types.h"

namespace gs {

/**
 * A sssp implementation for labeled graph
 * @tparam FRAG_T
 */
template <typename FRAG_T>
class PropertySSSPContext : public gs::GartLabeledVertexDataContext<FRAG_T> {
  using vid_t = typename FRAG_T::vid_t;
  using oid_t = typename FRAG_T::oid_t;
  using label_id_t = typename FRAG_T::label_id_t;

 public:
  explicit PropertySSSPContext(const FRAG_T& fragment)
      : gs::GartLabeledVertexDataContext<FRAG_T>(fragment) {}

  void Init(grape::DefaultMessageManager& messages, std::string label,
            oid_t src_oid, std::string weight_name) {
    auto& frag = this->fragment();
    auto vertex_label_num = frag.vertex_label_num();
    this->label_id = frag.GetVertexLabelId(label);
    this->source_id = src_oid;
    this->weight_name = weight_name;
    result.resize(vertex_label_num);
    updated.resize(vertex_label_num);
    updated_next.resize(vertex_label_num);

    for (auto v_label = 0; v_label < vertex_label_num; v_label++) {
      auto vertices_iter = frag.Vertices(v_label);
      updated[v_label].Init(&frag, vertices_iter, false);
      updated_next[v_label].Init(&frag, vertices_iter, false);
      result[v_label].Init(&frag, vertices_iter,
                           std::numeric_limits<int>::max());
    }
  }

  void Output(std::ostream& os) override {
    auto& frag = this->fragment();
    auto v_label_num = frag.vertex_label_num();
    std::vector<std::tuple<std::string, oid_t, int>> result_vec;
    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      std::string v_label_str = frag.GetVertexLabelName(v_label);
      auto vertices_iter = frag.InnerVertices(v_label);
      while (vertices_iter.valid()) {
        auto v = vertices_iter.vertex();
        auto v_data = result[v_label][v];
        if (v_data != std::numeric_limits<int>::max()) {
          result_vec.push_back(
              std::make_tuple(v_label_str, frag.GetId(v), v_data));
        }
        vertices_iter.next();
      }
    }

    vineyard::json result_json;

    for (const auto& tup : result_vec) {
      // You can use get<n>(tup) to access the nth element of the tuple
      nlohmann::json jsonObj;
      jsonObj["label_id"] = std::get<0>(tup);
      jsonObj["oid"] = std::get<1>(tup);
      jsonObj["distance"] = std::get<2>(tup);

      // Add the JSON object to the array
      result_json.push_back(jsonObj);
    }

    std::string json_str = result_json.dump();
    std::cout << json_str;
  }

  std::vector<gart::GartVertexArray<gart::vid_t, int>> result;
  std::vector<gart::GartVertexArray<gart::vid_t, int>> updated;
  std::vector<gart::GartVertexArray<gart::vid_t, int>> updated_next;
  label_id_t label_id;
  oid_t source_id;
  std::string weight_name;
};

template <typename FRAG_T>
class PropertySSSP : public AppBase<FRAG_T, PropertySSSPContext<FRAG_T>> {
 public:
  INSTALL_DEFAULT_WORKER(PropertySSSP<FRAG_T>, PropertySSSPContext<FRAG_T>,
                         FRAG_T)

  using vertex_t = typename fragment_t::vertex_t;

  static constexpr bool need_split_edges = false;
  static constexpr grape::MessageStrategy message_strategy =
      grape::MessageStrategy::kSyncOnOuterVertex;
  static constexpr grape::LoadStrategy load_strategy =
      grape::LoadStrategy::kBothOutIn;

  void PEval(const fragment_t& frag, context_t& ctx,
             message_manager_t& messages) {
    auto e_label_num = frag.edge_label_num();
    bool is_native = false;
    vertex_t src_vertex;
    if (!frag.Oid2Gid(ctx.label_id, ctx.source_id, src_vertex)) {
      std::cout << "source vertex not found" << std::endl;
      return;
    }
    is_native = frag.IsInnerVertex(src_vertex);
    if (is_native) {
      auto src_label = frag.vertex_label(src_vertex);
      ctx.result[src_label][src_vertex] = 0;
      for (auto e_label = 0; e_label < e_label_num; e_label++) {
        auto edge_iter = frag.GetOutgoingAdjList(src_vertex, e_label);
        auto prop_id = frag.GetEdgePropId(e_label, ctx.weight_name);
        while (edge_iter.valid()) {
          auto dst_vertex = edge_iter.neighbor();
          auto dst_label = frag.vertex_label(dst_vertex);
          int e_data = 1;
          if (prop_id != -1) {
            e_data = edge_iter.template get_data<int>(prop_id);
          }
          ctx.result[dst_label][dst_vertex] =
              std::min(ctx.result[dst_label][dst_vertex], e_data);
          if (frag.IsInnerVertex(dst_vertex)) {
            ctx.updated[dst_label][dst_vertex] = true;
          } else {
            messages.SyncStateOnOuterVertex(frag, dst_vertex,
                                            ctx.result[dst_label][dst_vertex]);
          }
          edge_iter.next();
        }
      }
    }

    messages.ForceContinue();
  }

  void IncEval(const fragment_t& frag, context_t& ctx,
               message_manager_t& messages) {
    auto v_label_num = frag.vertex_label_num();
    auto e_label_num = frag.edge_label_num();
    int val;
    vertex_t v;
    bool require_force_continue = false;
    while (messages.GetMessage<fragment_t, int>(frag, v, val)) {
      auto v_label = frag.vertex_label(v);
      if (ctx.result[v_label][v] > val) {
        ctx.result[v_label][v] = val;
        ctx.updated[v_label][v] = true;
      }
    }

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      ctx.updated[v_label].Swap(ctx.updated_next[v_label]);
      ctx.updated[v_label].SetValue(false);
    }

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        auto src = inner_vertices_iter.vertex();
        if (ctx.updated_next[v_label][src] == false) {
          inner_vertices_iter.next();
          continue;
        }
        int dist_src = ctx.result[v_label][src];
        for (auto e_label = 0; e_label < e_label_num; e_label++) {
          auto edge_iter = frag.GetOutgoingAdjList(src, e_label);
          auto prop_id = frag.GetEdgePropId(e_label, ctx.weight_name);
          while (edge_iter.valid()) {
            auto dst = edge_iter.neighbor();
            auto dst_label = frag.vertex_label(dst);
            int e_data = 1;
            if (prop_id != -1) {
              e_data = edge_iter.template get_data<int>(prop_id);
            }
            int new_dist_dst = dist_src + e_data;
            if (new_dist_dst < ctx.result[dst_label][dst]) {
              ctx.result[dst_label][dst] = new_dist_dst;
              if (frag.IsInnerVertex(dst)) {
                ctx.updated[dst_label][dst] = true;
                require_force_continue = true;
              } else {
                messages.SyncStateOnOuterVertex(frag, dst, new_dist_dst);
              }
            }
            edge_iter.next();
          }
        }
        inner_vertices_iter.next();
      }
    }

    if (require_force_continue) {
      messages.ForceContinue();
    }
  }
};

}  // namespace gs

#endif  // APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_SSSP_H_
