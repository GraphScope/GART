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
#include "grin/include/topology/vertexlist.h"

#ifdef GRIN_ENABLE_VERTEX_LIST
GRIN_VERTEX_LIST grin_get_vertex_list(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  GRIN_VERTEX_LIST_T* vl = new GRIN_VERTEX_LIST_T();
  vl->all_master_mirror = 0;
  auto vertex_label_num = _g->vertex_label_num();
  for (auto idx = 0; idx < vertex_label_num; ++idx) {
    vl->vertex_types.push_back(idx);
  }
  return vl;
}

void grin_destroy_vertex_list(GRIN_GRAPH g, GRIN_VERTEX_LIST vl) {
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  delete _vl;
}
   
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST_ITERATOR
GRIN_VERTEX_LIST_ITERATOR grin_get_vertex_list_begin(GRIN_GRAPH g, GRIN_VERTEX_LIST vl) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  auto iter = new GRIN_VERTEX_LIST_ITERATOR_T();
  iter->all_master_mirror = _vl->all_master_mirror;
  for (size_t idx = 0; idx < _vl->vertex_types.size(); ++idx) {
    iter->vertex_types.push_back(_vl->vertex_types[idx]);
  }
  gart::VertexIterator vertex_iter;
  for (size_t idx = 0; idx <  _vl->vertex_types.size(); ++idx) {
    if (_vl->all_master_mirror == 0) {
      vertex_iter = _g->Vertices(_vl->vertex_types[idx]);
    } else if (_vl->all_master_mirror == 1) {
      vertex_iter = _g->InnerVertices(_vl->vertex_types[idx]);
    } else {
      vertex_iter = _g->OuterVertices(_vl->vertex_types[idx]);
    }
    iter->current_index = idx;
    if (vertex_iter.valid()) {
      iter->current_iterator = vertex_iter;
      break;
    }
  }
  return iter;
}

void grin_destroy_vertex_list_iter(GRIN_GRAPH g, GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  delete _iter;
}

void grin_get_next_vertex_list_iter(GRIN_GRAPH g, GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  auto vertex_iter = _iter->current_iterator;
  vertex_iter.next();
  if (vertex_iter.valid()) {
    _iter->current_iterator = vertex_iter;
    return;
  } else {
    _iter->current_index++;
    if (_iter->current_index == _iter->vertex_types.size()) {
      return;
    }
    auto v_type = _iter->vertex_types[_iter->current_index];

    if (_iter->all_master_mirror == 0) {
      vertex_iter = _g->Vertices(v_type);
    } else if (_iter->all_master_mirror == 1) {
      vertex_iter = _g->InnerVertices(v_type);
    } else {
      vertex_iter = _g->OuterVertices(v_type);
    }
    _iter->current_iterator = vertex_iter;

    while (!vertex_iter.valid()) {
      _iter->current_index++;
      if (_iter->current_index == _iter->vertex_types.size()) {
        break;
      }
      v_type = _iter->vertex_types[_iter->current_index];
      if (_iter->all_master_mirror == 0) {
        vertex_iter = _g->Vertices(v_type);
      } else if (_iter->all_master_mirror == 1) {
        vertex_iter = _g->InnerVertices(v_type);
      } else {
        vertex_iter = _g->OuterVertices(v_type);
      }
      _iter->current_iterator = vertex_iter;
    }
  }
}

bool grin_is_vertex_list_end(GRIN_GRAPH g, GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  if (_iter->current_index == _iter->vertex_types.size()) {
    return true;
  }
  if (!_iter->current_iterator.valid()) {
    return true;
  }
  return false;
}

GRIN_VERTEX grin_get_vertex_from_iter(GRIN_GRAPH g, GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  auto vertex_iter = _iter->current_iterator;
  if (vertex_iter.valid()) {
    GRIN_VERTEX vertex = vertex_iter.vertex().GetValue();
    return vertex;
  }
  return GRIN_NULL_VERTEX;
}
#endif