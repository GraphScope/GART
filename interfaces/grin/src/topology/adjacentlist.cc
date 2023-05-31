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

#include "grin/include/topology/adjacentlist.h"
#include "grin/src/predefine.h"

#ifdef GRIN_ENABLE_ADJACENT_LIST
GRIN_ADJACENT_LIST grin_get_adjacent_list(GRIN_GRAPH g, GRIN_DIRECTION dir,
                                          GRIN_VERTEX v) {
  if (dir == GRIN_DIRECTION::BOTH)
    return GRIN_NULL_LIST;
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto adj_list = new GRIN_ADJACENT_LIST_T();
  adj_list->v = v;
  adj_list->dir = dir;
  for (auto idx = 0; idx < _g->edge_label_num(); ++idx) {
    adj_list->edge_types.push_back(idx);
  }
  return adj_list;
}

void grin_destroy_adjacent_list(GRIN_GRAPH g, GRIN_ADJACENT_LIST adj_list) {
  auto _adj_list = static_cast<GRIN_ADJACENT_LIST_T*>(adj_list);
  delete _adj_list;
}
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST_ITERATOR
GRIN_ADJACENT_LIST_ITERATOR grin_get_adjacent_list_begin(
    GRIN_GRAPH g, GRIN_ADJACENT_LIST adj_list) {
  auto _adj_list = static_cast<GRIN_ADJACENT_LIST_T*>(adj_list);
  if (_adj_list->dir == GRIN_DIRECTION::BOTH)
    return GRIN_NULL_LIST_ITERATOR;
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _v = _adj_list->v;
  auto edge_iter = new GRIN_ADJACENT_LIST_ITERATOR_T();
  edge_iter->v = _v;
  edge_iter->dir = _adj_list->dir;
  for (size_t idx = 0; idx < _adj_list->edge_types.size(); ++idx) {
    edge_iter->edge_types.push_back(_adj_list->edge_types[idx]);
  }
  for (size_t idx = 0; idx < _adj_list->edge_types.size(); ++idx) {
    edge_iter->current_index = idx;
    gart::EdgeIterator gart_edge_iter;
    if (_adj_list->dir == GRIN_DIRECTION::IN) {
      gart_edge_iter = _g->GetIncomingAdjList(_GRIN_VERTEX_T(_v),
                                              _adj_list->edge_types[idx]);
    } else {
      gart_edge_iter = _g->GetOutgoingAdjList(_GRIN_VERTEX_T(_v),
                                              _adj_list->edge_types[idx]);
    }
    if (gart_edge_iter.valid()) {
      edge_iter->current_iterator = gart_edge_iter;
      break;
    }
  }
  return edge_iter;
}

void grin_destroy_adjacent_list_iter(GRIN_GRAPH g,
                                     GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  delete _iter;
}

void grin_get_next_adjacent_list_iter(GRIN_GRAPH g,
                                      GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  auto gart_edge_iter = _iter->current_iterator;
  gart_edge_iter.next();
  if (gart_edge_iter.valid()) {
    _iter->current_iterator = gart_edge_iter;
    return;
  } else {
    _iter->current_index++;
    if (_iter->current_index >= _iter->edge_types.size()) {
      return;
    }
    auto etype = _iter->edge_types[_iter->current_index];
    if (_iter->dir == GRIN_DIRECTION::IN) {
      gart_edge_iter = _g->GetIncomingAdjList(_GRIN_VERTEX_T(_iter->v), etype);
    } else {
      gart_edge_iter = _g->GetOutgoingAdjList(_GRIN_VERTEX_T(_iter->v), etype);
    }
    _iter->current_iterator = gart_edge_iter;
    while (!gart_edge_iter.valid()) {
      _iter->current_index++;
      if (_iter->current_index == _iter->edge_types.size()) {
        return;
      }
      etype = _iter->edge_types[_iter->current_index];
      if (_iter->dir == GRIN_DIRECTION::IN) {
        gart_edge_iter =
            _g->GetIncomingAdjList(_GRIN_VERTEX_T(_iter->v), etype);
      } else {
        gart_edge_iter =
            _g->GetOutgoingAdjList(_GRIN_VERTEX_T(_iter->v), etype);
      }
      _iter->current_iterator = gart_edge_iter;
    }
  }
}

bool grin_is_adjacent_list_end(GRIN_GRAPH g, GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  if (_iter->current_index == _iter->edge_types.size()) {
    return true;
  }
  if (!_iter->current_iterator.valid()) {
    return true;
  }
  return false;
}

GRIN_VERTEX grin_get_neighbor_from_adjacent_list_iter(
    GRIN_GRAPH g, GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  auto edge_iter = _iter->current_iterator;
  if (edge_iter.valid()) {
    GRIN_VERTEX dst = edge_iter.neighbor().GetValue();
    return dst;
  }
  return GRIN_NULL_VERTEX;
}

GRIN_EDGE grin_get_edge_from_adjacent_list_iter(
    GRIN_GRAPH g, GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  auto edge_iter = _iter->current_iterator;
  if (edge_iter.valid()) {
    auto edge = new GRIN_EDGE_T();
    if (_iter->dir == GRIN_DIRECTION::IN) {
      edge->src = edge_iter.neighbor().GetValue();
      edge->dst = _iter->v;
    } else {
      edge->src = _iter->v;
      edge->dst = edge_iter.neighbor().GetValue();
    }
    edge->dir = _iter->dir;
    edge->etype = _iter->edge_types[_iter->current_index];
    edge->edata = edge_iter.get_data();
    return edge;
  }
  std::cout << "edge_iter is not valid" << std::endl;
  return GRIN_NULL_EDGE;
}
#endif