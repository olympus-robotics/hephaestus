#pragma once

#include <iomanip>
#include <source_location>
#include <sstream>
#include <string>
#include <vector>

namespace heph::telemetry {

static constexpr std::string_view LOG_PATTERN{ "level=%^%l%$ time=%Y-%m-%dT%H:%M:%S%z %v" };

template <typename T>
concept NotQuotable = !std::convertible_to<T, std::string> && requires(std::ostream& os, T t) {
  { os << t } -> std::same_as<std::ostream&>;
};

template <typename T>
concept Quotable = std::convertible_to<T, std::string>;

template <typename T>
struct Field final {
  std::string key;
  T val;
};

/**
 * @brief A class that allows easy composition of logs for structured logging.
 *        Example(see also struclog.cpp):
 *        namespace ht=heph::telemetry;
 *        LOG(INFO) << ht::Log{"adding"} | "id"_f(12345) | "tag"_f("test");
 *
 *        logs
 *        'level=info file=struclog.h:123 time=2023-12-3T8:52:02+0 message="adding" id=12345
 * tag="test"'
 *
 *        Remark: We would like to use libfmt with quoted formatting as proposed in
 *                https://github.com/fmtlib/fmt/issues/825 once its included (instead of sstream).
 *
 */
class Log {
public:
  explicit Log(const std::string& msg, std::source_location location = std::source_location::current());

  /*
   * @brief General loginfo consumer, should be used like Log("my message") | "field"_f(1234)
   *        Converted to string with stringstream.
   *
   * @tparam T any type that is consumeable by stringstream
   * @param key identifier for the logging data
   * @param val value of the logging data
   * @return Log&&
   */
  template <NotQuotable T>
  auto operator|(const Field<T>& field) -> Log&& {
    std::stringstream ss;
    ss << field.key << FIELD_SEPERATOR << field.val;
    logging_data_.emplace_back(ss.str());

    return std::move(*this);
  }

  /*
   * @brief Specialized loginfo consumer for string types so that they are quoted, should be used like
   * Log("my message") | "field"_f("mystring")
   *
   * @tparam T any type that is consumeable by stringstream
   * @param key identifier for the logging data
   * @param val value of the logging data
   * @return Log&&
   */
  template <Quotable S>
  auto operator|(const Field<S>& field) -> Log&& {
    std::stringstream ss;
    // Pointer decay is wanted here to catch char[n] messages
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    ss << field.key << FIELD_SEPERATOR << std::quoted(field.val);
    logging_data_.emplace_back(ss.str());

    return std::move(*this);
  }

  [[nodiscard]] auto format() const -> std::string;

private:
  std::vector<std::string> logging_data_;
  static constexpr char FIELD_SEPERATOR{ '=' };
  static constexpr char ELEMENT_SEPERATOR{ ' ' };
};

inline auto operator<<(std::ostream& os, const Log& obj) -> std::ostream& {
  os << obj.format();

  return os;
}

}  // namespace heph::telemetry

namespace heph::telemetry::literals {

// User-defined string literal to enable
// Field f="data"_f(mydata);
constexpr auto operator""_f(const char* key, [[maybe_unused]] size_t _) {
  return [key](auto value) { return heph::telemetry::Field{ key, value }; };
}
}  // namespace heph::telemetry::literals
