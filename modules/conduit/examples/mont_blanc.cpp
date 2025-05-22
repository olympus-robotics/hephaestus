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
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

namespace mont_blanc {
// NOLINTBEGIN(readability-identifier-naming,misc-use-internal-linkage)
struct Image {
  std::int32_t width;
  std::int32_t height;
};
auto format_as(Image image) {
  return fmt::format("Image(width={}, height={})", image.width, image.height);
}
struct PointCloud2 {};
auto format_as(PointCloud2 /**/) {
  return fmt::format("PointCloud2()");
}
struct LaserScan {};
auto format_as(LaserScan /**/) {
  return fmt::format("LaserScan()");
}
struct Quaternion {};
auto format_as(Quaternion /**/) {
  return fmt::format("Quaternion()");
}
struct Vector3 {};
auto format_as(Vector3 /**/) {
  return fmt::format("Vector3()");
}
struct Pose {};
auto format_as(Pose /**/) {
  return fmt::format("Pose()");
}
struct Twist {};
auto format_as(Twist /**/) {
  return fmt::format("Twist()");
}
struct TwistWithCovarianceStamped {};
auto format_as(TwistWithCovarianceStamped /**/) {
  return fmt::format("TwistWithCovarianceStamped()");
}
struct Vector3Stamped {};
auto format_as(Vector3Stamped /**/) {
  return fmt::format("Vector3Stamped()");
}
struct WrenchStamped {};
auto format_as(WrenchStamped /**/) {
  return fmt::format("WrenchStamped()");
}
// NOLINTEND(readability-identifier-naming,misc-use-internal-linkage)

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

  static auto execute(Delhi& self) -> Image {
    return { .width = self.data().generate(), .height = self.data().generate() };
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

  heph::conduit::QueuedInput<Image> columbia{ this, "columbia" };

  static auto trigger(Taipei& self) {
    return self.columbia.get();
  }

  static auto execute(Image image) {
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
  heph::conduit::QueuedInput<Image> colorado{ this, "colorado" };
  heph::conduit::QueuedInput<Image, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL>>
      columbia{ this, "columbia" };

  heph::conduit::Output<PointCloud2> salween{ this, "salween" };
  heph::conduit::Output<LaserScan> godavari{ this, "godawari" };

  static auto trigger(Osaka& self) {
    return stdexec::when_all(self.parana.get(), self.colorado.get(), self.columbia.get());
  }

  static auto execute(Osaka& self, std::optional<std::string> const& s, Image /**/,
                      std::optional<Image> /**/) {
    heph::log(heph::INFO, "osaka", "parana", s);
    return stdexec::when_all(self.salween.setValue(self.engine(), PointCloud2{}),
                             self.godavari.setValue(self.engine(), LaserScan{}));
  }
};

struct Hebron : heph::conduit::Node<Hebron> {
  static auto name() -> std::string_view {
    return "hebron";
  }

  static constexpr auto PERIOD = std::chrono::milliseconds(100);

  static auto execute() {
    return Quaternion{};
  }
};

struct Kingston : heph::conduit::Node<Kingston> {
  static auto name() -> std::string_view {
    return "kingston";
  }

  static constexpr auto PERIOD = std::chrono::milliseconds(100);

  static auto execute() {
    return Vector3{};
  }
};

struct Tripoli : heph::conduit::Node<Tripoli> {
  static auto name() -> std::string_view {
    return "tripoli";
  }

  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<Image, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                               heph::conduit::SetMethod::OVERWRITE>>
      columbia{ this, "columbia" };

  static auto trigger(Tripoli& self) {
    return stdexec::when_all(self.godavari.get(), self.columbia.get());
  }

  static auto execute(LaserScan /*scan*/, std::optional<Image> /*image*/) {
    return PointCloud2{};
  }
};

struct Mandalay : heph::conduit::Node<Mandalay> {
  static auto name() -> std::string_view {
    return "mandalay";
  }

  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };
  heph::conduit::QueuedInput<Quaternion> chenab{ this, "chenab" };
  heph::conduit::QueuedInput<PointCloud2> salween{ this, "salween" };
  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<Vector3> yamuna{ this, "yamuna" };
  heph::conduit::QueuedInput<PointCloud2> loire{ this, "loire" };

  heph::conduit::Output<Pose> tagus{ this, "tagus" };
  heph::conduit::Output<Image> missouri{ this, "missouri" };
  heph::conduit::Output<PointCloud2> brazos{ this, "brazos" };

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
    return stdexec::when_all(self.tagus.setValue(self.engine(), Pose{}),
                             self.missouri.setValue(self.engine(), Image{}),
                             self.brazos.setValue(self.engine(), PointCloud2{}));
  }
};

