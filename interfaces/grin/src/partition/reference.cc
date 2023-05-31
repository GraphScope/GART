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

#include <sstream>

#include "grin/include/partition/reference.h"
#include "grin/src/predefine.h"

#ifdef GRIN_ENABLE_VERTEX_REF
GRIN_VERTEX_REF grin_get_vertex_ref_by_vertex(GRIN_GRAPH g, GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto gid = _g->Vertex2Gid(_GRIN_VERTEX_T(v));
  return gid;
}

void grin_destroy_vertex_ref(GRIN_GRAPH g, GRIN_VERTEX_REF vr) {}

GRIN_VERTEX grin_get_vertex_from_vertex_ref(GRIN_GRAPH g, GRIN_VERTEX_REF vr) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  _GRIN_VERTEX_T v;
  if (_g->Gid2Vertex(vr, v)) {
    return v.GetValue();
  }
  return GRIN_NULL_VERTEX;
}

GRIN_PARTITION grin_get_master_partition_from_vertex_ref(GRIN_GRAPH g,
                                                         GRIN_VERTEX_REF vr) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->GetFragIdFromGid(vr);
}

const char* grin_serialize_vertex_ref(GRIN_GRAPH g, GRIN_VERTEX_REF vr) {
  std::stringstream ss;
  ss << vr;
  auto len = ss.str().length() + 1;
  char* out = new char[len];
  snprintf(out, len, "%s", ss.str().c_str());
  return out;
}

#ifdef GRIN_TRAIT_FAST_VERTEX_REF
long long int grin_serialize_vertex_ref_as_int64(GRIN_GRAPH g,
                                                 GRIN_VERTEX_REF vr) {
  return vr;
}

GRIN_VERTEX_REF grin_deserialize_int64_to_vertex_ref(GRIN_GRAPH g,
                                                     long long int svr) {
  return svr;
}
#endif

void grin_destroy_serialized_vertex_ref(GRIN_GRAPH g, const char* name) {
  delete[] name;
}

GRIN_VERTEX_REF grin_deserialize_to_vertex_ref(GRIN_GRAPH g, const char* msg) {
  std::stringstream ss(msg);
  GRIN_VERTEX_REF gid;
  ss >> gid;
  return gid;
}

bool grin_is_master_vertex(GRIN_GRAPH g, GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->IsInnerVertex(_GRIN_VERTEX_T(v));
}

bool grin_is_mirror_vertex(GRIN_GRAPH g, GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->IsOuterVertex(_GRIN_VERTEX_T(v));
}
#endif

#ifdef GRIN_TRAIT_MASTER_VERTEX_MIRROR_PARTITION_LIST
GRIN_PARTITION_LIST grin_get_master_vertex_mirror_partition_list(GRIN_GRAPH,
                                                                 GRIN_VERTEX);
#endif

#ifdef GRIN_TRAIT_MASTER_VERTEX_MIRROR_PARTITION_LIST
GRIN_PARTITION_LIST grin_get_mirror_vertex_mirror_partition_list(GRIN_GRAPH,
                                                                 GRIN_VERTEX);
#endif

#ifdef GRIN_ENABLE_EDGE_REF
GRIN_EDGE_REF grin_get_edge_ref_for_edge(GRIN_GRAPH, GRIN_EDGE);

void grin_destroy_edge_ref(GRIN_GRAPH, GRIN_EDGE_REF);

GRIN_EDGE grin_get_edge_from_edge_ref(GRIN_GRAPH, GRIN_EDGE_REF);

GRIN_PARTITION grin_get_master_partition_from_edge_ref(GRIN_GRAPH,
                                                       GRIN_EDGE_REF);

const char* grin_serialize_edge_ref(GRIN_GRAPH, GRIN_EDGE_REF);

void grin_destroy_serialized_edge_ref(GRIN_GRAPH, GRIN_EDGE_REF);

GRIN_EDGE_REF grin_deserialize_to_edge_ref(GRIN_GRAPH, const char*);

bool grin_is_master_edge(GRIN_GRAPH, GRIN_EDGE);

bool grin_is_mirror_edge(GRIN_GRAPH, GRIN_EDGE);
#endif

#ifdef GRIN_TRAIT_MASTER_EDGE_MIRROR_PARTITION_LIST
GRIN_PARTITION_LIST grin_get_master_edge_mirror_partition_list(GRIN_GRAPH,
                                                               GRIN_EDGE);
#endif

#ifdef GRIN_TRAIT_MASTER_EDGE_MIRROR_PARTITION_LIST
GRIN_PARTITION_LIST grin_get_mirror_edge_mirror_partition_list(GRIN_GRAPH,
                                                               GRIN_EDGE);
#endif