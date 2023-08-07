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

#ifndef INTERFACES_FRAGMENT_ITERATOR_H_
#define INTERFACES_FRAGMENT_ITERATOR_H_

#include <cstdint>
#include "interfaces/fragment/types.h"
#include "seggraph/blocks.hpp"

namespace gart {
template <typename VID_T, typename VDATA_T>
class GartVertexArray;
class VertexIterator {
 public:
  VertexIterator() {
    vertex_table_addr_ = nullptr;
    high_to_low_flags_.push_back(true);
    cur_ = nullptr;
    begin_ = nullptr;
    end_ = nullptr;
    loc_ = 0;
    high_to_low_flag_ = true;
  }
  VertexIterator(std::vector<std::pair<vid_t*, vid_t*>> addrs,
                 std::vector<bool> high_to_low_flags, vid_t* vertex_table_addr,
                 int vlabel) {
    vertex_table_addr_ = vertex_table_addr;
    vlabel_ = vlabel;
    for (size_t i = 0; i < addrs.size(); i++) {
      addrs_.push_back(std::make_pair(addrs[i].first, addrs[i].second));
      high_to_low_flags_.push_back(high_to_low_flags[i]);
    }
    cur_ = addrs_[0].first;
    begin_ = addrs_[0].first;
    end_ = addrs_[0].second;
    high_to_low_flag_ = high_to_low_flags_[0];
    loc_ = 0;
    find_next_valid_cursor();
  }

  ~VertexIterator() {}

  bool valid() {
    if (unlikely(cur_ == end_)) {
      return switch_next_range();
    } else {
      return true;
    }
  }

  void next() {
    if (!high_to_low_flag_) {
      cur_++;
      while (cur_ != end_) {
        vid_t v = *cur_;
        auto delete_flag = v >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 1) {
          auto delete_offset_mask =
              (((vid_t) 1) << (sizeof(vid_t) * 8 - 1)) - (vid_t) 1;
          auto delete_offset = v & delete_offset_mask;
          outer_delete_offset_.push(delete_offset);
        } else if (delete_flag == 0) {
          if (outer_delete_offset_.empty()) {
            break;
          }
          if (cur_ - vertex_table_addr_ != outer_delete_offset_.top()) {
            break;
          } else {
            outer_delete_offset_.pop();
          }
        }
        cur_++;
      }
    } else {
      cur_--;
      while (cur_ != end_) {
        vid_t v = *cur_;
        auto delete_flag = v >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 1) {
          auto delete_offset_mask =
              (((vid_t) 1) << (sizeof(vid_t) * 8 - 1)) - (vid_t) 1;
          auto delete_offset = v & delete_offset_mask;
          inner_delete_offset_.push(delete_offset);
        } else if (delete_flag == 0) {
          if (inner_delete_offset_.empty()) {
            break;
          }
          if (cur_ - vertex_table_addr_ != inner_delete_offset_.top()) {
            break;
          } else {
            inner_delete_offset_.pop();
          }
        }
        cur_--;
      }
    }
  }

  bool switch_next_range() {
    if (!((loc_ == 0) && (addrs_.size() == 2))) {
      return false;
    } else {
      loc_++;
      cur_ = addrs_[loc_].first;
      begin_ = addrs_[loc_].first;
      end_ = addrs_[loc_].second;
      high_to_low_flag_ = high_to_low_flags_[loc_];

      find_next_valid_cursor();

      if (cur_ != end_) {
        return true;
      } else {
        return false;
      }
    }
  }

  void find_next_valid_cursor() {
    if (!high_to_low_flag_) {
      while (cur_ != end_) {
        vid_t v = *cur_;
        auto delete_flag = v >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 1) {
          auto delete_offset_mask =
              (((vid_t) 1) << (sizeof(vid_t) * 8 - 1)) - (vid_t) 1;
          auto delete_offset = v & delete_offset_mask;
          outer_delete_offset_.push(delete_offset);
        } else if (delete_flag == 0) {
          if (outer_delete_offset_.empty()) {
            break;
          }
          if (cur_ - vertex_table_addr_ != outer_delete_offset_.top()) {
            break;
          } else {
            outer_delete_offset_.pop();
          }
        }
        cur_++;
      }
    } else {
      while (cur_ != end_) {
        vid_t v = *cur_;
        auto delete_flag = v >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 1) {
          auto delete_offset_mask =
              (((vid_t) 1) << (sizeof(vid_t) * 8 - 1)) - (vid_t) 1;
          auto delete_offset = v & delete_offset_mask;
          inner_delete_offset_.push(delete_offset);
        }
        if (delete_flag == 0) {
          if (inner_delete_offset_.empty()) {
            break;
          }
          if (cur_ - vertex_table_addr_ != inner_delete_offset_.top()) {
            break;
          } else {
            inner_delete_offset_.pop();
          }
        }
        cur_--;
      }
    }
  }

  vertex_t vertex() {
    vertex_t v;
    v.SetValue(*cur_);
    return v;
  }

  // TODO(wanglei)
  size_t size() { return 0; }

  // TODO(wanglei)
  bool empty() {
    bool flag = false;
    return flag;
  }

 private:
  vid_t* vertex_table_addr_;
  std::vector<std::pair<vid_t*, vid_t*>> addrs_;
  std::vector<bool> high_to_low_flags_;
  vid_t* cur_;
  vid_t* begin_;
  vid_t* end_;
  int loc_;
  bool high_to_low_flag_;
  std::priority_queue<int64_t> inner_delete_offset_;
  std::priority_queue<int64_t, std::vector<int64_t>, std::greater<int64_t>>
      outer_delete_offset_;
  int vlabel_;
  template <typename VID_T, typename VDATA_T>
  friend class GartVertexArray;
};

