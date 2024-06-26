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

#ifndef CONVERTER_KAFKA_HELPER_H_
#define CONVERTER_KAFKA_HELPER_H_

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "glog/logging.h"
#include "librdkafka/rdkafkacpp.h"

namespace converter {

/** Kafka producer class
 *
 * A Kafka producer class based on librdkafka, can be used to produce
 * stream data to one topic.
 */
class KafkaProducer {
 public:
  KafkaProducer() = default;

  explicit KafkaProducer(const std::string& broker_list,
                         const std::string& topic)
      : brokers_(broker_list), topic_(topic) {
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    std::string rdkafka_err;
    if (conf->set("metadata.broker.list", broker_list, rdkafka_err) !=
        RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "Failed to set metadata.broker.list: " << rdkafka_err;
    }
    // for producer's internal queue.
    if (conf->set("queue.buffering.max.messages",
                  std::to_string(internal_buffer_size_),
                  rdkafka_err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "Failed to set queue.buffering.max.messages: "
                 << rdkafka_err;
    }

    producer_ = std::unique_ptr<RdKafka::Producer>(
        RdKafka::Producer::create(conf, rdkafka_err));
    if (!producer_) {
      LOG(ERROR) << "Failed to create kafka producer: " << rdkafka_err;
    }
    delete conf;  // release the memory resource
  }

  ~KafkaProducer() = default;

  void add_message(const std::string& message) {
    if (message.empty()) {
      return;
    }
    RdKafka::ErrorCode err = producer_->produce(
        topic_, RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
        static_cast<void*>(const_cast<char*>(message.c_str())) /* value */,
        message.size() /* size */, NULL, 0, 0 /* timestamp */,
        NULL /* delivery report */);
    if (err != RdKafka::ERR_NO_ERROR) {
      LOG(ERROR) << "Failed to output to kafka: " << RdKafka::err2str(err);
    }
    pending_count_ += 1;
    if (pending_count_ == 1024 * 128) {
      producer_->flush(1000 * 60);  // 60s
      pending_count_ = 0;
    }
  }

  inline std::string topic() const { return topic_; }

 private:
  static const constexpr int internal_buffer_size_ = 1024 * 1024;

  const std::string brokers_;
  const std::string topic_;

  size_t pending_count_ = 0;
  std::unique_ptr<RdKafka::Producer> producer_;
};

/**
 * A kafka output stream that can be used to flush messages into kafka prodcuer.
 */
class KafkaOutputStream : public std::ostream {
 public:
  explicit KafkaOutputStream(std::shared_ptr<KafkaProducer> producer)
      : std::ostream(new KafkaBuffer(producer)) {}

  ~KafkaOutputStream() { delete rdbuf(); }

  void close() {}

 private:
  class KafkaBuffer : public std::stringbuf {
   public:
    explicit KafkaBuffer(std::shared_ptr<KafkaProducer>& prodcuer)
        : producer_(prodcuer) {}

    int sync() override {
      producer_->add_message(this->str());
      this->str("");
      return 0;
    }

   private:
    std::shared_ptr<KafkaProducer> producer_;
  };
};

/** Kafka consumer class
 *
 * A Kafka consumer class based on librdkafka, can be used to consumer
 * stream data from one topic.
 */
class KafkaConsumer {
 public:
  explicit KafkaConsumer(
      const std::string& broker_list, const std::string& topic,
      const std::string& group_id, int partition = 0,
      int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING)
      : brokers_(broker_list),
        topic_(topic),
        group_id_(group_id),
        start_offset_(start_offset),
        partition_(partition) {
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    std::string rdkafka_err;
    if (conf->set("metadata.broker.list", broker_list, rdkafka_err) !=
        RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "Failed to set metadata.broker.list: " << rdkafka_err;
    }
    if (conf->set("group.id", group_id_, rdkafka_err) !=
        RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "Failed to set group.id: " << rdkafka_err;
    }
    if (conf->set("enable.auto.commit", "false", rdkafka_err) !=
        RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "Failed to set enable.auto.commit: " << rdkafka_err;
    }
    if (conf->set("auto.offset.reset", "earliest", rdkafka_err) !=
        RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "Failed to set auto.offset.reset: " << rdkafka_err;
    }

    consumer_ = std::unique_ptr<RdKafka::Consumer>(
        RdKafka::Consumer::create(conf, rdkafka_err));
    if (!consumer_) {
      LOG(ERROR) << "Failed to create consumer: " << rdkafka_err << std::endl;
      exit(1);
    }

    RdKafka::Conf* tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

    topic_ptr_ =
        RdKafka::Topic::create(consumer_.get(), topic_, tconf, rdkafka_err);

    RdKafka::ErrorCode resp =
        consumer_->start(topic_ptr_, partition_, start_offset_);

    if (resp != RdKafka::ERR_NO_ERROR) {
      LOG(ERROR) << "Failed to start consumer: " << RdKafka::err2str(resp);
      exit(1);
    }
  }

