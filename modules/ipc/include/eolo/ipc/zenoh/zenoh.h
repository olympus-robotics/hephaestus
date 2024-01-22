//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

struct PublisherConfig {
  enum Mode { PEER, CLIENT };
  std::string topic;
  bool enable_shared_memory = false;  //! NOTE: With shared-memory enabled, the publisher still uses the
                                      //! network transport layer to notify subscribers of the shared-memory
                                      //! segment to read. Therefore, for very small messages, shared -
                                      //! memory transport could be less efficient than using the default
                                      //! network transport to directly carry the payload.
  Mode mode = PEER;
  // NOLINTNEXTLINE(readability-redundant-string-init) otherwise we need to specify in constructor
  std::string router = "";  //! If specified connect to the given router endpoint.
  std::size_t cache_size = 0;
};

class Publisher {
public:
  explicit Publisher(PublisherConfig config);
  ~Publisher();
  Publisher(const Publisher&) = delete;
  Publisher(Publisher&&) = default;
  auto operator=(const Publisher&) -> Publisher& = delete;
  auto operator=(Publisher&&) -> Publisher& = default;

  [[nodiscard]] auto publish(std::span<std::byte> data) -> bool;

private:
  [[nodiscard]] auto createZenohConfig() -> zenohc::Config;

private:
  PublisherConfig config_;

  std::unique_ptr<zenohc::Session> session_;
  std::unique_ptr<zenohc::Publisher> publisher_;
  std::size_t pub_msg_count_ = 0;

  zc_owned_liveliness_token_t liveliness_token_{};
  ze_owned_publication_cache_t pub_cache_{};

  zenohc::PublisherPutOptions put_options_;

  std::unordered_map<std::string, std::string> attachment_;
};

Publisher::Publisher(PublisherConfig config) : config_(std::move(config)) {
  auto zconfig = createZenohConfig();

  // Create the publisher
  session_ = expectAsPtr(open(std::move(zconfig)));

  // Enable publishing of a liveliness token.
  liveliness_token_ =
      zc_liveliness_declare_token(session_->loan(), z_keyexpr(config_.topic.c_str()), nullptr);
  throwExceptionIf<FailedZenohOperation>(!z_check(liveliness_token_), "failed to create livelines token");

  // Enable cache.
  if (config_.cache_size > 0) {
    auto pub_cache_opts = ze_publication_cache_options_default();
    pub_cache_opts.history = config_.cache_size;
    pub_cache_ =
        ze_declare_publication_cache(session_->loan(), z_keyexpr(config_.topic.c_str()), &pub_cache_opts);
    throwExceptionIf<FailedZenohOperation>(!z_check(pub_cache_), "failed to enable cache");
  }

  zenohc::PublisherOptions pub_options;
  // TODO: add support to priorty
  publisher_ = expectAsPtr(session_->declare_publisher(config_.topic, pub_options));

  put_options_.set_encoding(Z_ENCODING_PREFIX_APP_CUSTOM);
  attachment_[messageCounterKey()] = "0";
  put_options_.set_attachment(attachment_);
}

Publisher::~Publisher() {
  z_drop(z_move(liveliness_token_));
  z_drop(z_move(pub_cache_));
}

auto Publisher::publish(std::span<std::byte> data) -> bool {
  attachment_[messageCounterKey()] = std::to_string(pub_msg_count_++);

  return publisher_->put({ data.data(), data.size() }, put_options_);
}

auto Publisher::createZenohConfig() -> zenohc::Config {
  zenohc::Config zconfig;
  // A timestamp is add to every published message.
  zconfig.insert_json(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");

  // Enable shared memory support.
  if (config_.enable_shared_memory) {
    zconfig.insert_json("transport/shared_memory/enabled", "true");
  }

  // Set node in client mode.
  if (config_.mode == PublisherConfig::CLIENT) {
    static constexpr std::string_view DEFAULT_ROUTER = "localhost:7447";
    if (config_.router.empty()) {
      config_.router = DEFAULT_ROUTER;
    }

    zconfig.insert_json(Z_CONFIG_MODE_KEY, R"("client")");
  }

  // Add router endpoint.
  if (!config_.router.empty()) {
    const auto router_endpoint = std::format(R"(["tcp/{}"])", config_.router);
    zconfig.insert_json(Z_CONFIG_CONNECT_KEY, router_endpoint.c_str());
  }

  return zconfig;
}

}  // namespace eolo::ipc::zenoh
