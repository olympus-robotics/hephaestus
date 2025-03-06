
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/sequence.hpp>
#include <exec/sequence/ignore_all_values.hpp>
#include <exec/sequence/iterate.hpp>
#include <exec/sequence/transform_each.hpp>
#include <fmt/base.h>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/context.h"
#include "hephaestus/conduit/node_operation.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

namespace heph::conduit {

template <typename T, typename F>
struct AccumulatedInput {
  auto after(std::chrono::steady_clock::duration /**/) {
  }
  auto just() {
  }

  T data;
  F accumulator;
};

template <typename T, std::size_t QueueDepth>
struct AggregatedInput {
  auto after(std::chrono::steady_clock::duration /**/) {
  }
  auto just() {
  }

  std::array<T, QueueDepth> data{};
  std::size_t entries{ 0 };
};
}  // namespace heph::conduit

namespace {
void queuedInputTest() {
  heph::conduit::QueuedInput<int, 1> input1{ nullptr, "input1" };

  {
    auto [res] = stdexec::sync_wait(input1.just()).value_or(std::tuple<int>{ 0 });
    if (!res.has_value()) {
      fmt::println("No value, test OK");
    }
  }

  static constexpr int REFERENCE = 7;
  input1.setValue(REFERENCE);
  {
    auto [res] = stdexec::sync_wait(input1.just()).value_or(std::tuple<int>{ 0 });
    if (res.has_value() && *res == REFERENCE) {
      fmt::println("value {}, test OK", *res);
    }
  }
  {
    auto [res] = stdexec::sync_wait(input1.just()).value_or(std::tuple<int>{ 0 });
    if (!res.has_value()) {
      fmt::println("No value, test OK");
    }
  }
  input1.setValue(REFERENCE);
  if (input1.setValue(REFERENCE) != heph::conduit::InputState::OVERFLOW) {
    fmt::println("test failed, should be overflow\n");
  }

  static constexpr std::size_t QUEUE_DEPTH = 5;
  heph::conduit::QueuedInput<std::size_t, QUEUE_DEPTH> input2{ nullptr, "input2" };

  for (std::size_t i = 0; i < QUEUE_DEPTH * 2; ++i) {
    input2.setValue(i);
  }
  for (std::size_t i = 0; i < QUEUE_DEPTH; ++i) {
    auto [res] = stdexec::sync_wait(input2.just()).value_or(std::tuple<std::size_t>{ 0 });
    if (res.has_value() && *res == i) {
      fmt::println("value {}, test OK", *res);
    } else {
      fmt::println("value {}, test NOT OK {}", i, res);
    }
  }
  for (std::size_t i = 0; i < QUEUE_DEPTH; ++i) {
    auto [res] = stdexec::sync_wait(input2.just()).value_or(std::tuple<std::size_t>{ 0 });
    if (res.has_value()) {
      fmt::println("test failed, should be empty");
    }
  }
}
}  // namespace

namespace heph::conduit {
struct Sink : NodeOperation<Sink> {
  heph::conduit::QueuedInput<std::size_t> input1{ this, "input1" };
  heph::conduit::QueuedInput<std::string> input2{ this, "input2" };
  std::string_view name = "Sink";

  auto trigger(Context& /*context*/) {
    return stdexec::when_all(input1.just(), input2.just());
  }

  void operator()(std::optional<std::size_t> i1, std::optional<std::string> i2) const {
    fmt::println("sink process {} {}", i1, i2);
  }
};

struct SinkAll : NodeOperation<SinkAll> {
  heph::conduit::QueuedInput<std::size_t> input1{ this, "input1" };
  heph::conduit::QueuedInput<std::string> input2{ this, "input2" };
  std::string_view name = "SinkAll";

  auto trigger(Context& /*context*/) {
    return stdexec::when_all(input1.await(), input2.await());
  }

  void operator()(std::size_t i1, const std::string& i2) const {
    heph::log(heph::INFO, "sink", "input1", i1, "input2", i2);
  }
};
}  // namespace heph::conduit

namespace {
void processInputTest() {
  heph::conduit::Context context;
  heph::conduit::Sink s;

  stdexec::sync_wait(s.execute(context));

  static constexpr std::size_t REFERENCE = 96;
  s.input1.setValue(REFERENCE);
  stdexec::sync_wait(s.execute(context));

  s.input1.setValue(REFERENCE);
  s.input2.setValue(std::string("yayaya"));
  stdexec::sync_wait(s.execute(context));

  s.input2.setValue(std::string("buuh"));
  stdexec::sync_wait(s.execute(context));
}
void processInputBlockTest() {
  heph::conduit::Context context;
  heph::conduit::SinkAll s;

  static constexpr std::size_t REFERENCE = 97;
  s.input1.setValue(REFERENCE);
  s.input2.setValue(std::string("yuppie"));
  stdexec::sync_wait(s.execute(context));
}
}  // namespace
namespace heph::conduit {
template <typename F>
struct Generator : NodeOperation<Generator<F>, std::invoke_result_t<F>> {
  std::string_view name = "Generator";

  F generator;
  std::chrono::steady_clock::duration delay;
  auto trigger(Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() {
    return generator();
  }
};

template <typename F>
// NOLINTNEXTLINE(misc-use-internal-linkage) - clang-tidy thinks CTAD has linkage...
Generator(NodeOperation<F, std::invoke_result_t<F>>, std::string_view, F, std::chrono::steady_clock::duration)
    -> Generator<F>;
}  // namespace heph::conduit

namespace {
void processGeneratorTest() {
  for (const double time_scale_factor : { 0.0, 0.5, 1.0, 1.5, 2.0 }) {
    auto begin = std::chrono::steady_clock::now();
    heph::conduit::Context context{ { .time_scale_factor = time_scale_factor } };

    std::size_t count{ 0 };
    static constexpr std::size_t MAX_COUNT = 10;
    heph::conduit::Generator g1{ {},
                                 "g1",
                                 [&context, &count]() -> std::size_t {
                                   if (count == MAX_COUNT) {
                                     context.requestStop();
                                   }
                                   ++count;

                                   return count;
                                 },
                                 std::chrono::milliseconds(1) };
    heph::conduit::Generator g2{
      {}, "g2", []() -> std::string { return "dfsgfd"; }, std::chrono::milliseconds(1)
    };
    heph::conduit::SinkAll s;

    g1.connectTo(s.input1);
    g2.connectTo(s.input2);

    fmt::println("Running now!");

    // Any of the following can be used to intiate the run as we have a fully connected graph
    // in this example
    // g1.run(context);
    // g2.run(context);
    s.run(context);

    context.run();
    auto end = std::chrono::steady_clock::now();
    fmt::println("Run took {:.2}", std::chrono::duration<double>(end - begin));
  }
}

void scheduleTest() {
  heph::conduit::Context context{ { .time_scale_factor = 1.0 } };
  exec::async_scope scope;

  scope.spawn(context.scheduleAfter(std::chrono::seconds(1)) |
              stdexec::then([&context, start = std::chrono::steady_clock::now()] {
                fmt::println("scheduled after {:.2}...",
                             std::chrono::duration<double>(std::chrono::steady_clock::now() - start));
                context.requestStop();
              }));

  scope.spawn(context.schedule() | stdexec::then([] { fmt::println("scheduled..."); }));
  context.run();
}
}  // namespace

auto main() -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    scheduleTest();
    queuedInputTest();
    processInputTest();
    processInputBlockTest();
    processGeneratorTest();

  } catch (...) {
    fmt::println("unexcepted exception...");
  }
}