  RdKafka::Message* consume(bool* is_timeout = nullptr,
                            int timeout_ms = 1000) const {
    RdKafka::Message* msg =
        consumer_->consume(topic_ptr_, partition_, timeout_ms);
    if (msg == nullptr) {
      LOG(ERROR) << "Failed to consume message";
    }

    if (msg->err() == RdKafka::ERR__PARTITION_EOF) {
      LOG(ERROR) << "Reached the end of the queue";
      return nullptr;
    } else if (msg->err() != RdKafka::ERR__TIMED_OUT &&
               msg->err() != RdKafka::ERR_NO_ERROR) {
      LOG(ERROR) << "Failed to consume message: "
                 << RdKafka::err2str(msg->err());
      return nullptr;
    }

    if (msg->err() == RdKafka::ERR_NO_ERROR && msg->len() == 0) {
      LOG(ERROR) << "Received message (" << msg->len() << " bytes)";
      return nullptr;
    }

    if (msg->err() == RdKafka::ERR__TIMED_OUT) {
      LOG(INFO) << "Timed out";
      if (is_timeout)
        *is_timeout = true;
    } else {
      if (is_timeout)
        *is_timeout = false;
    }

    return msg;
  }

  inline void delete_message(RdKafka::Message* msg) const {
    if (msg) {
      delete msg;
    }
  }

  inline std::pair<int64_t, int64_t> query_watermark_offsets() const {
    int64_t low, high;
    RdKafka::ErrorCode err = consumer_->query_watermark_offsets(
        topic_, partition_, &low, &high, 5000);
    if (err != RdKafka::ERR_NO_ERROR) {
      LOG(INFO) << "Failed to query watermark offsets: "
                << RdKafka::err2str(err);
      return std::make_pair(0, 0);
    }
    return std::make_pair(low, high);
  }

  inline RdKafka::ErrorCode stop() const {
    return consumer_->stop(topic_ptr_, partition_);
  }

  inline RdKafka::ErrorCode seek(int64_t offset) const {
    return consumer_->seek(topic_ptr_, partition_, offset, 5000);
  }

  inline RdKafka::ErrorCode start(int64_t new_offset) const {
    return consumer_->start(topic_ptr_, partition_, new_offset);
  }

  inline bool topic_exist() {
    RdKafka::Metadata* metadata = nullptr;
    RdKafka::ErrorCode metadata_err =
        consumer_->metadata(false, topic_ptr_, &metadata, 5000);
    delete metadata;
    if (metadata_err != RdKafka::ERR_NO_ERROR) {
      std::cout << "Failed to get metadata: " << RdKafka::err2str(metadata_err)
                << std::endl;
      return false;
    } else {
      return true;
    }
  }

  ~KafkaConsumer() = default;

 private:
  const std::string brokers_;
  const std::string topic_;
  const std::string group_id_;
  const int partition_;
  const int64_t start_offset_;

  RdKafka::Topic* topic_ptr_;
  std::unique_ptr<RdKafka::Consumer> consumer_;
};

}  // namespace converter

#endif  // CONVERTER_KAFKA_HELPER_H_
