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

#include <fstream>

#include "converter/flags.h"
#include "converter/kafka_helper.h"
#include "converter/parser.h"

using converter::KafkaConsumer;
using converter::KafkaOutputStream;
using converter::KafkaProducer;
using converter::LogEntry;
using converter::TxnLogParser;

using std::ifstream;
using std::shared_ptr;
using std::string;

using std::cout;
using std::endl;
using std::flush;
using std::make_shared;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  shared_ptr<KafkaConsumer> consumer = make_shared<KafkaConsumer>(
      FLAGS_read_kafka_broker_list, FLAGS_read_kafka_topic, "gart_consumer");
  shared_ptr<KafkaProducer> producer = make_shared<KafkaProducer>(
      FLAGS_write_kafka_broker_list, FLAGS_write_kafka_topic);
  KafkaOutputStream ostream(producer);

  TxnLogParser parser(FLAGS_rg_mapping_file_path, FLAGS_subgraph_num);

  int log_count = 0;
  bool is_timeout = false;

#ifdef USE_DEBEZIUM
  // bulk load data
  if (FLAGS_enable_bulkload) {
    cout << "Bulk load data start" << endl;
    int init_logs = 0;
    while (1) {
      RdKafka::Message* msg = consumer->consume(&is_timeout);

      if (is_timeout) {
        // skip empty message to avoid JSON parser error
        cout << "Waiting for snapshot complete" << endl;
        continue;
      }

      string line(static_cast<const char*>(msg->payload()), msg->len());
      LogEntry log_entry;
      parser.parse(log_entry, line, 0);
      ++init_logs;
      if (!log_entry.valid()) {
        continue;
      }
      if (init_logs % 100000 == 0 && init_logs) {
        cout << "Bulk load data: " << init_logs << " logs" << endl;
      }

      if (log_entry.last_snapshot()) {
        cout << "Bulk load data finished: " << init_logs << " logs" << endl;
        log_count = FLAGS_logs_per_epoch;  // for the first epoch
        ostream << LogEntry::bulk_load_end().to_string() << flush;
        break;
      }

      ostream << log_entry.to_string() << flush;
    }
  }
#endif

  while (1) {
    RdKafka::Message* msg = consumer->consume(&is_timeout);

    // skip empty message to avoid JSON parser error
    if (is_timeout) {
      continue;
    }

    string line(static_cast<const char*>(msg->payload()), msg->len());
    int epoch = log_count / FLAGS_logs_per_epoch;

    LogEntry log_entry;
    parser.parse(log_entry, line, epoch);

    while (log_entry.more_entires()) {
      ostream << log_entry.to_string() << flush;
      parser.parse(log_entry, line, epoch);

      if (!log_entry.valid()) {
        break;
      }
    }

    if (!log_entry.valid()) {
      continue;
    }

    ostream << log_entry.to_string() << flush;
    log_count++;
  }
}
