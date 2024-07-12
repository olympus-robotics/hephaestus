#pragma once

#include <chrono>
#include <string>

#include <magic_enum.hpp>

#include "example.pb.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/utils/exception.h"

namespace telemetry_example {

enum class MotorStatus : uint8_t { OK = 0, DISCONNECTED = 1, FAULT = 2, OVERHEATING = 3 };

struct MotorLog {
  MotorStatus status;
  double current_amp{};
  int64_t velocity_rps{};
  std::string error_message;
  std::chrono::milliseconds elapsed_time;
  uint32_t counter{};
  int32_t temperature_celsius{};
};

}  // namespace telemetry_example

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<telemetry_example::MotorLog> {
  using Type = telemetry_example::proto::MotorLog;
};
}  // namespace heph::serdes::protobuf

namespace telemetry_example {
inline void toProto(proto::MotorLog& proto_motor_log, const MotorLog& motor_log) {
  proto_motor_log.set_status(static_cast<proto::MotorStatus>(motor_log.status));
  proto_motor_log.set_current_amp(motor_log.current_amp);
  proto_motor_log.set_velocity_rps(motor_log.velocity_rps);
  proto_motor_log.set_error_message(motor_log.error_message);
  proto_motor_log.set_elapsed_time_ns(
      std::chrono::duration_cast<std::chrono::nanoseconds>(motor_log.elapsed_time).count());
  proto_motor_log.set_counter(motor_log.counter);
  proto_motor_log.set_temperature_celsius(motor_log.temperature_celsius);
}

inline void fromProto(const proto::MotorLog& proto_motor_log, MotorLog& motor_log) {
  auto status = magic_enum::enum_cast<MotorStatus>(static_cast<uint8_t>(proto_motor_log.status()));
  heph::throwExceptionIf<heph::InvalidDataException>(!status.has_value(), "failed to convert MotorStatus");
  motor_log.status = status.value();  // NOLINT(bugprone-unchecked-optional-access)
  motor_log.current_amp = proto_motor_log.current_amp();
  motor_log.velocity_rps = proto_motor_log.velocity_rps();
  motor_log.error_message = proto_motor_log.error_message();
  motor_log.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::nanoseconds{ proto_motor_log.elapsed_time_ns() });
  motor_log.counter = proto_motor_log.counter();
  motor_log.temperature_celsius = proto_motor_log.temperature_celsius();
}
}  // namespace telemetry_example
