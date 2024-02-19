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

#include "postgres.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands/dbcommands.h"
#include "executor/executor.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "tcop/utility.h"
#include "utility.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static int nested_level = 0;
static void write_file(const char* str);
static char* read_file(FILE* fp);

Datum pg_all_queries(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_all_queries);

Datum gart_set_config(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_set_config);

Datum gart_get_connection(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_get_connection);

Datum gart_define_graph(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_define_graph);

Datum gart_define_graph_by_sql(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_define_graph_by_sql);

Datum gart_define_graph_by_yaml(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_define_graph_by_yaml);

static void process_utility(PlannedStmt* pstmt, const char* queryString,
                            ProcessUtilityContext context, ParamListInfo params,
                            QueryEnvironment* queryEnv, DestReceiver* dest,
                            char* completionTag);

static void executor_run(QueryDesc* queryDesc, ScanDirection direction,
                         uint64 count, bool execute_once);

void _PG_init(void) {
  ProcessUtility_hook = process_utility;
  ExecutorRun_hook = executor_run;
}

void _PG_fini(void) {}

static void write_file(const char* str) {
  FILE* fp = fopen("/tmp/log.stat", "a+");
  if (fp == NULL)
    elog(ERROR, "log: unable to open log file");
  fputs(str, fp);
  fputs("\n", fp);
  fclose(fp);
}

static char* read_file(FILE* fp) {
  static char query[1024];
  char* rc = NULL;

  rc = fgets(query, 1023, fp);
  return rc ? query : NULL;
}

static void process_utility(PlannedStmt* pstmt, const char* queryString,
                            ProcessUtilityContext context, ParamListInfo params,
                            QueryEnvironment* queryEnv, DestReceiver* dest,
                            char* completionTag) {
  nested_level++;
  PG_TRY();
  {
    standard_ProcessUtility(pstmt, queryString, context, params, queryEnv, dest,
                            completionTag);
    if (queryString)
      write_file(queryString);
    nested_level--;
  }
  PG_CATCH();
  {
    nested_level--;
    PG_RE_THROW();
  }
  PG_END_TRY();
}

static void executor_run(QueryDesc* queryDesc, ScanDirection direction,
                         uint64 count, bool execute_once) {
  nested_level++;
  PG_TRY();
  {
    standard_ExecutorRun(queryDesc, direction, count, execute_once);
    if (queryDesc->sourceText)
      write_file(queryDesc->sourceText);
    nested_level--;
  }
  PG_CATCH();
  {
    nested_level--;
    PG_RE_THROW();
  }
  PG_END_TRY();
}

Datum pg_all_queries(PG_FUNCTION_ARGS) {
  ReturnSetInfo* rsinfo = (ReturnSetInfo*) fcinfo->resultinfo;
  TupleDesc tupdesc;
  Tuplestorestate* tupstore;
  MemoryContext per_query_ctx;
  MemoryContext oldcontext;
  Datum values[2];
  bool nulls[2] = {0};
  char pid[25];
  char* query;
  FILE* fp;

  per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  oldcontext = MemoryContextSwitchTo(per_query_ctx);
  tupstore = tuplestore_begin_heap(true, false, work_mem);

  fp = fopen("/tmp/log.stat", "r");
  if (fp == NULL) {
    elog(WARNING, "log: unable to open log file");
    query = "no more queries";
    sprintf(pid, "%s", "invalid pid");
  } else {
    sprintf(pid, "%d", (int) getpid());
    query = read_file(fp);
  }
  while (query) {
    values[0] = CStringGetTextDatum(query);
    values[1] = CStringGetTextDatum(pid);
    /* Build a tuple descriptor for our result type */
    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
      elog(ERROR, "return type must be a row type");

    rsinfo->returnMode = SFRM_Materialize;
    rsinfo->setResult = tupstore;
    rsinfo->setDesc = tupdesc;
    tuplestore_putvalues(tupstore, tupdesc, values, nulls);

    if (fp == NULL)
      break;
    query = read_file(fp);
    if (query == NULL)
      break;
  }
  if (fp)
    fclose(fp);
  tuplestore_donestoring(tupstore);
  MemoryContextSwitchTo(oldcontext);
  return (Datum) 0;
}

