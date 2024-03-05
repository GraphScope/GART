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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "postgres.h"

#include "catalog/pg_type.h"
#include "commands/dbcommands.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/resowner.h"

#include "utility.h"

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

Datum gart_launch_graph_server(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_launch_graph_server);

Datum gart_stop_graph_server(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_stop_graph_server);

Datum gart_run_networkx_app(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_run_networkx_app);

Datum gart_show_graph_server_info(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_show_graph_server_info);

Datum gart_run_sssp(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(gart_run_sssp);

static void my_resource_cleanup(ResourceReleasePhase phase, bool isCommit,
                                bool isTopLevel, void* arg);

void _PG_init(void) {
  // TODO(ssj): need to release resource when session exit
  // RegisterResourceReleaseCallback(my_resource_cleanup, NULL);
}

void _PG_fini(void) {
  // UnregisterResourceReleaseCallback(my_resource_cleanup, NULL);
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
static FILE* nx_server_info_file = NULL;

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
  text* config_file_name_text;
  char config_buffer[256];

  const char* nx_info_filename = "/opt/postgresql/nx_server_info";

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
    PG_RETURN_INT32(0);
  }

  // open file for writing NetworkX server info
  if (nx_server_info_file) {
    fclose(nx_server_info_file);
  }
  nx_server_info_file = fopen(nx_info_filename, "r+");

  if (nx_server_info_file == NULL) {
    nx_server_info_file = fopen(nx_info_filename, "w+");
  }

  if (nx_server_info_file == NULL) {
    elog(ERROR, "Cannot open NetworkX server info file: %s", nx_info_filename);
    PG_RETURN_INT32(0);
  }

  init_server_info(nx_server_info_file);

  // set config values
  safe_find_value("path", "KAFKA_HOME", config_kafka_home);

  safe_find_value("path", "GART_HOME", config_gart_home);

  safe_find_value("gart", "rgmapping-file", config_gart_yaml_path);

  safe_find_value("gart", "etcd-endpoints", config_etcd_endpoints);

  safe_find_value("gart", "etcd-prefix", config_etcd_prefix);

  safe_find_value("gart", "subgraph-num", config_buffer);
  config_subgraph_num = atoi(config_buffer);

  config_inited = 1;

  elog(INFO, "Set config file name: %s", config_file_name);

  PG_RETURN_INT32(1);
}

Datum gart_get_connection(PG_FUNCTION_ARGS) {
  Oid userid;
  char* username;
  Oid databaseid;
  char* databasename;

  text* password_text;
  char password[512];

  FILE* fp = NULL;

  char log_line[1024];
  char cmd[1024];

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
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
    PG_RETURN_INT32(0);
  }

  sprintf(cmd, "sh %s/apps/pgx/run.sh -c %s -u %s -p %s -b %s",
          config_gart_home, config_file_name, username, password, databasename);
  elog(INFO, "Command: %s", cmd);

  fflush(log_file);

  // execute command
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    PG_RETURN_INT32(0);
  }

  // output to logs line by line
  while (1) {
    int char_written;
    int read_stat;

    CHECK_FOR_INTERRUPTS();

    read_stat = non_blocking_fgets(log_line, sizeof(log_line), fp);

    if (strstr(log_line, "GART started completely.")) {
      elog(INFO, "GART started completely!");
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

  elog(INFO, "gart_get_connection completely");

  PG_RETURN_INT32(0);
}

Datum gart_release_connection(PG_FUNCTION_ARGS) {
  FILE* fp = NULL;
  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
  }

  sprintf(cmd, "sh %s/apps/pgx/run.sh -c %s --stop", config_gart_home,
          config_file_name);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    PG_RETURN_INT32(0);
  }

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    fprintf(log_file, "%s", log_line);
    fflush(log_file);
    is_read = 1;
  }

  if (!is_read) {
    elog(ERROR, "Cannot read from command: %s", cmd);
    pclose(fp);
    PG_RETURN_INT32(0);
  }

  pclose(fp);

  PG_RETURN_INT32(1);
}

Datum gart_define_graph(PG_FUNCTION_ARGS) {
  FILE *output_yaml, *fp;
  char sql_str[4098];
  text* sql_text;

  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
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
    PG_RETURN_INT32(0);
  }

  // Use JAVA converter to convert SQL to YAML
  sprintf(cmd, "(cd %s/pgql/; sh run.sh sql2yaml_str '%s' %s)",
          config_gart_home, sql_str, config_gart_yaml_path);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    fclose(output_yaml);
    PG_RETURN_INT32(0);
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
    PG_RETURN_INT32(0);
  }

  fclose(output_yaml);
  pclose(fp);

  PG_RETURN_INT32(1);
}

