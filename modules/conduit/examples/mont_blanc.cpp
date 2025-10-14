//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/base.h>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/format/generic_formatter.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"
#include "hephaestus/types_proto/numeric_value.h"
#include "hephaestus/types_proto/string.h"
#include "hephaestus/utils/signal_handler.h"

namespace mont_blanc {

template <typename T>
class RandomData {
public:
  [[nodiscard]] auto generate() -> T {
    return distribution_(gen_);
  }

private:
  std::random_device rd_;
  std::mt19937 gen_{ rd_() };

  std::conditional_t<std::is_floating_point_v<T>, std::uniform_real_distribution<T>,
                     std::uniform_int_distribution<T>>
      distribution_;
};
struct Cordoba : heph::conduit::Node<Cordoba, RandomData<float>> {
  static constexpr auto NAME = "cordoba";
  static constexpr auto PERIOD = std::chrono::milliseconds(100);

  static auto execute(Cordoba& self) -> float {
    return self.data().generate();
  }
};

struct Lyon : heph::conduit::Node<Lyon> {
  static auto name() -> std::string_view {
    return "lyon";
  }

  heph::conduit::QueuedInput<float> amazon{ this, "amazon" };

  static auto trigger(Lyon& self) {
    return self.amazon.get();
  }

  static auto execute(float f) -> float {
    heph::log(heph::INFO, "lyon", "amazon", f);
    return f;
  }
};

struct Freeport : heph::conduit::Node<Freeport, RandomData<std::int64_t>> {
  static constexpr auto PERIOD = std::chrono::milliseconds(50);

  static auto name() -> std::string_view {
    return "freeport";
  }

  static auto execute(Freeport& self) -> std::int64_t {
    return self.data().generate();
  }
};

struct Medelin : heph::conduit::Node<Medelin, RandomData<std::int32_t>> {
  static auto name() -> std::string_view {
    return "medelin";
  }

  static constexpr auto PERIOD = std::chrono::milliseconds(10);

  static auto execute(Medelin& self) -> std::int32_t {
    return self.data().generate();
  }
};

struct Portsmouth : heph::conduit::Node<Portsmouth, RandomData<std::int32_t>> {
  static auto name() -> std::string_view {
    return "portsmouth";
  }

  static constexpr auto PERIOD = std::chrono::milliseconds(200);

  static auto execute(Portsmouth& self) -> std::string {
    return fmt::format("0x{:x}{:x}{:x}{:x}", self.data().generate(), self.data().generate(),
                       self.data().generate(), self.data().generate());
  }
};

struct Delhi : heph::conduit::Node<Delhi, RandomData<std::int32_t>> {
  static auto name() -> std::string_view {
    return "delhi";
  }

  static constexpr auto PERIOD = std::chrono::seconds(1);

  static auto execute(Delhi& self) -> heph::types::DummyType {
    heph::types::DummyType dummy;
    dummy.dummy_primitives_type.dummy_int32_t = self.data().generate();

    return dummy;
  }
};

struct Hamburg : heph::conduit::Node<Hamburg> {
  static auto name() -> std::string_view {
    return "hamburg";
  }

  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<float, InputPolicyT> tigris{ this, "tigris" };
  heph::conduit::QueuedInput<std::int64_t, InputPolicyT> ganges{ this, "ganges" };
  heph::conduit::QueuedInput<std::int32_t, InputPolicyT> nile{ this, "nile" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };

  static auto trigger(Hamburg& self) {
    return stdexec::when_all(self.tigris.get(), self.ganges.get(), self.nile.get(), self.danube.get());
  }

  static auto execute(std::optional<float> t, std::optional<std::int64_t> g, std::optional<std::int32_t> n,
                      std::string d) -> std::string {
    heph::log(heph::INFO, "hamburg", "tigris", t, "ganges", g, "nile", n, "danube", d);

    return fmt::format("hamburg/parana:{}", d);
  }
};

struct Taipei : heph::conduit::Node<Taipei> {
  static auto name() -> std::string_view {
    return "taipei";
  }

  heph::conduit::QueuedInput<heph::types::DummyType> columbia{ this, "columbia" };

  static auto trigger(Taipei& self) {
    return self.columbia.get();
  }

