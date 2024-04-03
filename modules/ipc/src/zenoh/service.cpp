//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/service.h"

namespace heph::ipc::zenoh {

StringService::StringService(SessionPtr session, std::string topic, Callback&& callback)
  : session_(std::move(session)), topic_(std::move(topic)), callback_(std::move(callback)) {
  auto query = [this](const zenohc::Query& query) mutable {
    StringQueryRequest request{ .topic = std::string{ query.get_keyexpr().as_string_view() },
                                .parameters = std::string{ query.get_parameters().as_string_view() },
                                .value = std::string{ query.get_value().as_string_view() } };
    auto result = this->callback_(request);

    zenohc::QueryReplyOptions options;
    options.set_encoding(zenohc::Encoding{ Z_ENCODING_PREFIX_TEXT_PLAIN });
    query.reply(this->topic_, result, options);
  };

  queryable_ =
      expectAsUniquePtr(session_->zenoh_session.declare_queryable(topic_, { std::move(query), []() {} }));
}
}  // namespace heph::ipc::zenoh
