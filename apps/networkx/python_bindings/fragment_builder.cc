// impl.cc
#include "fragment_builder.h"

FragmentBuilder::FragmentBuilder(std::string etcd_endpoint,
                                 std::string meta_prefix, int read_epoch) {
  etcd_endpoint_ = etcd_endpoint;
  meta_prefix_ = meta_prefix;
  read_epoch_ = read_epoch;
  etcd_client_ = std::make_shared<etcd::Client>(etcd_endpoint_);
  std::string schema_key = meta_prefix + "gart_schema_p0";
  etcd::Response response = etcd_client_->get(schema_key).get();
  if (!response.is_ok()) {
    std::cerr << "Failed to get graph schema from etcd by the key: "
              << schema_key << std::endl;
    exit(-1);
  }
  std::string graph_schema_str = response.value().as_string();
  if (graph_schema_str.empty()) {
    std::cerr << "Empty graph schema" << std::endl;
    exit(-1);
  }
  try {
    graph_schema_ = vineyard::json::parse(graph_schema_str);
  } catch (std::exception& e) {
    std::cerr << "Failed to parse graph schema: " << e.what() << std::endl;
    exit(-1);
  }

  schema_key = meta_prefix_ + "gart_blob_m" + std::to_string(0) + "_p0" + "_e" +
               std::to_string(read_epoch);
  response = etcd_client_->get(schema_key).get();
  assert(response.is_ok());
  std::string blob_schema_str = response.value().as_string();
  vineyard::json blob_schema = vineyard::json::parse(blob_schema_str);
  fragment_ = std::make_shared<GraphType>();
  fragment_->Init(blob_schema, graph_schema_);
}

FragmentBuilder::~FragmentBuilder() {}

std::shared_ptr<GraphType> FragmentBuilder::get_graph() const {
  return fragment_;
}
