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

#ifndef APPS_PGX_UTILITY_H_
#define APPS_PGX_UTILITY_H_

#include <stdio.h>

/*
 * Utility functions for basic I/O functions
 */
// Non-blocking reads using FILE* handles
int non_blocking_fgets(char* buffer, int size, FILE* file_stream);

/*
 * Utility functions for parsing ini files
 */
// parse the ini file
void init_parse_ini(const char* file_name);

// find the value of the key in the section
// if the section is NULL, find the value of the key in the global section
// the max length of the value is 1024 (MAX_VALUE)
// return the length of the value, -1 if the key is not found
int find_value(const char* section, const char* key, char* value);

/*
 * Utility functions for NX server information
 */

int init_server_info(FILE* file);

int get_next_server_id();

// add server information to the file
// return the server_id of the server
int add_server_info(FILE* file, const char* hostname, int port, int read_epoch);

// find the server information in the file
// return 0 if the server_id is not found, 1 if the server_id is found
int get_server_info(FILE* file, int server_id, char* hostname, int* port,
                    int* read_epoch);

// delete the server information in the file
// return 0 if the server_id is not found, 1 if the server_id is found
int delete_server_info(FILE* file, int server_id);

#endif  // APPS_PGX_UTILITY_H_
