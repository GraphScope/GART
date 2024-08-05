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
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef USE_TBB
#include <tbb/concurrent_queue.h>
#else
#include <readerwriterqueue/readerwriterqueue.h>
#endif

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

#ifdef USE_TBB
std::vector<tbb::concurrent_queue<std::string>> binlog_queues;
std::vector<tbb::concurrent_queue<LogEntry>> unified_log_queues;
std::vector<tbb::concurrent_queue<int>> unified_log_counts;
#else
std::vector<std::shared_ptr<moodycamel::ReaderWriterQueue<std::string>>>
    binlog_queues;
std::vector<std::shared_ptr<moodycamel::ReaderWriterQueue<LogEntry>>>
    unified_log_queues;
std::vector<std::shared_ptr<moodycamel::ReaderWriterQueue<int>>>
    unified_log_counts;
#endif

bool bulk_load_finished = false;
bool enable_bulk_load = false;
bool catch_up_mode = true;
int64_t last_processed_offset = 0;

constexpr char NO_MESSAGE[] = "No message";

void process_binlog_step_1(int thread_id, TxnLogParser& parser) {
  while (true) {
    std::string line;
    while (true) {
#ifndef USE_TBB
      if (binlog_queues[thread_id]->try_dequeue(line)) {
#else
      if (binlog_queues[thread_id].try_pop(line)) {
#endif
        break;
      }
      std::this_thread::yield();
    }

    if (line == NO_MESSAGE) {
#ifndef USE_TBB
      while (!unified_log_counts[thread_id]->try_enqueue(0)) {
        std::this_thread::yield();
      }
#else
      unified_log_counts[thread_id].push(0);
#endif
      continue;
    }

    LogEntry log_entry;
    GART_CHECK_OK(parser.parse(log_entry, line));
    if (!log_entry.valid()) {
#ifndef USE_TBB
      while (!unified_log_counts[thread_id]->try_enqueue(0)) {
        std::this_thread::yield();
      }
#else
      unified_log_counts[thread_id].push(0);
#endif
      continue;
    }
    int unified_log_count = 0;

    while (log_entry.more_entires()) {
      if (log_entry.complete()) {
        unified_log_count++;
#ifndef USE_TBB
        while (!unified_log_queues[thread_id]->try_enqueue(log_entry)) {
          std::this_thread::yield();
        }
#else
        unified_log_queues[thread_id].push(log_entry);
#endif
      }
      GART_CHECK_OK(parser.parse(log_entry, line));
    }

    if (log_entry.complete()) {
      unified_log_count++;
#ifndef USE_TBB
      while (!unified_log_queues[thread_id]->try_enqueue(log_entry)) {
        std::this_thread::yield();
      }
#else
      unified_log_queues[thread_id].push(log_entry);
#endif
    }
#ifndef USE_TBB
    while (!unified_log_counts[thread_id]->try_enqueue(unified_log_count)) {
      std::this_thread::yield();
    }
#else
    unified_log_counts[thread_id].push(unified_log_count);
#endif
  }
}

void process_binlog_step_2(KafkaOutputStream& ostream, TxnLogParser& parser) {
  int epoch = 0;
  uint64_t log_count = 0;
  int last_tx_id = -1;
  int last_log_count = 0;

  int64_t processed_count = 0;
  auto start = std::chrono::steady_clock::now();
  auto interval = std::chrono::seconds(FLAGS_seconds_per_epoch);
  auto start_timer = std::chrono::steady_clock::now();

  while (true) {
    for (auto idx = 0; idx < unified_log_counts.size(); idx++) {
      int unified_log_count = 0;
      while (true) {
#ifndef USE_TBB
        if (unified_log_counts[idx]->try_dequeue(unified_log_count)) {
#else
        if (unified_log_counts[idx].try_pop(unified_log_count)) {
#endif
          break;
        }
        std::this_thread::yield();
      }
      if (unified_log_count == 0) {
        continue;
      }
      log_count++;
      converter::LogEntry log_entry;
      for (auto log_id = 0; log_id < unified_log_count; log_id++) {
        if (catch_up_mode) {
          if (processed_count >= last_processed_offset) {
            catch_up_mode = false;
          }
        }
        processed_count++;
        while (true) {
#ifndef USE_TBB
          if (unified_log_queues[idx]->try_dequeue(log_entry)) {
#else
          if (unified_log_queues[idx].try_pop(log_entry)) {
#endif
            break;
          }
          std::this_thread::yield();
        }
        GART_CHECK_OK(parser.parse_again(log_entry, epoch));
        if (!catch_up_mode) {
          ostream << log_entry.to_string(processed_count) << flush;
        }
      }

      if (unlikely(log_count % 1000000 == 0)) {
        auto end_timer = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            end_timer - start_timer)
                            .count();
        std::cout << "Processed 1000000 "
                  << " logs from log ids from " << log_count - 1000000 << " to "
                  << log_count << " in " << duration << " milliseconds"
                  << std::endl
                  << std::flush;
        start_timer = end_timer;
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
      if (!enable_bulk_load || (enable_bulk_load && bulk_load_finished)) {
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
      }

#endif

      if (unlikely(log_entry.last_snapshot())) {
        bulk_load_finished = true;
        std::cout << "Bulk load finished after " << log_count
                  << " logs processed" << std::endl;
        last_tx_id = log_entry.get_tx_id();
        last_log_count = log_count;
        epoch = 1;
        if (catch_up_mode) {
          if (processed_count >= last_processed_offset) {
            catch_up_mode = false;
          }
        }
        processed_count++;
        if (likely(!catch_up_mode)) {
          ostream << LogEntry::bulk_load_end().to_string(processed_count)
                  << flush;
        }
        start = std::chrono::steady_clock::now();
      }
    }
  }
}

#ifdef ENABLE_CHECKPOINT
std::mutex checkpoint_mutex;
#endif  // ENABLE_CHECKPOINT

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  shared_ptr<KafkaProducer> producer = make_shared<KafkaProducer>(
      FLAGS_write_kafka_broker_list, FLAGS_write_kafka_topic);
  KafkaOutputStream ostream(producer);

  int num_threads = FLAGS_num_threads;
  enable_bulk_load = FLAGS_enable_bulkload;

#ifdef USE_TBB
  binlog_queues.resize(num_threads);
  unified_log_queues.resize(num_threads);
  unified_log_counts.resize(num_threads);
#else
  for (auto idx = 0; idx < num_threads; idx++) {
    binlog_queues.push_back(
        std::make_shared<moodycamel::ReaderWriterQueue<std::string>>(
            FLAGS_max_queue_size));
    unified_log_queues.push_back(
        std::make_shared<moodycamel::ReaderWriterQueue<LogEntry>>(
            FLAGS_max_queue_size));
    unified_log_counts.push_back(
        std::make_shared<moodycamel::ReaderWriterQueue<int>>(
            FLAGS_max_queue_size));
  }
#endif

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

  bool is_timeout = false;

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

  std::vector<std::thread> threads_for_step_1(num_threads);
  for (auto idx = 0; idx < num_threads; idx++) {
    threads_for_step_1[idx] =
        std::thread(process_binlog_step_1, idx, std::ref(parser));
  }

  std::thread thread_for_step_2(process_binlog_step_2, std::ref(ostream),
                                std::ref(parser));

  while (true) {
    for (auto idx = 0; idx < num_threads; idx++) {
      RdKafka::Message* msg = consumer->consume(&is_timeout);
      if (is_timeout) {
        std::string line = NO_MESSAGE;
#ifndef USE_TBB
        while (!binlog_queues[idx]->try_enqueue(line)) {
          std::this_thread::yield();
        }
#else
        binlog_queues[idx].push(line);
#endif
        continue;
      }
      std::string line(static_cast<const char*>(msg->payload()), msg->len());
#ifndef USE_TBB
      while (!binlog_queues[idx]->try_enqueue(line)) {
        std::this_thread::yield();
      }
#else
      binlog_queues[idx].push(line);
#endif
      consumer->delete_message(msg);
    }
  }
}
