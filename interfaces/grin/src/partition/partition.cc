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

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"
#include "vineyard/client/client.h"
#include "vineyard/client/ds/blob.h"
#include "vineyard/common/util/json.h"

#include "grin/src/predefine.h"

#include "grin/include/include/partition/partition.h"

#ifdef GRIN_ENABLE_GRAPH_PARTITION

GRIN_PARTITIONED_GRAPH grin_get_partitioned_graph_from_storage(
    const char* uri) {
  GRIN_PARTITIONED_GRAPH_T* pg = new GRIN_PARTITIONED_GRAPH_T();
  std::string uri_str(uri);
  URI config(uri_str);
  std::string protocol = config.getProtocol();
  assert(protocol == "gart");
  std::string etcd_endpoint = config.getEtcdEndpoint();
  pg->etcd_endpoint = etcd_endpoint;
  auto params = config.getParams();
  pg->total_partition_num = std::stoul(params["total_partition_num"]);
  auto start_partition_id = std::stoul(params["start_partition_id"]);
  auto local_partition_num = std::stoul(params["local_partition_num"]);
  for (uint32_t idx = 0; idx < local_partition_num; ++idx) {
    pg->local_partition_list.push_back(start_partition_id + idx);
  }
  pg->read_epoch = std::stoi(params["read_epoch"]);
  pg->meta_prefix = params["meta_prefix"];
  return pg;
}

void grin_destroy_partitioned_graph(GRIN_PARTITIONED_GRAPH pg) {
  auto _pg = static_cast<GRIN_PARTITIONED_GRAPH_T*>(pg);
  delete _pg;
}

size_t grin_get_total_partitions_number(GRIN_PARTITIONED_GRAPH pg) {
  auto _pg = static_cast<GRIN_PARTITIONED_GRAPH_T*>(pg);
  return _pg->total_partition_num;
}

GRIN_PARTITION_LIST grin_get_local_partition_list(GRIN_PARTITIONED_GRAPH pg) {
  auto _pg = static_cast<GRIN_PARTITIONED_GRAPH_T*>(pg);
  auto pl = new GRIN_PARTITION_LIST_T();
  for (auto& pid : _pg->local_partition_list) {
    pl->push_back(pid);
  }
  return pl;
}

void grin_destroy_partition_list(GRIN_PARTITIONED_GRAPH pg,
                                 GRIN_PARTITION_LIST pl) {
  auto _pl = static_cast<GRIN_PARTITION_LIST_T*>(pl);
  delete _pl;
}

GRIN_PARTITION_LIST grin_create_partition_list(GRIN_PARTITIONED_GRAPH pg) {
  auto pl = new GRIN_PARTITION_LIST_T();
  return pl;
}

bool grin_insert_partition_to_list(GRIN_PARTITIONED_GRAPH pg,
                                   GRIN_PARTITION_LIST pl, GRIN_PARTITION p) {
  auto _pl = static_cast<GRIN_PARTITION_LIST_T*>(pl);
  _pl->push_back(p);
  return true;
}

size_t grin_get_partition_list_size(GRIN_PARTITIONED_GRAPH pg,
                                    GRIN_PARTITION_LIST pl) {
  auto _pl = static_cast<GRIN_PARTITION_LIST_T*>(pl);
  return _pl->size();
}

GRIN_PARTITION grin_get_partition_from_list(GRIN_PARTITIONED_GRAPH pg,
                                            GRIN_PARTITION_LIST pl,
                                            size_t idx) {
  auto _pl = static_cast<GRIN_PARTITION_LIST_T*>(pl);
  return (*_pl)[idx];
}

bool grin_equal_partition(GRIN_PARTITIONED_GRAPH pg, GRIN_PARTITION p1,
                          GRIN_PARTITION p2) {
  return (p1 == p2);
}

void grin_destroy_partition(GRIN_PARTITIONED_GRAPH pg, GRIN_PARTITION p) {}

