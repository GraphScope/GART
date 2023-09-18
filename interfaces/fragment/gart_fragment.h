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

#ifndef INTERFACES_FRAGMENT_GART_FRAGMENT_H_
#define INTERFACES_FRAGMENT_GART_FRAGMENT_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "grape/fragment/fragment_base.h"
#include "vineyard/basic/ds/hashmap_mvcc.h"

#include "fragment/id_parser.h"
#include "interfaces/fragment/iterator.h"
#include "interfaces/fragment/property_util.h"
#include "types.h"
#include "util/bitset.h"

namespace gart {

template <typename OID_T, typename VID_T>
class GartFragment {
 public:
  using oid_t = OID_T;
  using vid_t = VID_T;
  using fid_t = grape::fid_t;
  using eid_t = vineyard::property_graph_types::EID_TYPE;
  using prop_id_t = vineyard::property_graph_types::PROP_ID_TYPE;
  using json = vineyard::json;
  using label_id_t = vineyard::property_graph_types::LABEL_ID_TYPE;
  using vertex_t = grape::Vertex<vid_t>;
  using EpochBlockHeader = seggraph::EpochBlockHeader;
  using VegitoEdgeBlockHeader = seggraph::VegitoEdgeBlockHeader;
  using VegitoSegmentHeader = seggraph::VegitoSegmentHeader;
  using EdgeLabelBlockHeader = seggraph::EdgeLabelBlockHeader;
  using dir_t = seggraph::dir_t;
  using hashmap_t = vineyard::HashmapMVCC<int64_t, int64_t>;

  static constexpr grape::LoadStrategy load_strategy =
      grape::LoadStrategy::kBothOutIn;

 public:
  GartFragment() {}

  ~GartFragment() = default;

