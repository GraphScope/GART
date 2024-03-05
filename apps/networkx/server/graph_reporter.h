/** Copyright 2020-2023 Alibaba Group Holding Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef APPS_NETWORKX_SERVER_GRAPH_REPORTER_H_
#define APPS_NETWORKX_SERVER_GRAPH_REPORTER_H_

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include "etcd/Client.hpp"
#include "grape/serialization/in_archive.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"

#include "interfaces/fragment/types.h"
#include "server/utils/dynamic.h"
#include "server/utils/msgpack_utils.h"
#include "server/utils/property_converter.h"

#include "types.grpc.pb.h"  // NOLINT(build/include_subdir)
#include "types.pb.h"       // NOLINT(build/include_subdir)

namespace gart {
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using gart::rpc::QueryGraphService;
using gart::rpc::ReportType;
using gart::rpc::Request;
using gart::rpc::Response;

using GraphType = gart::GartFragment<uint64_t, uint64_t>;
using json = vineyard::json;
using oid_t = GraphType::oid_t;
using vid_t = GraphType::vid_t;
using vertex_t = GraphType::vertex_t;

#define MESSAGE_CHUNK_SIZE static_cast<size_t>(1024ll * 1024 * 100)  // 100MB

class QueryGraphServiceImpl final : public QueryGraphService::Service {
 public:
  QueryGraphServiceImpl(std::string etcd_endpoint, std::string meta_prefix) {
    fragment_ = std::make_shared<GraphType>();
    std::shared_ptr<etcd::Client> etcd_client =
        std::make_shared<etcd::Client>(etcd_endpoint);
    std::string schema_key = meta_prefix + "gart_schema_p0";
    etcd::Response response = etcd_client->get(schema_key).get();
    assert(response.is_ok());
    std::string graph_schema_str = response.value().as_string();

    std::string latest_epoch_str = meta_prefix + "gart_latest_epoch_p0";
    response = etcd_client->get(latest_epoch_str).get();
    assert(response.is_ok());
    read_epoch_ = std::stoull(response.value().as_string());

    schema_key = meta_prefix + "gart_blob_m" + std::to_string(0) + "_p0" +
                 "_e" + std::to_string(read_epoch_);
    response = etcd_client->get(schema_key).get();
    assert(response.is_ok());
    std::string blob_schema_str = response.value().as_string();
    json graph_schema = json::parse(graph_schema_str);
    json blob_schema = json::parse(blob_schema_str);
    fragment_->Init(blob_schema, graph_schema);
  }

  Status getData(grpc::ServerContext* context,
                 const gart::rpc::Request* request,
                 grpc::ServerWriter<gart::rpc::Response>* writer) override {
    auto op = request->op();
    std::string args = request->args();
    auto in_archive = std::make_unique<grape::InArchive>();
    switch (op) {
    case gart::rpc::NODE_NUM: {
      size_t node_num = fragment_->GetVerticesNum();
      *in_archive << node_num;
      break;
    }
    case gart::rpc::EDGE_NUM: {
      size_t edge_num = fragment_->GetEdgeNum();
      *in_archive << edge_num;
      break;
    }
    case gart::rpc::HAS_NODE: {
      gart::dynamic::Value node;
      gart::dynamic::Parse(args, node);
      bool is_exist = false;
      if (node.Size() != 2 || !node[0].IsNumber() || !node[1].IsNumber()) {
        std::cout << "Invalid node format: " << args << std::endl;
      } else {
        label_id_t label_id = node[0].GetInt();
        oid_t oid = node[1].GetInt64();
        vid_t gid;
        is_exist = fragment_->Oid2Gid(label_id, oid, gid);
      }
      *in_archive << is_exist;
      break;
    }
    case gart::rpc::HAS_EDGE: {
      // the input edge format: ((src_label_id, src_oid), (dst_label_id,
      // dst_oid))
      gart::dynamic::Value edge;
      dynamic::Parse(args, edge);
      bool result = false;
      if (edge.Size() != 2 || !edge[0].IsArray() || !edge[1].IsArray() ||
          edge[0].Size() != 2 || edge[1].Size() != 2 || !edge[0][0].IsNumber() || 
          !edge[0][1].IsNumber() || !edge[1][0].IsNumber() || !edge[1][1].IsNumber()) {
        std::cout << "Invalid edge format: " << args << std::endl;
      } else {
        label_id_t src_label_id = edge[0][0].GetInt();
        label_id_t dst_label_id = edge[1][0].GetInt();
        oid_t src_oid = edge[0][1].GetInt64();
        oid_t dst_oid = edge[1][1].GetInt64();
        result = hasEdge(src_label_id, src_oid, dst_label_id, dst_oid);
      }
      *in_archive << result;
      break;
    }
    case gart::rpc::NODE_DATA: {
      // the input node format: (label_id, oid)
      gart::dynamic::Value node;
      gart::dynamic::Parse(args, node);
      if (node.Size() != 2 || !node[0].IsNumber() || !node[1].IsNumber()) {
        std::cout << "Invalid node format: " << args << std::endl;
        break;
      }
      label_id_t label_id = node[0].GetInt();
      // in gart, oid is a uint64_t
      oid_t oid = node[1].GetInt64();
      getNodeData(label_id, oid, *in_archive);
      break;
    }
    case gart::rpc::EDGE_DATA: {
      // the input edge format: ((src_label_id, src_oid), (dst_label_id,
      // dst_oid))
      gart::dynamic::Value edge;
      dynamic::Parse(args, edge);
      if (edge.Size() != 2 || !edge[0].IsArray() || !edge[1].IsArray() ||
          edge[0].Size() != 2 || edge[1].Size() != 2 || !edge[0][0].IsNumber() || 
          !edge[0][1].IsNumber() || !edge[1][0].IsNumber() || !edge[1][1].IsNumber()) {
        std::cout << "Invalid edge format: " << args << std::endl;
        break;
      }
      label_id_t src_label_id = edge[0][0].GetInt();
      label_id_t dst_label_id = edge[1][0].GetInt();
      oid_t src_oid = edge[0][1].GetInt64();
      oid_t dst_oid = edge[1][1].GetInt64();
      getEdgeData(src_label_id, src_oid, dst_label_id, dst_oid, *in_archive);
      break;
    }
    case gart::rpc::PREDS_BY_NODE:
    case gart::rpc::SUCCS_BY_NODE: {
      // the input node format: (label_id, oid)
      gart::dynamic::Value node;
      gart::dynamic::Parse(args, node);
      if (node.Size() != 2 || !node[0].IsNumber() || !node[1].IsNumber()) {
        std::cout << "Invalid node format: " << args << std::endl;
        break;
      }
      label_id_t label_id = node[0].GetInt();
      // in gart, oid is a uint64_t
      oid_t oid = node[1].GetInt64();
      getNeighborsList(label_id, oid, op, *in_archive);
      break;
    }
    case gart::rpc::SUCC_ATTR_BY_NODE:
    case gart::rpc::PRED_ATTR_BY_NODE: {
      // the input node format: (label_id, oid)
      gart::dynamic::Value node;
      gart::dynamic::Parse(args, node);
      if (node.Size() != 2 || !node[0].IsNumber() || !node[1].IsNumber()) {
        std::cout << "Invalid node format: " << args << std::endl;
        break;
      }
      label_id_t label_id = node[0].GetInt();
      // in gart, oid is a uint64_t
      oid_t oid = node[1].GetInt64();
      getNeighborsAttrList(label_id, oid, op, *in_archive);
      break;
    }
    case gart::rpc::NODES: {
      getNodeList(*in_archive);
      break;
    }
    default: {
      std::cout << "Unknown op: " << op << std::endl;
    }
    }
    auto msg_buffer = in_archive->GetBuffer();
    for (size_t idx = 0; idx < in_archive->GetSize();
         idx += MESSAGE_CHUNK_SIZE) {
      gart::rpc::Response response;
      response.mutable_result()->assign(
          msg_buffer + idx,
          msg_buffer + idx +
              std::min(MESSAGE_CHUNK_SIZE, in_archive->GetSize() - idx));
      writer->Write(response);
    }
    return Status::OK;
  }

 private:
  int read_epoch_;
  std::shared_ptr<GraphType> fragment_;

  void getNeighborsList(label_id_t label_id, const oid_t& oid,
                        const gart::rpc::ReportType& report_type,
                        grape::InArchive& arc) {
    vertex_t src;
    bool is_exist = fragment_->GetVertex(label_id, oid, src);
    if (!is_exist) {
      return;
    }
    gart::dynamic::Value id_array(rapidjson::kArrayType);
    for (label_id_t e_label = 0; e_label < fragment_->edge_label_num();
         e_label++) {
      gart::EdgeIterator edge_iter;
      if (report_type == gart::rpc::PREDS_BY_NODE) {
        edge_iter = fragment_->GetIncomingAdjList(src, e_label);
      } else {
        edge_iter = fragment_->GetOutgoingAdjList(src, e_label);
      }
      while (edge_iter.valid()) {
        vertex_t dst = edge_iter.neighbor();
        label_id_t dst_label = fragment_->vertex_label(dst);
        oid_t dst_oid = fragment_->GetId(dst);
        id_array.PushBack(dynamic::Value(rapidjson::kArrayType)
                              .PushBack(dst_label)
                              .PushBack(dst_oid));
        edge_iter.next();
      }
    }
    msgpack::sbuffer sbuf;
    msgpack::pack(&sbuf, id_array);
    arc << sbuf;
  }

  void getNodeData(label_id_t label_id, const oid_t& oid,
                   grape::InArchive& arc) {
    vertex_t src;
    bool is_exist = fragment_->GetVertex(label_id, oid, src);
    if (!is_exist) {
      return;
    }
    gart::dynamic::Value ref_data(rapidjson::kObjectType);
    auto vertex_prop_num = fragment_->vertex_property_num(label_id);
    for (auto prop_id = 0; prop_id < vertex_prop_num; prop_id++) {
      auto dtype = fragment_->GetVertexPropDataType(label_id, prop_id);
      auto prop_name = fragment_->GetVertexPropName(label_id, prop_id);
      PropertyConverter<GraphType>::NodeValue(fragment_, src, dtype, prop_name,
                                              prop_id, ref_data);
    }
    msgpack::sbuffer sbuf;
    msgpack::pack(&sbuf, ref_data);
    arc << sbuf;
  }

  void getNodeList(grape::InArchive& arc) {
    gart::dynamic::Value id_array(rapidjson::kArrayType);
    auto vertex_label_num = fragment_->vertex_label_num();
    for (auto v_label = 0; v_label < vertex_label_num; v_label++) {
      auto vertices_iter = fragment_->InnerVertices(v_label);
      while (vertices_iter.valid()) {
        auto v = vertices_iter.vertex();
        auto v_oid = fragment_->GetId(v);
        id_array.PushBack(dynamic::Value(rapidjson::kArrayType)
                              .PushBack(v_label)
                              .PushBack(v_oid));
        vertices_iter.next();
      }
    }
    msgpack::sbuffer sbuf;
    msgpack::pack(&sbuf, id_array);
    arc << sbuf;
  }

  bool hasEdge(label_id_t src_label_id, const oid_t& src_oid,
               label_id_t dst_label_id, const oid_t& dst_oid) {
    vertex_t src, dst;
    bool src_exist = fragment_->Oid2Gid(src_label_id, src_oid, src);
    bool dst_exist = fragment_->Oid2Gid(dst_label_id, dst_oid, dst);
    if (src_exist && dst_exist) {
      for (auto e_label = 0; e_label < fragment_->edge_label_num(); e_label++) {
        label_id_t src_label_of_elabel =
            fragment_->edge2vertex_map.find(e_label)->second.first;
        label_id_t dst_label_of_elabel =
            fragment_->edge2vertex_map.find(e_label)->second.second;
        if (!(src_label_id == src_label_of_elabel &&
              dst_label_id == dst_label_of_elabel)) {
          continue;
        }
        auto edge_iter = fragment_->GetOutgoingAdjList(src, e_label);
        while (edge_iter.valid()) {
          if (edge_iter.neighbor() == dst) {
            return true;
          }
          edge_iter.next();
        }
      }
    }
    return false;
  }

  void getEdgeData(label_id_t src_label_id, const oid_t& src_oid,
                   label_id_t dst_label_id, const oid_t& dst_oid,
                   grape::InArchive& arc) {
    vertex_t src, dst;
    bool src_exist = fragment_->Oid2Gid(src_label_id, src_oid, src);
    bool dst_exist = fragment_->Oid2Gid(dst_label_id, dst_oid, dst);
    if (src_exist && dst_exist) {
      for (auto e_label = 0; e_label < fragment_->edge_label_num(); e_label++) {
        label_id_t src_label_of_elabel =
            fragment_->edge2vertex_map.find(e_label)->second.first;
        label_id_t dst_label_of_elabel =
            fragment_->edge2vertex_map.find(e_label)->second.second;
        if (!(src_label_id == src_label_of_elabel &&
              dst_label_id == dst_label_of_elabel)) {
          continue;
        }
        auto edge_iter = fragment_->GetOutgoingAdjList(src, e_label);
        while (edge_iter.valid()) {
          if (edge_iter.neighbor() == dst) {
            gart::dynamic::Value ref_data(rapidjson::kObjectType);
            auto edge_prop_num = fragment_->edge_property_num(e_label);
            for (auto prop_id = 0; prop_id < edge_prop_num; prop_id++) {
              std::string dtype =
                  fragment_->GetEdgePropDataType(e_label, prop_id);
              std::string prop_name =
                  fragment_->GetEdgePropName(e_label, prop_id);
              PropertyConverter<GraphType>::EdgeValue(
                  fragment_, edge_iter, dtype, prop_name, prop_id, ref_data);
            }
            msgpack::sbuffer sbuf;
            msgpack::pack(&sbuf, ref_data);
            arc << sbuf;
            return;
          }
          edge_iter.next();
        }
      }
    }
  }

  void getNeighborsAttrList(label_id_t label_id, const oid_t& oid,
                            const rpc::ReportType& report_type,
                            grape::InArchive& arc) {
    vertex_t src;
    bool is_exist = fragment_->GetVertex(label_id, oid, src);
    if (!is_exist) {
      return;
    }

    gart::dynamic::Value data_array(rapidjson::kArrayType);
    for (auto e_label = 0; e_label < fragment_->edge_label_num(); e_label++) {
      auto edge_prop_num = fragment_->edge_property_num(e_label);
      gart::EdgeIterator edge_iter;
      if (report_type == gart::rpc::PRED_ATTR_BY_NODE) {
        edge_iter = fragment_->GetIncomingAdjList(src, e_label);
      } else {
        edge_iter = fragment_->GetOutgoingAdjList(src, e_label);
      }
      while (edge_iter.valid()) {
        gart::dynamic::Value prop_data(rapidjson::kObjectType);
        for (auto prop_id = 0; prop_id < edge_prop_num; prop_id++) {
          std::string dtype = fragment_->GetEdgePropDataType(e_label, prop_id);
          std::string prop_name = fragment_->GetEdgePropName(e_label, prop_id);
          PropertyConverter<GraphType>::EdgeValue(
              fragment_, edge_iter, dtype, prop_name, prop_id, prop_data);
        }
        data_array.PushBack(prop_data);
        edge_iter.next();
      }
    }
    msgpack::sbuffer sbuf;
    msgpack::pack(&sbuf, data_array);
    arc << sbuf;
  }
};
}  // namespace gart

#endif  // APPS_NETWORKX_SERVER_GRAPH_REPORTER_H_
