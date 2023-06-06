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

#ifndef VEGITO_SRC_FRAGMENT_ID_PARSER_H_
#define VEGITO_SRC_FRAGMENT_ID_PARSER_H_

#include <cassert>
#include <cstdlib>

namespace gart {

// Hardcoded the max vertex label num to 30
constexpr int MAX_VLABELS = 30;

static inline int num_to_bitwidth(int num) {
  if (num <= 2) {
    return 1;
  }
  int max = num - 1;
  int width = 0;
  while (max) {
    ++width;
    max >>= 1;
  }
  return width;
}

template <typename ID_TYPE>
class IdParser {
  using LabelIDT = int;  // LABEL_ID_TYPE
  using fid_t = ID_TYPE;

 public:
  IdParser() {}
  ~IdParser() {}

  void Init(fid_t fnum, LabelIDT label_num) {
    assert(label_num <= MAX_VLABELS);

    int fid_width = num_to_bitwidth(fnum);
    offset_offset_ = fid_width;
    int label_width = num_to_bitwidth(MAX_VLABELS);
    int offset_width = (sizeof(ID_TYPE) * 8) - fid_width - label_width - 1;
    offset_width_ = offset_width;
    label_id_offset_ = (sizeof(ID_TYPE) * 8) - label_width - 1;
    fid_mask_ = (((ID_TYPE) 1) << offset_offset_) - (ID_TYPE) 1;
    lid_mask_ = ((((ID_TYPE) 1) << ((sizeof(ID_TYPE) * 8) - fid_width - 1)) -
                 (ID_TYPE) 1)
                << offset_offset_;
    label_id_mask_ = ((((ID_TYPE) 1) << label_width) - (ID_TYPE) 1)
                     << label_id_offset_;
    offset_mask_ = ((((ID_TYPE) 1) << offset_width) - (ID_TYPE) 1)
                   << offset_offset_;
  }

  fid_t GetFid(ID_TYPE v) const { return (v & fid_mask_); }

  LabelIDT GetLabelId(ID_TYPE v) const {
    return (v & label_id_mask_) >> label_id_offset_;
  }

  int64_t GetOffset(ID_TYPE v) const {
    return (v & offset_mask_) >> offset_offset_;
  }

  ID_TYPE GetLid(ID_TYPE v) const { return (v & lid_mask_); }

  ID_TYPE GenerateId(fid_t fid, LabelIDT label, int64_t offset) const {
    return (((ID_TYPE) fid) & fid_mask_) |
           ((((ID_TYPE) offset) << offset_offset_) & offset_mask_) |
           ((((ID_TYPE) label) << label_id_offset_) & label_id_mask_);
  }

  ID_TYPE GenerateDeletedId(int64_t offset) const {
    ID_TYPE vid = static_cast<ID_TYPE>(offset);
    ID_TYPE mask = ((ID_TYPE) 1) << (sizeof(ID_TYPE) * 8 - 1);
    return vid | mask;
  }

  ID_TYPE GenerateOuterId(fid_t fid, LabelIDT label, int64_t offset) const {
    ID_TYPE outer_max = (((ID_TYPE) 1) << offset_width_) - (ID_TYPE) 1;
    ID_TYPE outer_offset = outer_max - (ID_TYPE) offset;
    return (((ID_TYPE) fid) & fid_mask_) |
           ((((ID_TYPE) outer_offset) << offset_offset_) & offset_mask_) |
           ((((ID_TYPE) label) << label_id_offset_) & label_id_mask_);
  }

  int GetOffsetWidth() const { return offset_width_; }

 private:
  int offset_offset_;
  int label_id_offset_;
  ID_TYPE fid_mask_;
  ID_TYPE lid_mask_;
  ID_TYPE label_id_mask_;
  ID_TYPE offset_mask_;
  int offset_width_;
};

}  // namespace gart

#endif  // VEGITO_SRC_FRAGMENT_ID_PARSER_H_
