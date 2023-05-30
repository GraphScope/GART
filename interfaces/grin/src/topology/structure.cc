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

#include "grin/src/predefine.h"
#include "grin/include/topology/structure.h"

GRIN_GRAPH grin_get_graph_from_storage(int, char**);

void grin_destroy_graph(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  delete _g;
}

// Graph
#if defined(GRIN_ASSUME_HAS_DIRECTED_GRAPH) && defined(GRIN_ASSUME_HAS_UNDIRECTED_GRAPH)
bool grin_is_directed(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->directed();
}
#endif

#ifdef GRIN_ASSUME_HAS_MULTI_EDGE_GRAPH
bool grin_is_multigraph(GRIN_GRAPH g) {
  return true;
}
#endif

size_t grin_get_vertex_num(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->GetVerticesNum();
}

size_t grin_get_edge_num(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->GetEdgeNum();
}


// Vertex
void grin_destroy_vertex(GRIN_GRAPH g, GRIN_VERTEX v) { }

bool grin_equal_vertex(GRIN_GRAPH g, GRIN_VERTEX v1, GRIN_VERTEX v2) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->Vertex2Gid(_GRIN_VERTEX_T(v1)) == _g->Vertex2Gid(_GRIN_VERTEX_T(v2));
}

/*
#ifdef GRIN_WITH_VERTEX_ORIGINAL_ID
void grin_destroy_vertex_original_id(GRIN_GRAPH g, GRIN_VERTEX_ORIGINAL_ID oid) {
  auto _oid = static_cast<GRIN_VERTEX_ORIGINAL_ID_T*>(oid);
  delete _oid;
}

GRIN_DATATYPE grin_get_vertex_original_id_type(GRIN_GRAPH g) {
  return GRIN_DATATYPE_ENUM<GRIN_VERTEX_ORIGINAL_ID_T>::value;
}

GRIN_VERTEX_ORIGINAL_ID grin_get_vertex_original_id(GRIN_GRAPH g, GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _v = static_cast<GRIN_VERTEX_T*>(v);
  auto oid = new GRIN_VERTEX_ORIGINAL_ID_T();
  *oid = _g->Vertex2Gid(*_v);  // In gart, gid equals to oid
  return oid;
}
#endif

#if defined(GRIN_WITH_VERTEX_ORIGINAL_ID) && !defined(GRIN_ASSUME_BY_TYPE_VERTEX_ORIGINAL_ID)
GRIN_VERTEX grin_get_vertex_from_original_id(GRIN_GRAPH, GRIN_VERTEX_ORIGINAL_ID);
#endif

// Data
void grin_destroy_value(GRIN_GRAPH g, GRIN_DATATYPE dt, const void* value) {
  switch (dt) {
    case GRIN_DATATYPE::Int32:
        delete static_cast<int32_t*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::UInt32:
        delete static_cast<uint32_t*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::Int64:
        delete static_cast<int64_t*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::UInt64:
        delete static_cast<uint64_t*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::Float:
        delete static_cast<float*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::Double:
        delete static_cast<double*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::String:
        delete static_cast<std::string*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::Date32:
        delete static_cast<int32_t*>(const_cast<void*>(value));
        break;
    case GRIN_DATATYPE::Date64:
        delete static_cast<int64_t*>(const_cast<void*>(value));
        break;
    default:
        break;
    }
}

void grin_destroy_name(GRIN_GRAPH g, const char* name) {
  delete[] name;
}

#ifdef GRIN_WITH_VERTEX_DATA
GRIN_DATATYPE grin_get_vertex_data_type(GRIN_GRAPH, GRIN_VERTEX);

const void* grin_get_vertex_data_value(GRIN_GRAPH, GRIN_VERTEX);
#endif
*/
// Edge
void grin_destroy_edge(GRIN_GRAPH g, GRIN_EDGE e) {
  auto _e = static_cast<GRIN_EDGE_T*>(e);
  delete _e;
}

GRIN_VERTEX grin_get_src_vertex_from_edge(GRIN_GRAPH g, GRIN_EDGE e) {
  auto _e = static_cast<GRIN_EDGE_T*>(e);
  return _e->src;
}

GRIN_VERTEX grin_get_dst_vertex_from_edge(GRIN_GRAPH g, GRIN_EDGE e) {
  auto _e = static_cast<GRIN_EDGE_T*>(e);
  return _e->dst;
}

#ifdef GRIN_WITH_EDGE_DATA
GRIN_DATATYPE grin_get_edge_data_datatype(GRIN_GRAPH, GRIN_EDGE);

const void* grin_get_edge_data_value(GRIN_GRAPH, GRIN_EDGE);
#endif