Datum gart_define_graph_by_sql(PG_FUNCTION_ARGS) {
  FILE *input_sql, *output_yaml, *fp;
  char input_sql_path[1024];
  text* sql_text;

  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
  }

  sql_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(sql_text, input_sql_path);

  input_sql = fopen(input_sql_path, "rb");
  if (input_sql == NULL) {
    elog(ERROR, "Cannot open input SQL file: %s.", input_sql_path);
    PG_RETURN_INT32(0);
  }

  output_yaml = fopen(config_gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    elog(ERROR, "Cannot open output YAML file: %s.", config_gart_yaml_path);
    fclose(input_sql);
    PG_RETURN_INT32(0);
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
    PG_RETURN_INT32(0);
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
    PG_RETURN_INT32(0);
  }

  fclose(input_sql);
  fclose(output_yaml);
  pclose(fp);

  PG_RETURN_INT32(1);
}

Datum gart_define_graph_by_yaml(PG_FUNCTION_ARGS) {
  FILE *input_yaml, *output_yaml;
  char input_yaml_path[1024];
  text* yaml_text;

  char log_line[1024];
  size_t bytes_read;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
  }

  yaml_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(yaml_text, input_yaml_path);

  input_yaml = fopen(input_yaml_path, "rb");
  if (input_yaml == NULL) {
    elog(ERROR, "Cannot open input YAML file: %s.", input_yaml_path);
    PG_RETURN_INT32(0);
  }

  output_yaml = fopen(config_gart_yaml_path, "wb");
  if (output_yaml == NULL) {
    elog(ERROR, "Cannot open output YAML file: %s.", config_gart_yaml_path);
    fclose(input_yaml);
    PG_RETURN_INT32(0);
  }

  // copy file
  while ((bytes_read = fread(log_line, 1, sizeof(log_line), input_yaml)) > 0) {
    fwrite(log_line, 1, bytes_read, output_yaml);
  }

  fclose(input_yaml);
  fclose(output_yaml);
  PG_RETURN_INT32(1);
}

Datum gart_get_lastest_epoch(PG_FUNCTION_ARGS) {
  char etcd_key[256];
  char cmd[1024];
  char log_line[1024];

  int found = 0;
  int epoch_num = -1;

  FILE* fp = NULL;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  // TODO(ssj): get latest epoch from etcd using partition id = 0
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
      epoch_num = atoi(log_line);
      found = 0;
      break;
    }
    if (strstr(log_line, etcd_key)) {
      found = 1;
    }
  }

  PG_RETURN_INT32(epoch_num);
}

#define MAX_HOSTNAME_LEN 126

// index: server_id
Datum gart_launch_graph_server(PG_FUNCTION_ARGS) {
  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  text* server_host_text;
  FILE* fp = NULL;
  char server_host[MAX_HOSTNAME_LEN + 1];
  int server_port, server_id;

  server_host_text = PG_GETARG_TEXT_PP(0);
  safe_text_to_cstring(server_host_text, server_host);

  server_port = PG_GETARG_INT32(1);

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
  }

  sprintf(cmd,
          "%s/apps/networkx/build/gart_networkx_server --etcd_endpoint %s "
          "--meta_prefix %s --server_addr %s:%d &",
          config_gart_home, config_etcd_endpoints, config_etcd_prefix,
          server_host, server_port);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    PG_RETURN_INT32(0);
  }

  server_id = add_server_info(nx_server_info_file, server_host, server_port);

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    fprintf(log_file, "%s", log_line);
    fflush(log_file);
    elog(INFO, "%s", log_line);
    is_read = 1;

    if (strstr(log_line, "Server listening on")) {
      break;
    }
  }

  if (!is_read) {
    elog(ERROR, "Cannot read from command: %s", cmd);
    pclose(fp);
    PG_RETURN_INT32(0);
  }

  pclose(fp);

  PG_RETURN_INT32(server_id);
}