struct Ponce : heph::conduit::Node<Ponce> {
  static auto name() -> std::string_view {
    return "ponce";
  }

  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::BLOCK,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<Pose, InputPolicyT> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<std::string, InputPolicyT> danube{ this, "danube" };
  heph::conduit::QueuedInput<Image, InputPolicyT> missouri{ this, "missouri" };
  heph::conduit::QueuedInput<PointCloud2> brazos{ this, "brazos" };
  heph::conduit::QueuedInput<Vector3, InputPolicyT> yamuna{ this, "yamuna" };
  heph::conduit::QueuedInput<LaserScan, InputPolicyT> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<PointCloud2, InputPolicyT> loire{ this, "loire" };
  heph::conduit::QueuedInput<float, InputPolicyT> ohio{ this, "ohio" };
  heph::conduit::QueuedInput<double, InputPolicyT> volga{ this, "volga" };

  heph::conduit::Output<Twist> congo{ this, "congo" };
  heph::conduit::Output<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  static auto trigger(Ponce& self) {
    return self.brazos.get();
  }

  static auto execute(Ponce& self, PointCloud2 cloud) {
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
    return stdexec::when_all(self.congo.setValue(self.engine(), Twist{}),
                             self.meckong.setValue(self.engine(), TwistWithCovarianceStamped{}));
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
  heph::conduit::QueuedInput<Pose, InputPolicyT> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<Twist, InputPolicyT> congo{ this, "congo" };

  static auto trigger(Geneva& self) {
    return self.parana.get();
  }

  static auto execute(Geneva& self, std::string const& s) {
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

  heph::conduit::QueuedInput<Twist> congo{ this, "congo" };

  static auto trigger(Monaco& self) {
    return self.congo.get();
  }

  static auto execute(Twist /**/) {
    return float{};
  }
};

struct Rotterdam : heph::conduit::Node<Rotterdam> {
  static auto name() -> std::string_view {
    return "rotterdam";
  }

  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  static auto trigger(Rotterdam& self) {
    return self.meckong.get();
  }

  static auto execute(TwistWithCovarianceStamped /**/) {
    return Vector3Stamped{};
  }
};

struct Barcelona : heph::conduit::Node<Barcelona> {
  static auto name() -> std::string_view {
    return "barcelona";
  }

  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  static auto trigger(Barcelona& self) {
    return self.meckong.get();
  }

  static auto execute(TwistWithCovarianceStamped /**/) {
    return WrenchStamped{};
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

  static auto execute(std::string const& /**/) {
  }
};

struct Georgetown : heph::conduit::Node<Georgetown> {
  static auto name() -> std::string_view {
    return "georgetown";
  }
  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::BLOCK,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<Vector3Stamped, InputPolicyT> murray{ this, "murray" };
  heph::conduit::QueuedInput<WrenchStamped, InputPolicyT> lena{ this, "lena" };

  static constexpr auto PERIOD = std::chrono::milliseconds(50);

  static auto trigger(Georgetown& self) {
    return stdexec::when_all(self.murray.get(), self.lena.get());
  }

  static auto execute(Vector3Stamped m, WrenchStamped l) {
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

    engine.run();

  } catch (...) {
    fmt::println("unexcepted exception...");
  }
}
