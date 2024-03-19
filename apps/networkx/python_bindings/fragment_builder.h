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

#ifndef APPS_NETWORKX_PYTHON_BINDINGS_FRAGMENT_BUILDER_H_
#define APPS_NETWORKX_PYTHON_BINDINGS_FRAGMENT_BUILDER_H_

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

#endif  // APPS_NETWORKX_PYTHON_BINDINGS_FRAGMENT_BUILDER_H_
