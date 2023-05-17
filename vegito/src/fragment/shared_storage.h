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

#ifndef RESEARCH_GART_VEGITO_SRC_FRAGMENT_SHARED_STORAGE_H_
#define RESEARCH_GART_VEGITO_SRC_FRAGMENT_SHARED_STORAGE_H_

#include "vineyard/common/util/json.h"
#include "vineyard/common/util/uuid.h"

#include "seggraph/core/blocks.hpp"

namespace gart {

// TODO: need to refine typedef
typedef vineyard::ObjectID oid_t;  // object id for Blob
typedef uint64_t index_t;

#define get_blob_ptr(oid) ((char *)nullptr)

// ================== Layout ===================

// Vertex table is an array used to store vertex Lid (organized by IdParser)
// [min, max_inner) ... [min_outer, max)
//   inner ID ->    ...    <- outer ID
struct VTable {
  typedef uint64_t lid_t;  // TODO: need to refine this type

  lid_t lids[0];
};

// each EdgeSegment is for a segment
// several Epochtables are store consecutively in the Blob
struct EdgeSegment {
  seggraph::VegitoSegmentHeader header;
  seggraph::VegitoEdgeEntry entries[0];
};

// each EpochTable is for a vertex
// several Epochtables are store consecutively in the Blob
// which is indexed by `epoch_tables` in `VegitoSegmentHeader`
struct EpochTable {
  seggraph::EpochBlockHeader header;
  seggraph::VegitoEpochEntry entries[0];
};

struct ELabel2Seg {
  uintptr_t edge_label_ptrs[0];  // stored as seggraph::EdgeLabelBlockHeader

  seggraph::EdgeLabelBlockHeader *get_edge_label_ptr(index_t i,
                                                     oid_t block_oid) const {
    return reinterpret_cast<seggraph::EdgeLabelBlockHeader *>(
        get_blob_ptr(block_oid) + edge_label_ptrs[i]);
  }
};

struct PageHeader {
  uint64_t ver;
  uintptr_t prev_ptr;

  // useless for reader
  uint64_t min_ver;
  void *prev;
  void *next;

  // payload
  char content[0];
};

struct FlexColHeader {
  int num_row_per_page;
  uintptr_t page_ptr[0];
};

// ================== Scheme ===================

// Vertex table is an array used to store vertex Lid (organized by IdParser)
// [min, max_inner) ... [min_outer, max)
//   inner ID ->    ...    <- outer ID
struct VTableMeta {
 public:
  VTableMeta() {}

  VTableMeta(oid_t oid, index_t max)
      : object_id(oid),
        max(max),
        max_inner(0),
        min_outer(max),
        max_inner_location(0),
        min_outer_location(max) {}

  vineyard::json json() const {
    using json = vineyard::json;
    json res;
    res["object_id"] = object_id;
    res["max"] = max;
    res["max_inner"] = max_inner;
    res["min_outer"] = min_outer;
    res["max_inner_location"] = max_inner_location;
    res["min_outer_location"] = min_outer_location;

    return res;
  }

  inline oid_t get_object_id() const { return object_id; }

  inline void set_boundary(uint64_t num_inner, uint64_t num_outer) {
    max_inner = 0 + num_inner;
    min_outer = max - num_outer;
  }

  inline void set_loc(uint64_t inner_loc, uint64_t outer_loc) {
    max_inner_location = inner_loc;
    min_outer_location = max - outer_loc;
  }

 private:
  // object id for Blob
  oid_t object_id;
  index_t max;  // default is blob.size() / sizeof(element)

  // index boundary
  index_t max_inner;
  index_t min_outer;
  // index_t min;   // default is 0

  index_t max_inner_location;
  index_t min_outer_location;
};

// ELabel2Seg is an array used to store segment Blob indexed by edge label id
struct ArrayMeta {
  ArrayMeta() {}

  ArrayMeta(oid_t id, uint64_t max) : object_id(id), len_ele(max) {}

  ELabel2Seg *get_ptr() const {
    return reinterpret_cast<ELabel2Seg *>(get_blob_ptr(object_id));
  }
  // also can use EpochGraphWriter::locate_segment()

