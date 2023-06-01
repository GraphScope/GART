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

#include <execinfo.h>
#include <csignal>

#include "framework/bench_runner.h"
#include "framework/config.h"
#include "util/spinlock.h"

namespace {
// some helper functions
SpinLock exit_lock;  // race between two signals

void print_trace(int sig) {
  printf("print_trace: got signal %d\n", sig);

  void* array[32]; /* Array to store backtrace symbols */
  size_t size;     /* To store the exact no of values stored */
  char** strings;  /* To store functions from the backtrace list in ARRAY */
  size_t nCnt;

  size = backtrace(array, 32);

  strings = backtrace_symbols(array, size);

  /* prints each string of function names of trace*/
  for (nCnt = 0; nCnt < size; nCnt++)
    fprintf(stderr, "%s\n", strings[nCnt]);
}

void sigsegv_handler(int sig) {
  exit_lock.Lock();
  fprintf(stderr, "Meet a segmentation fault!\n");
  print_trace(sig);
  exit(-1);
}

void sigint_handler(int sig) {
  exit_lock.Lock();
  fprintf(stderr, "Meet an interrupt!\n");
  exit(-1);
}

void sigabrt_handler(int sig) {
  exit_lock.Lock();
  fprintf(stderr, "Meet an assertion failure!\n");
  exit(-1);
}

}  // anonymous namespace

int main(int argc, char** argv) {
  /* install the event handler if necessary */
  signal(SIGSEGV, sigsegv_handler);
  signal(SIGABRT, sigabrt_handler);
  signal(SIGINT, sigint_handler);

  /* parse arguments for whole system */
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gart::framework::config.parse_sys_args(argc, argv);

  gart::framework::Runner* runner = new gart::framework::Runner();
  runner->run();
  return 0;
}