  static auto execute(heph::types::DummyType image) {
    return image;
  }
};

struct Osaka : heph::conduit::Node<Osaka> {
  static auto name() -> std::string_view {
    return "osaka";
  }

  heph::conduit::QueuedInput<std::string, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                                     heph::conduit::SetMethod::OVERWRITE>>
      parana{ this, "parana" };
  heph::conduit::QueuedInput<heph::types::DummyType> colorado{ this, "colorado" };
  heph::conduit::QueuedInput<heph::types::DummyType,
                             heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL>>
      columbia{ this, "columbia" };

  heph::conduit::Output<heph::types::DummyType> salween{ this, "salween" };
  heph::conduit::Output<heph::types::DummyPrimitivesType> godavari{ this, "godawari" };

  static auto trigger(Osaka& self) {
    return stdexec::when_all(self.parana.get(), self.colorado.get(), self.columbia.get());
  }

  static auto execute(Osaka& self, const std::optional<std::string>& s, const heph::types::DummyType& /**/,
                      const std::optional<heph::types::DummyType>& /**/) {
    heph::log(heph::INFO, "osaka", "parana", s);
    return stdexec::when_all(self.salween.setValue(self.engine(), heph::types::DummyType{}),
                             self.godavari.setValue(self.engine(), heph::types::DummyPrimitivesType{}));
  }
};

struct Hebron : heph::conduit::Node<Hebron> {
  static auto name() -> std::string_view {
    return "hebron";
  }

  static constexpr auto PERIOD = std::chrono::milliseconds(100);

  static auto execute() {
    return size_t{};
  }
};

struct Kingston : heph::conduit::Node<Kingston> {
  static auto name() -> std::string_view {
    return "kingston";
  }

  static constexpr auto PERIOD = std::chrono::milliseconds(100);

  static auto execute() {
    return std::string{};
  }
};

struct Tripoli : heph::conduit::Node<Tripoli> {
  static auto name() -> std::string_view {
    return "tripoli";
  }

  heph::conduit::QueuedInput<heph::types::DummyPrimitivesType> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<heph::types::DummyType,
                             heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                        heph::conduit::SetMethod::OVERWRITE>>
      columbia{ this, "columbia" };

  static auto trigger(Tripoli& self) {
    return stdexec::when_all(self.godavari.get(), self.columbia.get());
  }

  static auto execute(const heph::types::DummyPrimitivesType& /*scan*/,
                      const std::optional<heph::types::DummyType>& /*image*/) {
    return heph::types::DummyType{};
  }
};

struct Mandalay : heph::conduit::Node<Mandalay> {
  static auto name() -> std::string_view {
    return "mandalay";
  }

  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };
  heph::conduit::QueuedInput<size_t> chenab{ this, "chenab" };
  heph::conduit::QueuedInput<heph::types::DummyType> salween{ this, "salween" };
  heph::conduit::QueuedInput<heph::types::DummyPrimitivesType> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<std::string> yamuna{ this, "yamuna" };
  heph::conduit::QueuedInput<heph::types::DummyType> loire{ this, "loire" };

  heph::conduit::Output<int16_t> tagus{ this, "tagus" };
  heph::conduit::Output<heph::types::DummyType> missouri{ this, "missouri" };
  heph::conduit::Output<heph::types::DummyType> brazos{ this, "brazos" };

  static constexpr auto PERIOD = std::chrono::milliseconds(100);

  static auto execute(Mandalay& self) {
    heph::log(heph::INFO, "mandalay",
              //
              "danube", self.danube.getValue(),
              //
              "chenab", fmt::format("{}", self.chenab.getValue()),
              //
              "salween", fmt::format("{}", self.salween.getValue()),
              //
              "godavari", fmt::format("{}", self.godavari.getValue()),
              //
              "yamuna", fmt::format("{}", self.yamuna.getValue()),
              //
              "loire", fmt::format("{}", self.loire.getValue())
              //
    );
    return stdexec::when_all(self.tagus.setValue(self.engine(), int16_t{}),
                             self.missouri.setValue(self.engine(), heph::types::DummyType{}),
                             self.brazos.setValue(self.engine(), heph::types::DummyType{}));
  }
};