class EdgeIterator {
 public:
  using EpochBlockHeader = seggraph::EpochBlockHeader;
  using VegitoEdgeBlockHeader = seggraph::VegitoEdgeBlockHeader;
  using VegitoSegmentHeader = seggraph::VegitoSegmentHeader;
  using EdgeLabelBlockHeader = seggraph::EdgeLabelBlockHeader;
  using VegitoEdgeEntry = seggraph::VegitoEdgeEntry;

  EdgeIterator(VegitoSegmentHeader* seg_header,
               VegitoEdgeBlockHeader* edge_block_header,
               EpochBlockHeader* epoch_table_header, char* edge_blob_ptr,
               size_t num_entries, size_t edge_prop_size,
               size_t read_epoch_number, int* prop_offsets,
               char* string_buffer) {
    prop_offsets_ = prop_offsets;
    seg_header_ = seg_header;
    edge_block_header_ = edge_block_header;
    epoch_table_header_ = epoch_table_header;
    num_entries_ = num_entries;
    edge_prop_size_ = edge_prop_size;
    read_epoch_number_ = read_epoch_number;
    string_buffer_ = string_buffer;
    edge_blob_ptr_ = edge_blob_ptr;
    if (edge_block_header && epoch_table_header) {
      init();
      find_next_valid_cursor();
      seg_block_size_ = seg_header->get_block_size();
      edge_prop_offset_ =
          seg_header->get_allocated_edge_num((uintptr_t) edge_block_header_);
    }
  }

  EdgeIterator() = default;

  ~EdgeIterator() {}

 public:
  void init() {
    auto epoch_table_entries = epoch_table_header_->get_entries();
    auto num_epoches = epoch_table_header_->get_num_entries();
    int64_t read_end_offset = -1;
    for (size_t idx = 0; idx < num_epoches; idx++) {
      auto epoch_table_cursor = epoch_table_entries - num_epoches + idx;

      if (read_epoch_number_ >= epoch_table_cursor->get_epoch()) {
        if (idx == 0) {
          entries_ = edge_block_header_->get_entries();
          entries_cursor_ = entries_ - num_entries_;
          return;
        } else {
          auto last_cursor = (epoch_table_cursor - 1);
          read_end_offset = last_cursor->get_offset();
          break;
        }
      }
    }

    if (read_end_offset == -1) {
      edge_block_header_ = nullptr;
      entries_ = nullptr;
      entries_cursor_ = nullptr;
    } else {
      while (edge_block_header_->get_prev_num_entries() >=
             (uint64_t) read_end_offset) {
        edge_block_header_ =
            (VegitoEdgeBlockHeader*) (edge_block_header_->get_prev_pointer() +
                                      edge_blob_ptr_);
      }

      auto offset =
          read_end_offset - edge_block_header_->get_prev_num_entries();
      entries_ = (VegitoEdgeEntry*) (edge_block_header_->get_entries());
      entries_cursor_ = entries_ - offset;
    }
  }

  bool valid() {
    if (unlikely(entries_cursor_ == entries_)) {
      return false;
    } else {
      return true;
    }
  }

  void next() {
    entries_cursor_++;
    find_next_valid_cursor();
  }

