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

#ifndef GART_APPS_PGX_UTILITY_H_
#define GART_APPS_PGX_UTILITY_H_

#include <stdio.h>

// Non-blocking reads using FILE* handles
int non_blocking_fgets(char* buffer, int size, FILE* file_stream);

// parse the ini file
void init_parse_ini(const char* file_name);

// find the value of the key in the section
// if the section is NULL, find the value of the key in the global section
// the max length of the value is 1024 (MAX_VALUE)
void find_value(char* section, char* key, char* value);

#endif  // GART_APPS_PGX_UTILITY_H_