/** Copyright 2020 Alibaba Group Holding Limited.
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

extern "C" {
#include "grin/include/property/topology.h"
}
#include "grin/src/predefine.h"

#ifdef GRIN_WITH_VERTEX_PROPERTY
size_t grin_get_vertex_num_by_type(GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->GetVerticesNum(vt);
}
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY
size_t grin_get_edge_num_by_type(GRIN_GRAPH g, GRIN_EDGE_TYPE et) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->GetEdgeNum(et);
}
#endif

/*
#ifdef GRIN_ASSUME_BY_TYPE_VERTEX_ORIGINAL_ID
GRIN_VERTEX grin_get_vertex_from_original_id_by_type(GRIN_GRAPH g,
GRIN_VERTEX_TYPE vt, GRIN_VERTEX_ORIGINAL_ID oid) { auto _g =
static_cast<GRIN_GRAPH_T*>(g); auto _oid =
static_cast<GRIN_VERTEX_ORIGINAL_ID_T*>(oid);  // oid equals to gid in gart auto
v = new GRIN_VERTEX_T(); if (_g->Gid2Vertex(*_oid, *v)) { return v;
  }
  return GRIN_NULL_VERTEX;
}
#endif
*/

#ifdef GRIN_TRAIT_SELECT_TYPE_FOR_VERTEX_LIST
GRIN_VERTEX_LIST grin_select_type_for_vertex_list(GRIN_GRAPH g,
                                                  GRIN_VERTEX_TYPE vt,
                                                  GRIN_VERTEX_LIST vl) {
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  auto new_vl = new GRIN_VERTEX_LIST_T();
  new_vl->all_master_mirror = _vl->all_master_mirror;
  new_vl->vertex_types.push_back(vt);
  return new_vl;
}
#endif

#ifdef GRIN_TRAIT_SELECT_TYPE_FOR_EDGE_LIST
GRIN_EDGE_LIST grin_select_type_for_edge_list(GRIN_GRAPH, GRIN_EDGE_TYPE,
                                              GRIN_EDGE_LIST);
#endif

#ifdef GRIN_TRAIT_SELECT_NEIGHBOR_TYPE_FOR_ADJACENT_LIST
GRIN_ADJACENT_LIST grin_select_neighbor_type_for_adjacent_list(
    GRIN_GRAPH, GRIN_VERTEX_TYPE, GRIN_ADJACENT_LIST){};
#endif

#ifdef GRIN_TRAIT_SELECT_EDGE_TYPE_FOR_ADJACENT_LIST
GRIN_ADJACENT_LIST grin_select_edge_type_for_adjacent_list(
    GRIN_GRAPH g, GRIN_EDGE_TYPE et, GRIN_ADJACENT_LIST adj_list) {
  auto _adj_list = static_cast<GRIN_ADJACENT_LIST_T*>(adj_list);
  auto new_adj_list = new GRIN_ADJACENT_LIST_T();
  new_adj_list->v = _adj_list->v;
  new_adj_list->dir = _adj_list->dir;
  new_adj_list->edge_types.push_back(et);
  return new_adj_list;
}
#endif