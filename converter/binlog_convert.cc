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

#include <sys/types.h>
#include <chrono>
#include <fstream>
#include <iostream>

#ifdef ENABLE_CHECKPOINT
#include <mutex>
#include <thread>
#endif  // ENABLE_CHECKPOINT

#include "converter/flags.h"
#include "converter/kafka_helper.h"
#include "converter/parser.h"
#include "util/macros.h"

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

#ifdef ENABLE_CHECKPOINT
std::mutex checkpoint_mutex;
#endif  // ENABLE_CHECKPOINT

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  shared_ptr<KafkaProducer> producer = make_shared<KafkaProducer>(
      FLAGS_write_kafka_broker_list, FLAGS_write_kafka_topic);
  KafkaOutputStream ostream(producer);

  TxnLogParser parser(FLAGS_etcd_endpoint, FLAGS_etcd_prefix,
                      FLAGS_subgraph_num);
#ifdef ENABLE_CHECKPOINT
  std::thread checkpoint_thread([&]() {
    while (1) {
      std::this_thread::sleep_for(
          std::chrono::minutes(FLAGS_checkpoint_interval));
      {
        std::lock_guard<std::mutex> guard(checkpoint_mutex);
        parser.checkpoint_vertex_maps(FLAGS_checkpoint_dir);
      }
    }
  });
  checkpoint_thread.detach();
  {
    std::lock_guard<std::mutex> guard(checkpoint_mutex);
    parser.load_vertex_maps_checkpoint(FLAGS_checkpoint_dir);
  }
#endif  // ENABLE_CHECKPOINT

  int log_count = 0;
  bool is_timeout = false;

  int epoch = 0;

  // catch up mode is used for failover
  bool catch_up_mode = true;
  int64_t last_processed_offset = 0;
  int64_t processed_count = 0;

  shared_ptr<KafkaConsumer> consumer_for_get_last_commit =
      make_shared<KafkaConsumer>(FLAGS_write_kafka_broker_list,
                                 FLAGS_write_kafka_topic, "gart_consumer", 0,
                                 RdKafka::Topic::OFFSET_BEGINNING);
  bool topic_exist = consumer_for_get_last_commit->topic_exist();

  if (!topic_exist) {
    consumer_for_get_last_commit->stop();
    std::cout << "Empty...Will start to read topic messages from beginning."
              << std::endl;
  } else {
    std::pair<int64_t, int64_t> low_high_pair =
        consumer_for_get_last_commit->query_watermark_offsets();
    consumer_for_get_last_commit->stop();
    int64_t high_offset = low_high_pair.second;
    std::cout << "Low offset: " << low_high_pair.first
              << ", High offset: " << low_high_pair.second << std::endl;
    if (high_offset == 0) {
      std::cout << "Will start to read topic messages from beginning."
                << std::endl;
    } else {
      consumer_for_get_last_commit->start(high_offset - 1);
      RdKafka::Message* last_commit_msg =
          consumer_for_get_last_commit->consume(nullptr, 1000);
      string line(static_cast<const char*>(last_commit_msg->payload()),
                  last_commit_msg->len());
      std::cout << "Last message is " << line << std::endl;
      char delimiter = '|';
      size_t pos = line.find_last_of(delimiter);
      last_processed_offset = std::stoll(line.substr(pos + 1));
      std::cout << "Will start to read topic messages from "
                << last_processed_offset << std::endl;
      consumer_for_get_last_commit->delete_message(last_commit_msg);
      consumer_for_get_last_commit->stop();
    }
  }

  shared_ptr<KafkaConsumer> consumer = make_shared<KafkaConsumer>(
      FLAGS_read_kafka_broker_list, FLAGS_read_kafka_topic, "gart_consumer", 0,
      RdKafka::Topic::OFFSET_BEGINNING);

