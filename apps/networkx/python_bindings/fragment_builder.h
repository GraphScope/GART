// impl.h
#ifndef IMPL_H
#define IMPL_H
#include <memory>
#include <string>

#include "basic/ds/arrow.h"
#include "etcd/Client.hpp"
#include "vineyard/common/util/json.h"

#include "interfaces/fragment/gart_fragment.h"

using GraphType = gart::GartFragment<uint64_t, uint64_t>;

class FragmentBuilder {
 public:
  FragmentBuilder(std::string etcd_endpoint, std::string meta_prefix,
                  int read_epoch);
  ~FragmentBuilder();

  std::shared_ptr<GraphType> get_graph() const;

 private:
  int value_;
  std::string etcd_endpoint_;
  std::string meta_prefix_;
  int read_epoch_;
  std::shared_ptr<etcd::Client> etcd_client_;
  vineyard::json graph_schema_;
  std::shared_ptr<GraphType> fragment_;
};

#endif  // IMPL_H
