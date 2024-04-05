#include "hephaestus/utils/string_utils.h"

namespace heph::utils {

// constexpr auto truncate(std::string_view str, std::string_view start_token,
//                         std::string_view end_token /* = std::string_view("") */) -> std::string_view {
//   const auto start_pos = str.find(start_token);
//   const auto end_pos = end_token.empty() ? std::string_view::npos : str.find(end_token);
//   return (start_pos != std::string_view::npos) ? str.substr(start_pos, end_pos - start_pos) :
//                                                  str.substr(0, end_pos);
// }

}  // namespace heph::utils
