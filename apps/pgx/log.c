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
#include <unistd.h>

#include "executor/executor.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "tcop/utility.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

static int nested_level = 0;
static char query[1024];
static void write_file(const char* str);
static char* read_file(FILE* fp);

Datum pg_all_queries(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_all_queries);

Datum pg_call_shell(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pg_call_shell);

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

Datum pg_call_shell(PG_FUNCTION_ARGS) {
  text* command = PG_GETARG_TEXT_PP(0);
  system("sh /opt/ssj/projects/pgx-example/test.sh");
  char result[100];
  sprintf(result, "Hello World! %s", VARDATA_ANY(command));
  PG_RETURN_TEXT_P(cstring_to_text(result));
  return (Datum) 0;
}