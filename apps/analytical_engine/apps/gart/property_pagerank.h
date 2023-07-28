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

#ifndef APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_PAGERANK_H_
#define APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_PAGERANK_H_

#include <vector>

#include "core/app/app_base.h"
#include "core/context/gart_vertex_data_context.h"
#include "core/utils/gart_vertex_array.h"

namespace gs {

/**
 * A PageRank implementation for labeled graph
 * @tparam FRAG_T
 */
template <typename FRAG_T>
class PropertyPageRankContext
    : public gs::GartLabeledVertexDataContext<FRAG_T> {
  using vid_t = typename FRAG_T::vid_t;
  using oid_t = typename FRAG_T::oid_t;

 public:
  explicit PropertyPageRankContext(const FRAG_T& fragment)
      : gs::GartLabeledVertexDataContext<FRAG_T>(fragment) {}

  void Init(grape::DefaultMessageManager& messages, double delta_input,
            int max_round_input) {
    auto& frag = this->fragment();
    auto vertex_label_num = frag.vertex_label_num();
    result.resize(vertex_label_num);
    result_next.resize(vertex_label_num);
    degree.resize(vertex_label_num);
    delta = delta_input;
    max_round = max_round_input;
    current_round = 0;
    total_vertex_num = 0;

    for (auto v_label = 0; v_label < vertex_label_num; v_label++) {
      auto vertices_iter = frag.Vertices(v_label);
      result[v_label].Init(&frag, vertices_iter, 0);
      result_next[v_label].Init(&frag, vertices_iter, 0);
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      degree[v_label].Init(&frag, inner_vertices_iter, 0);
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

  std::vector<gart::GartVertexArray<gart::vid_t, double>> result;
  std::vector<gart::GartVertexArray<gart::vid_t, double>> result_next;
  std::vector<gart::GartVertexArray<gart::vid_t, int>> degree;
  int max_round;
  double delta;
  int current_round;
  int total_vertex_num;
  double dangling_sum = 0.0;
  int total_dangling_vnum = 0;
};

template <typename FRAG_T>
class PropertyPageRank
    : public AppBase<FRAG_T, PropertyPageRankContext<FRAG_T>>,
      public grape::Communicator {
 public:
  INSTALL_DEFAULT_WORKER(PropertyPageRank<FRAG_T>,
                         PropertyPageRankContext<FRAG_T>, FRAG_T)

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

    int local_vertex_num = 0;

    /*
    for (auto v_label = 0; v_label < 1; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        auto v = inner_vertices_iter.vertex();
        std::string v_data = frag.template GetData<std::string>(v, 2);
        if (frag.fid() == 0) {
          std::cout << "v_ofset " << frag.GetOffset(v) << " v_data: " << v_data
    << std::endl;
        }
        inner_vertices_iter.next();
      }
    }
    */

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        local_vertex_num++;
        inner_vertices_iter.next();
      }
    }

    Sum(local_vertex_num, ctx.total_vertex_num);

    std::cout << "total_vertex_num: " << ctx.total_vertex_num
              << " local_vertex_num: " << local_vertex_num << std::endl;

    double p = 1.0 / ctx.total_vertex_num;

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        auto src = inner_vertices_iter.vertex();
        int edge_num = 0;
        for (auto e_label = 0; e_label < e_label_num; e_label++) {
          auto edge_iter = frag.GetOutgoingAdjList(src, e_label);
          while (edge_iter.valid()) {
            edge_num++;
            edge_iter.next();
          }
        }
        ctx.degree[v_label][src] = edge_num;
        inner_vertices_iter.next();
      }
    }

    int dangling_vnum = 0;

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto inner_vertices_iter = frag.InnerVertices(v_label);
      while (inner_vertices_iter.valid()) {
        auto src = inner_vertices_iter.vertex();
        ctx.result[v_label][src] = p;
        int edge_num = ctx.degree[v_label][src];
        if (edge_num > 0) {
          for (auto e_label = 0; e_label < e_label_num; e_label++) {
            auto edge_iter = frag.GetOutgoingAdjList(src, e_label);
            while (edge_iter.valid()) {
              auto dst = edge_iter.neighbor();
              auto dst_label = frag.vertex_label(dst);
              ctx.result_next[dst_label][dst] += p / edge_num;
              edge_iter.next();
            }
          }
        } else {
          dangling_vnum++;
        }
        inner_vertices_iter.next();
      }
    }

    Sum(dangling_vnum, ctx.total_dangling_vnum);
    ctx.dangling_sum = p * ctx.total_dangling_vnum;

    for (auto v_label = 0; v_label < v_label_num; v_label++) {
      auto outer_vertices_iter = frag.OuterVertices(v_label);
      while (outer_vertices_iter.valid()) {
        auto src = outer_vertices_iter.vertex();
        messages.SyncStateOnOuterVertex(frag, src,
                                        ctx.result_next[v_label][src]);
        ctx.result_next[v_label][src] = 0.0;
        outer_vertices_iter.next();
      }
    }

    messages.ForceContinue();
  }

  void IncEval(const fragment_t& frag, context_t& ctx,
               message_manager_t& messages) {
    ctx.current_round++;

    double base = (1.0 - ctx.delta) / ctx.total_vertex_num +
                  ctx.delta * ctx.dangling_sum / ctx.total_vertex_num;
    ctx.dangling_sum = base * ctx.total_dangling_vnum;

    auto v_label_num = frag.vertex_label_num();
    auto e_label_num = frag.edge_label_num();

    double val;
    vertex_t v;

    while (messages.GetMessage<fragment_t, double>(frag, v, val)) {
      auto v_label = frag.vertex_label(v);
      ctx.result_next[v_label][v] += val;
    }

    if (ctx.current_round == ctx.max_round) {
      for (auto v_label = 0; v_label < v_label_num; v_label++) {
        auto inner_vertices_iter = frag.InnerVertices(v_label);
        while (inner_vertices_iter.valid()) {
          auto src = inner_vertices_iter.vertex();
          ctx.result[v_label][src] =
              base + ctx.delta * ctx.result_next[v_label][src];
          inner_vertices_iter.next();
        }
      }
    } else {
      for (auto v_label = 0; v_label < v_label_num; v_label++) {
        auto inner_vertices_iter = frag.InnerVertices(v_label);
        while (inner_vertices_iter.valid()) {
          auto src = inner_vertices_iter.vertex();
          ctx.result[v_label][src] =
              base + ctx.delta * ctx.result_next[v_label][src];
          ctx.result_next[v_label][src] = 0.0;
          inner_vertices_iter.next();
        }
      }

      for (auto v_label = 0; v_label < v_label_num; v_label++) {
        auto inner_vertices_iter = frag.InnerVertices(v_label);
        while (inner_vertices_iter.valid()) {
          auto src = inner_vertices_iter.vertex();
          int edge_num = ctx.degree[v_label][src];
          if (edge_num > 0) {
            double msg = ctx.result[v_label][src] / edge_num;
            for (auto e_label = 0; e_label < e_label_num; e_label++) {
              auto edge_iter = frag.GetOutgoingAdjList(src, e_label);
              while (edge_iter.valid()) {
                auto dst = edge_iter.neighbor();
                auto dst_label = frag.vertex_label(dst);
                ctx.result_next[dst_label][dst] += msg;
                edge_iter.next();
              }
            }
          }
          inner_vertices_iter.next();
        }
      }
      for (auto v_label = 0; v_label < v_label_num; v_label++) {
        auto outer_vertices_iter = frag.OuterVertices(v_label);
        while (outer_vertices_iter.valid()) {
          auto src = outer_vertices_iter.vertex();
          messages.SyncStateOnOuterVertex(frag, src,
                                          ctx.result_next[v_label][src]);
          ctx.result_next[v_label][src] = 0.0;
          outer_vertices_iter.next();
        }
      }
    }
  }
};

}  // namespace gs

#endif  // APPS_ANALYTICAL_ENGINE_APPS_GART_PROPERTY_PAGERANK_H_
