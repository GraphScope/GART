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

#include "grape/grape.h"
#include "grape/util.h"

#include "gart/gae/fragment/gart_fragment.h"
#include "gart/grin/include/partition/partition.h"
#include "gart/grin/include/partition/reference.h"
#include "gart/grin/include/partition/topology.h"
#include "gart/grin/include/property/property.h"
#include "gart/grin/include/property/propertylist.h"
#include "gart/grin/include/property/propertytable.h"
#include "gart/grin/include/property/topology.h"
#include "gart/grin/include/property/type.h"
#include "gart/grin/include/topology/adjacentlist.h"
#include "gart/grin/include/topology/structure.h"
#include "gart/grin/include/topology/vertexlist.h"
#include "gart/grin/src/predefine.h"

int main(int argc, char** argv) {
  grape::InitMPIComm();
  {
    grape::CommSpec comm_spec;
    comm_spec.Init(MPI_COMM_WORLD);
    int read_epoch = 4;
    std::string etcd_endpoint = "http://127.0.0.1:23799";
    char** argv = new char*[4];
    argv[0] = new char[etcd_endpoint.length() + 1];
    argv[1] = new char[std::to_string(comm_spec.fnum()).length() + 1];
    argv[2] = new char[std::to_string(comm_spec.fid()).length() + 1];
    argv[3] = new char[std::to_string(read_epoch).length() + 1];

    strcpy(argv[0], etcd_endpoint.c_str());
    strcpy(argv[1], std::to_string(comm_spec.fnum()).c_str());
    strcpy(argv[2], std::to_string(comm_spec.fid()).c_str());
    strcpy(argv[3], std::to_string(read_epoch).c_str());
    auto pg = grin_get_partitioned_graph_from_storage(4, argv);
    auto local_partitions = grin_get_local_partition_list(pg);
    size_t pnum = grin_get_partition_list_size(pg, local_partitions);
    std::cout << "partition number = " << pnum << std::endl;
    auto pg_partition = grin_get_partition_from_list(pg, local_partitions, 0);
    auto fragment = grin_get_local_graph_by_partition(pg, pg_partition);
    GRIN_GRAPH_T* _g = static_cast<GRIN_GRAPH_T*>(fragment);
    std::cout << "vertex list test ..." << std::endl;
    auto vertex_list = grin_get_vertex_list(fragment);
    auto vertex_iter = grin_get_vertex_list_begin(fragment, vertex_list);
    auto inner_vertex_list =
        grin_select_master_for_vertex_list(fragment, vertex_list);
    auto outer_vertex_list =
        grin_select_mirror_for_vertex_list(fragment, vertex_list);
    /*
    while (!grin_is_vertex_list_end(fragment, vertex_iter)) {
      auto v = grin_get_vertex_from_iter(fragment, vertex_iter);
      auto _v = static_cast<GRIN_VERTEX_T*>(v);
      //std::cout << "vertex label = " << _g->vertex_label(*_v) << " offset = "
    << _g->vertex_offset(*_v)
      //          << " is inner vertex = " << _g->IsInnerVertex(*_v) << " fid =
    " << _g->fid()<< std::endl; grin_destroy_vertex(fragment, v);
      grin_get_next_vertex_list_iter(fragment, vertex_iter);
    }
    grin_destroy_vertex_list_iter(fragment, vertex_iter);

    std::cout << "inner vertex list test ..." << std::endl;
    auto inner_vertex_iter = grin_get_vertex_list_begin(fragment,
    inner_vertex_list); while (!grin_is_vertex_list_end(fragment,
    inner_vertex_iter)) { auto v = grin_get_vertex_from_iter(fragment,
    inner_vertex_iter); auto _v = static_cast<GRIN_VERTEX_T*>(v);
      //std::cout << "inner vertex label = " << _g->vertex_label(*_v)
      //          << " offset = " << _g->vertex_offset(*_v) << " fid = " <<
    _g->fid()<< std::endl;

      auto vertex_type = grin_get_vertex_type(fragment, v);
      auto vertex_property_table =
    grin_get_vertex_property_table_by_type(fragment, vertex_type); auto
    vertex_property_list = grin_get_vertex_property_list_by_type(fragment,
    vertex_type); size_t v_prop_size =
    grin_get_vertex_property_list_size(fragment, vertex_property_list); for
    (auto idx = 1; idx < v_prop_size; ++idx) { auto vertex_property =
    grin_get_vertex_property_from_id(fragment, vertex_type, idx); auto v_data =
    grin_get_value_from_vertex_property_table(fragment,vertex_property_table, v,
    vertex_property); auto v_data_type =
    grin_get_vertex_property_data_type(fragment, vertex_property); if
    (v_data_type == GRIN_DATATYPE::Int32) {
          //std::cout << " vdata = " <<
    *(static_cast<int32_t*>(const_cast<void*>(v_data))) << std::endl; } else if
    (v_data_type == GRIN_DATATYPE::Double) {
          //std::cout << " vdata = " <<
    *(static_cast<double*>(const_cast<void*>(v_data))) << std::endl; } else {
          //std::cout << "unsupported data type ..." << std::endl;
        }
        grin_destroy_vertex_property(fragment, vertex_property);
        grin_destroy_value(fragment, v_data_type, v_data);
      }

      auto vertex_property_row =
    grin_get_row_from_vertex_property_table(fragment, vertex_property_table, v,
    vertex_property_list); for (auto idx = 1; idx < v_prop_size; idx++) { auto
    vertex_property = grin_get_vertex_property_from_id(fragment, vertex_type,
    idx); auto v_data_type = grin_get_vertex_property_data_type(fragment,
    vertex_property); auto v_data = grin_get_value_from_row(fragment,
    vertex_property_row, v_data_type, idx); if (v_data_type ==
    GRIN_DATATYPE::Int32) {
          //std::cout << "row vdata = " <<
    *(static_cast<int32_t*>(const_cast<void*>(v_data))) << std::endl; } else if
    (v_data_type == GRIN_DATATYPE::Double) {
          //std::cout << "row vdata = " <<
    *(static_cast<double*>(const_cast<void*>(v_data))) << std::endl; } else {
          //std::cout << "unsupported data type ..." << std::endl;
        }
        grin_destroy_vertex_property(fragment, vertex_property);
        grin_destroy_value(fragment, v_data_type, v_data);
      }
      grin_destroy_row(fragment, vertex_property_row);
      grin_destroy_vertex(fragment, v);
      grin_destroy_vertex_property_list(fragment, vertex_property_list);
      grin_destroy_vertex_property_table(fragment, vertex_property_table);
      grin_destroy_vertex_type(fragment, vertex_type);
      grin_get_next_vertex_list_iter(fragment, inner_vertex_iter);
    }
    // grin_destroy_vertex_list_iter(fragment, inner_vertex_iter);

    std::cout << "outer vertex list test ..." << std::endl;

    auto outer_vertex_iter = grin_get_vertex_list_begin(fragment,
    outer_vertex_list);

    while (!grin_is_vertex_list_end(fragment, outer_vertex_iter)) {
      auto v = grin_get_vertex_from_iter(fragment, outer_vertex_iter);
      auto _v = static_cast<GRIN_VERTEX_T*>(v);
      //std::cout << "outer vertex label = " << _g->vertex_label(*_v)
      //          << " offset = " << _g->vertex_offset(*_v) << " fid = " <<
    _g->fid()<< std::endl; grin_destroy_vertex(fragment, v);
      grin_get_next_vertex_list_iter(fragment, outer_vertex_iter);
    }
    grin_destroy_vertex_list_iter(fragment, outer_vertex_iter);

    std::cout << "start vertex type filter test ..." << std::endl;
    auto vtype = new GRIN_VERTEX_TYPE_T(0);
    auto inner_select_vertex_list = grin_select_type_for_vertex_list(fragment,
    vtype, inner_vertex_list); auto inner_select_vertex_iter =
    grin_get_vertex_list_begin(fragment, inner_select_vertex_list); while
    (!grin_is_vertex_list_end(fragment, inner_select_vertex_iter)) { auto v =
    grin_get_vertex_from_iter(fragment, inner_select_vertex_iter); auto _v =
    static_cast<GRIN_VERTEX_T*>(v);
      //std::cout << "inner select vertex label = " << _g->vertex_label(*_v)
      //          << " offset = " << _g->vertex_offset(*_v) << " fid = " <<
    _g->fid()<< std::endl; grin_destroy_vertex(fragment, v);
      grin_get_next_vertex_list_iter(fragment, inner_select_vertex_iter);
    }
    grin_destroy_vertex_type(fragment, vtype);
    grin_destroy_vertex_list_iter(fragment, inner_select_vertex_iter);

    grin_destroy_vertex_list(fragment, inner_select_vertex_list);
    //grin_destroy_vertex_list(fragment, vertex_list);
    //grin_destroy_vertex_list(fragment, inner_vertex_list);
    grin_destroy_vertex_list(fragment, outer_vertex_list);
    */
    std::cout << "start edge test ..." << std::endl;

    vertex_list = grin_get_vertex_list(fragment);
    inner_vertex_list =
        grin_select_master_for_vertex_list(fragment, vertex_list);
    auto inner_vertex_iter =
        grin_get_vertex_list_begin(fragment, inner_vertex_list);
    while (!grin_is_vertex_list_end(fragment, inner_vertex_iter)) {
      auto v = grin_get_vertex_from_iter(fragment, inner_vertex_iter);
      auto _v = static_cast<GRIN_VERTEX_T*>(v);
      auto adj_list = grin_get_adjacent_list(fragment, GRIN_DIRECTION::OUT, v);
      auto edge_iter = grin_get_adjacent_list_begin(fragment, adj_list);
      while (!grin_is_adjacent_list_end(fragment, edge_iter)) {
        auto edge = grin_get_edge_from_adjacent_list_iter(fragment, edge_iter);
        auto edge_type = grin_get_edge_type(fragment, edge);
        auto edge_property_list =
            grin_get_edge_property_list_by_type(fragment, edge_type);
        auto edge_prop_row =
            grin_get_edge_row(fragment, edge, edge_property_list);
        size_t edge_prop_size =
            grin_get_edge_property_list_size(fragment, edge_property_list);

        for (auto idx = 0; idx < edge_prop_size; idx++) {
          auto edge_property =
              grin_get_edge_property_from_id(fragment, edge_type, idx);
          auto edge_data_type =
              grin_get_edge_property_data_type(fragment, edge_property);
          auto edge_data = grin_get_value_from_row(fragment, edge_prop_row,
                                                   edge_data_type, idx);
          if (edge_data_type == GRIN_DATATYPE::Int32) {
            std::cout << " edata = "
                      << *(static_cast<int32_t*>(const_cast<void*>(edge_data)))
                      << std::endl;
          } else if (edge_data_type == GRIN_DATATYPE::Double) {
            std::cout << " edata = "
                      << *(static_cast<double*>(const_cast<void*>(edge_data)))
                      << std::endl;
          } else {
            std::cout << "unsupported data type ..." << std::endl;
          }
          grin_destroy_edge_property(fragment, edge_property);
          grin_destroy_value(fragment, edge_data_type, edge_data);
        }

        grin_destroy_edge_property_list(fragment, edge_property_list);
        grin_destroy_row(fragment, edge_prop_row);
        grin_destroy_edge_type(fragment, edge_type);
        auto nbr = grin_get_edge_dst(fragment, edge);
        auto _nbr = static_cast<GRIN_VERTEX_T*>(nbr);
        std::cout << "src vertex label = " << _g->vertex_label(*_v)
                  << " offset = " << _g->vertex_offset(*_v)
                  << " dst vertex label = " << _g->vertex_label(*_nbr)
                  << " offset = " << _g->vertex_offset(*_nbr)
                  << " fid = " << _g->fid()
                  << "edge prop size = " << edge_prop_size << std::endl;
        grin_destroy_edge(fragment, edge);
        grin_destroy_vertex(fragment, nbr);
        grin_get_next_adjacent_list_iter(fragment, edge_iter);
      }
      grin_destroy_adjacent_list(fragment, adj_list);
      grin_destroy_adjacent_list_iter(fragment, edge_iter);
      grin_destroy_vertex(fragment, v);
      grin_get_next_vertex_list_iter(fragment, inner_vertex_iter);
    }
    grin_destroy_vertex_list_iter(fragment, inner_vertex_iter);
    grin_destroy_vertex_list(fragment, vertex_list);
    grin_destroy_vertex_list(fragment, inner_vertex_list);
    std::cout << "vertex num = " << grin_get_vertex_num(fragment)
              << " fid = " << _g->fid() << std::endl;
    std::cout << "edge num = " << grin_get_edge_num(fragment)
              << " fid = " << _g->fid() << std::endl;

    MPI_Barrier(comm_spec.comm());
  }
  grape::FinalizeMPIComm();
  return 0;
}