static int kill_networkx_server(int server_id) {
  char cmd[1024];
  FILE* fp = NULL;
  char hostname[MAX_HOSTNAME_LEN + 1];
  int port;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return -1;
  }

  if (!get_server_info(nx_server_info_file, server_id, hostname, &port)) {
    elog(ERROR, "Cannot find server info for server id: %d", server_id);
    return -1;
  }

  // kill $(pgrep -f ".*gart_networkx_server .* %s") > /dev/null 2>&1
  sprintf(cmd, "kill $(pgrep -f \".*gart_networkx_server .* %s:%d\")", hostname,
          port);
  elog(INFO, "Command: %s", cmd);

  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    return -1;
  }

  pclose(fp);

  if (!delete_server_info(nx_server_info_file, server_id)) {
    elog(ERROR, "Cannot delete server info for server id: %d", server_id);
    return -1;
  }

  return 0;
}

Datum gart_stop_graph_server(PG_FUNCTION_ARGS) {
  int server_id;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    PG_RETURN_INT32(0);
  }

  server_id = PG_GETARG_INT32(0);
  if (server_id < 0) {
    elog(ERROR, "Invalid server id: %d", server_id);
    PG_RETURN_INT32(0);
  }

  if (kill_networkx_server(server_id) < 0) {
    PG_RETURN_INT32(0);
  }

  PG_RETURN_INT32(1);
}

Datum gart_run_networkx_app(PG_FUNCTION_ARGS) {
  char result[512] = "Run NetworkX app successfully!\n";

  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  int server_handle;
  char hostname[MAX_HOSTNAME_LEN + 1];
  int port;

  text* script_file_text;
  char script_file[256];
  FILE* fp = NULL;

  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    return (Datum) 0;
  }

  // TODO(ssj): server_handle is not used
  server_handle = PG_GETARG_INT32(0);
  script_file_text = PG_GETARG_TEXT_PP(1);
  safe_text_to_cstring(script_file_text, script_file);

  if (!get_server_info(nx_server_info_file, server_handle, hostname, &port)) {
    elog(ERROR, "Cannot find server info for server id: %d", server_handle);
    return (Datum) 0;
  }

  elog(INFO, "Run NetworkX app on server %s:%d", hostname, port);

  sprintf(cmd, "python3 %s", script_file);
  elog(INFO, "Command: %s", cmd);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    elog(ERROR, "Cannot execute command: %s", cmd);
    return (Datum) 0;
  }

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    size_t len = strlen(log_line);
    if (len > 0 && log_line[len - 1] == '\n') {
      log_line[len - 1] = '\0';
    }

    fprintf(log_file, "%s", log_line);
    fflush(log_file);
    elog(INFO, "%s", log_line);
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

static void my_resource_cleanup(ResourceReleasePhase phase, bool isCommit,
                                bool isTopLevel, void* arg) {
  if (phase == RESOURCE_RELEASE_AFTER_LOCKS) {
    if (log_file) {
      fprintf(log_file, "Session Exit");
      fflush(log_file);
      fclose(log_file);
      log_file = NULL;
    }

    // TODO(ssj): kill all NetworkX servers
    // for (int i = 0; i < server_id_counter; ++i) {
    //   if (server_info[i].server_host) {
    //     kill_networkx_server(i);
    //   }
    // }
  }
}

Datum gart_show_graph_server_info(PG_FUNCTION_ARGS) {
#define NUM_COLS 3
  FuncCallContext* funcctx;
  TupleDesc tuple_desc;
  HeapTuple tuple;
  Datum values[NUM_COLS];
  bool nulls[NUM_COLS] = {false, false, false};

  int server_id_num = get_next_server_id();
  int* server_id_ptr;

  // First call, setup context
  if (SRF_IS_FIRSTCALL()) {
    MemoryContext oldcontext;
    funcctx = SRF_FIRSTCALL_INIT();
    oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

    // Define the tuple descriptor for our result type
    tuple_desc = CreateTemplateTupleDesc(NUM_COLS);
    TupleDescInitEntry(tuple_desc, (AttrNumber) 1, "id", INT4OID, -1, 0);
    TupleDescInitEntry(tuple_desc, (AttrNumber) 2, "hostname", TEXTOID, -1, 0);
    TupleDescInitEntry(tuple_desc, (AttrNumber) 3, "port", INT4OID, -1, 0);

    funcctx->tuple_desc = BlessTupleDesc(tuple_desc);

    // Initialize the function call context
    funcctx->user_fctx =
        MemoryContextAlloc(funcctx->multi_call_memory_ctx, sizeof(int));
    *(int*) (funcctx->user_fctx) = 0;  // Start with server ID 0
    funcctx->max_calls = server_id_num;

    MemoryContextSwitchTo(oldcontext);
  }

  funcctx = SRF_PERCALL_SETUP();
  if (!config_inited) {
    elog(ERROR, "Config file is not set.");
    SRF_RETURN_DONE(funcctx);
  }

  server_id_ptr = (int*) (funcctx->user_fctx);

  while (*server_id_ptr < funcctx->max_calls) {
    char hostname[MAX_HOSTNAME_LEN + 1];
    int port;

    if (get_server_info(nx_server_info_file, *server_id_ptr, hostname, &port)) {
      values[0] = Int32GetDatum(*server_id_ptr);
      values[1] = CStringGetTextDatum(hostname);
      values[2] = Int32GetDatum(port);

      tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

      (*server_id_ptr)++;

      SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    } else {
      (*server_id_ptr)++;
    }
  }

  SRF_RETURN_DONE(funcctx);
#undef NUM_COLS
}

