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

#ifndef APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_WCC_H_
#define APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_WCC_H_

#include <limits>
#include <vector>

#include "core/app/app_base.h"
#include "core/context/gart_vertex_data_context.h"
#include "core/utils/gart_vertex_array.h"

namespace gs {

/**
 * A wcc implementation for labeled graph
 * @tparam FRAG_T
 */
template <typename FRAG_T>
class PropertyWCCContext : public gs::GartLabeledVertexDataContext<FRAG_T> {
  using vid_t = typename FRAG_T::vid_t;
  using oid_t = typename FRAG_T::oid_t;

 public:
  explicit PropertyWCCContext(const FRAG_T& fragment)
      : gs::GartLabeledVertexDataContext<FRAG_T>(fragment) {}

  void Init(grape::DefaultMessageManager& messages) {
    auto& frag = this->fragment();
    auto vertex_label_num = frag.vertex_label_num();
    result.resize(vertex_label_num);

    for (auto v_label = 0; v_label < vertex_label_num; v_label++) {
      auto vertices_iter = frag.Vertices(v_label);
      result[v_label].Init(&frag, vertices_iter,
                           std::numeric_limits<oid_t>::max());
    }
  }

  void Output(std::ostream& os) override {
    auto& frag = this->fragment();
    auto v_label_num = frag.vertex_label_num();
    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto vertices_iter = frag.InnerVertices(v_label);
      while (vertices_iter.valid()) {
        auto v = vertices_iter.vertex();
        auto v_data = result[v_label][v];
        os << frag.GetId(v) << " " << v_data << std::endl;
        vertices_iter.next();
      }
    }
  }

  std::vector<gart::GartVertexArray<gart::vid_t, oid_t>> result;
};

template <typename FRAG_T>
class PropertyWCC : public AppBase<FRAG_T, PropertyWCCContext<FRAG_T>> {
 public:
  INSTALL_DEFAULT_WORKER(PropertyWCC<FRAG_T>, PropertyWCCContext<FRAG_T>,
                         FRAG_T)

  using vertex_t = typename fragment_t::vertex_t;
  using oid_t = typename fragment_t::oid_t;

  static constexpr bool need_split_edges = false;
  static constexpr grape::MessageStrategy message_strategy =
      grape::MessageStrategy::kSyncOnOuterVertex;
  static constexpr grape::LoadStrategy load_strategy =
      grape::LoadStrategy::kBothOutIn;

  void PEval(const fragment_t& frag, context_t& ctx,
             message_manager_t& messages) {
    auto v_label_num = frag.vertex_label_num();
    auto e_label_num = frag.edge_label_num();

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto vertices_iter = frag.Vertices(v_label);
      while (vertices_iter.valid()) {
        vertex_t src = vertices_iter.vertex();
        ctx.result[v_label][src] = frag.GetId(src);
        vertices_iter.next();
      }
    }

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        auto src = inner_vertices_iter.vertex();
        for (auto e_label = 0; e_label < e_label_num; e_label++) {
          auto edge_iter = frag.GetIncomingAdjList(src, e_label);
          while (edge_iter.valid()) {
            auto dst = edge_iter.neighbor();
            if (frag.IsInnerVertex(dst)) {
              auto dst_label = frag.vertex_label(dst);
              oid_t dst_data = ctx.result[dst_label][dst];
              if (dst_data < ctx.result[v_label][src]) {
                ctx.result[v_label][src] = dst_data;
              }
            }
            edge_iter.next();
          }
        }
        inner_vertices_iter.next();
      }
    }

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto outer_vertices_iter = frag.OuterVertices(v_label);
      while (outer_vertices_iter.valid()) {
        auto src = outer_vertices_iter.vertex();
        oid_t old_data = ctx.result[v_label][src];
        oid_t new_data = old_data;
        for (auto e_label = 0; e_label < e_label_num; e_label++) {
          auto edge_iter = frag.GetIncomingAdjList(src, e_label);
          while (edge_iter.valid()) {
            auto dst = edge_iter.neighbor();
            auto dst_label = frag.vertex_label(dst);
            if (ctx.result[dst_label][dst] < new_data) {
              new_data = ctx.result[dst_label][dst];
            }
            edge_iter.next();
          }
        }
        if (new_data < old_data) {
          ctx.result[v_label][src] = new_data;
          messages.SyncStateOnOuterVertex<fragment_t, oid_t>(frag, src,
                                                             new_data);
        }
        outer_vertices_iter.next();
      }
    }

    messages.ForceContinue();
  }

  void IncEval(const fragment_t& frag, context_t& ctx,
               message_manager_t& messages) {
    auto v_label_num = frag.vertex_label_num();
    auto e_label_num = frag.edge_label_num();

    vertex_t v;
    oid_t val;
    while (messages.GetMessage<fragment_t, oid_t>(frag, v, val)) {
      auto v_label = frag.vertex_label(v);
      if (val < ctx.result[v_label][v]) {
        ctx.result[v_label][v] = val;
      }
    }

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        auto src = inner_vertices_iter.vertex();
        for (auto e_label = 0; e_label < e_label_num; e_label++) {
          auto edge_iter = frag.GetIncomingAdjList(src, e_label);
          while (edge_iter.valid()) {
            auto dst = edge_iter.neighbor();
            if (frag.IsInnerVertex(dst)) {
              auto dst_label = frag.vertex_label(dst);
              oid_t dst_data = ctx.result[dst_label][dst];
              if (dst_data < ctx.result[v_label][src]) {
                ctx.result[v_label][src] = dst_data;
              }
            }
            edge_iter.next();
          }
        }
        inner_vertices_iter.next();
      }
    }

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto outer_vertices_iter = frag.OuterVertices(v_label);
      while (outer_vertices_iter.valid()) {
        auto src = outer_vertices_iter.vertex();
        oid_t old_data = ctx.result[v_label][src];
        oid_t new_data = old_data;
        for (auto e_label = 0; e_label < e_label_num; e_label++) {
          auto edge_iter = frag.GetIncomingAdjList(src, e_label);
          while (edge_iter.valid()) {
            auto dst = edge_iter.neighbor();
            auto dst_label = frag.vertex_label(dst);
            if (ctx.result[dst_label][dst] < new_data) {
              new_data = ctx.result[dst_label][dst];
            }
            edge_iter.next();
          }
        }
        if (new_data < old_data) {
          ctx.result[v_label][src] = new_data;
          messages.SyncStateOnOuterVertex<fragment_t, oid_t>(frag, src,
                                                             new_data);
        }
        outer_vertices_iter.next();
      }
    }
  }
};

}  // namespace gs

#endif  // APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_WCC_H_
