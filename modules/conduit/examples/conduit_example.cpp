
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/queued_input.h"

struct OperatorData {
  // possible data ...
};

struct Operator : heph::conduit::Node<Operator /*, OperatorData*/> {
  // inputs...
  heph::conduit::QueuedInput<std::string> input{ this, "input" };

  static auto name(const Operator& /*self*/) -> std::string_view {
    return "operator";
  };
  static auto period(const Operator& /*self*/) {};
  static auto trigger(const Operator& /*self*/) {
  }
  static auto execute(Operator& /*self*/, const std::string& input) {
    // Access to data:
    // self.data();
    // Access to engine (if required):
    // self.engine();
    (void)input;
  }
};

auto main() -> int {
  heph::conduit::NodeEngine engine{ {} };
  const auto& node = engine.createNode<Operator>(/* potentially initialize OperatorData */);

  node.input.connectTo(/*...*/);
}