  vertex_t neighbor() {
    vertex_t v;
    v.SetValue(entries_cursor_->get_dst());
    return v;
  }

  char* get_data() {
    char* data = (char*) ((uintptr_t) seg_header_ + seg_block_size_ -
                          (edge_prop_offset_ + entries_ - entries_cursor_) *
                              edge_prop_size_);
    return data;
  }

  template <typename EDATA_T>
  EDATA_T get_data(int prop_id) {
    EDATA_T t{};
    get_data_impl(t, prop_id);
    return t;
  }

  template <typename EDATA_T>
  EDATA_T get_data_impl(EDATA_T& t, int prop_id) {
    char* data = (char*) ((uintptr_t) seg_header_ + seg_block_size_ -
                          (edge_prop_offset_ + entries_ - entries_cursor_) *
                              edge_prop_size_);
    if (prop_id == 0) {
      t = *(EDATA_T*) (data);
      return t;
    }

    t = *(EDATA_T*) (data + prop_offsets_[prop_id - 1]);
    return t;
  }

  std::string get_data_impl(std::string& t, int prop_id) {
    char* data = (char*) ((uintptr_t) seg_header_ + seg_block_size_ -
                          (edge_prop_offset_ + entries_ - entries_cursor_) *
                              edge_prop_size_);
    int64_t value;
    if (prop_id == 0) {
      value = *(int64_t*) (data);
    } else {
      value = *(int64_t*) (data + prop_offsets_[prop_id - 1]);
    }
    int64_t str_offset = value >> 16;
    int64_t str_len = value & 0xffff;
    return std::string(string_buffer_ + str_offset, str_len);
  }

  uintptr_t get_edge_property_offset() {
    return (uintptr_t) seg_header_ + seg_block_size_ -
           (edge_prop_offset_ + entries_ - entries_cursor_) * edge_prop_size_;
  }

  void find_next_valid_cursor() {
    if ((entries_cursor_ == nullptr) && (entries_ == nullptr)) {
      return;
    }
    bool is_founded = false;
    while (true) {
      while (entries_cursor_ != entries_) {
        vid_t vid = entries_cursor_->get_dst();
        auto delete_flag = vid >> (sizeof(vid_t) * 8 - 1);
        if (delete_flag == 1) {
          auto delete_offset_mask =
              (((vid_t) 1) << (sizeof(vid_t) * 8 - 1)) - (vid_t) 1;
          auto delete_offset = vid & delete_offset_mask;
          delete_offsets_.push(delete_offset);
        } else if (delete_flag == 0) {
          if (delete_offsets_.empty()) {
            is_founded = true;
            break;
          } else if (delete_offsets_.top() !=
                     ((entries_ - entries_cursor_ - 1) +
                      edge_block_header_->get_prev_num_entries())) {
            is_founded = true;
            break;
          } else {
            delete_offsets_.pop();
          }
        }
        entries_cursor_++;
      }
      if (is_founded) {
        break;
      }
      if (!edge_block_header_->get_prev_pointer()) {
        break;
      }
      edge_block_header_ =
          (VegitoEdgeBlockHeader*) (edge_block_header_->get_prev_pointer() +
                                    edge_blob_ptr_);
      if (!edge_block_header_) {
        break;
      } else {
        auto num_entries = edge_block_header_->get_num_entries();
        entries_ = edge_block_header_->get_entries();
        entries_cursor_ = entries_ - num_entries;  // at the begining
        edge_prop_offset_ =
            seg_header_->get_allocated_edge_num((uintptr_t) edge_block_header_);
      }
    }
  }

  size_t size() {
    // TODO(wanglei): not implemented
    return 0;
  }

  bool empty() {
    // TODO(wanglei): not implemented
    return false;
  }

 private:
  VegitoSegmentHeader* seg_header_;
  VegitoEdgeBlockHeader* edge_block_header_;
  EpochBlockHeader* epoch_table_header_;
  char* edge_blob_ptr_;  // for switch block

  VegitoEdgeEntry* entries_cursor_ = nullptr;
  VegitoEdgeEntry* entries_ = nullptr;

  size_t num_entries_;
  size_t edge_prop_size_;

  int64_t read_epoch_number_;

  std::priority_queue<size_t> delete_offsets_;
  // for edge property
  size_t seg_block_size_;
  size_t edge_prop_offset_;
  int* prop_offsets_;
  char* string_buffer_;
};
}  // namespace gart

#endif  // INTERFACES_FRAGMENT_ITERATOR_H_
