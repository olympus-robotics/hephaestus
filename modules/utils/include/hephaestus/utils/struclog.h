#pragma once

#pragma once

#include <iomanip>
#include <source_location>
#include <sstream>
#include <string>
#include <vector>

namespace heph::utils::struclog {

/**
 * @brief A class that allows easy composition of logs for sturctured logging with spdlog.
 *        Example(see also struclog.cpp):
 *        namespace sl=struclog;
 *        sl::error(sl::Log("adding super-frame")("id", 12345)("tag", "test"));
 *
 *        logs
 *        'level=info file=struclog.h:123 time=2023-12-3T8:52:02+0 message="adding super-frame" id=12345
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
   * @brief General loginfo consumer, should be used like Log("my message")("field", 1234)
   *        Converted to string with stringstream.
   * @todo  When we update to cpp23, we can use operator[] that makes a clearer interface.
   *
   * @tparam T any type that is consumeable by stringstream
   * @param key identifier for the logging data
   * @param val value of the logging data
   * @return Log&&
   */
  template <typename T>
  auto operator()(const std::string& key, const T& val) -> Log&& {
    std::stringstream ss;
    ss << key << FIELD_SEPERATOR << val;
    logging_data_.emplace_back(ss.str());

    return std::move(*this);
  }

  /*
   * @brief Specialized loginfo consumer for string types so that they are quoted, should be used like Log("my
   * message")("field", "mystring")
   * @todo  When we update to cpp23, we can use operator[] that makes a clearer interface.
   *
   * @tparam T any type that is consumeable by stringstream
   * @param key identifier for the logging data
   * @param val value of the logging data
   * @return Log&&
   */
  template <std::convertible_to<std::string> S>
  auto operator()(const std::string& key, const S& val) -> Log&& {
    std::stringstream ss;
    // Pointer decay is wanted here to catch char[n] messages
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    ss << key << FIELD_SEPERATOR << std::quoted(val);
    logging_data_.emplace_back(ss.str());

    return std::move(*this);
  }

  [[nodiscard]] auto format() const -> std::string;

private:
  static void setPattern();

  std::vector<std::string> logging_data_;
  static constexpr char FIELD_SEPERATOR{ '=' };
  static constexpr char ELEMENT_SEPERATOR{ ' ' };
};

void init();

void trace(const Log& s);
void debug(const Log& s);
void info(const Log& s);
void warn(const Log& s);
void error(const Log& s);
void critical(const Log& s);
}  // namespace heph::utils::struclog