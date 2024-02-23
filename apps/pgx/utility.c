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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "postgres.h"
#include "utils/elog.h"

#define BUFFER_SIZE 1024

// This function reads from the file descriptor associated with file_stream
// into buffer without blocking. It reads up to size-1 characters or until a
// newline is encountered. It will return -1 on error, 0 on timeout, and the
// number of bytes read on success (excluding the terminating null byte).
int non_blocking_fgets(char* buffer, int size, FILE* file_stream) {
  const int TIMEOUT_SEC = 5;  // 5-second timeout
  struct timeval tv;
  int ret = -1;

  int filedes =
      fileno(file_stream);  // Get the file descriptor from the file stream

  // Initialize the file descriptor set
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(filedes, &fds);

  // Set the timeout duration
  tv.tv_sec = TIMEOUT_SEC;
  tv.tv_usec = 0;

  // Check if the file descriptor is ready for reading
  ret = select(filedes + 1, &fds, NULL, NULL, &tv);
  if (ret == -1) {
    const char* error_message = strerror(errno);
    elog(WARNING, "[non_blocking_fgets] select() failed: %s", error_message);
    return -1;
  } else if (ret == 0) {
    // Timeout: no data available after waiting for `TIMEOUT_SEC` seconds
    // printf("Timeout occurred! No data after %d seconds.\n", TIMEOUT_SEC);
    return 0;
  } else {
    if (FD_ISSET(filedes, &fds)) {
      // The file descriptor is ready for reading, perform a read operation
      ssize_t read_bytes = read(filedes, buffer, size - 1);
      if (read_bytes > 0) {
        char* newline;

        buffer[read_bytes] = '\0';  // Null-terminate the string

        // Search for a newline character and replace it with a null byte
        newline = strchr(buffer, '\n');
        if (newline)
          *newline = '\0';

        return read_bytes;  // Return the number of bytes read
      } else if (read_bytes == 0) {
        // End of file reached
        return 0;
      } else {
        // An error occurred during read
        const char* error_message = strerror(errno);
        elog(ERROR, "[non_blocking_fgets] read() failed: %s", error_message);
        return -1;
      }
    }
  }

  return -1;  // This should not be reached
}

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
    elog(ERROR, "[parse_ini_file] Error opening file: %s", file_name);
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

int find_value(const char* section, const char* key, char* value) {
  for (int i = 0; i < num_section; i++) {
    if (strcmp(section, sections[i]) == 0) {
      for (int j = section_heads[i]; j < section_heads[i + 1]; j++) {
        if (strcmp(key, keys[j]) == 0) {
          strcpy(value, values[j]);
          return strlen(values[j]);
        }
      }
    }
  }
  return -1;
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