Datum gart_run_sssp(PG_FUNCTION_ARGS) {
  char cmd[1024];
  char log_line[1024];
  int is_read = 0;

  int server_handle;
  char hostname[MAX_HOSTNAME_LEN + 1];
  int port;

  text* src_node_text;
  char src_node[32];
  FILE* fp = NULL;

  const char* result_file = "/opt/postgresql/sssp.out";

  // used for reading result from file
  FuncCallContext* funcctx;
  TupleDesc tuple_desc;
  HeapTuple tuple;
  Datum values[2];
  bool nulls[2] = {false, false};

  // First call, setup context
  if (SRF_IS_FIRSTCALL()) {
    MemoryContext oldcontext;

    funcctx = SRF_FIRSTCALL_INIT();
    oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

    // Define the tuple descriptor for our result type
    tuple_desc = CreateTemplateTupleDesc(2);
    TupleDescInitEntry(tuple_desc, (AttrNumber) 1, "dst_node", TEXTOID, -1, 0);
    TupleDescInitEntry(tuple_desc, (AttrNumber) 2, "distance", INT4OID, -1, 0);
    funcctx->tuple_desc = BlessTupleDesc(tuple_desc);

    MemoryContextSwitchTo(oldcontext);

    if (!config_inited) {
      elog(ERROR, "Config file is not set.");
      SRF_RETURN_DONE(funcctx);
    }

    server_handle = PG_GETARG_INT32(0);
    src_node_text = PG_GETARG_TEXT_PP(1);
    safe_text_to_cstring(src_node_text, src_node);

    if (!get_server_info(nx_server_info_file, server_handle, hostname, &port)) {
      elog(ERROR, "Cannot find server info for server id: %d", server_handle);
      SRF_RETURN_DONE(funcctx);
    }

    elog(INFO, "Run SSSP algorithm on server %s:%d", hostname, port);

    sprintf(cmd,
            "python3 %s/apps/networkx/client/test/algorithm.py --host %s "
            "--port %d --output %s %s --source \"%s\"",
            config_gart_home, hostname, port, result_file, "sssp", src_node);
    elog(INFO, "Command: %s", cmd);
    fp = popen(cmd, "r");
    if (fp == NULL) {
      elog(ERROR, "Cannot execute command: %s", cmd);
      SRF_RETURN_DONE(funcctx);
    }

    while (fgets(log_line, sizeof(log_line), fp) != NULL) {
      size_t len = strlen(log_line);
      if (len > 0 && log_line[len - 1] == '\n') {
        log_line[len - 1] = '\0';
      }

      fprintf(log_file, "%s", log_line);
      fflush(log_file);
      elog(INFO, "%s", log_line);
      is_read = 1;
    }

    if (!is_read) {
      elog(ERROR, "Cannot read from command: %s", cmd);
      pclose(fp);
      SRF_RETURN_DONE(funcctx);
    }

    pclose(fp);

    // read result from file
    fp = fopen(result_file, "r");
    if (fp == NULL) {
      elog(ERROR, "Cannot open result file: %s", result_file);
      return (Datum) 0;
    }

    funcctx->user_fctx = fp;
  }

  funcctx = SRF_PERCALL_SETUP();
  fp = (FILE*) funcctx->user_fctx;

  while (fgets(log_line, sizeof(log_line), fp) != NULL) {
    char dst_node[32];
    int label, id, distance;

    if (sscanf(log_line, "(%d, %d): %d\n", &label, &id, &distance) == 3) {
      sprintf(dst_node, "(%d, %d)", label, id);
      values[0] = CStringGetTextDatum(dst_node);
      values[1] = Int32GetDatum(distance);

      tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

      SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }
  }

  SRF_RETURN_DONE(funcctx);
}
