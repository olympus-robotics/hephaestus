//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <exec/repeat_effect_until.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log_sink.h"

namespace heph::conduit::detail {

template <typename T>
auto extractResult(T& t) -> T* {
  return &t;
}
template <typename T>
auto extractResult(std::optional<T>& t) -> T* {
  if (!t.has_value()) {
    return nullptr;
  }
  return &t.value();
}

class OutputConnections {
public:
  static constexpr std::string_view INPUT_OVERFLOW_WARNING =
      "Delaying Output operation because receiving input would overflow";
  auto propagate(NodeEngine& engine) {
    return stdexec::let_value([this, &engine]<typename... Ts>(Ts&&... ts) {
      // If the continuation didn't get any parameters, the operator
      // return void, and we can move on ...
      // Otherwise, we attempt to set the result to connected inputs.
      if constexpr (sizeof...(Ts) == 1) {
        // TODO: find better way to timeout based on the inputs timing...
        static constexpr std::array TIMEOUTS = {
          std::chrono::milliseconds(0),    std::chrono::milliseconds(100), std::chrono::milliseconds(200),
          std::chrono::milliseconds(400),  std::chrono::milliseconds(800), std::chrono::milliseconds(500),
          std::chrono::milliseconds(1600), std::chrono::milliseconds(3200)
        };
        auto args = std::make_tuple(std::forward<Ts>(ts)...);
        return exec::repeat_effect_until(
            stdexec::just(this) | stdexec::let_value([this, &engine](OutputConnections* self) {
              auto timeout = TIMEOUTS.at(self->retry_ % TIMEOUTS.size());

              if (retry_ > 0) {
                // TODO: add proper names
                heph::log(heph::WARN, std::string(INPUT_OVERFLOW_WARNING), "output", "dummy", "input",
                          "dummy", "retry", retry_, "delay", timeout);
              }
              return engine.scheduler().scheduleAfter(timeout);
            }) |
            stdexec::then([this, args = std::move(args)] {
              bool done = std::apply(
                  [this]<typename T>(T result) {
                    // Extract result: In case of an optional without a value, we
                    // get a nullptr and return immediately without propagating
                    // anything.
                    auto* result_ptr = extractResult(result);
                    if (result_ptr == nullptr) {
                      return true;
                    }

                    std::size_t propagated_count{ 0 };
                    for (auto& [input, set_value, generation] : inputs_) {
                      if (generation != generation_) {
                        ++propagated_count;
                        continue;
                      }
                      if (set_value(input, result_ptr) == InputState::OK) {
                        ++propagated_count;
                        ++generation;
                      }
                    }
                    if (propagated_count == inputs_.size()) {
                      // Everything is propagated, now reset...
                      generation_++;
                      retry_ = 0;
                      return true;
                    }
                    ++retry_;
                    return false;
                  },
                  args);
              return done;
            })

        );
      } else {
        (void)this;
        static_assert(sizeof...(Ts) == 0, "Node returning more than one output should be impossible");
        return stdexec::just();
      }
    });
  }

  template <typename Input>
  void registerInput(Input* input) {
    inputs_.emplace_back(
        input,
        [](void* input, void* ptr) {
          return static_cast<Input*>(input)->setValue(*static_cast<typename Input::ValueT*>(ptr));
        },
        0);
  }

private:
  using InputT = InputState (*)(void*, void*);
  std::vector<std::tuple<void*, InputT, std::size_t>> inputs_;
  std::size_t generation_{ 0 };
  std::size_t retry_{ 0 };
};
}  // namespace heph::conduit::detail
