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
#include "utils/elog.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

Datum gart_set_config(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_set_config);

Datum gart_get_connection(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_get_connection);

Datum gart_release_connection(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_release_connection);

Datum gart_define_graph(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_define_graph);

Datum gart_define_graph_by_sql(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_define_graph_by_sql);

Datum gart_define_graph_by_yaml(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_define_graph_by_yaml);

Datum gart_get_lastest_epoch(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_get_lastest_epoch);

static int nested_level = 0;
static void write_file(const char* str);
static char* read_file(FILE* fp);

Datum pg_all_queries(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_all_queries);

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

// config file and its content
static int config_inited = 0;
static char config_file_name[512] = {0};
static char config_log_file_name[128];
static char config_kafka_home[256];
static char config_gart_home[256];
static char config_gart_yaml_path[256];
static char config_etcd_endpoints[256];
static char config_etcd_prefix[64];
static int config_subgraph_num;

// handler for log file
static FILE* log_file = NULL;

static inline void safe_find_value(const char* section, const char* key,
                                   char* value) {
  if (find_value(section, key, value) < 0) {
    elog(WARNING, "Cannot find [%s, %s] in config file: %s", section, key,
         config_file_name);
  } else {
    elog(INFO, "[%s, %s]: %s", section, key, value);
  }
}

