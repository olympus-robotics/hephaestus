//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>
#include <zenohc.hxx>

#include <zenoh.h>

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
};

class Publisher {
public:
  explicit Publisher(PublisherConfig config);

  [[nodiscard]] auto publish(std::span<std::byte> data) -> bool;

private:
  PublisherConfig config_;

  std::unique_ptr<zenohc::Session> session_;
  std::unique_ptr<zenohc::Publisher> publisher_;
  std::size_t pub_msg_count_ = 0;

  zenohc::PublisherPutOptions put_options_;

  std::unordered_map<std::string, std::string> attachment_;
};

Publisher::Publisher(PublisherConfig config) : config_(std::move(config)) {
  zenohc::Config zconfig;
  zconfig.insert_json(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");
  if (config_.enable_shared_memory) {
    zconfig.insert_json("transport/shared_memory/enabled", "true");
  }

  if (config_.mode == PublisherConfig::CLIENT) {
    static constexpr std::string_view DEFAULT_ROUTER = "localhost:7447";
    if (config_.router.empty()) {
      config_.router = DEFAULT_ROUTER;
    }

    zconfig.insert_json(Z_CONFIG_MODE_KEY, R"("client")");
  }

  if (!config_.router.empty()) {
    const auto router_endpoint = std::format(R"(["tcp/{}"])", config_.router);
    zconfig.insert_json(Z_CONFIG_CONNECT_KEY, router_endpoint.c_str());
  }

  session_ = expectAsPtr(open(std::move(zconfig)));

  zenohc::PublisherOptions pub_options;
  publisher_ = expectAsPtr(session_->declare_publisher(config_.topic, pub_options));

  put_options_.set_encoding(Z_ENCODING_PREFIX_APP_CUSTOM);
  attachment_[messageCounterKey()] = "0";
  put_options_.set_attachment(attachment_);
}

auto Publisher::publish(std::span<std::byte> data) -> bool {
  attachment_[messageCounterKey()] = std::to_string(pub_msg_count_++);

  return publisher_->put({ data.data(), data.size() }, put_options_);
}

}  // namespace eolo::ipc::zenoh