struct Ponce : heph::conduit::Node<Ponce> {
  static auto name() -> std::string_view {
    return "ponce";
  }

  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::BLOCK,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<int16_t, InputPolicyT> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<std::string, InputPolicyT> danube{ this, "danube" };
  heph::conduit::QueuedInput<heph::types::DummyType, InputPolicyT> missouri{ this, "missouri" };
  heph::conduit::QueuedInput<heph::types::DummyType> brazos{ this, "brazos" };
  heph::conduit::QueuedInput<std::string, InputPolicyT> yamuna{ this, "yamuna" };
  heph::conduit::QueuedInput<heph::types::DummyPrimitivesType, InputPolicyT> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<heph::types::DummyType, InputPolicyT> loire{ this, "loire" };
  heph::conduit::QueuedInput<float, InputPolicyT> ohio{ this, "ohio" };
  heph::conduit::QueuedInput<double, InputPolicyT> volga{ this, "volga" };

  heph::conduit::Output<int64_t> congo{ this, "congo" };
  heph::conduit::Output<int8_t> meckong{ this, "meckong" };

  static auto trigger(Ponce& self) {
    return self.brazos.get();
  }

  static auto execute(Ponce& self, const heph::types::DummyType& cloud) {
    heph::log(heph::INFO, "ponce",
              //
              "tagus", self.tagus.getValue(),
              //
              "danube", self.danube.getValue(),
              //
              "missouri", self.missouri.getValue(),
              //
              "brazos", cloud,
              //
              "yamuna", self.yamuna.getValue(),
              //
              "godavari", self.godavari.getValue(),
              //
              "loire", self.loire.getValue(),
              //
              "ohio", self.ohio.getValue(),
              //
              "volga", self.volga.getValue()
              //
    );
    return stdexec::when_all(self.congo.setValue(self.engine(), int64_t{}),
                             self.meckong.setValue(self.engine(), int8_t{}));
  }
};

struct Geneva : heph::conduit::Node<Geneva> {
  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  static auto name() -> std::string_view {
    return "geneva";
  }

  heph::conduit::QueuedInput<std::string> parana{ this, "parana" };
  heph::conduit::QueuedInput<std::string, InputPolicyT> danube{ this, "danube" };
  heph::conduit::QueuedInput<int16_t, InputPolicyT> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<int64_t, InputPolicyT> congo{ this, "congo" };

  static auto trigger(Geneva& self) {
    return self.parana.get();
  }

  static auto execute(Geneva& self, const std::string& s) {
    heph::log(heph::INFO, "geneva",
              //
              "parana", s,
              //
              "danube", self.danube.getValue(),
              //
              "tagus", self.tagus.getValue(),
              //
              "congo", self.congo.getValue()
              //
    );
    return std::string{};
  }
};

struct Monaco : heph::conduit::Node<Monaco> {
  static auto name() -> std::string_view {
    return "monaco";
  }

  heph::conduit::QueuedInput<int64_t> congo{ this, "congo" };

  static auto trigger(Monaco& self) {
    return self.congo.get();
  }

  static auto execute(int64_t /**/) {
    return float{};
  }
};

struct Rotterdam : heph::conduit::Node<Rotterdam> {
  static auto name() -> std::string_view {
    return "rotterdam";
  }

  heph::conduit::QueuedInput<int8_t> meckong{ this, "meckong" };

  static auto trigger(Rotterdam& self) {
    return self.meckong.get();
  }

  static auto execute(int8_t /**/) {
    return uint64_t{};
  }
};

struct Barcelona : heph::conduit::Node<Barcelona> {
  static auto name() -> std::string_view {
    return "barcelona";
  }

  heph::conduit::QueuedInput<int8_t> meckong{ this, "meckong" };

  static auto trigger(Barcelona& self) {
    return self.meckong.get();
  }

  static auto execute(int8_t /**/) {
    return uint16_t{};
  }
};

struct Arequipa : heph::conduit::Node<Arequipa> {
  static auto name() -> std::string_view {
    return "arequipa";
  }

  heph::conduit::QueuedInput<std::string> arkansas{ this, "arkansas" };

  static auto trigger(Arequipa& self) {
    return self.arkansas.get();
  }

  static auto execute(const std::string& /**/) {
  }
};

struct Georgetown : heph::conduit::Node<Georgetown> {
  static auto name() -> std::string_view {
    return "georgetown";
  }
  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::BLOCK,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<uint64_t, InputPolicyT> murray{ this, "murray" };
  heph::conduit::QueuedInput<uint16_t, InputPolicyT> lena{ this, "lena" };

