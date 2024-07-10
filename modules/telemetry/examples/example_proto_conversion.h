#pragma once

#include <chrono>
#include <string>

#include <magic_enum.hpp>

#include "example.pb.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/utils/exception.h"

namespace heph {
namespace telemetry::examples {

enum class MotorStatus : uint8_t { OK = 0, DISCONNECTED = 1, FAULT = 2, OVERHEATING = 3 };

struct MotorLog {
  MotorStatus status;
  double current_amp{};
  double velocity_rps{};
  std::string error_message;
  std::chrono::milliseconds elapsed_time;
};

}  // namespace telemetry::examples

namespace serdes::protobuf {
template <>
struct ProtoAssociation<telemetry::examples::MotorLog> {
  using Type = telemetry::examples::proto::MotorLog;
};
}  // namespace serdes::protobuf

namespace telemetry::examples {
inline void toProto(proto::MotorLog& proto_motor_log, const MotorLog& motor_log) {
  proto_motor_log.set_status(static_cast<examples::proto::MotorStatus>(motor_log.status));
  proto_motor_log.set_current_amp(motor_log.current_amp);
  proto_motor_log.set_velocity_rps(motor_log.velocity_rps);
  proto_motor_log.set_error_message(motor_log.error_message);
  proto_motor_log.set_elapsed_time_ns(
      std::chrono::duration_cast<std::chrono::nanoseconds>(motor_log.elapsed_time).count());
}

inline void fromProto(const proto::MotorLog& proto_motor_log, MotorLog& motor_log) {
  auto status = magic_enum::enum_cast<MotorStatus>(static_cast<uint8_t>(proto_motor_log.status()));
  throwExceptionIf<InvalidDataException>(!status.has_value(), "failed to convert MotorStatus");
  motor_log.status = status.value();  // NOLINT(bugprone-unchecked-optional-access)
  motor_log.current_amp = proto_motor_log.current_amp();
  motor_log.velocity_rps = proto_motor_log.velocity_rps();
  motor_log.error_message = proto_motor_log.error_message();
  motor_log.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::nanoseconds{ proto_motor_log.elapsed_time_ns() });
}
}  // namespace telemetry::examples

}  // namespace heph
