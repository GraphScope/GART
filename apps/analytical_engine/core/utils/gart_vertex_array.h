/** Copyright 2020-2023 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef ANALYTICAL_ENGINE_CORE_UTILS_GART_VERTEX_ARRAY_H_
#define ANALYTICAL_ENGINE_CORE_UTILS_GART_VERTEX_ARRAY_H_

#include <algorithm>
#include <utility>

#include "grape/config.h"
#include "grape/serialization/in_archive.h"
#include "grape/serialization/out_archive.h"
#include "interfaces/fragment/gart_fragment.h"
#include "interfaces/fragment/iterator.h"

namespace gart {

template <typename VID_T, typename T>
class GartVertexArray {
 public:
  GartVertexArray() {}
  explicit GartVertexArray(const gart::VertexIterator& iter) {}

  GartVertexArray(const gart::VertexIterator& iter, const T& value) {}

  ~GartVertexArray() = default;

  void Init(const gart::GartFragment<VID_T, VID_T>* frag,
            const gart::VertexIterator& iter) {
    frag_ = frag;
    size_t total_size = 0;
    size_t iter_size = iter.addrs_.size();
    for (size_t idx = 0; idx < iter_size; idx++) {
      if (iter.high_to_low_flags_[idx] == true) {
        total_size += frag_->GetMaxInnerVerticesNum(iter.vlabel_);
        if (iter.addrs_[idx].first == nullptr || iter.addrs_[idx].second) {
          inner_size_ = 0;
        } else {
          inner_size_ = iter.addrs_[idx].first - iter.addrs_[idx].second;
        }
      } else {
        total_size += frag_->GetMaxOuterVerticesNum(iter.vlabel_);
      }
    }

    data_.resize(total_size);
  }

  void Init(const gart::GartFragment<VID_T, VID_T>* frag,
            const gart::VertexIterator& iter, const T& value) {
    frag_ = frag;
    size_t total_size = 0;
    size_t iter_size = iter.addrs_.size();
    for (size_t idx = 0; idx < iter_size; idx++) {
      if (iter.high_to_low_flags_[idx] == true) {
        total_size += frag_->GetMaxInnerVerticesNum(iter.vlabel_);
        if (iter.addrs_[idx].first == nullptr || iter.addrs_[idx].second) {
          inner_size_ = 0;
        } else {
          inner_size_ = iter.addrs_[idx].first - iter.addrs_[idx].second;
        }
      } else {
        total_size += frag_->GetMaxOuterVerticesNum(iter.vlabel_);
      }
    }

    data_.assign(total_size, value);
  }

  void SetValue(gart::VertexIterator& iter, const T& value) {
    LOG(FATAL) << "Not implemented yet!";
  }
  void SetValue(const grape::Vertex<VID_T>& loc, const T& value) {
    auto offset = frag_->GetOffset(loc);
    if (frag_->IsInnerVertex(loc)) {
      data_[offset] = value;
    } else if (frag_->IsOuterVertex(loc)) {
      offset = offset + inner_size_;
      data_[offset] = value;
    }
  }

  void SetValue(const T& value) { data_.assign(data_.size(), value); }

  inline T& operator[](const grape::Vertex<VID_T>& loc) {
    auto offset = frag_->GetOffset(loc);
    if (frag_->IsOuterVertex(loc)) {
      offset = offset + inner_size_;
    }

    if (unlikely(offset >= data_.size())) {
      LOG(ERROR) << "offset: " << offset << " data size: " << data_.size()
                 << " loc: " << loc.GetValue();
    }
    return data_[offset];
  }
  inline const T& operator[](const grape::Vertex<VID_T>& loc) const {
    auto offset = frag_->GetOffset(loc);
    if (frag_->IsOuterVertex(loc)) {
      offset = offset + inner_size_;
    }

    if (unlikely(offset >= data_.size())) {
      LOG(ERROR) << "offset: " << offset << " data size: " << data_.size()
                 << "loc: " << loc.GetValue();
    }
    return data_[offset];
  }

  void Swap(GartVertexArray& rhs) {
    data_.swap(rhs.data_);
    std::swap(frag_, rhs.frag_);
    std::swap(inner_size_, rhs.inner_size_);
  }

  void Clear() {
    data_.clear();
    frag_ = nullptr;
    inner_size_ = 0;
  }

 private:
  void Resize() {}

  std::vector<T> data_;
  const gart::GartFragment<VID_T, VID_T>* frag_;
  size_t inner_size_ = 0;
};

}  // namespace gart

#endif  // ANALYTICAL_ENGINE_CORE_UTILS_GART_VERTEX_ARRAY_H_
