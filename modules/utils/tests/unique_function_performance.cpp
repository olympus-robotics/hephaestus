#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <fmt/base.h>

#include "hephaestus/utils/unique_function.h"

struct Updateable {
  virtual ~Updateable() = default;
  virtual void update(float dt) = 0;
};

struct UpdateableA : Updateable {
  UpdateableA() = default;
  void update(float /**/) override {
    ++calls;
  }

  size_t calls{ 0 };
};
struct UpdateableB : Updateable {
  void update(float /**/) override {
    ++calls;
  }

  static size_t calls;
};
size_t UpdateableB::calls = 0;

struct LambdaA {
  template <typename F>
  explicit LambdaA(std::vector<F>& update_loop) {
    update_loop.emplace_back([this](float dt) { update(dt); });
  }
  void update(float /**/) {
    ++calls;
  }
  size_t calls{ 0 };
};

struct LambdaB {
  template <typename F>
  explicit LambdaB(std::vector<F>& update_loop) {
    update_loop.emplace_back([this](float dt) { update(dt); });
  }
  void update(float /*unused*/) {
    ++calls;
    (void)this;
  }
  static size_t calls;
};
size_t LambdaB::calls = 0;

struct ScopedMeasurer {
  ScopedMeasurer(const ScopedMeasurer&) = default;
  ScopedMeasurer(ScopedMeasurer&&) = delete;
  auto operator=(const ScopedMeasurer&) -> ScopedMeasurer& = default;
  auto operator=(ScopedMeasurer&&) -> ScopedMeasurer& = delete;
  explicit ScopedMeasurer(std::string name)
    : name(std::move(name)), clock(), before(std::chrono::high_resolution_clock::now()) {
  }
  ~ScopedMeasurer() {
    auto time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - before);
    fmt::println("{}: {}\n", name, time_spent);
  }
  std::string name;
  std::chrono::high_resolution_clock clock;
  std::chrono::time_point<std::chrono::high_resolution_clock> before;
};

static const size_t NUM_ALLOCATIONS = 1000;
#ifdef _DEBUG
static const size_t num_calls = 10000;
#else
static const size_t NUM_CALLS = 100000;
#endif
static const float NUMBER = 0.016f;
namespace {
void measureOnlyCall(const std::vector<Updateable*>& container) {
  const ScopedMeasurer measure("virtual function");
  for (size_t i = 0; i < NUM_CALLS; ++i) {
    for (const auto& updateable : container) {
      updateable->update(NUMBER);
    }
  }
}

void timeVirtual(unsigned seed) {
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> random_int(0, 1);
  std::vector<std::unique_ptr<Updateable>> updateables;
  std::vector<Updateable*> update_loop;
  for (size_t i = 0; i < NUM_ALLOCATIONS; ++i) {
    if (random_int(generator) != 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      updateables.emplace_back(new UpdateableA);
    } else {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      updateables.emplace_back(new UpdateableB);
    }
    update_loop.push_back(updateables.back().get());
  }
  measureOnlyCall(update_loop);
}

template <typename Container>
void measureOnlyCall(const Container& container, const std::string& name) {
  const ScopedMeasurer measure(name);
  for (size_t i = 0; i < NUM_CALLS; ++i) {
    for (auto& updateable : container) {
      updateable(NUMBER);
    }
  }
}

template <typename FunctionT>
void timeStdFunction(unsigned seed, const std::string& name) {
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> random_int(0, 1);
  std::vector<FunctionT> update_loop;
  std::vector<std::unique_ptr<LambdaA>> slots_a;
  std::vector<std::unique_ptr<LambdaB>> slots_b;
  for (size_t i = 0; i < NUM_ALLOCATIONS; ++i) {
    if (random_int(generator)) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      slots_a.emplace_back(new LambdaA(update_loop));
    } else {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      slots_b.emplace_back(new LambdaB(update_loop));
    }
  }
  measureOnlyCall(update_loop, name);
}
}  // namespace

auto main(int argc, char** /*argv*/) -> int {
  try {
    auto seed = static_cast<unsigned>(argc);
    timeVirtual(seed);
    timeStdFunction<heph::UniqueFunction<void(float)>>(seed, "heph::UniqueFunction");
    timeStdFunction<std::function<void(float)>>(seed, "std::function");
  } catch (...) {
    fmt::println(stderr, "Unknown failure");
    return 1;
  }
}