static inline void safe_text_to_cstring(text* src, char dst[]) {
  strncpy(dst, VARDATA_ANY(src), VARSIZE_ANY_EXHDR(src));
  dst[VARSIZE_ANY_EXHDR(src)] = '\0';
}

char config_file_name[512] = {0};
FILE* log_file;
char log_file_name[128];
char log_line[1024];

Datum gart_set_config(PG_FUNCTION_ARGS) {
  char result[1024];
  text* config_file_name_text;

  config_file_name_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(config_file_name_text, config_file_name);
  init_parse_ini(config_file_name);

  // prepare log file
  find_value("log", "log_path", log_file_name);

  // open file for writing logs
  log_file = fopen(log_file_name, "w");
  if (log_file == NULL) {
    sprintf(result, "Cannot open log file: %s\n", log_file_name);
    pclose(log_file);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  sprintf(result, "Set config file name: %s", config_file_name);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_get_connection(PG_FUNCTION_ARGS) {
  char result[2048];

  Oid userid;
  char* username;
  Oid databaseid;
  char* databasename;

  text* password_text;
  char password[512];

  FILE* fp;

  char cmd[1024];
  char value_buf[1024];

  int timeout_count = 0;
  const int MAX_TIMEOUT = 8;

  if (strlen(config_file_name) == 0) {
    sprintf(result, "Config file name is not set.\n");
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  // get username and database name from Postgres
  userid = GetUserId();
  username = GetUserNameFromId(userid, false);

  databaseid = MyDatabaseId;
  databasename = get_database_name(databaseid);

  // parse arguments
  password_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(password_text, password);

  // parse ini file
  if (fopen(config_file_name, "r") == NULL) {
    sprintf(result, "Cannot open config file: %s.\n", config_file_name);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  find_value("path", "KAFKA_HOME", value_buf);
  fprintf(log_file, "C KAFKA_HOME = %s\n", value_buf);
  find_value("path", "GART_HOME", value_buf);
  fprintf(log_file, "C GART_HOME = %s\n", value_buf);

  sprintf(cmd, "sh %s/apps/pgx/run.sh %s %s %s", value_buf, username, password,
          databasename);
  fprintf(log_file, "Command: %s\n", cmd);

  fflush(log_file);

  // execute command
  fp = popen(cmd, "r");
  if (fp == NULL) {
    fprintf(stderr, "Execute command error: %s\n", cmd);
    exit(1);
  }

  // output to logs line by line
  while (1) {
    int char_written;
    int read_stat;

    CHECK_FOR_INTERRUPTS();

    read_stat = non_blocking_fgets(log_line, sizeof(log_line), fp);
    if (strcmp(log_line, "GART started completely") == 0) {
      fprintf(log_file, "Script Complete\n");
      fflush(log_file);
      break;
    }
    if (read_stat == -1) {
      fprintf(log_file, "error status!\n");
      fflush(log_file);
      continue;
    } else if (read_stat == 0) {
      // fprintf(log_file, "timeout!\n");
      // fflush(log_file);
      ++timeout_count;
      // if (timeout_count > MAX_TIMEOUT) {
      //   fprintf(log_file, "timeout count exceeded!\n");
      //   fflush(log_file);
      //   break;
      // }
      continue;
    }

    char_written = fprintf(log_file, "%s\n", log_line);
    fflush(log_file);
    sprintf(result, "%s\n%d: %s", result, char_written, log_line);
    if (char_written < 0) {
      sprintf(result, "Cannot write log file: %s\n", log_file_name);
      pclose(fp);
      PG_RETURN_TEXT_P(cstring_to_text(result));
    }

    fflush(log_file);
  }

  pclose(fp);

  fprintf(log_file, "End the main loop!\n");
  fflush(log_file);
  fclose(log_file);

  elog(INFO, "GART started completely: %s\n", result);

  PG_RETURN_TEXT_P(cstring_to_text("GART started completely!"));
}

Datum gart_define_graph(PG_FUNCTION_ARGS) {
  char result[512] = "Build graph successfully!\n";
  char gart_yaml_path[1024];

  FILE *output_yaml, *fp;
  char sql_str[4098];
  text* sql_text;

  char gart_home_buffer[512];
  char cmd[1024];
  char buffer[1024];
  int is_read = 0;

  if (strlen(config_file_name) == 0) {
    sprintf(result, "Config file name is not set.\n");
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  sql_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(sql_text, sql_str);
  for (int i = 0; i < strlen(sql_str); ++i) {
    if (sql_str[i] == '\n') {
      sql_str[i] = ' ';
    }
  }

  find_value("gart", "rgmapping-file", gart_yaml_path);
  output_yaml = fopen(gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    sprintf(result, "Cannot open output YAML file: %s.\n", gart_yaml_path);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  find_value("path", "GART_HOME", gart_home_buffer);

  // Use JAVA converter to convert SQL to YAML
  sprintf(cmd, "(cd %s/pgql/; sh run.sh sql2yaml_str '%s' %s)",
          gart_home_buffer, sql_str, gart_yaml_path);
  fprintf(log_file, "Command: %s\n", cmd);
  fflush(log_file);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    sprintf(result, "Cannot execute command: %s\n", cmd);
    fclose(output_yaml);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    fprintf(log_file, "%s", buffer);
    fflush(log_file);
    is_read = 1;
  }

  if (!is_read) {
    sprintf(result, "Cannot read from command: %s\n", cmd);
    fclose(output_yaml);
    pclose(fp);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  pclose(fp);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_define_graph_by_sql(PG_FUNCTION_ARGS) {
  char result[512] = "Build graph by SQL successfully!\n";
  char gart_yaml_path[1024];

  FILE *input_sql, *output_yaml, *fp;
  char input_sql_path[1024];
  text* sql_text;

  char gart_home_buffer[512];
  char cmd[1024];
  char buffer[1024];
  int is_read = 0;

  if (strlen(config_file_name) == 0) {
    sprintf(result, "Config file name is not set.\n");
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  sql_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(sql_text, input_sql_path);

  input_sql = fopen(input_sql_path, "rb");
  if (input_sql == NULL) {
    sprintf(result, "Cannot open input SQL file: %s.\n", input_sql_path);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  find_value("gart", "rgmapping-file", gart_yaml_path);
  output_yaml = fopen(gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    sprintf(result, "Cannot open output YAML file: %s.\n", gart_yaml_path);
    fclose(input_sql);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  find_value("path", "GART_HOME", gart_home_buffer);

  // Use JAVA converter to convert SQL to YAML
  sprintf(cmd, "(cd %s/pgql/; sh run.sh sql2yaml %s %s)", gart_home_buffer,
          input_sql_path, gart_yaml_path);
  fprintf(log_file, "Command: %s\n", cmd);
  fflush(log_file);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    sprintf(result, "Cannot execute command: %s\n", cmd);
    fclose(input_sql);
    fclose(output_yaml);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    fprintf(log_file, "%s", buffer);
    fflush(log_file);
    is_read = 1;
  }

  if (!is_read) {
    sprintf(result, "Cannot read from command: %s\n", cmd);
    fclose(input_sql);
    fclose(output_yaml);
    pclose(fp);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  pclose(fp);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_define_graph_by_yaml(PG_FUNCTION_ARGS) {
  char result[512] = "Build graph by YAML successfully!\n";
  char gart_yaml_path[1024];

  FILE *input_yaml, *output_yaml;
  char input_yaml_path[1024];
  text* yaml_text;

  char buffer[1024];
  size_t bytes_read;

  if (strlen(config_file_name) == 0) {
    sprintf(result, "Config file name is not set.\n");
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  yaml_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(yaml_text, input_yaml_path);

  input_yaml = fopen(input_yaml_path, "rb");
  if (input_yaml == NULL) {
    sprintf(result, "Cannot open input YAML file: %s.\n", input_yaml_path);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  find_value("gart", "rgmapping-file", gart_yaml_path);
  output_yaml = fopen(gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    sprintf(result, "Cannot open output YAML file: %s.\n", gart_yaml_path);
    fclose(input_yaml);
    PG_RETURN_TEXT_P(cstring_to_text(result));
  }

  // copy file
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_yaml)) > 0) {
    fwrite(buffer, 1, bytes_read, output_yaml);
  }

  fclose(input_yaml);
  fclose(output_yaml);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}
