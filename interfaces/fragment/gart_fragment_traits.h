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

#ifndef INTERFACES_FRAGMENT_GART_FRAGMENT_TRAITS_H_
#define INTERFACES_FRAGMENT_GART_FRAGMENT_TRAITS_H_

#include "interfaces/fragment/gart_fragment.h"

namespace gart {
template <typename OID_T, typename VID_T>
class GartFragment;
}  // namespace gart

namespace vineyard {
template <typename OID_T, typename VID_T>
struct is_property_fragment<gart::GartFragment<OID_T, VID_T>> {
  using type = std::true_type;
  static constexpr bool value = true;
};
}  // namespace vineyard

#endif  // INTERFACES_FRAGMENT_GART_FRAGMENT_TRAITS_H_