const void* grin_get_partition_info(GRIN_PARTITIONED_GRAPH pg,
                                    GRIN_PARTITION p) {
  return NULL;
}

GRIN_GRAPH grin_get_local_graph_by_partition(GRIN_PARTITIONED_GRAPH pg,
                                             GRIN_PARTITION p) {
  auto _pg = static_cast<GRIN_PARTITIONED_GRAPH_T*>(pg);
  GRIN_FRAGMENT_T* fragment = new GRIN_FRAGMENT_T();
  std::shared_ptr<etcd::Client> etcd_client =
      std::make_shared<etcd::Client>(_pg->etcd_endpoint);
  std::string schema_key =
      _pg->meta_prefix + "gart_schema_p" + std::to_string(p);
  etcd::Response response = etcd_client->get(schema_key).get();
  assert(response.is_ok());
  std::string graph_schema_config_str = response.value().as_string();

  schema_key = _pg->meta_prefix + "gart_blob_m" + std::to_string(0) + "_p" +
               std::to_string(p) + "_e" + std::to_string(_pg->read_epoch);
  response = etcd_client->get(schema_key).get();
  assert(response.is_ok());
  std::string graph_blob_config_str = response.value().as_string();

  vineyard::json graph_schema_config =
      vineyard::json::parse(graph_schema_config_str);
  vineyard::json graph_blob_config =
      vineyard::json::parse(graph_blob_config_str);

  fragment->Init(graph_blob_config, graph_schema_config);

  GRIN_GRAPH_T* grin_graph = new GRIN_GRAPH_T();
  grin_graph->frag = fragment;

  GRIN_LABEL_NAME_MAP_T* vertex_label_name_map = new GRIN_LABEL_NAME_MAP_T();

  for (auto idx = 0; idx < fragment->vertex_label_num(); idx++) {
    vertex_label_name_map->emplace(idx, fragment->GetVertexLabelName(idx));
  }

  grin_graph->vertex_label2name_map = vertex_label_name_map;

  GRIN_LABEL_NAME_MAP_T* edge_label_name_map = new GRIN_LABEL_NAME_MAP_T();

  for (auto idx = 0; idx < fragment->edge_label_num(); idx++) {
    edge_label_name_map->emplace(idx, fragment->GetEdgeLabelName(idx));
  }

  grin_graph->edge_label2name_map = edge_label_name_map;

  GRIN_PROPERTY_NAME_MAP_T* vertex_property_name_map =
      new GRIN_PROPERTY_NAME_MAP_T();

  for (auto idx = 0; idx < fragment->vertex_label_num(); idx++) {
    auto prop_size = fragment->vertex_property_num(idx);
    for (auto prop_id = 0; prop_id < prop_size; prop_id++) {
      vertex_property_name_map->emplace(
          std::make_pair(idx, prop_id),
          fragment->GetVertexPropName(idx, prop_id));
    }
  }

  grin_graph->vertex_property2name_map = vertex_property_name_map;

  GRIN_PROPERTY_NAME_MAP_T* edge_property_name_map =
      new GRIN_PROPERTY_NAME_MAP_T();

  for (auto idx = 0; idx < fragment->edge_label_num(); idx++) {
    auto prop_size = fragment->edge_property_num(idx);
    for (auto prop_id = 0; prop_id < prop_size; prop_id++) {
      edge_property_name_map->emplace(std::make_pair(idx, prop_id),
                                      fragment->GetEdgePropName(idx, prop_id));
    }
  }

  grin_graph->edge_property2name_map = edge_property_name_map;

  return grin_graph;
}
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_PARTITION
GRIN_PARTITION grin_get_partition_by_id(GRIN_PARTITIONED_GRAPH g,
                                        GRIN_PARTITION_ID pid) {
  return pid;
}

GRIN_PARTITION_ID grin_get_partition_id(GRIN_PARTITIONED_GRAPH g,
                                        GRIN_PARTITION p) {
  return p;
}

#endif