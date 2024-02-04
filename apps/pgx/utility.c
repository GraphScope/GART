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

#include "utility.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_SECTION 50
#define MAX_KEY 50
#define MAX_VALUE 1024

// Remove whitespace characters at the beginning and end of a string
static void trim(char* str) {
  char *start, *end;

  // Trim leading space
  for (start = str; *start != '\0' && isspace((unsigned char) *start);
       start++) {}

  // All spaces?
  if (*start == '\0') {
    *str = '\0';
    return;
  }

  // Trim trailing space
  for (end = start + strlen(start) - 1;
       end > start && isspace((unsigned char) *end); end--) {}

  // Write new null terminator character
  end[1] = '\0';

  // Move trimmed string to the beginning of str
  memmove(str, start, end - start + 2);
}

// Parsing key-value pairs in INI files
static int parse_line(char* line, char* section, char* key, char* value) {
  char* start = line;
  char* end;
  char* equal_sign;

  // Skip empty lines and comment lines
  if (line[0] == ';' || line[0] == '#' || line[0] == '\n') {
    return 0;
  }

  // Checks if it's a section
  if (line[0] == '[') {
    end = strchr(line, ']');
    if (end != NULL) {
      *end = '\0';  // Modify the string directly to remove the ']'.
      strncpy(section, start + 1, MAX_SECTION);
      trim(section);  // Remove possible leading and trailing spaces
      return 2;
    }
  }

  // Parsing key-value pairs
  equal_sign = strchr(line, '=');
  if (equal_sign != NULL) {
    *equal_sign = '\0';
    strncpy(key, start, MAX_KEY);
    trim(key);  // Remove possible leading and trailing spaces
    strncpy(value, equal_sign + 1, MAX_VALUE);
    trim(value);  // Remove possible leading and trailing spaces
    return 1;
  }

  // Unparsable formats
  return -1;
}

static int parse_ini_file(const char* file_name, char sections[][MAX_SECTION],
                          char keys[][MAX_KEY], char values[][MAX_VALUE],
                          int section_heads[], int* num_section, int* num_key) {
  FILE* file = fopen(file_name, "r");
  char line[MAX_LINE_LENGTH];
  char section[MAX_SECTION] = {0};
  char key[MAX_KEY];
  char value[MAX_VALUE];
  char last_section[MAX_SECTION] = {
      0};  // Used to save the name of the previous section
  *num_section = 0;
  *num_key = 0;
  section_heads[*num_section] = *num_key;

  if (file == NULL) {
    perror("Error opening file");
    return 1;
  }

  while (fgets(line, sizeof(line), file)) {
    switch (parse_line(line, section, key, value)) {
    case 1:
      // Handling key-value pairs
      if (strcmp(last_section, section) != 0) {
        // printf("num_section %d, Section: [%s]\n", *num_section, section);
        strcpy(last_section, section);
        strncpy(sections[*num_section], section, MAX_SECTION);
        section_heads[*num_section] = *num_key;
        (*num_section)++;
      }
      strcpy(keys[*num_key], key);
      strncpy(values[*num_key], value, MAX_VALUE);
      // printf("num_key: %d, Key = %s, Value = %s\n", *num_key, key, value);
      (*num_key)++;
      break;
    case 2:
      // Handling the start of a new section
      break;
    case -1:
      // formatting error
      fprintf(stderr, "Parsing error: invalid line:\n%s\n", line);
      break;
    }
  }

  section_heads[*num_section] = *num_key;

  fclose(file);
  return 0;
}

static int section_heads[MAX_LINE_LENGTH] = {0};
static char sections[MAX_LINE_LENGTH][MAX_SECTION] = {0};
static char keys[MAX_LINE_LENGTH][MAX_KEY] = {0};
static char values[MAX_LINE_LENGTH][MAX_VALUE] = {0};

static int num_section = 0;
static int num_key = 0;

void init_parse_ini(const char* file_name) {
  parse_ini_file(file_name, sections, keys, values, section_heads, &num_section,
                 &num_key);

#if 0  // debug print

  for (int i = 0; i < num_section; i++) {
    printf("Section: [%s]\n", sections[i]);
    for (int j = section_heads[i]; j < section_heads[i + 1]; j++) {
      printf("Key = %s, Value = %s\n", keys[j], values[j]);
    }
  }

#endif
}

void find_value(char* section, char* key, char* value) {
  for (int i = 0; i < num_section; i++) {
    if (strcmp(section, sections[i]) == 0) {
      for (int j = section_heads[i]; j < section_heads[i + 1]; j++) {
        if (strcmp(key, keys[j]) == 0) {
          strcpy(value, values[j]);
          return;
        }
      }
    }
  }
  value = NULL;
}

#if 0  // test
int main() {
  char* file_name =
      "/opt/ssj/projects/gart/apps/pgx/gart-pgx-config-template.ini";
  init_parse_ini(file_name);
  char value[MAX_VALUE];
  find_value("path", "KAFKA_HOME", value);
  printf("KAFKA_HOME = %s\n", value);
  find_value("path", "GART_HOME", value);
  printf("GART_HOME = %s\n", value);
  find_value("log", "log_path", value);
  printf("log_path = %s\n", value);
  return 0;
}
#endif