 public:
  void Init(json& config, json& edge_config) {
    ipc_socket_ = config["ipc_socket"].get<std::string>();
    VINEYARD_CHECK_OK(client_.Connect(ipc_socket_));
    LOG(INFO) << "Connected to IPCServer: " << ipc_socket_;

    fnum_ = config["fnum"].get<fid_t>();
    fid_ = config["fid"].get<fid_t>();
    vertex_label_num_ = config["vertex_label_num"].get<int>();
    read_epoch_number_ = config["epoch"].get<size_t>();
    auto string_buffer_object_id =
        config["string_buffer_object_id"].get<uint64_t>();
    std::shared_ptr<vineyard::Blob> string_buffer_blob;
    VINEYARD_CHECK_OK(
        client_.GetBlob(string_buffer_object_id, true, string_buffer_blob));
    string_buffer_ = (char*) string_buffer_blob->data();

    vertex_tables_.resize(vertex_label_num_, nullptr);
    ovl2g_.resize(vertex_label_num_);
    valid_ovl2g_element_.resize(vertex_label_num_);
    ovg2l_maps_.resize(vertex_label_num_);
    vertex_maps_.resize(vertex_label_num_);
    inner_offsets_.resize(vertex_label_num_, -1);
    outer_offsets_.resize(vertex_label_num_, -1);
    max_inner_offsets_.resize(vertex_label_num_, -1);
    vertex_table_lens_.resize(vertex_label_num_);
    ivnums_.resize(vertex_label_num_);
    ovnums_.resize(vertex_label_num_);
    tvnums_.resize(vertex_label_num_);

    inner_edge_blob_ptrs_.resize(vertex_label_num_, nullptr);
    outer_edge_blob_ptrs_.resize(vertex_label_num_, nullptr);
    inner_edge_label_ptrs_.resize(vertex_label_num_, nullptr);
    outer_edge_label_ptrs_.resize(vertex_label_num_, nullptr);

    idst_.resize(vertex_label_num_);
    odst_.resize(vertex_label_num_);
    iodst_.resize(vertex_label_num_);

    idoffset_.resize(vertex_label_num_);
    odoffset_.resize(vertex_label_num_);
    iodoffset_.resize(vertex_label_num_);

    prop_cols_meta.resize(vertex_label_num_);
    vertex_prop_blob_ptrs_.resize(vertex_label_num_);
    vertex_prop_column_family_id_.resize(vertex_label_num_);
    vertex_prop_column_family_offset_.resize(vertex_label_num_);
    vertex_prop_num_per_column_family_.resize(vertex_label_num_);
    column_family_data_length_.resize(vertex_label_num_);
    vertex_prop_id_in_column_family_.resize(vertex_label_num_);

    vertex_prop_nums_.resize(vertex_label_num_);

    vertex_ext_id_ptrs_.resize(vertex_label_num_, nullptr);
    outer_vertex_ext_id_ptrs_.resize(vertex_label_num_, nullptr);
    vertex_ext_id_dtypes_.resize(vertex_label_num_);

    vid_parser.Init(fnum_, vertex_label_num_);
    max_outer_id_offset_ =
        (((vid_t) 1) << vid_parser.GetOffsetWidth()) - (vid_t) 1;
    min_outer_offsets_.resize(vertex_label_num_, max_outer_id_offset_ + 1);

    edge_label_num_ = 0;
    std::map<std::string, int> vertex_name_id_map;
    auto edge_info = edge_config["types"];
    for (uint64_t idx = 0; idx < edge_info.size(); idx++) {
      auto type = edge_info[idx]["type"].get<std::string>();
      if (type == "VERTEX") {
        std::string name = edge_info[idx]["label"].get<std::string>();
        int v_label_id = edge_info[idx]["id"].get<int>();
        vertex_name_id_map.emplace(name, v_label_id);
        vertex_name2label_.emplace(name, v_label_id);
        vertex_label2name_.emplace(v_label_id, name);
        auto vertex_prop_info = edge_info[idx]["propertyDefList"];
        vertex_prop_nums_[v_label_id] = vertex_prop_info.size();
        auto vertex_prop_num = vertex_prop_info.size();
        vertex_prop_column_family_id_[v_label_id].resize(vertex_prop_num);
        vertex_prop_column_family_offset_[v_label_id].resize(vertex_prop_num);
        vertex_prop_num_per_column_family_[v_label_id].resize(vertex_prop_num,
                                                              0);
        column_family_data_length_[v_label_id].resize(vertex_prop_num, 0);
        vertex_prop_id_in_column_family_[v_label_id].resize(vertex_prop_num, 0);
        for (size_t prop_id = 0; prop_id < vertex_prop_num; prop_id++) {
          auto prop_name = vertex_prop_info[prop_id]["name"].get<std::string>();
          vertex_prop2name_.emplace(std::make_pair(v_label_id, prop_id),
                                    prop_name);
          vertex_name2prop_.emplace(std::make_pair(v_label_id, prop_name),
                                    prop_id);
          int column_family_id =
              vertex_prop_info[prop_id]["column_family_id"].get<int>();
          vertex_prop_column_family_id_[v_label_id][prop_id] = column_family_id;
          int column_family_offset =
              vertex_prop_info[prop_id]["column_family_offset"].get<int>();
          vertex_prop_column_family_offset_[v_label_id][prop_id] =
              column_family_offset;
          vertex_prop_id_in_column_family_[v_label_id][prop_id] =
              vertex_prop_num_per_column_family_[v_label_id][column_family_id];
          vertex_prop_num_per_column_family_[v_label_id][column_family_id]++;
          auto dtype =
              vertex_prop_info[prop_id]["data_type"].get<std::string>();
          vertex_prop2dtype_.emplace(std::make_pair(v_label_id, prop_id),
                                     dtype);
          if (dtype == "INT") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(int);
          } else if (dtype == "FLOAT") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(float);
          } else if (dtype == "DOUBLE") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(double);
          } else if (dtype == "LONG") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(int64_t);
          } else if (dtype == "CHAR") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(char);
          } else if (dtype == "STRING") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(uint64_t);
          } else if (dtype == "DATE") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(gart::Date);
          } else if (dtype == "DATETIME") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(gart::DateTime);
          } else if (dtype == "TIME") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(gart::Time);
          } else if (dtype == "TIMESTAMP") {
            column_family_data_length_[v_label_id][column_family_id] +=
                sizeof(gart::TimeStamp);
          } else {
            LOG(FATAL) << "Unsupported data type: " << dtype;
            assert(false);
          }
        }
        continue;
      }
      auto e_label_id = edge_info[idx]["id"].get<int>();
      auto e_name = edge_info[idx]["label"].get<std::string>();
      edge_label2name_.emplace(e_label_id - vertex_label_num_, e_name);
      edge_name2label_.emplace(e_name, e_label_id - vertex_label_num_);
      auto edge_prop_info = edge_info[idx]["propertyDefList"];
      edge_prop_nums_.push_back(edge_prop_info.size());
      edge_bitmap_size_.push_back(BYTE_SIZE(edge_prop_info.size()));
      std::vector<int> edge_prop_offset;
      std::vector<int> edge_prop_dtype;
      for (uint64_t prop_idx = 0; prop_idx < edge_prop_info.size();
           prop_idx++) {
        auto prop_name = edge_prop_info[prop_idx]["name"].get<std::string>();
        edge_prop2name_.emplace(
            std::make_pair(e_label_id - vertex_label_num_, prop_idx),
            prop_name);
        edge_name2prop_.emplace(
            std::make_pair(e_label_id - vertex_label_num_, prop_name),
            prop_idx);
        auto dtype = edge_prop_info[prop_idx]["data_type"].get<std::string>();
        edge_prop2dtype_.emplace(
            std::make_pair(e_label_id - vertex_label_num_, prop_idx), dtype);
        int accum_offset = 0;
        if (edge_prop_offset.size() != 0) {
          accum_offset = edge_prop_offset[edge_prop_offset.size() - 1];
        }
        if (dtype == "INT") {
          edge_prop_offset.push_back(accum_offset + sizeof(int));
          edge_prop_dtype.push_back(INT);
        } else if (dtype == "FLOAT") {
          edge_prop_offset.push_back(accum_offset + sizeof(float));
          edge_prop_dtype.push_back(FLOAT);
        } else if (dtype == "DOUBLE") {
          edge_prop_offset.push_back(accum_offset + sizeof(double));
          edge_prop_dtype.push_back(DOUBLE);
        } else if (dtype == "LONG") {
          edge_prop_offset.push_back(accum_offset + sizeof(uint64_t));
          edge_prop_dtype.push_back(LONG);
        } else if (dtype == "CHAR") {
          edge_prop_offset.push_back(accum_offset + sizeof(char));
          edge_prop_dtype.push_back(CHAR);
        } else if (dtype == "STRING") {
          edge_prop_offset.push_back(accum_offset + sizeof(uint64_t));
          edge_prop_dtype.push_back(STRING);
        } else if (dtype == "DATE") {
          edge_prop_offset.push_back(accum_offset + sizeof(gart::Date));
          edge_prop_dtype.push_back(DATE);
        } else if (dtype == "DATETIME") {
          edge_prop_offset.push_back(accum_offset + sizeof(gart::DateTime));
          edge_prop_dtype.push_back(DATETIME);
        } else if (dtype == "TIME") {
          edge_prop_offset.push_back(accum_offset + sizeof(gart::Time));
          edge_prop_dtype.push_back(TIME);
        } else if (dtype == "TIMESTAMP") {
          edge_prop_offset.push_back(accum_offset + sizeof(gart::TimeStamp));
          edge_prop_dtype.push_back(TIMESTAMP);
        } else {
          LOG(FATAL) << "Unsupported data type: " << dtype;
          assert(false);
        }
      }
      edge_prop_offsets.push_back(edge_prop_offset);
      edge_prop_dtypes.push_back(edge_prop_dtype);

      auto edge_src_dst_info = edge_info[idx]["rawRelationShips"].at(0);

      auto src_name = edge_src_dst_info["srcVertexLabel"].get<std::string>();
      auto dst_name = edge_src_dst_info["dstVertexLabel"].get<std::string>();

      auto src_id = vertex_name_id_map.find(src_name)->second;
      auto dst_id = vertex_name_id_map.find(dst_name)->second;

      vertex2edge_map.emplace(std::make_pair(src_id, dst_id), e_label_id);
      edge2vertex_map.emplace(e_label_id, std::make_pair(src_id, dst_id));

      edge_label_num_++;
    }

    auto blob_info = config["blob"];
    for (size_t i = 0; i < blob_info.size(); i++) {
      int vlabel = blob_info[i]["vlabel"].get<int>();
      // init vertex table
      uint64_t vertex_table_obj_id =
          blob_info[i]["vertex_table"]["object_id"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> vertex_table_blob;
      VINEYARD_CHECK_OK(
          client_.GetBlob(vertex_table_obj_id, true, vertex_table_blob));
      vertex_tables_[vlabel] = (vid_t*) vertex_table_blob->data();

      inner_offsets_[vlabel] =
          blob_info[i]["vertex_table"]["max_inner_location"].get<int64_t>() - 1;
      outer_offsets_[vlabel] =
          blob_info[i]["vertex_table"]["min_outer_location"].get<int64_t>();
      vertex_table_lens_[vlabel] =
          vertex_table_blob->allocated_size() / sizeof(vid_t);

      auto ovg2l_blob_id = blob_info[i]["ovg2l_blob"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> ovg2l_blob;
      VINEYARD_CHECK_OK(client_.GetBlob(ovg2l_blob_id, true, ovg2l_blob));
      std::shared_ptr<const hashmap_t> hmapview;
      VINEYARD_CHECK_OK(hashmap_t::View(client_, ovg2l_blob, hmapview));
      ovg2l_maps_[vlabel] = hmapview;

      auto vertex_map_blob_id = blob_info[i]["vertex_map_oid"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> vertex_map_blob;
      VINEYARD_CHECK_OK(
          client_.GetBlob(vertex_map_blob_id, true, vertex_map_blob));
      std::shared_ptr<const hashmap_t> vertex_map_hmapview;
      VINEYARD_CHECK_OK(
          hashmap_t::View(client_, vertex_map_blob, vertex_map_hmapview));
      vertex_maps_[vlabel] = vertex_map_hmapview;

      for (int64_t j = inner_offsets_[vlabel]; j >= 0; j--) {
        vid_t v = vertex_tables_[vlabel][j];
        auto delete_flag = v >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 0) {
          max_inner_offsets_[vlabel] = vid_parser.GetOffset(v);
          break;
        }
      }

      for (int64_t j = outer_offsets_[vlabel]; j < vertex_table_lens_[vlabel];
           j++) {
        vid_t v = vertex_tables_[vlabel][j];
        auto delete_flag = v >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 0) {
          min_outer_offsets_[vlabel] = vid_parser.GetOffset(v);
          break;
        }
      }

      // init ovl2g
      uint64_t ovl2g_obj_id =
          blob_info[i]["ovl2g"]["object_id"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> ovl2g_blob;
      VINEYARD_CHECK_OK(client_.GetBlob(ovl2g_obj_id, true, ovl2g_blob));
      ovl2g_[vlabel] = (vid_t*) ovl2g_blob->data();

      valid_ovl2g_element_[vlabel] =
          blob_info[i]["ovl2g"]["len_ele"].get<uint64_t>();

      // init edge blobs
      uint64_t inner_edge_label_obj_id =
          blob_info[i]["elabel2seg"]["object_id"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> inner_edge_label_blob;
      VINEYARD_CHECK_OK(client_.GetBlob(inner_edge_label_obj_id, true,
                                        inner_edge_label_blob));
      inner_edge_label_ptrs_[vlabel] =
          (uint64_t*) inner_edge_label_blob->data();

      uint64_t outer_edge_label_obj_id =
          blob_info[i]["ov_elabel2seg"]["object_id"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> outer_edge_label_blob;
      VINEYARD_CHECK_OK(client_.GetBlob(outer_edge_label_obj_id, true,
                                        outer_edge_label_blob));
      outer_edge_label_ptrs_[vlabel] =
          (uint64_t*) outer_edge_label_blob->data();

      uint64_t inner_edge_blob_obj_id =
          blob_info[i]["block_oid"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> inner_edge_blob;
      VINEYARD_CHECK_OK(
          client_.GetBlob(inner_edge_blob_obj_id, true, inner_edge_blob));
      inner_edge_blob_ptrs_[vlabel] = (char*) inner_edge_blob->data();

      uint64_t outer_edge_blob_obj_id =
          blob_info[i]["ov_block_oid"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> outer_edge_blob;
      VINEYARD_CHECK_OK(
          client_.GetBlob(outer_edge_blob_obj_id, true, outer_edge_blob));
      outer_edge_blob_ptrs_[vlabel] = (char*) outer_edge_blob->data();

      // init I(O)EDst
      idst_[vlabel].resize(edge_label_num_);
      odst_[vlabel].resize(edge_label_num_);
      iodst_[vlabel].resize(edge_label_num_);

      idoffset_[vlabel].resize(edge_label_num_);
      odoffset_[vlabel].resize(edge_label_num_);
      iodoffset_[vlabel].resize(edge_label_num_);

      // init vertex property
      int vertex_prop_column_family_num = blob_info[i]["num_vprops"].get<int>();
      auto vertex_prop_config = blob_info[i]["vprops"];
      prop_cols_meta[vlabel].resize(vertex_prop_column_family_num);
      vertex_prop_blob_ptrs_[vlabel].resize(vertex_prop_column_family_num);

      uint64_t vertex_external_id_oid =
          blob_info[i]["external_id_oid"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> vertex_external_id_blob;
      VINEYARD_CHECK_OK(client_.GetBlob(vertex_external_id_oid, true,
                                        vertex_external_id_blob));
      vertex_ext_id_ptrs_[vlabel] = (int64_t*) vertex_external_id_blob->data();

      uint64_t outer_vertex_external_id_oid =
          blob_info[i]["outer_external_id_oid"].get<uint64_t>();
      std::shared_ptr<vineyard::Blob> outer_vertex_external_id_blob;
      VINEYARD_CHECK_OK(client_.GetBlob(outer_vertex_external_id_oid, true,
                                        outer_vertex_external_id_blob));
      outer_vertex_ext_id_ptrs_[vlabel] =
          (int64_t*) outer_vertex_external_id_blob->data();

      vertex_ext_id_dtypes_[vlabel] =
          blob_info[i]["external_id_dtype"].get<std::string>();

      for (uint64_t idx = 0; idx < vertex_prop_config.size(); idx++) {
        auto prop_id = vertex_prop_config[idx]["prop_id"].get<int>();
        auto v_prop_obj_id =
            vertex_prop_config[idx]["object_id"].get<uint64_t>();
        std::shared_ptr<vineyard::Blob> vertex_prop_blob;
        VINEYARD_CHECK_OK(
            client_.GetBlob(v_prop_obj_id, true, vertex_prop_blob));
        vertex_prop_blob_ptrs_[vlabel][prop_id] =
            (char*) vertex_prop_blob->data();
        VertexPropMeta prop_meta;
        prop_meta.prop_id = prop_id;
        prop_meta.updatable = vertex_prop_config[idx]["updatable"].get<bool>();
        prop_meta.header = vertex_prop_config[idx]["header"].get<uint64_t>();
        prop_meta.object_id = v_prop_obj_id;
        prop_meta.dtype = vertex_prop_config[idx]["type"].get<int>();
        prop_cols_meta[vlabel][prop_id] = prop_meta;
      }
    }
  }

  bool directed() const { return directed_; }

  const std::string vid_typename() const { return vid_type; }

  const std::string oid_typename() const { return oid_type; }

  fid_t fid() const { return fid_; }

  fid_t fnum() const { return fnum_; }

  label_id_t vertex_label(const vertex_t& v) const {
    return vid_parser.GetLabelId(v.GetValue());
  }

  int64_t vertex_offset(const vertex_t& v) const {
    return vid_parser.GetOffset(v.GetValue());
  }

  label_id_t vertex_label_num() const { return vertex_label_num_; }

  label_id_t edge_label_num() const { return edge_label_num_; }

  prop_id_t vertex_property_num(label_id_t label) const {
    return vertex_prop_nums_[label];
  }

  prop_id_t edge_property_num(label_id_t label) const {
    return edge_prop_nums_[label];
  }

  gart::VertexIterator Vertices(label_id_t label_id) const {
    vid_t* table_addr = vertex_tables_[label_id];
    int64_t inner_offset = inner_offsets_[label_id];
    int64_t outer_offset = outer_offsets_[label_id];
    vid_t* inner_begin_addr = nullptr;
    vid_t* inner_end_addr = nullptr;
    if (table_addr != nullptr) {
      inner_begin_addr = table_addr + inner_offset;
      inner_end_addr = table_addr - 1;
    }
    vid_t* outer_begin_addr = nullptr;
    vid_t* outer_end_addr = nullptr;
    if (table_addr != nullptr) {
      outer_begin_addr = table_addr + outer_offset;
      outer_end_addr = table_addr + vertex_table_lens_[label_id];
    }

    std::pair<vid_t*, vid_t*> inner_addr =
        std::make_pair(inner_begin_addr, inner_end_addr);
    std::pair<vid_t*, vid_t*> outer_addr =
        std::make_pair(outer_begin_addr, outer_end_addr);
    std::vector<std::pair<vid_t*, vid_t*>> addr_vec;
    addr_vec.push_back(inner_addr);
    addr_vec.push_back(outer_addr);
    std::vector<bool> high_to_low_vec;
    high_to_low_vec.push_back(true);
    high_to_low_vec.push_back(false);

    return gart::VertexIterator(addr_vec, high_to_low_vec, table_addr,
                                label_id);
  }

  gart::VertexIterator InnerVertices(label_id_t label_id) const {
    vid_t* table_addr = vertex_tables_[label_id];
    int64_t inner_offset = inner_offsets_[label_id];
    vid_t* begin_addr = nullptr;
    vid_t* end_addr = nullptr;
    if (table_addr != nullptr) {
      begin_addr = table_addr + inner_offset;
      end_addr = table_addr - 1;
    }
    std::pair<vid_t*, vid_t*> addr = std::make_pair(begin_addr, end_addr);
    std::vector<std::pair<vid_t*, vid_t*>> addr_vec;
    addr_vec.push_back(addr);
    std::vector<bool> high_to_low_vec;
    high_to_low_vec.push_back(true);
    return gart::VertexIterator(addr_vec, high_to_low_vec, table_addr,
                                label_id);
  }

  gart::VertexIterator OuterVertices(label_id_t label_id) const {
    vid_t* table_addr = vertex_tables_[label_id];
    int64_t outer_offset = outer_offsets_[label_id];
    vid_t* begin_addr = nullptr;
    vid_t* end_addr = nullptr;
    if (table_addr != nullptr) {
      begin_addr = table_addr + outer_offset;
      end_addr = table_addr + vertex_table_lens_[label_id];
    }
    std::pair<vid_t*, vid_t*> addr = std::make_pair(begin_addr, end_addr);
    std::vector<std::pair<vid_t*, vid_t*>> addr_vec;
    addr_vec.push_back(addr);
    std::vector<bool> high_to_low_vec;
    high_to_low_vec.push_back(false);
    return gart::VertexIterator(addr_vec, high_to_low_vec, table_addr,
                                label_id);
  }

  inline vid_t GetVerticesNum(label_id_t label_id) {
    if (vertex_mata_known_ == false) {
      computeVertexNum();
    }
    return tvnums_[label_id];
  }

  bool GetVertex(label_id_t label, const oid_t& oid, vertex_t& v) const {
    auto vertex_map = vertex_maps_[label];
    auto vertex_iter = vertex_map->find(oid);
    vid_t gid;
    if (vertex_iter != vertex_map->end()) {
      gid = vertex_iter->second;
    } else {
      return false;
    }
    return (vid_parser.GetFid(gid) == fid_) ? InnerVertexGid2Vertex(gid, v)
                                            : OuterVertexGid2Vertex(gid, v);
  }

  oid_t GetId(const vertex_t& v) const {
    auto label_id = vid_parser.GetLabelId(v.GetValue());
    auto offset = GetOffset(v);
    if (IsInnerVertex(v)) {
      return vertex_ext_id_ptrs_[label_id][offset];
    }
    return outer_vertex_ext_id_ptrs_[label_id][offset];
  }

  fid_t GetFragId(const vertex_t& u) const {
    if (IsInnerVertex(u)) {
      return fid_;
    } else {
      auto v_offset = vid_parser.GetOffset(u.GetValue());
      auto v_label = vid_parser.GetLabelId(u.GetValue());
      vid_t gid = ovl2g_[v_label][max_outer_id_offset_ - v_offset];
      return vid_parser.GetFid(gid);
    }
  }

  fid_t GetFragIdFromGid(const vid_t& gid) const {
    return vid_parser.GetFid(gid);
  }

  size_t GetVerticesNum() {
    if (vertex_mata_known_ == false) {
      computeVertexNum();
    }
    size_t total_num = 0;
    for (size_t idx = 0; idx < vertex_label_num_; ++idx) {
      total_num += ivnums_[idx];
    }
    return total_num;
  }

  size_t GetTotalNodesNum() const {
    // TODO(wanglei):not implemented
    return 0;
  }
  size_t GetTotalVerticesNum() const {
    // TODO(wanglei):not implemented
    return 0;
  }

  size_t GetTotalVerticesNum(label_id_t label) const { return 0; }

  size_t GetEdgeNum() {
    if (edge_mata_known_ == false) {
      computeEdgeNum();
    }
    size_t total_num = 0;
    for (size_t idx = 0; idx < edge_label_num_; ++idx) {
      total_num += tenums_[idx];
    }
    return total_num;
  }

  size_t GetEdgeNum(label_id_t label) {
    if (edge_mata_known_ == false) {
      computeEdgeNum();
    }
    return tenums_[label];
  }

  size_t GetInEdgeNum() const {
    // TODO(wanglei):not implemented
    return 0;
  }

  size_t GetOutEdgeNum() const {
    // TODO(wanglei):not implemented
    return 0;
  }

  std::string GetVertexLabelName(label_id_t label_id) const {
    return vertex_label2name_.find(label_id)->second;
  }

  label_id_t GetVertexLabelId(const std::string& label_name) const {
    auto iter = vertex_name2label_.find(label_name);
    if (iter == vertex_name2label_.end()) {
      return -1;
    }
    return iter->second;
  }

  std::string GetEdgeLabelName(label_id_t label_id) const {
    return edge_label2name_.find(label_id)->second;
  }

  label_id_t GetEdgeLabelId(const std::string& label_name) const {
    auto iter = edge_name2label_.find(label_name);
    if (iter == edge_name2label_.end()) {
      return -1;
    }
    return edge_name2label_.find(label_name)->second;
  }

  std::string GetVertexPropName(label_id_t label_id, prop_id_t prop_id) const {
    return vertex_prop2name_.find(std::make_pair(label_id, prop_id))->second;
  }

  prop_id_t GetVertexPropId(label_id_t label_id,
                            const std::string& prop_name) const {
    auto iter = vertex_name2prop_.find(std::make_pair(label_id, prop_name));
    if (iter == vertex_name2prop_.end()) {
      return -1;
    }
    return iter->second;
  }

  std::string GetEdgePropName(label_id_t label_id, prop_id_t prop_id) const {
    return edge_prop2name_.find(std::make_pair(label_id, prop_id))->second;
  }

  prop_id_t GetEdgePropId(label_id_t label_id,
                          const std::string& prop_name) const {
    auto iter = edge_name2prop_.find(std::make_pair(label_id, prop_name));
    if (iter == edge_name2prop_.end()) {
      return -1;
    }
    return iter->second;
  }

  std::string GetVertexPropDataType(label_id_t label_id,
                                    prop_id_t prop_id) const {
    return vertex_prop2dtype_.find(std::make_pair(label_id, prop_id))->second;
  }

  std::string GetEdgePropDataType(label_id_t label_id,
                                  prop_id_t prop_id) const {
    return edge_prop2dtype_.find(std::make_pair(label_id, prop_id))->second;
  }

  bool VertexPropValueIsValid(const vertex_t& v, prop_id_t prop_id) const {
    assert(IsInnerVertex(v));
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    int column_family_id = vertex_prop_column_family_id_[label_id][prop_id];
    auto v_offset = GetOffset(v);
    auto header_offset = prop_cols_meta[label_id][column_family_id].header;

    const char* blob_base = vertex_prop_blob_ptrs_[label_id][column_family_id];
    FlexColBlobHeader* header =
        (FlexColBlobHeader*) (blob_base + header_offset);
    int vertex_per_page = header->get_num_row_per_page();
    int page_id = v_offset / vertex_per_page;
    PageHeader* page_header = header->get_page_header_ptr(blob_base, page_id);
    int page_idx = v_offset % vertex_per_page;
    const char* data = nullptr;

    for (; page_header; page_header = page_header->get_prev(blob_base)) {
      if (page_header->get_epoch() <= (int) read_epoch_number_) {
        data = page_header->get_data();
        break;
      }
    }

    return get_bit(
               (uint8_t*) data,
               page_idx * vertex_prop_num_per_column_family_[label_id]
                                                            [column_family_id] +
                   vertex_prop_id_in_column_family_[label_id][prop_id]) ==
           false;
  }

  template <typename T>
  const char* GetDataAddr(const vertex_t& v, prop_id_t prop_id) const {
    T t{};
    return GetDataAddrImpl(t, v, prop_id);
  }

  template <typename T>
  const char* GetDataAddrImpl(T& t, const vertex_t& v,
                              prop_id_t prop_id) const {
    assert(IsInnerVertex(v));
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    int column_family_id = vertex_prop_column_family_id_[label_id][prop_id];
    int column_family_offset =
        vertex_prop_column_family_offset_[label_id][prop_id];
    auto v_offset = GetOffset(v);
    auto header_offset = prop_cols_meta[label_id][column_family_id].header;
    if (prop_cols_meta[label_id][column_family_id].updatable == true) {
      const char* blob_base =
          vertex_prop_blob_ptrs_[label_id][column_family_id];
      FlexColBlobHeader* header =
          (FlexColBlobHeader*) (blob_base + header_offset);
      int vertex_per_page = header->get_num_row_per_page();
      int page_id = v_offset / vertex_per_page;
      PageHeader* page_header = header->get_page_header_ptr(blob_base, page_id);
      int page_idx = v_offset % vertex_per_page;

      for (; page_header; page_header = page_header->get_prev(blob_base)) {
        if (page_header->get_epoch() <= (int) read_epoch_number_) {
          const char* data =
              page_header->get_data() +
              BYTE_SIZE(vertex_per_page *
                        vertex_prop_num_per_column_family_[label_id]
                                                          [column_family_id]);
          return data +
                 page_idx *
                     column_family_data_length_[label_id][column_family_id] +
                 column_family_offset;
        }
      }
    } else {
      char* data =
          (char*) (vertex_prop_blob_ptrs_[label_id][prop_id] + header_offset);
      return data + sizeof(T) * v_offset;
    }
    return nullptr;
  }

  char* GetDataAddrImpl(std::string_view& t, const vertex_t& v,
                        prop_id_t prop_id) const {
    assert(IsInnerVertex(v));
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    int column_family_id = vertex_prop_column_family_id_[label_id][prop_id];
    int column_family_offset =
        vertex_prop_column_family_offset_[label_id][prop_id];
    auto v_offset = GetOffset(v);
    auto header_offset = prop_cols_meta[label_id][column_family_id].header;
    if (prop_cols_meta[label_id][column_family_id].updatable == true) {
      const char* blob_base =
          vertex_prop_blob_ptrs_[label_id][column_family_id];
      FlexColBlobHeader* header =
          (FlexColBlobHeader*) (blob_base + header_offset);
      int vertex_per_page = header->get_num_row_per_page();
      int page_id = v_offset / vertex_per_page;
      PageHeader* page_header = header->get_page_header_ptr(blob_base, page_id);
      int page_idx = v_offset % vertex_per_page;

      for (; page_header; page_header = page_header->get_prev(blob_base)) {
        if (page_header->get_epoch() <= (int) read_epoch_number_) {
          const char* data =
              page_header->get_data() +
              BYTE_SIZE(vertex_per_page *
                        vertex_prop_num_per_column_family_[label_id]
                                                          [column_family_id]);
          int64_t value =
              *((int64_t*) (data +
                            page_idx *
                                column_family_data_length_[label_id]
                                                          [column_family_id] +
                            column_family_offset));
          int64_t str_offset = value >> 16;
          return string_buffer_ + str_offset;
        }
      }
    } else {
      char* data =
          (char*) (vertex_prop_blob_ptrs_[label_id][prop_id] + header_offset);
      int64_t value = *(((int64_t*) data) + v_offset);
      int64_t str_offset = value >> 16;
      return string_buffer_ + str_offset;
    }
    return nullptr;
  }

  const char* GetRowDataAddr(const vertex_t& v,
                             prop_id_t column_family_id) const {
    assert(IsInnerVertex(v));
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    auto v_offset = GetOffset(v);
    auto header_offset = prop_cols_meta[label_id][column_family_id].header;
    const char* blob_base = vertex_prop_blob_ptrs_[label_id][column_family_id];
    FlexColBlobHeader* header =
        (FlexColBlobHeader*) (blob_base + header_offset);
    int vertex_per_page = header->get_num_row_per_page();
    int page_id = v_offset / vertex_per_page;
    PageHeader* page_header = header->get_page_header_ptr(blob_base, page_id);
    int page_idx = v_offset % vertex_per_page;
    for (; page_header; page_header = page_header->get_prev(blob_base)) {
      if (page_header->get_epoch() <= (int) read_epoch_number_) {
        const char* data =
            page_header->get_data() +
            BYTE_SIZE(
                vertex_per_page *
                vertex_prop_num_per_column_family_[label_id][column_family_id]);
        return data +
               page_idx *
                   column_family_data_length_[label_id][column_family_id];
      }
    }
    return nullptr;
  }

  template <typename T>
  T GetData(const vertex_t& v, prop_id_t prop_id) const {
    T t{};
    GetDataImpl(t, v, prop_id);
    return t;
  }

  template <typename T>
  void GetDataImpl(T& t, const vertex_t& v, prop_id_t prop_id) const {
    assert(IsInnerVertex(v));
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    int column_family_id = vertex_prop_column_family_id_[label_id][prop_id];
    int column_family_offset =
        vertex_prop_column_family_offset_[label_id][prop_id];
    auto v_offset = GetOffset(v);
    auto header_offset = prop_cols_meta[label_id][column_family_id].header;
    if (prop_cols_meta[label_id][column_family_id].updatable == true) {
      const char* blob_base =
          vertex_prop_blob_ptrs_[label_id][column_family_id];
      FlexColBlobHeader* header =
          (FlexColBlobHeader*) (blob_base + header_offset);
      int vertex_per_page = header->get_num_row_per_page();
      int page_id = v_offset / vertex_per_page;
      PageHeader* page_header = header->get_page_header_ptr(blob_base, page_id);
      int page_idx = v_offset % vertex_per_page;

      for (; page_header; page_header = page_header->get_prev(blob_base)) {
        if (page_header->get_epoch() <= (int) read_epoch_number_) {
          const char* data =
              page_header->get_data() +
              BYTE_SIZE(vertex_per_page *
                        vertex_prop_num_per_column_family_[label_id]
                                                          [column_family_id]);
          t = *(
              (T*) (data +
                    page_idx *
                        column_family_data_length_[label_id][column_family_id] +
                    column_family_offset));
          return;
        }
      }
    } else {
      char* data =
          (char*) (vertex_prop_blob_ptrs_[label_id][prop_id] + header_offset);
      t = *(((T*) data) + v_offset);
      return;
    }
    assert(false);
    return;
  }

  void GetDataImpl(std::string_view& t, const vertex_t& v,
                   prop_id_t prop_id) const {
    assert(IsInnerVertex(v));
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    int column_family_id = vertex_prop_column_family_id_[label_id][prop_id];
    int column_family_offset =
        vertex_prop_column_family_offset_[label_id][prop_id];
    auto v_offset = GetOffset(v);
    auto header_offset = prop_cols_meta[label_id][column_family_id].header;
    if (prop_cols_meta[label_id][column_family_id].updatable == true) {
      const char* blob_base =
          vertex_prop_blob_ptrs_[label_id][column_family_id];
      FlexColBlobHeader* header =
          (FlexColBlobHeader*) (blob_base + header_offset);
      int vertex_per_page = header->get_num_row_per_page();
      int page_id = v_offset / vertex_per_page;
      PageHeader* page_header = header->get_page_header_ptr(blob_base, page_id);
      int page_idx = v_offset % vertex_per_page;

      for (; page_header; page_header = page_header->get_prev(blob_base)) {
        if (page_header->get_epoch() <= (int) read_epoch_number_) {
          const char* data =
              page_header->get_data() +
              BYTE_SIZE(vertex_per_page *
                        vertex_prop_num_per_column_family_[label_id]
                                                          [column_family_id]);
          int64_t value =
              *((int64_t*) (data +
                            page_idx *
                                column_family_data_length_[label_id]
                                                          [column_family_id] +
                            column_family_offset));
          int64_t str_offset = value >> 16;
          int64_t str_len = value & 0xffff;
          t = std::string_view(string_buffer_ + str_offset, str_len);
          return;
        }
      }
    } else {
      char* data =
          (char*) (vertex_prop_blob_ptrs_[label_id][prop_id] + header_offset);
      int64_t value = *(((int64_t*) data) + v_offset);
      int64_t str_offset = value >> 16;
      int64_t str_len = value & 0xffff;
      t = std::string_view(string_buffer_ + str_offset, str_len);
      return;
    }
    return;
  }

  int64_t GetExternalIdAsInt64(const vertex_t& v) const {
    auto label_id = vid_parser.GetLabelId(v.GetValue());
    auto offset = GetOffset(v);
    if (IsInnerVertex(v)) {
      return vertex_ext_id_ptrs_[label_id][offset];
    }
    return outer_vertex_ext_id_ptrs_[label_id][offset];
  }

  std::string_view GetExternalIdAsString(const vertex_t& v) const {
    auto label_id = vid_parser.GetLabelId(v.GetValue());
    auto offset = GetOffset(v);
    int64_t faka_value;
    if (IsInnerVertex(v)) {
      faka_value = vertex_ext_id_ptrs_[label_id][offset];
    } else {
      faka_value = outer_vertex_ext_id_ptrs_[label_id][offset];
    }
    int64_t str_offset = faka_value >> 16;
    int64_t str_len = faka_value & 0xffff;
    return std::string_view(string_buffer_ + str_offset, str_len);
  }

  int GetLocalOutDegree(const vertex_t& v, label_id_t e_label) const {
    // TODO(wanglei):not implemented
    return 0;
  }

  int GetLocalInDegree(const vertex_t& v, label_id_t e_label) const {
    // TODO(wanglei):not implemented
    return 0;
  }

  bool Gid2Vertex(const vid_t& gid, vertex_t& v) const {
    return (vid_parser.GetFid(gid) == fid_) ? InnerVertexGid2Vertex(gid, v)
                                            : OuterVertexGid2Vertex(gid, v);
  }

  vid_t Vertex2Gid(const vertex_t& v) const {
    return IsInnerVertex(v) ? GetInnerVertexGid(v) : GetOuterVertexGid(v);
  }

  inline vid_t GetInnerVerticesNum(label_id_t label_id) const {
    return ivnums_[label_id];
  }

  inline vid_t GetOuterVerticesNum(label_id_t label_id) const {
    return ovnums_[label_id];
  }

  inline bool IsInnerVertex(const vertex_t& v) const {
    auto v_label = vid_parser.GetLabelId(v.GetValue());
    int64_t v_offset = vid_parser.GetOffset(v.GetValue());
    return v_offset <= max_inner_offsets_[v_label] ? true : false;
  }

  inline bool IsOuterVertex(const vertex_t& v) const {
    auto v_label = vid_parser.GetLabelId(v.GetValue());
    int64_t v_offset = vid_parser.GetOffset(v.GetValue());
    return v_offset >= min_outer_offsets_[v_label] ? true : false;
  }

  bool GetInnerVertex(label_id_t label, const oid_t& oid, vertex_t& v) const {
    auto vertex_map = vertex_maps_[label];
    auto vertex_iter = vertex_map->find(oid);
    vid_t gid;
    if (vertex_iter != vertex_map->end()) {
      gid = vertex_iter->second;
    } else {
      return false;
    }
    assert(vid_parser.GetLabelId(gid) == label);
    return InnerVertexGid2Vertex(gid, v);
  }

  bool GetOuterVertex(label_id_t label, const oid_t& oid, vertex_t& v) const {
    auto vertex_map = vertex_maps_[label];
    auto vertex_iter = vertex_map->find(oid);
    vid_t gid;
    if (vertex_iter != vertex_map->end()) {
      gid = vertex_iter->second;
    } else {
      return false;
    }
    assert(vid_parser.GetLabelId(gid) == label);
    return OuterVertexGid2Vertex(gid, v);
  }

  inline oid_t Gid2Oid(const vid_t& gid) const {
    auto v_label = vid_parser.GetLabelId(gid);
    if (vid_parser.GetFid(gid) == fid_) {
      auto v_offset = vid_parser.GetOffset(gid);
      return vertex_ext_id_ptrs_[v_label][v_offset];
    }
    vertex_t v;
    OuterVertexGid2Vertex(gid, v);
    auto v_offset = GetOffset(v);
    return outer_vertex_ext_id_ptrs_[v_label][v_offset];
  }

  inline bool Oid2Gid(label_id_t label, const oid_t& oid, vid_t& gid) const {
    auto vertex_map = vertex_maps_[label];
    auto vertex_iter = vertex_map->find(oid);
    if (vertex_iter != vertex_map->end()) {
      gid = vertex_iter->second;
      return true;
    } else {
      return false;
    }
  }

  inline bool Oid2Gid(label_id_t label, const oid_t& oid, vertex_t& v) const {
    auto vertex_map = vertex_maps_[label];
    auto vertex_iter = vertex_map->find(oid);
    vid_t gid;
    if (vertex_iter != vertex_map->end()) {
      gid = vertex_iter->second;
    } else {
      return false;
    }
    return Gid2Vertex(gid, v);
  }

  inline bool InnerVertexGid2Vertex(const vid_t& gid, vertex_t& v) const {
    if (vid_parser.GetFid(gid) == fid_) {
      v.SetValue(vid_parser.GetLid(gid));
      return true;
    } else {
      return false;
    }
  }

  inline bool OuterVertexGid2Vertex(const vid_t& gid, vertex_t& v) const {
    auto map = ovg2l_maps_[vid_parser.GetLabelId(gid)];
    auto iter = map->find(gid);
    if (iter != map->end()) {
      v.SetValue(iter->second);
      return true;
    } else {
      return false;
    }
  }

  inline vid_t GetOuterVertexGid(const vertex_t& v) const {
    auto v_offset = vid_parser.GetOffset(v.GetValue());
    auto v_label = vid_parser.GetLabelId(v.GetValue());
    return ovl2g_[v_label][max_outer_id_offset_ - v_offset];
  }

  inline vid_t GetInnerVertexGid(const vertex_t& v) const {
    return vid_parser.GenerateId(fid_, vid_parser.GetLabelId(v.GetValue()),
                                 vid_parser.GetOffset(v.GetValue()));
  }

  inline gart::EdgeIterator GetIncomingAdjList(const vertex_t& v,
                                               label_id_t e_label) const {
    auto segment = locate_segment_(v, e_label, seggraph::EIN);
    auto prop_num = edge_prop_nums_[e_label];
    int* prop_offsets = nullptr;
    int prop_bytes = 0;
    if (prop_num > 0) {
      prop_bytes =
          edge_prop_offsets[e_label][prop_num - 1] + edge_bitmap_size_[e_label];
      prop_offsets = (int*) edge_prop_offsets[e_label].data();
    }
    return get_edges_in_seg_(segment, v, prop_bytes, prop_offsets,
                             edge_bitmap_size_[e_label]);
  }

  inline gart::EdgeIterator GetOutgoingAdjList(const vertex_t& v,
                                               label_id_t e_label) const {
    auto segment = locate_segment_(v, e_label, seggraph::EOUT);

    auto prop_num = edge_prop_nums_[e_label];

    int* prop_offsets = nullptr;
    int prop_bytes = 0;
    if (prop_num > 0) {
      prop_bytes =
          edge_prop_offsets[e_label][prop_num - 1] + edge_bitmap_size_[e_label];
      prop_offsets = (int*) edge_prop_offsets[e_label].data();
    }
    return get_edges_in_seg_(segment, v, prop_bytes, prop_offsets,
                             edge_bitmap_size_[e_label]);
  }

  inline grape::DestList IEDests(const vertex_t& v, label_id_t e_label) const {
    int64_t offset = vid_parser.GetOffset(v.GetValue());
    auto v_label = vertex_label(v);

    return grape::DestList(idoffset_[v_label][e_label][offset],
                           idoffset_[v_label][e_label][offset + 1]);
  }

  inline grape::DestList OEDests(const vertex_t& v, label_id_t e_label) const {
    int64_t offset = vid_parser.GetOffset(v.GetValue());
    auto v_label = vertex_label(v);

    return grape::DestList(odoffset_[v_label][e_label][offset],
                           odoffset_[v_label][e_label][offset + 1]);
  }

  inline grape::DestList IOEDests(const vertex_t& v, label_id_t e_label) const {
    int64_t offset = vid_parser.GetOffset(v.GetValue());
    auto v_label = vertex_label(v);

    return grape::DestList(iodoffset_[v_label][e_label][offset],
                           iodoffset_[v_label][e_label][offset + 1]);
  }

  inline size_t GetOffset(const vertex_t& v) const {
    auto vid = v.GetValue();
    if (IsInnerVertex(v)) {
      return vid_parser.GetOffset(vid);
    } else {
      return max_outer_id_offset_ - vid_parser.GetOffset(vid);
    }
  }

  inline vid_t GetMaxInnerVerticesNum(int vlabel) const {
    return max_inner_offsets_[vlabel] + 1;
  }

  inline vid_t GetMaxOuterVerticesNum(int vlabel) const {
    return max_outer_id_offset_ - min_outer_offsets_[vlabel] + 1;
  }

  inline vid_t GetMaxOuterIdOffset() const { return max_outer_id_offset_; }

  inline char* GetStringBuffer() const { return string_buffer_; }

  void PrepareToRunApp(const grape::CommSpec& comm_spec,
                       grape::PrepareConf conf) {
    if (conf.message_strategy ==
        grape::MessageStrategy::kAlongEdgeToOuterVertex) {
      initDestFidList(true, true, iodst_, iodoffset_);
    } else if (conf.message_strategy ==
               grape::MessageStrategy::kAlongIncomingEdgeToOuterVertex) {
      initDestFidList(true, false, idst_, idoffset_);
    } else if (conf.message_strategy ==
               grape::MessageStrategy::kAlongOutgoingEdgeToOuterVertex) {
      initDestFidList(false, true, odst_, odoffset_);
    }
    for (auto vlabel = 0; vlabel < vertex_label_num_; vlabel++) {
      size_t ivnum = 0;
      size_t ovnum = 0;
      auto inner_vertex_iter = InnerVertices(vlabel);
      while (inner_vertex_iter.valid()) {
        ivnum++;
        inner_vertex_iter.next();
      }
      ivnums_[vlabel] = ivnum;
      auto outer_vertex_iter = OuterVertices(vlabel);
      while (outer_vertex_iter.valid()) {
        ovnum++;
        outer_vertex_iter.next();
      }
      ovnums_[vlabel] = ovnum;
      tvnums_[vlabel] = ivnum + ovnum;
    }
  }
  std::pair<vid_t*, vid_t*> get_inner_vertices_addr(int label_id) const {
    vid_t* end = vertex_tables_[label_id] - 1;
    vid_t* cur = end + inner_offsets_[label_id] + 2;
    return std::make_pair(cur, end);
  }

 private:
  inline seggraph::VegitoSegmentHeader* locate_segment_(const vertex_t& v,
                                                        label_id_t e_label,
                                                        dir_t dir) const {
    uint64_t header_offset = 0;
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    char* edge_blob_ptr = nullptr;
    if (IsInnerVertex(v)) {
      auto seg_id = vid_parser.GetOffset(v.GetValue()) / VERTEX_PER_SEG;
      header_offset = inner_edge_label_ptrs_[label_id][seg_id];
      edge_blob_ptr = inner_edge_blob_ptrs_[label_id];
    } else {
      auto seg_id =
          (max_outer_id_offset_ - vid_parser.GetOffset(v.GetValue())) /
          VERTEX_PER_SEG;
      header_offset = outer_edge_label_ptrs_[label_id][seg_id];
      edge_blob_ptr = outer_edge_blob_ptrs_[label_id];
    }
    if (header_offset == 0) {
      return nullptr;
    }
    auto edge_label_block =
        (EdgeLabelBlockHeader*) (edge_blob_ptr + header_offset);
    for (size_t i = 0; i < edge_label_block->get_num_entries(); i++) {
      auto label_entry = edge_label_block->get_entries()[i];

      if (label_entry.get_label() == e_label) {
        if (label_entry.get_pointer(dir) == 0) {
          return nullptr;
        }
        return (VegitoSegmentHeader*) (edge_blob_ptr +
                                       label_entry.get_pointer(dir));
      }
    }

    return nullptr;
  }

  inline gart::EdgeIterator get_edges_in_seg_(VegitoSegmentHeader* segment,
                                              const vertex_t& v,
                                              size_t edge_prop_size,
                                              int* prop_offsets,
                                              size_t bitmap_size) const {
    if (!segment) {
      return gart::EdgeIterator(nullptr, nullptr, nullptr, nullptr, 0, 0,
                                read_epoch_number_, nullptr, string_buffer_,
                                bitmap_size);
    }
    label_id_t label_id = vid_parser.GetLabelId(v.GetValue());
    char* edge_blob_ptr = nullptr;
    uint64_t seg_idx = 0;
    if (IsInnerVertex(v)) {
      seg_idx = vid_parser.GetOffset(v.GetValue()) % VERTEX_PER_SEG;
      edge_blob_ptr = inner_edge_blob_ptrs_[label_id];
    } else {
      seg_idx = (max_outer_id_offset_ - vid_parser.GetOffset(v.GetValue())) %
                VERTEX_PER_SEG;
      edge_blob_ptr = outer_edge_blob_ptrs_[label_id];
    }
    auto epoch_table_offset = segment->get_epoch_table(seg_idx);
    EpochBlockHeader* epoch_table =
        (EpochBlockHeader*) (edge_blob_ptr + epoch_table_offset);

    auto edge_block_offset = segment->get_region_ptr(seg_idx);
    VegitoEdgeBlockHeader* edge_block =
        (VegitoEdgeBlockHeader*) (edge_blob_ptr + edge_block_offset);
    if (!epoch_table || !edge_block || epoch_table_offset == 0 ||
        edge_block_offset == 0) {
      return gart::EdgeIterator(nullptr, nullptr, nullptr, nullptr, 0, 0,
                                read_epoch_number_, nullptr, string_buffer_,
                                bitmap_size);
    }

    auto num_entries = edge_block->get_num_entries();
    if (num_entries == 0) {  // no edges to read
      return gart::EdgeIterator(nullptr, nullptr, nullptr, nullptr, 0, 0,
                                read_epoch_number_, nullptr, string_buffer_,
                                bitmap_size);
    }

    return gart::EdgeIterator(segment, edge_block, epoch_table, edge_blob_ptr,
                              num_entries, edge_prop_size, read_epoch_number_,
                              prop_offsets, string_buffer_, bitmap_size);
  }

  void initDestFidList(
      bool in_edge, bool out_edge,
      std::vector<std::vector<std::vector<fid_t>>>& fid_lists,
      std::vector<std::vector<std::vector<fid_t*>>>& fid_lists_offset) {
    for (auto v_label_id = 0; v_label_id < vertex_label_num_; v_label_id++) {
      auto inner_vertices_iter = InnerVertices(v_label_id);
      int ivnum = 0;
      while (inner_vertices_iter.valid()) {
        ivnum++;
        inner_vertices_iter.next();
      }
      for (auto e_label_id = 0; e_label_id < edge_label_num_; e_label_id++) {
        std::set<fid_t> dstset;
        std::vector<int> id_num(ivnum, 0);
        auto& fid_list = fid_lists[v_label_id][e_label_id];
        auto& fid_list_offset = fid_lists_offset[v_label_id][e_label_id];
        assert(fid_list_offset.empty());
        fid_list_offset.resize(ivnum + 1);
        auto vertices_iter = InnerVertices(v_label_id);
        int idx = 0;
        while (vertices_iter.valid()) {
          dstset.clear();
          auto v = vertices_iter.vertex();
          if (in_edge) {
            auto edge_iter = GetIncomingAdjList(v, e_label_id);
            while (edge_iter.valid()) {
              auto nbr = edge_iter.neighbor();
              auto f = GetFragId(nbr);
              if (f != fid_) {
                dstset.insert(f);
              }
              edge_iter.next();
            }
          }

          if (out_edge) {
            auto edge_iter = GetOutgoingAdjList(v, e_label_id);
            while (edge_iter.valid()) {
              auto nbr = edge_iter.neighbor();
              auto f = GetFragId(nbr);
              if (f != fid_) {
                dstset.insert(f);
              }
              edge_iter.next();
            }

            id_num[idx] = dstset.size();
            for (auto fid : dstset) {
              fid_list.push_back(fid);
            }
          }
          vertices_iter.next();
          idx++;
        }
        fid_list.shrink_to_fit();
        fid_list_offset[0] = fid_list.data();
        for (auto i = 0; i < ivnum; i++) {
          fid_list_offset[i + 1] = fid_list_offset[i] + id_num[i];
        }
      }
    }
  }

  void computeVertexNum() {
    for (auto v_label_id = 0; v_label_id < vertex_label_num_; v_label_id++) {
      auto inner_vertices_iter = InnerVertices(v_label_id);
      size_t ivnum = 0;
      while (inner_vertices_iter.valid()) {
        ivnum++;
        inner_vertices_iter.next();
      }
      ivnums_[v_label_id] = ivnum;
      auto outer_vertices_iter = OuterVertices(v_label_id);
      size_t ovnum = 0;
      while (outer_vertices_iter.valid()) {
        ovnum++;
        outer_vertices_iter.next();
      }
      ovnums_[v_label_id] = ovnum;
      tvnums_[v_label_id] = ivnum + ovnum;
    }
    vertex_mata_known_ = true;
  }

  void computeEdgeNum() {
    tenums_.resize(edge_label_num_, 0);
    for (auto v_label_id = 0; v_label_id < vertex_label_num_; v_label_id++) {
      auto inner_vertices_iter = InnerVertices(v_label_id);
      while (inner_vertices_iter.valid()) {
        auto v = inner_vertices_iter.vertex();
        for (auto e_label_id = 0; e_label_id < edge_label_num_; e_label_id++) {
          auto edge_iter = GetOutgoingAdjList(v, e_label_id);
          while (edge_iter.valid()) {
            tenums_[e_label_id]++;
            edge_iter.next();
          }
        }
        inner_vertices_iter.next();
      }
    }
    edge_mata_known_ = true;
  }

 private:
  vineyard::Client client_;
  std::string ipc_socket_;

  size_t read_epoch_number_;
  std::vector<int64_t> inner_offsets_, outer_offsets_;
  std::vector<vid_t*> vertex_tables_;
  std::vector<int64_t> vertex_table_lens_;
  std::vector<int64_t> max_inner_offsets_, min_outer_offsets_;
  vid_t max_outer_id_offset_;
  char* string_buffer_;

  std::vector<size_t> ivnums_, ovnums_, tvnums_;
  std::vector<size_t> tenums_;

  std::vector<vid_t*> ovl2g_;
  std::vector<size_t> valid_ovl2g_element_;
  std::vector<std::shared_ptr<const hashmap_t>> ovg2l_maps_;

  std::vector<uint64_t*> inner_edge_label_ptrs_;
  std::vector<uint64_t*> outer_edge_label_ptrs_;
  std::vector<char*> inner_edge_blob_ptrs_, outer_edge_blob_ptrs_;

  std::vector<std::vector<std::vector<fid_t>>> idst_, odst_, iodst_;
  std::vector<std::vector<std::vector<fid_t*>>> idoffset_, odoffset_,
      iodoffset_;

  // for edge property
  std::vector<int> edge_prop_nums_;

  // for vertex property
  std::vector<int> vertex_prop_nums_;
  std::vector<std::vector<char*>> vertex_prop_blob_ptrs_;
  std::vector<std::vector<int>> vertex_prop_column_family_id_;
  std::vector<std::vector<int>> vertex_prop_column_family_offset_;
  std::vector<std::vector<int>> vertex_prop_num_per_column_family_;
  std::vector<std::vector<int>> column_family_data_length_;
  std::vector<std::vector<int>> vertex_prop_id_in_column_family_;

  // for vertex external id
  std::vector<int64_t*> vertex_ext_id_ptrs_;
  std::vector<int64_t*> outer_vertex_ext_id_ptrs_;
  std::vector<std::string> vertex_ext_id_dtypes_;
  std::vector<std::shared_ptr<const hashmap_t>> vertex_maps_;

  fid_t fid_, fnum_;
  bool directed_;
  int vertex_label_num_;
  int edge_label_num_;
  bool vertex_mata_known_ = false;
  bool edge_mata_known_ = false;

  std::map<label_id_t, std::string> vertex_label2name_;
  std::map<std::string, label_id_t> vertex_name2label_;
  std::map<label_id_t, std::string> edge_label2name_;
  std::map<std::string, label_id_t> edge_name2label_;
  std::map<std::pair<label_id_t, prop_id_t>, std::string> vertex_prop2name_;
  std::map<std::pair<label_id_t, std::string>, prop_id_t> vertex_name2prop_;
  std::map<std::pair<label_id_t, prop_id_t>, std::string> edge_prop2name_;
  std::map<std::pair<label_id_t, std::string>, prop_id_t> edge_name2prop_;

  std::map<std::pair<label_id_t, prop_id_t>, std::string> vertex_prop2dtype_;
  std::map<std::pair<label_id_t, prop_id_t>, std::string> edge_prop2dtype_;

  std::vector<size_t> edge_bitmap_size_;

  std::string oid_type, vid_type;

 public:
  gart::IdParser<vid_t> vid_parser;
  std::vector<std::vector<VertexPropMeta>> prop_cols_meta;
  std::vector<std::vector<int>> edge_prop_offsets;
  std::map<std::pair<label_id_t, label_id_t>, label_id_t> vertex2edge_map;
  std::map<label_id_t, std::pair<label_id_t, label_id_t>> edge2vertex_map;
  std::vector<std::vector<int>> edge_prop_dtypes;
};

}  // namespace gart

#endif  // INTERFACES_FRAGMENT_GART_FRAGMENT_H_
