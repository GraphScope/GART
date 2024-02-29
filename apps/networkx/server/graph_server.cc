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
#include <iostream>
#include <memory>
#include <string>

#include "server/graph_reporter.h"
#include "server/graph_server_flags.h"

int main(int argc, char** argv) {
  /* parse arguments for whole system */
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  gart::QueryGraphServiceImpl service(FLAGS_etcd_endpoint, FLAGS_meta_prefix);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_server_addr,
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << FLAGS_server_addr << std::endl;
  server->Wait();

  return 0;
}
