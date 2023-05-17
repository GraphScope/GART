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

#ifndef RESEARCH_GART_VEGITO_SRC_FRAMEWORK_CONFIG_H_
#define RESEARCH_GART_VEGITO_SRC_FRAMEWORK_CONFIG_H_

#include <string>

namespace gart {
namespace framework {

// config for whole system
class Config {
 public:
  // 1. Environments
  inline const std::string &getExeName() const { return exe_name_; }

  // 2. server config
  inline int getNumServers() const { return num_servers_; }  // #machines
  inline int getServerID() const { return server_id_; }      // machine id

  // 3. Property Store
  inline int getPropertyType() const { return property_type_; }

  inline std::string getIPCScoket() const { return ipc_socket_; }

  void parse_sys_args(int argc, char **argv);
  void printConfig() const;

 private:
  std::string exe_name_;
  int num_servers_ = 1;
  int server_id_ = 0;

  // 0 for kv-store, 1 for row-store, 2 for column-store, 3 for naive col
  // FIXME: fix this hard code according PropertyStoreType
  int property_type_ = 2;

  std::string ipc_socket_;
};  // class Config

extern Config config;

}  // namespace framework
}  // namespace gart

#endif  // RESEARCH_GART_VEGITO_SRC_FRAMEWORK_CONFIG_H_
