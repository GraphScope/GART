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

#include <filesystem>
#include <fstream>

#include "etcd/Client.hpp"

#include "apps/gart/property_pagerank.h"
#include "apps/gart/property_sssp.h"
#include "apps/gart/property_wcc.h"
#include "flags.h"  // NOLINT(build/include_subdir)

namespace fs = std::filesystem;

using GraphType = gart::GartFragment<uint64_t, uint64_t>;
using json = vineyard::json;

namespace {
inline void output_file_stream(std::ofstream& os, const std::string& dir,
                               int fid) {
  try {
    if (!fs::exists(dir)) {
      std::cout << "create directory: " << dir << std::endl;
      fs::create_directory(dir);
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what() << std::endl;
    exit(-1);
  }
  std::string output_path = grape::GetResultFilename(dir, fid);
  std::cout << "output_path: " << output_path << std::endl;

  os.open(output_path);
  if (os.fail()) {
    LOG(ERROR) << "Failed to open file " << output_path;
  }
}
}  // namespace

void RunPropertySSSP(std::shared_ptr<GraphType> fragment,
                     const grape::CommSpec& comm_spec,
                     const std::string& out_prefix) {
  using AppType = gs::PropertySSSP<GraphType>;
  auto app = std::make_shared<AppType>();

  auto worker = AppType::CreateWorker(app, fragment);

  auto spec = grape::DefaultParallelEngineSpec();

  worker->Init(comm_spec, spec);
  MPI_Barrier(comm_spec.comm());
  worker->Query(5);

  std::ofstream ostream;
  output_file_stream(ostream, out_prefix, fragment->fid());
  worker->Output(ostream);
  ostream.close();

  worker->Finalize();
}

void RunPropertyWCC(std::shared_ptr<GraphType> fragment,
                    const grape::CommSpec& comm_spec,
                    const std::string& out_prefix) {
  using AppType = gs::PropertyWCC<GraphType>;
  auto app = std::make_shared<AppType>();

  auto worker = AppType::CreateWorker(app, fragment);

  auto spec = grape::DefaultParallelEngineSpec();

  worker->Init(comm_spec, spec);
  MPI_Barrier(comm_spec.comm());
  worker->Query();

  std::ofstream ostream;
  output_file_stream(ostream, out_prefix, fragment->fid());
  worker->Output(ostream);
  ostream.close();

  worker->Finalize();
}

void RunPropertyPageRank(std::shared_ptr<GraphType> fragment,
                         const grape::CommSpec& comm_spec,
                         const std::string& out_prefix) {
  using AppType = gs::PropertyPageRank<GraphType>;
  auto app = std::make_shared<AppType>();

  auto worker = AppType::CreateWorker(app, fragment);

  auto spec = grape::DefaultParallelEngineSpec();

  worker->Init(comm_spec, spec);
  MPI_Barrier(comm_spec.comm());
  worker->Query(0.85, 5);

  std::ofstream ostream;
  output_file_stream(ostream, out_prefix, fragment->fid());
  worker->Output(ostream);
  ostream.close();

  worker->Finalize();
}

uint64_t get_latest_epoch(const grape::CommSpec& comm_spec,
                          std::shared_ptr<etcd::Client> etcd_client) {
  uint64_t write_epoch = std::numeric_limits<uint64_t>::max();

  if (comm_spec.fid() == 0) {
    for (uint idx = 0; idx < comm_spec.fnum(); idx++) {
      std::string latest_epoch_str =
          FLAGS_meta_prefix + "gart_latest_epoch_p" + std::to_string(idx);
      etcd::Response response = etcd_client->get(latest_epoch_str).get();
      assert(response.is_ok());
      uint64_t latest_epoch = std::stoull(response.value().as_string());
      if (latest_epoch < write_epoch) {
        write_epoch = latest_epoch;
      }
    }
    for (uint idx = 1; idx < comm_spec.fnum(); idx++) {
      MPI_Send(&write_epoch, 1, MPI_UNSIGNED_LONG, idx, 0, MPI_COMM_WORLD);
    }
  } else {
    MPI_Recv(&write_epoch, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }
  std::cout << "read_epoch " << write_epoch << " fid " << comm_spec.fid()
            << std::endl;

  MPI_Barrier(comm_spec.comm());
  return write_epoch;
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  grape::InitMPIComm();
  {
    grape::CommSpec comm_spec;
    comm_spec.Init(MPI_COMM_WORLD);

    std::shared_ptr<GraphType> fragment(
        new gart::GartFragment<uint64_t, uint64_t>());
    grape::gflags::ParseCommandLineFlags(&argc, &argv, true);
    std::shared_ptr<etcd::Client> etcd_client =
        std::make_shared<etcd::Client>(FLAGS_etcd_endpoint);
    std::string schema_key =
        FLAGS_meta_prefix + "gart_schema_p" + std::to_string(comm_spec.fid());
    etcd::Response response = etcd_client->get(schema_key).get();
    assert(response.is_ok());
    std::string edge_config_str = response.value().as_string();
    uint64_t write_epoch = get_latest_epoch(comm_spec, etcd_client);
    schema_key = FLAGS_meta_prefix + "gart_blob_m" + std::to_string(0) + "_p" +
                 std::to_string(comm_spec.fid()) + "_e" +
                 std::to_string(write_epoch);
    response = etcd_client->get(schema_key).get();
    assert(response.is_ok());
    std::string config_str = response.value().as_string();
    json config = json::parse(config_str);
    json edge_config = json::parse(edge_config_str);

    fragment->Init(config, edge_config);

    MPI_Barrier(comm_spec.comm());

    std::cout << "start to run sssp " << std::endl;

    RunPropertySSSP(fragment, comm_spec, "./output_property_sssp");

    MPI_Barrier(comm_spec.comm());

    std::cout << "start to run wcc " << std::endl;

    RunPropertyWCC(fragment, comm_spec, "./output_property_wcc");

    MPI_Barrier(comm_spec.comm());

    std::cout << "start to run pagereank " << std::endl;

    RunPropertyPageRank(fragment, comm_spec, "./output_property_pr");

    MPI_Barrier(comm_spec.comm());
  }
  grape::FinalizeMPIComm();

  return 0;
}
