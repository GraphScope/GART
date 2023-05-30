/** Copyright 2020 Alibaba Group Holding Limited.

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

#include <memory>
#include <string>

#include "grin/src/predefine.h"

std::string GetDataTypeName(GRIN_DATATYPE type) {
  switch (type) {
    case GRIN_DATATYPE::Int32:
      return "int32";
    case GRIN_DATATYPE::UInt32:
      return "uint32";
    case GRIN_DATATYPE::Int64:
      return "int64";
    case GRIN_DATATYPE::UInt64:
      return "uint64";
    case GRIN_DATATYPE::Float:
      return "float";
    case GRIN_DATATYPE::Double:
      return "double";
    case GRIN_DATATYPE::String:
      return "string";
    // TODO(wanglei): support date types
    default:
      return "undefined";
  }
}

GRIN_DATATYPE StringToDataType(std::string dtype_str) {
  if (dtype_str == "INT") {
    return GRIN_DATATYPE::Int32;
  } else if (dtype_str == "DOUBLE") { 
    return GRIN_DATATYPE::Double;
  } else if (dtype_str == "FLOAT") {
    return GRIN_DATATYPE::Float;
  } else if (dtype_str == "LONG") {
    return GRIN_DATATYPE::UInt64;
  } else if (dtype_str == "STRING") {
    return GRIN_DATATYPE::String;
  } else if (dtype_str == "TEXT") {
    return GRIN_DATATYPE::String;
  } else if (dtype_str == "LONGSTRING") {
    return GRIN_DATATYPE::String;
  } else if (dtype_str == "DATE") {
    return GRIN_DATATYPE::String;
  } else if (dtype_str == "DATETIME") {
    return GRIN_DATATYPE::String;
  } else {
    GRIN_DATATYPE::Undefined;
  }
}

unsigned _grin_get_type_from_property(unsigned long long int prop) {
    return (unsigned)(prop >> 32);
}

unsigned _grin_get_prop_from_property(unsigned long long int prop) {
    return (unsigned)(prop & 0xffffffff);
}

unsigned long long int _grin_create_property(unsigned type, unsigned prop) {
    return ((unsigned long long int)type << 32) | prop;
}