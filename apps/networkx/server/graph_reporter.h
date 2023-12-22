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
#ifndef APPS_NETWORKX_SERVER_GRAPH_SERVER_H_
#define APPS_NETWORKX_SERVER_GRAPH_SERVER_H_

#include <cstddef>
#include <string>

#include "etcd/Client.hpp"
#include "grape/serialization/in_archive.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "grpcpp/server_context.h"

#include "fragment/gart_fragment.h"

#include "types.grpc.pb.h"
#include "types.pb.h"

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

class QueryGraphServiceImpl final : public QueryGraphService::Service {
 public:
  QueryGraphServiceImpl(int read_epoch, std::string etcd_endpoint,
                        std::string meta_prefix) {
    read_epoch_ = read_epoch;
    fragment_ = std::make_shared<GraphType>();
    std::shared_ptr<etcd::Client> etcd_client =
        std::make_shared<etcd::Client>(etcd_endpoint);
    std::string schema_key = meta_prefix + "gart_schema_p0";
    etcd::Response response = etcd_client->get(schema_key).get();
    assert(response.is_ok());
    std::string graph_schema_str = response.value().as_string();
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
                 gart::rpc::Response* response) override {
    auto op = request->op();
    auto in_archive = std::make_unique<grape::InArchive>();
    switch (op) {
    case gart::rpc::NODE_NUM: {
      size_t node_num = fragment_->GetVerticesNum();
      *in_archive << node_num;
      response->mutable_result()->assign(
          in_archive->GetBuffer(),
          in_archive->GetBuffer() + in_archive->GetSize());
      break;
    }
    case gart::rpc::EDGE_NUM: {
      break;
    }
    case gart::rpc::HAS_NODE: {
      break;
    }
    case gart::rpc::HAS_EDGE: {
      break;
    }
    case gart::rpc::NODE_DATA: {
      break;
    }
    case gart::rpc::EDGE_DATA: {
      break;
    }
    case gart::rpc::SUCCS_BY_NODE: {
      break;
    }
    case gart::rpc::NODES: {
      break;
    }
    default: {
      std::cout << "Unknown op: " << op << std::endl;
    }
    }
    return Status::OK;
  }

 private:
  int read_epoch_;
  std::shared_ptr<GraphType> fragment_;
};
}  // namespace gart

#endif  // APPS_NETWORKX_SERVER_GRAPH_SERVER_H_