Datum gart_set_config(PG_FUNCTION_ARGS) {
  char result[1024];
  text* config_file_name_text;
  char config_buffer[256];

  config_file_name_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(config_file_name_text, config_file_name);
  init_parse_ini(config_file_name);

  // prepare log file
  safe_find_value("log", "log_path", config_log_file_name);

  // open file for writing logs
  if (log_file) {
    fclose(log_file);
  }
  log_file = fopen(config_log_file_name, "w");
  if (log_file == NULL) {
    elog(ERROR, "Cannot open log file: %s", config_log_file_name);
    return (Datum) 0;
  }

  // set config values
  safe_find_value("path", "KAFKA_HOME", config_kafka_home);

  safe_find_value("path", "GART_HOME", config_gart_home);

  safe_find_value("gart", "rgmapping-file", config_gart_yaml_path);

  safe_find_value("gart", "etcd-endpoints", config_etcd_endpoints);

  safe_find_value("gart", "etcd-prefix", config_etcd_prefix);

  safe_find_value("gart", "subgraph-num", config_buffer);
  config_subgraph_num = atoi(config_buffer);

  config_inited = 1;

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

  char log_line[1024];
  char cmd[1024];

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
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
    elog(ERROR, "Cannot open config file: %s.", config_file_name);
    return (Datum) 0;
  }

  sprintf(cmd, "sh %s/apps/pgx/run.sh -c %s -u %s -p %s -b %s",
          config_gart_home, config_file_name, username, password, databasename);
  elog(INFO, "Command: %s", cmd);

  fflush(log_file);

  // execute command
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    return (Datum) 0;
  }

  // output to logs line by line
  while (1) {
    int char_written;
    int read_stat;

    CHECK_FOR_INTERRUPTS();

    read_stat = non_blocking_fgets(log_line, sizeof(log_line), fp);

    if (strstr(log_line, "GART started completely.")) {
      sprintf(result, "GART started completely!\n");
      break;
    }

    if (strstr(log_line, "GART stopped abnormally.")) {
      elog(FATAL, "GART exit abnormally!");
      pclose(fp);
      return (Datum) 0;
    }

    if (read_stat == -1) {
      // elog(WARNING, "error read status!");
      continue;
    } else if (read_stat == 0) {
      // timemout
      continue;
    }

    char_written = fprintf(log_file, "%s\n", log_line);
    fflush(log_file);
    // sprintf(result, "%s\n%d: %s", result, char_written, log_line);
    if (char_written < 0) {
      elog(ERROR, "Cannot write log file: %s", config_log_file_name);
      pclose(fp);
      return (Datum) 0;
    }

    fflush(log_file);
  }

  pclose(fp);

  fflush(log_file);
  // fclose(log_file);

  elog(INFO, "gart_get_connection completely: %s", result);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_release_connection(PG_FUNCTION_ARGS) {
  char result[512] = "Release connection successfully!\n";
  FILE* fp;
  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  sprintf(cmd, "sh %s/apps/pgx/run.sh -c %s --stop", config_gart_home,
          config_file_name);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    return (Datum) 0;
  }

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    fprintf(log_file, "%s", log_line);
    fflush(log_file);
    is_read = 1;
  }

  if (!is_read) {
    elog(ERROR, "Cannot read from command: %s", cmd);
    pclose(fp);
    return (Datum) 0;
  }

  pclose(fp);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_define_graph(PG_FUNCTION_ARGS) {
  char result[512] = "Build graph successfully!\n";

  FILE *output_yaml, *fp;
  char sql_str[4098];
  text* sql_text;

  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  sql_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(sql_text, sql_str);
  for (int i = 0; i < strlen(sql_str); ++i) {
    if (sql_str[i] == '\n') {
      sql_str[i] = ' ';
    }
  }

  output_yaml = fopen(config_gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    elog(ERROR, "Cannot open output YAML file: %s.", config_gart_yaml_path);
    return (Datum) 0;
  }

  // Use JAVA converter to convert SQL to YAML
  sprintf(cmd, "(cd %s/pgql/; sh run.sh sql2yaml_str '%s' %s)",
          config_gart_home, sql_str, config_gart_yaml_path);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    fclose(output_yaml);
    return (Datum) 0;
  }

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    fprintf(log_file, "%s", log_line);
    fflush(log_file);
    is_read = 1;
  }

  if (!is_read) {
    elog(ERROR, "Cannot read from command: %s", cmd);
    fclose(output_yaml);
    pclose(fp);
    return (Datum) 0;
  }

  fclose(output_yaml);
  pclose(fp);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_define_graph_by_sql(PG_FUNCTION_ARGS) {
  char result[512] = "Build graph by SQL successfully!\n";

  FILE *input_sql, *output_yaml, *fp;
  char input_sql_path[1024];
  text* sql_text;

  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  sql_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(sql_text, input_sql_path);

  input_sql = fopen(input_sql_path, "rb");
  if (input_sql == NULL) {
    elog(ERROR, "Cannot open input SQL file: %s.", input_sql_path);
    return (Datum) 0;
  }

  output_yaml = fopen(config_gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    elog(ERROR, "Cannot open output YAML file: %s.", config_gart_yaml_path);
    fclose(input_sql);
    return (Datum) 0;
  }

  // Use JAVA converter to convert SQL to YAML
  sprintf(cmd, "(cd %s/pgql/; sh run.sh sql2yaml %s %s)", config_gart_home,
          input_sql_path, config_gart_yaml_path);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    fclose(input_sql);
    fclose(output_yaml);
    return (Datum) 0;
  }

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    fprintf(log_file, "%s", log_line);
    fflush(log_file);
    is_read = 1;
  }

  if (!is_read) {
    elog(ERROR, "Cannot read from command: %s", cmd);
    fclose(input_sql);
    fclose(output_yaml);
    pclose(fp);
    return (Datum) 0;
  }

  fclose(input_sql);
  fclose(output_yaml);
  pclose(fp);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_define_graph_by_yaml(PG_FUNCTION_ARGS) {
  char result[512] = "Build graph by YAML successfully!\n";

  FILE *input_yaml, *output_yaml;
  char input_yaml_path[1024];
  text* yaml_text;

  char log_line[1024];
  size_t bytes_read;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  yaml_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(yaml_text, input_yaml_path);

  input_yaml = fopen(input_yaml_path, "rb");
  if (input_yaml == NULL) {
    elog(ERROR, "Cannot open input YAML file: %s.", input_yaml_path);
    return (Datum) 0;
  }

  output_yaml = fopen(config_gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    elog(ERROR, "Cannot open output YAML file: %s.", config_gart_yaml_path);
    fclose(input_yaml);
    return (Datum) 0;
  }

  // copy file
  while ((bytes_read = fread(log_line, 1, sizeof(log_line), input_yaml)) > 0) {
    fwrite(log_line, 1, bytes_read, output_yaml);
  }

  fclose(input_yaml);
  fclose(output_yaml);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}

Datum gart_get_lastest_epoch(PG_FUNCTION_ARGS) {
  char result[512];
  char etcd_key[256];
  char cmd[1024];
  char log_line[1024];

  int found = 0;
  int epoch;

  FILE* fp;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  // TODO: get latest epoch from etcd using partition id = 0
  sprintf(etcd_key, "%sgart_latest_epoch_p%d", config_etcd_prefix, 0);
  sprintf(cmd, "etcdctl --endpoints=%s get --prefix %s", config_etcd_endpoints,
          etcd_key);

  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    return (Datum) 0;
  }

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    fprintf(log_file, "%s", log_line);
    fflush(log_file);

    if (found) {
      epoch = atoi(log_line);
      found = 0;
      break;
    }
    if (strstr(log_line, etcd_key)) {
      found = 1;
    }
  }

  sprintf(result, "%d", epoch);

  PG_RETURN_TEXT_P(cstring_to_text(result));
}