  oid_t get_object_id() const { return object_id; }

  vineyard::json json() const {
    using json = vineyard::json;
    json res;
    res["object_id"] = object_id;
    res["len_ele"] = len_ele;

    return res;
  }

 private:
  oid_t object_id;   // Blob of the ELabel2Seg
  uint64_t len_ele;  // size of array in number of elements
};

// Meta for each vertex property (column)
struct VPropMeta {
  VPropMeta() {}

  void init(uint64_t prop_id, int val_sz, bool updatable, short type) {
    this->prop_id = prop_id;
    this->val_sz = val_sz;
    this->updatable = updatable;
    this->type = type;
  }

  void init_obj(oid_t object_id, uintptr_t header) {
    this->object_id = object_id;
    this->header = header;
  }

  vineyard::json json() const {
    using json = vineyard::json;
    json res;
    res["object_id"] = object_id;
    res["prop_id"] = prop_id;
    res["val_sz"] = val_sz;
    res["type"] = type;
    res["updatable"] = updatable;
    res["header"] = header;

    return res;
  }

 private:
  oid_t object_id;
  uintptr_t header;  // for extension
  int prop_id;
  short val_sz;  // size of value in bytes
  short type;
  bool updatable;
};

// Schema for each vertex label
class BlobSchema {
 public:
  BlobSchema() {}

  void set_vlabel(uint64_t v) { vlabel = v; }

  void set_block_oid(oid_t oid) { block_oid = oid; }

  void set_elabel2segs(const ArrayMeta &meta) { elabel2seg = meta; }

  void set_ov_block_oid(oid_t oid) { ov_block_oid = oid; }

  void set_ov_elabel2segs(const ArrayMeta &meta) { ov_elabel2seg = meta; }

  void set_vtable_meta(const VTableMeta &meta) { vertex_table = meta; }

  void set_ovl2g_meta(const ArrayMeta &meta) { ovl2g = meta; }

  void set_vtable_bound(uint64_t num_inner, uint64_t num_outer) {
    vertex_table.set_boundary(num_inner, num_outer);
  }

  void set_vtable_location(uint64_t loc_inner, uint64_t loc_outer) {
    vertex_table.set_loc(loc_inner, loc_outer);
  }

  void set_prop_meta(const std::vector<VPropMeta> &meta) { vprops = meta; }

  oid_t get_block_oid() const { return block_oid; }

  oid_t get_vertex_table_oid() const { return vertex_table.get_object_id(); }

  oid_t get_ovl2g_oid() const { return ovl2g.get_object_id(); }

  const ArrayMeta &get_elabel2segs() const { return elabel2seg; }

  vineyard::json json() const {
    using json = vineyard::json;

    json single_blob_schema;
    single_blob_schema["vlabel"] = vlabel;
    single_blob_schema["block_oid"] = block_oid;
    single_blob_schema["elabel2seg"] = elabel2seg.json();
    single_blob_schema["num_vprops"] = vprops.size();

    json vprop_schema = json::array();

    for (const auto &vprop : vprops) {
      vprop_schema.push_back(vprop.json());
    }

    single_blob_schema["vprops"] = vprop_schema;
    single_blob_schema["ov_block_oid"] = ov_block_oid;
    single_blob_schema["ov_elabel2seg"] = ov_elabel2seg.json();
    single_blob_schema["vertex_table"] = vertex_table.json();
    single_blob_schema["ovl2g"] = ovl2g.json();

    return single_blob_schema;
  }

 private:
  uint64_t vlabel;

  oid_t block_oid;       // Blob of blocks created by BlockManger
  ArrayMeta elabel2seg;  // indexed by vertex label

  // uint64_t num_vprops;
  std::vector<VPropMeta> vprops;

  oid_t ov_block_oid;
  ArrayMeta ov_elabel2seg;

  VTableMeta vertex_table;  // indexed by vertex label
  ArrayMeta ovl2g;          // indexed by vertex label, array
  oid_t ovg2l;              // TODO: hash map

  // TODO: properties
};

}  // namespace gart

#endif  // RESEARCH_GART_VEGITO_SRC_FRAGMENT_SHARED_STORAGE_H_
