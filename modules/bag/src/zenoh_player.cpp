//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/bag/zenoh_player.h"

#include <chrono>

#include <fmt/chrono.h>

#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/utils/exception.h"

namespace heph::bag {

class ZenohPlayer::Impl {
public:
  explicit Impl(ZenohPlayerParams params);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

  void wait() const;

private:
  void createPublisher(const mcap::Channel& channel);

  void run();

private:
  ipc::zenoh::SessionPtr session_;
  std::unique_ptr<mcap::McapReader> bag_reader_;

  bool wait_for_readers_to_connect_;

  std::size_t channel_count_{};
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::Publisher>> publishers_;
  std::unordered_set<std::string> publishers_with_subscriber_;
  std::atomic_flag all_publisher_connected_ = ATOMIC_FLAG_INIT;

  std::future<void> run_job_;

  std::atomic_bool terminate_ = false;
  std::condition_variable play_cv_;
  std::mutex play_mutex_;
};

ZenohPlayer::Impl::Impl(ZenohPlayerParams params)
  : session_(std::move(params.session))
  , bag_reader_(std::move(params.bag_reader))
  , wait_for_readers_to_connect_(params.wait_for_readers_to_connect) {
}

auto ZenohPlayer::Impl::start() -> std::future<void> {
  const auto status = bag_reader_->readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);
  throwExceptionIf<InvalidDataException>(!status.ok(),
                                         fmt::format("Failed to read bag summary: {}", status.message));

  const auto channels = bag_reader_->channels();
  channel_count_ = channels.size();
  LOG(INFO) << fmt::format("found {} channels in the bag", channels.size());
  for (const auto& [id, channel] : channels) {
    createPublisher(*channel);
  }

  run_job_ = std::async(std::launch::async, [this]() { run(); });

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

auto ZenohPlayer::Impl::stop() -> std::future<void> {
  throwExceptionIf<InvalidOperationException>(terminate_, "player is already stopped, cannot stop again");
  terminate_ = true;
  play_cv_.notify_all();

  return std::async(std::launch::async, [this]() { run_job_.get(); });
}

void ZenohPlayer::Impl::wait() const {
  run_job_.wait();
}

void ZenohPlayer::Impl::createPublisher(const mcap::Channel& channel) {
  throwExceptionIf<InvalidDataException>(
      publishers_.contains(channel.topic),
      fmt::format("failed to create publisher for topic: {}; topic already exist", channel.topic));

  const auto& schema = bag_reader_->schema(channel.schemaId);
  auto type_info = serdes::TypeInfo{
    .name = schema->name,
    .schema = schema->data,
    .serialization = serdes::TypeInfo::Serialization::PROTOBUF,  // TODO: get this from schema
    .original_type =
        "",  // TODO: figure out if there is a way to get this. We can use the channel metadata!!!
  };

  publishers_[channel.topic] = std::make_unique<ipc::zenoh::Publisher>(
      session_, ipc::TopicConfig{ .name = channel.topic }, std::move(type_info),
      [this, &channel](ipc::zenoh::MatchingStatus status) {
        if (!status.matching) {
          return;
        }

        publishers_with_subscriber_.insert(channel.topic);
        if (publishers_with_subscriber_.size() == channel_count_) {
          all_publisher_connected_.test_and_set();
          all_publisher_connected_.notify_all();
        }
      });

  LOG(INFO) << fmt::format("Created publisher for topic: {}", channel.topic);
}

void ZenohPlayer::Impl::run() {
  auto read_options = mcap::ReadMessageOptions{};
  read_options.readOrder = mcap::ReadMessageOptions::ReadOrder::LogTimeOrder;
  auto messages = bag_reader_->readMessages([](const auto&) {}, read_options);

  auto first_msg_timestamp = std::chrono::steady_clock::time_point{ std::chrono::nanoseconds{
      messages.begin()->message.publishTime } };

  std::size_t msgs_played_count = 0;
  std::size_t deadline_missed_count = 0;

  LOG_IF(WARNING, wait_for_readers_to_connect_) << "Waiting for subscribers to connect is NOT supported yet!";
  if (wait_for_readers_to_connect_) {
    all_publisher_connected_.wait(false);
  }

  const auto first_playback_timestamp = std::chrono::steady_clock::now();
  for (const auto& message : messages) {
    if (terminate_) {
      break;
    }

    const auto& topic = message.channel->topic;
    const auto& publisher = publishers_[topic];

    const auto current_msg_timestamp =
        std::chrono::steady_clock::time_point{ std::chrono::nanoseconds{ message.message.publishTime } };

    const auto write_timestamp = current_msg_timestamp - first_msg_timestamp + first_playback_timestamp;

    if (auto now = std::chrono::steady_clock::now(); now > write_timestamp && msgs_played_count > 0) {
      ++deadline_missed_count;
      LOG(WARNING) << fmt::format("failed to publish message {} in time for topic {}, delay {}",
                                  message.message.sequence, topic, now - write_timestamp);
    } else {
      std::unique_lock<std::mutex> guard(play_mutex_);
      if (play_cv_.wait_until(guard, write_timestamp, [this] { return terminate_.load(); })) {
        break;
      }
    }

    auto success = publisher->publish({ message.message.data, message.message.dataSize });
    LOG_IF(WARNING, !success) << fmt::format("Failed to publish message {} for topic {}",
                                             message.message.sequence, topic);

    ++msgs_played_count;
  }

  LOG(INFO) << fmt::format("Played {} messages, missed {} deadlines", msgs_played_count,
                           deadline_missed_count);
}

// ----------------------------------------------------------------------------------------------------------

ZenohPlayer::ZenohPlayer(ZenohPlayerParams params) : impl_(std::make_unique<Impl>(std::move(params))) {
}

ZenohPlayer::~ZenohPlayer() = default;

auto ZenohPlayer::start() -> std::future<void> {
  return impl_->start();
}

auto ZenohPlayer::stop() -> std::future<void> {
  return impl_->stop();
}

void ZenohPlayer::wait() const {
  impl_->wait();
}

auto ZenohPlayer::create(ZenohPlayerParams params) -> ZenohPlayer {
  return ZenohPlayer{ std::move(params) };
}
}  // namespace heph::bag
