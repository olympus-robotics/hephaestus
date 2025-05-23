
#pragma once

#include <string>

namespace heph::conduit::detail {
class NodeBase {
public:
  virtual ~NodeBase() = default;
  [[nodiscard]] virtual auto nodeName() const -> std::string = 0;
};

}  // namespace heph::conduit::detail
