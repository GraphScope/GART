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

#include "interfaces/fragment/gart_fragment.h"
#include "interfaces/fragment/iterator.h"
#include "grape/config.h"
#include "grape/serialization/in_archive.h"
#include "grape/serialization/out_archive.h"
#include "grape/utils/gcontainer.h"
#include "grape/utils/vertex_array.h"

namespace gart {

template <typename VID_T, typename T>
class GartVertexArray : public grape::Array<T, grape::Allocator<T>> {
  using Base = grape::Array<T, grape::Allocator<T>>;

 public:
  GartVertexArray() : Base(), fake_start_(NULL) {}
  explicit GartVertexArray(const gart::VertexIterator& iter)
      : Base(), fake_start_(NULL) {}

  GartVertexArray(const gart::VertexIterator& iter, const T& value)
      : Base(), fake_start_(NULL) {}

  ~GartVertexArray() = default;

  void Init(const gart::GartFragment<VID_T, VID_T>* frag,
            const gart::VertexIterator& iter) {
    frag_ = frag;
    size_t iter_size = iter.addrs_.size();
    size_t total_size = 0;
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

    Base::clear();
    Base::resize(total_size);
    fake_start_ = Base::data();
  }

  void Init(const gart::GartFragment<VID_T, VID_T>* frag,
            const gart::VertexIterator& iter, const T& value) {
    frag_ = frag;
    size_t iter_size = iter.addrs_.size();
    size_t total_size = 0;
    for (size_t idx = 0; idx < iter_size; idx++) {
      if (iter.high_to_low_flags_[idx] == true) {
        total_size += frag_->GetMaxInnerVerticesNum(iter.vlabel_);
        if (iter.addrs_[idx].first == nullptr ||
            iter.addrs_[idx].second == nullptr) {
          inner_size_ = 0;
        } else {
          inner_size_ = iter.addrs_[idx].first - iter.addrs_[idx].second;
        }
      } else {
        total_size += frag->GetMaxOuterVerticesNum(iter.vlabel_);
      }
    }
    Base::clear();
    Base::resize(total_size, value);
    fake_start_ = Base::data();
  }

  void SetValue(gart::VertexIterator& iter, const T& value) {
    LOG(FATAL) << "Not implemented yet!";
  }
  void SetValue(const grape::Vertex<VID_T>& loc, const T& value) {
    auto offset = frag_->GetOffset(loc);
    if (frag_->IsInnerVertex(loc)) {
      fake_start_[offset] = value;
    } else if (frag_->IsOuterVertex(loc)) {
      offset = offset + inner_size_;
      fake_start_[offset] = value;
    }
  }

  void SetValue(const T& value) {
    std::fill_n(Base::data(), Base::size(), value);
  }

  inline T& operator[](const grape::Vertex<VID_T>& loc) {
    auto offset = frag_->GetOffset(loc);
    if (frag_->IsOuterVertex(loc)) {
      offset = offset + inner_size_;
    }
    return fake_start_[offset];
  }
  inline const T& operator[](const grape::Vertex<VID_T>& loc) const {
    auto offset = frag_->GetOffset(loc);
    if (frag_->IsOuterVertex(loc)) {
      offset = offset + inner_size_;
    }
    return fake_start_[offset];
  }

  void Swap(GartVertexArray& rhs) {
    Base::swap((Base&) rhs);
    std::swap(fake_start_, rhs.fake_start_);
  }

  void Clear() {
    Base::clear();
    Base::resize(0);
    fake_start_ = NULL;
  }

  // const gart::VertexIterator& GetVertexRange() const { return iter_; }

 private:
  void Resize() {}

  // gart::VertexIterator iter_ = nullptr;
  T* fake_start_;
  const gart::GartFragment<VID_T, VID_T>* frag_;
  size_t inner_size_ = 0;
};

}  // namespace gart

#endif  // ANALYTICAL_ENGINE_CORE_UTILS_GART_VERTEX_ARRAY_H_
