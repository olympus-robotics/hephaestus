#include <map>
#include <print>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

using std::exception;
using std::vector, std::string;

namespace stdv = std::views;
namespace rng = std::ranges;

auto main() -> int {
  try {
    std::map<string, int> fm;
    fm["world"] = 2;
    fm["hello"] = 1;

    for (auto [k, v] : rng::reverse_view{ fm }) {
      std::print("{}:{}\n", k, v);
    }

    // std::print("{}\n", fm);  // {"hello": 1, "world": 2}

    std::vector<std::pair<string, int>> vv{ { "ciao", 1 }, { "boo", 2 } };

    // auto s = std::range_formatter::format("{}\n", a);
    for (auto [k, v] : rng::reverse_view{ vv }) {
      std::print("{}:{}\n", k, v);
    }
    // world:2
    // hello:1
  } catch (const std::exception& e) {
    std::exit(1);
  }
}
