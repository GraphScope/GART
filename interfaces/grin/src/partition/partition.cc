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

#include "grin/include/partition/partition.h"
#include "grin/src/predefine.h"

#ifdef GRIN_ENABLE_GRAPH_PARTITION

GRIN_PARTITIONED_GRAPH grin_get_partitioned_graph_from_storage(int argc,
                                                               char** argv) {
  if (argc < 5) {
    return nullptr;
  }
  GRIN_PARTITIONED_GRAPH_T* pg = new GRIN_PARTITIONED_GRAPH_T();
  pg->etcd_endpoint = argv[0];
  pg->total_partition_num = std::stoul(argv[1]);
  pg->local_id = std::stoul(argv[2]);
  pg->read_epoch = std::stoi(argv[3]);
  pg->meta_prefix = argv[4];
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
  pl->push_back(_pg->local_id);
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
  GRIN_GRAPH_T* fragment = new GRIN_GRAPH_T();
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

  return reinterpret_cast<GRIN_GRAPH>(fragment);
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