  static constexpr auto PERIOD = std::chrono::milliseconds(50);

  static auto trigger(Georgetown& self) {
    return stdexec::when_all(self.murray.get(), self.lena.get());
  }

  static auto execute(uint64_t m, uint16_t l) {
    heph::log(heph::INFO, "georgetown", "murray", m, "lena", l);

    return double{};
  }
};
}  // namespace mont_blanc

auto main() -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
    heph::conduit::NodeEngine engine{ {} };

    auto cordoba = engine.createNode<mont_blanc::Cordoba>();

    auto lyon = engine.createNode<mont_blanc::Lyon>();
    auto freeport = engine.createNode<mont_blanc::Freeport>();
    auto medellin = engine.createNode<mont_blanc::Medelin>();
    auto portsmouth = engine.createNode<mont_blanc::Portsmouth>();
    auto delhi = engine.createNode<mont_blanc::Delhi>();

    lyon->amazon.connectTo(cordoba);

    auto hamburg = engine.createNode<mont_blanc::Hamburg>();
    auto taipei = engine.createNode<mont_blanc::Taipei>();

    hamburg->tigris.connectTo(lyon);
    hamburg->ganges.connectTo(freeport);
    hamburg->nile.connectTo(medellin);
    hamburg->danube.connectTo(portsmouth);

    taipei->columbia.connectTo(delhi);

    auto osaka = engine.createNode<mont_blanc::Osaka>();

    osaka->parana.connectTo(hamburg);
    osaka->colorado.connectTo(taipei);
    osaka->columbia.connectTo(delhi);

    auto hebron = engine.createNode<mont_blanc::Hebron>();
    auto kingston = engine.createNode<mont_blanc::Kingston>();
    auto tripoli = engine.createNode<mont_blanc::Tripoli>();

    tripoli->godavari.connectTo(osaka->godavari);
    tripoli->columbia.connectTo(delhi);

    auto mandalay = engine.createNode<mont_blanc::Mandalay>();

    mandalay->danube.connectTo(portsmouth);
    mandalay->chenab.connectTo(hebron);
    mandalay->salween.connectTo(osaka->salween);
    mandalay->godavari.connectTo(osaka->godavari);
    mandalay->yamuna.connectTo(kingston);
    mandalay->loire.connectTo(tripoli);

    auto ponce = engine.createNode<mont_blanc::Ponce>();

    ponce->tagus.connectTo(mandalay->tagus);
    ponce->danube.connectTo(portsmouth);
    ponce->missouri.connectTo(mandalay->missouri);
    ponce->brazos.connectTo(mandalay->brazos);
    ponce->yamuna.connectTo(kingston);
    ponce->godavari.connectTo(osaka->godavari);
    ponce->loire.connectTo(tripoli);

    auto geneva = engine.createNode<mont_blanc::Geneva>();
    auto monaco = engine.createNode<mont_blanc::Monaco>();
    auto rotterdam = engine.createNode<mont_blanc::Rotterdam>();
    auto barcelona = engine.createNode<mont_blanc::Barcelona>();

    geneva->parana.connectTo(hamburg);
    geneva->danube.connectTo(portsmouth);
    geneva->tagus.connectTo(mandalay->tagus);
    geneva->congo.connectTo(ponce->congo);

    monaco->congo.connectTo(ponce->congo);

    rotterdam->meckong.connectTo(ponce->meckong);
    barcelona->meckong.connectTo(ponce->meckong);

    ponce->ohio.connectTo(monaco);

    auto arequipa = engine.createNode<mont_blanc::Arequipa>();
    auto georgetown = engine.createNode<mont_blanc::Georgetown>();

    arequipa->arkansas.connectTo(geneva);
    georgetown->murray.connectTo(rotterdam);
    georgetown->lena.connectTo(barcelona);

    ponce->volga.connectTo(georgetown);

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine]() { engine.requestStop(); });

    const auto dot_graph = engine.getDotGraph();
    fmt::println("Dot graph:\n-------\n{}\n-------\n", dot_graph);
    engine.run();

  } catch (...) {
    fmt::println("unexpected exception...");
  }
}