#ifdef USE_DEBEZIUM
  // used for consistent epoch calculation
  int last_tx_id = -1;
  int last_log_count = 0;

  // bulk load data
  if (FLAGS_enable_bulkload) {
    cout << "Bulk load data start" << endl;
    uint64_t init_logs = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (1) {
      RdKafka::Message* msg = consumer->consume(&is_timeout);

      if (is_timeout) {
        // skip empty message to avoid JSON parser error
        cout << "Waiting for snapshot complete, maybe the database is empty"
             << endl;
        continue;
      }

      string line(static_cast<const char*>(msg->payload()), msg->len());
      LogEntry log_entry;
      GART_CHECK_OK(parser.parse(log_entry, line, epoch));

      ++init_logs;

      // skip invalid log entry (unused tables)
      if (!log_entry.valid()) {
        consumer->delete_message(msg);
        continue;
      }

      while (log_entry.more_entires()) {
        if (catch_up_mode) {
          if (processed_count >= last_processed_offset) {
            catch_up_mode = false;
          }
        }
        processed_count++;
        if (likely(!catch_up_mode)) {
          ostream << log_entry.to_string(processed_count) << flush;
        }

        GART_CHECK_OK(parser.parse(log_entry, line, epoch));

        if (!log_entry.valid()) {
          break;
        }
      }

      if (catch_up_mode) {
        if (processed_count >= last_processed_offset) {
          catch_up_mode = false;
        }
      }

      processed_count++;
      if (likely(!catch_up_mode)) {
        ostream << log_entry.to_string(processed_count) << flush;
      }

      consumer->delete_message(msg);

      if (log_entry.last_snapshot()) {
        cout << "Bulk load data finished: " << init_logs << " logs" << endl;
        log_count = FLAGS_logs_per_epoch;  // for the first epoch
        last_tx_id = log_entry.get_tx_id();
        last_log_count = log_count;
        epoch = 1;
        if (catch_up_mode) {
          if (processed_count >= last_processed_offset) {
            catch_up_mode = false;
          }
        }
        processed_count++;
        if (!catch_up_mode) {
          ostream << LogEntry::bulk_load_end().to_string(processed_count)
                  << flush;
        }
        break;
      }

      const uint64_t kLogInterval = 1000000ull;
      const uint64_t kDotInterval = kLogInterval / 10;
      const char kDot = '.';

      if (init_logs % kDotInterval == 0) {
        cout << kDot << flush;
      }

      if (init_logs % kLogInterval == 0 && init_logs) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            endTime - startTime)
                            .count();

        startTime = endTime;

        cout << "\r";  // move cursor to the beginning of line

        cout << "Bulk load data: " << init_logs << " logs"
             << " time: " << duration << " ms,"
             << " speed: " << (kLogInterval * 1000 / duration) << " logs/s"
             << endl;
      }
    }
  }
#endif

  auto interval = std::chrono::seconds(FLAGS_seconds_per_epoch);
  auto start = std::chrono::steady_clock::now();

  while (1) {
    RdKafka::Message* msg = consumer->consume(&is_timeout);

    // skip empty message to avoid JSON parser error
    if (is_timeout) {
      continue;
    }

    string line(static_cast<const char*>(msg->payload()), msg->len());

    LogEntry log_entry;
    GART_CHECK_OK(parser.parse(log_entry, line, epoch));

    ++log_count;

    // skip invalid log entry (unused tables)
    if (!log_entry.valid()) {
      consumer->delete_message(msg);
      continue;
    }

#ifndef USE_DEBEZIUM
    if (FLAGS_use_logs_per_epoch) {
      epoch = log_count / FLAGS_logs_per_epoch;
    } else {
      auto now = std::chrono::steady_clock::now();
      if (now - start >= interval) {
        start = now;
        ++epoch;
      }
    }

#else
    // consistent epoch calculation
    int tx_id = log_entry.get_tx_id();
    if (tx_id == -1) {
      if (FLAGS_use_logs_per_epoch) {
        epoch = log_count / FLAGS_logs_per_epoch;
      } else {
        auto now = std::chrono::steady_clock::now();
        if (now - start >= interval) {
          start = now;
          ++epoch;
        }
      }
    } else {
      if (tx_id != last_tx_id) {
        last_tx_id = tx_id;
        if (FLAGS_use_logs_per_epoch) {
          if (log_count - last_log_count >= FLAGS_logs_per_epoch) {
            last_log_count = log_count;
            cout << "Epoch " << epoch << " finished" << endl;
            ++epoch;
          }
        } else {
          auto now = std::chrono::steady_clock::now();
          if (now - start >= interval) {
            start = now;
            cout << "Epoch " << epoch << " finished" << endl;
            ++epoch;
          }
        }
      }
    }
#endif

    while (log_entry.more_entires()) {
      if (catch_up_mode) {
        if (processed_count >= last_processed_offset) {
          catch_up_mode = false;
        }
      }
      processed_count++;
      if (!catch_up_mode) {
        ostream << log_entry.to_string(processed_count) << flush;
      }

      GART_CHECK_OK(parser.parse(log_entry, line, epoch));

      if (!log_entry.valid()) {
        break;
      }
    }

    if (catch_up_mode) {
      if (processed_count >= last_processed_offset) {
        catch_up_mode = false;
      }
    }

    processed_count++;
    if (!catch_up_mode) {
      ostream << log_entry.to_string(processed_count) << flush;
    }

    consumer->delete_message(msg);
  }
}
