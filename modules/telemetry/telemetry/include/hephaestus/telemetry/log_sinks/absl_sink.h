//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/telemetry/struclog.h"

namespace heph::telemetry {

/**
 * @brief A simple sink that passes the struclogs to ABSL.
 *        Per default entry is formatted via heph::telemetry::format.
 *
 */
class AbslLogSink : public ILogSink {
public:
  explicit AbslLogSink();
  explicit AbslLogSink(Formatter&& f);

  void send(const LogEntry& entry) override;

private:
  Formatter formatter_;
};

}  // namespace heph::telemetry