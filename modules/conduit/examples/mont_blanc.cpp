//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include <exec/when_any.hpp>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
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
[[maybe_unused]] auto format_as(Image image) {
  return fmt::format("Image(width={}, height={})", image.width, image.height);
}
struct PointCloud2 {};
[[maybe_unused]] auto format_as(PointCloud2 /**/) {
  return fmt::format("PointCloud2()");
}
struct LaserScan {};
[[maybe_unused]] auto format_as(LaserScan /**/) {
  return fmt::format("LaserScan()");
}
struct Quaternion {};
[[maybe_unused]] auto format_as(Quaternion /**/) {
  return fmt::format("Quaternion()");
}
struct Vector3 {};
[[maybe_unused]] auto format_as(Vector3 /**/) {
  return fmt::format("Vector3()");
}
struct Pose {};
[[maybe_unused]] auto format_as(Pose /**/) {
  return fmt::format("Pose()");
}
struct Twist {};
[[maybe_unused]] auto format_as(Twist /**/) {
  return fmt::format("Twist()");
}
struct TwistWithCovarianceStamped {};
[[maybe_unused]] auto format_as(TwistWithCovarianceStamped /**/) {
  return fmt::format("TwistWithCovarianceStamped()");
}
struct Vector3Stamped {};
[[maybe_unused]] auto format_as(Vector3Stamped /**/) {
  return fmt::format("Vector3Stamped()");
}
struct WrenchStamped {};
[[maybe_unused]] auto format_as(WrenchStamped /**/) {
  return fmt::format("WrenchStamped()");
}
// NOLINTEND(readability-identifier-naming,misc-use-internal-linkage)

struct Cordoba : heph::conduit::Node<Cordoba> {
  std::string_view name = "cordoba";

  static auto period() {
    return std::chrono::milliseconds(100);
  }

  auto operator()() -> float {
    return distribution(gen);
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_real_distribution<float> distribution{ 0.0f, 10.0f };
};

struct Lyon : heph::conduit::Node<Lyon> {
  std::string_view name = "lyon";
  heph::conduit::QueuedInput<float> amazon{ this, "/amazon" };

  auto trigger() {
    return amazon.get();
  }

  auto operator()(float f) -> float {
    heph::log(heph::INFO, "lyon", "/amazon", f);
    return f;
  }
};

struct Freeport : heph::conduit::Node<Freeport> {
  std::string_view name = "freeport";

  static auto period() {
    return std::chrono::milliseconds(50);
  }

  auto operator()() -> std::int64_t {
    return distribution(gen);
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int64_t> distribution{ 0, 10 };
};

struct Medelin : heph::conduit::Node<Medelin> {
  std::string_view name = "medelin";

  static auto period() {
    return std::chrono::milliseconds(10);
  }

  auto operator()() -> std::int32_t {
    return distribution(gen);
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int32_t> distribution{ 0, 10 };
};

struct Portsmouth : heph::conduit::Node<Portsmouth> {
  std::string_view name = "portsmouth";

  static auto period() {
    return std::chrono::milliseconds(200);
  }

  auto operator()() -> std::string {
    return fmt::format("0x{:x}{:x}{:x}{:x}", distribution(gen), distribution(gen), distribution(gen),
                       distribution(gen));
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int32_t> distribution{ 0, 10 };
};

struct Delhi : heph::conduit::Node<Delhi> {
  std::string_view name = "delhi";

  static auto period() {
    return std::chrono::seconds(1);
  }

  auto operator()() -> Image {
    return { .width = distribution(gen), .height = distribution(gen) };
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int32_t> distribution{ 0, 10 };
};

struct Hamburg : heph::conduit::Node<Hamburg> {
  std::string_view name = "hamburg";

  heph::conduit::QueuedInput<float> tigris{ this, "tigris" };
  heph::conduit::QueuedInput<std::int64_t, heph::conduit::InputPolicy<2>> ganges{ this, "ganges" };
  heph::conduit::QueuedInput<std::int32_t, heph::conduit::InputPolicy<10>> nile{ this, "nile" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };

  auto trigger() {
    return exec::when_any(tigris.get(), ganges.get(), nile.get()) | stdexec::let_value([this](auto value) {
             return stdexec::when_all(danube.get(), stdexec::just(value));
           });
  }

  template <typename T>
  auto operator()(std::string d, T value) -> std::string {
    if constexpr (std::is_same_v<T, float>) {
      tigris_last_value = value;
    }
    if constexpr (std::is_same_v<T, std::int64_t>) {
      ganges_last_value = value;
    }
    if constexpr (std::is_same_v<T, std::int32_t>) {
      nile_last_value = value;
    }

    heph::log(heph::INFO, "hamburg", "/tigris", tigris_last_value, "/ganges", ganges_last_value, "/nile",
              nile_last_value, "/danube", d);

    return fmt::format("hamburg/parana:{}", d);
  }
  float tigris_last_value{};
  std::int64_t ganges_last_value{};
  std::int32_t nile_last_value{};
};

struct Taipei : heph::conduit::Node<Taipei> {
  std::string_view name = "taipei";
  heph::conduit::QueuedInput<Image> columbia{ this, "columbia" };

  auto trigger() {
    return columbia.get();
  }

  auto operator()(Image image) {
    return image;
  }
};

struct Osaka : heph::conduit::Node<Osaka> {
  std::string_view name = "osaka";
  heph::conduit::QueuedInput<std::string, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL>>
      parana{ this, "parana" };
  heph::conduit::QueuedInput<Image> colorado{ this, "colorado" };
  heph::conduit::QueuedInput<Image, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL>>
      columbia{ this, "columbia" };

  heph::conduit::Output<PointCloud2> salween{ this, "salween" };
  heph::conduit::Output<LaserScan> godavari{ this, "godawari" };

  auto trigger() {
    return stdexec::when_all(parana.get(), colorado.get(), columbia.get());
  }

  auto operator()(heph::conduit::NodeEngine& engine, std::optional<std::string> s, Image,
                  std::optional<Image>) {
    heph::log(heph::INFO, "osaka", "/parana", s);
    return stdexec::when_all(salween.setValue(engine, PointCloud2{}), godavari.setValue(engine, LaserScan{}));
  }
};

struct Hebron : heph::conduit::Node<Hebron> {
  std::string_view name = "hebron";

  static auto period() {
    return std::chrono::milliseconds(100);
  }

  auto operator()() {
    return Quaternion{};
  }
};

struct Kingston : heph::conduit::Node<Kingston> {
  std::string_view name = "hebron";

  static auto period() {
    return std::chrono::milliseconds(100);
  }

  auto operator()() {
    return Vector3{};
  }
};

struct Tripoli : heph::conduit::Node<Tripoli> {
  std::string_view name = "hebron";

  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<Image, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL>>
      columbia{ this, "columbia" };

  auto trigger() {
    return stdexec::when_all(godavari.get(), columbia.get());
  }

  auto operator()(LaserScan /*scan*/, std::optional<Image> /*image*/) {
    return PointCloud2{};
  }
};

struct Mandalay : heph::conduit::Node<Mandalay> {
  std::string_view name = "mandalay";

  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };
  heph::conduit::QueuedInput<Quaternion> chenab{ this, "chenab" };
  heph::conduit::QueuedInput<PointCloud2> salween{ this, "salween" };
  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<Vector3> yamuna{ this, "yamuna" };
  heph::conduit::QueuedInput<PointCloud2> loire{ this, "loire" };

  heph::conduit::Output<Pose> tagus{ this, "tagus" };
  heph::conduit::Output<Image> missouri{ this, "missouri" };
  heph::conduit::Output<PointCloud2> brazos{ this, "brazos" };

  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(100) };

  static auto period() {
    return std::chrono::milliseconds(100);
  }

  auto operator()(heph::conduit::NodeEngine& engine) {
    heph::log(heph::INFO, "mandalay",
              //
              "danube", danube.getValue(),
              //
              "chenab", fmt::format("{}", chenab.getValue()),
              //
              "salween", fmt::format("{}", salween.getValue()),
              //
              "godavari", fmt::format("{}", godavari.getValue()),
              //
              "yamuna", fmt::format("{}", yamuna.getValue()),
              //
              "loire", fmt::format("{}", loire.getValue())
              //
    );
    return stdexec::when_all(tagus.setValue(engine, Pose{}), missouri.setValue(engine, Image{}),
                             brazos.setValue(engine, PointCloud2{}));
  }
};

struct Ponce : heph::conduit::Node<Ponce> {
  std::string_view name = "ponce";

  heph::conduit::QueuedInput<Pose> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };
  heph::conduit::QueuedInput<Image> missouri{ this, "missouri" };
  heph::conduit::QueuedInput<PointCloud2> brazos{ this, "brazos" };
  heph::conduit::QueuedInput<Vector3> yamuna{ this, "yamuna" };
  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<PointCloud2> loire{ this, "loire" };
  heph::conduit::QueuedInput<float> ohio{ this, "ohio" };
  heph::conduit::QueuedInput<double> volga{ this, "volga" };

  heph::conduit::Output<Twist> congo{ this, "congo" };
  heph::conduit::Output<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger() {
    return brazos.get();
  }
  auto operator()(heph::conduit::NodeEngine& engine, PointCloud2 cloud) {
    heph::log(heph::INFO, "mandalay",
              //
              "tagus", tagus.getValue(),
              //
              "danube", danube.getValue(),
              //
              "missouri", missouri.getValue(),
              //
              "brazos", cloud,
              //
              "yamuna", yamuna.getValue(),
              //
              "godavari", godavari.getValue(),
              //
              "loire", loire.getValue(),
              //
              "ohio", ohio.getValue(),
              //
              "volga", volga.getValue()
              //
    );
    return stdexec::when_all(congo.setValue(engine, Twist{}),
                             meckong.setValue(engine, TwistWithCovarianceStamped{}));
  }
};

struct Geneva : heph::conduit::Node<Geneva> {
  std::string_view name = "geneva";
  heph::conduit::QueuedInput<std::string> parana{ this, "parana" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };
  heph::conduit::QueuedInput<Pose> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<Twist> congo{ this, "congo" };

  auto trigger() {
    return parana.get();
  }

  auto operator()(std::string s) {
    heph::log(heph::INFO, "geneva",
              //
              "parana", s,
              //
              "danube", danube.getValue(),
              //
              "tagus", tagus.getValue(),
              //
              "congo", congo.getValue()
              //
    );
    return std::string{};
  }
};

struct Monaco : heph::conduit::Node<Monaco> {
  std::string_view name = "monaco";
  heph::conduit::QueuedInput<Twist> congo{ this, "congo" };

  auto trigger() {
    return congo.get();
  }

  auto operator()(Twist /**/) {
    return float{};
  }
};

struct Rotterdam : heph::conduit::Node<Rotterdam> {
  std::string_view name = "rotterdam";
  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger() {
    return meckong.get();
  }

  auto operator()(TwistWithCovarianceStamped /**/) {
    return Vector3Stamped{};
  }
};

struct Barcelona : heph::conduit::Node<Barcelona> {
  std::string_view name = "barcelona";
  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger() {
    return meckong.get();
  }

  auto operator()(TwistWithCovarianceStamped /**/) {
    return WrenchStamped{};
  }
};

struct Arequipa : heph::conduit::Node<Arequipa> {
  std::string_view name = "arequipa";
  heph::conduit::QueuedInput<std::string> arkansas{ this, "arkansas" };

  auto trigger() {
    return arkansas.get();
  }

  auto operator()(std::string const& /**/) {
  }
};

struct Georgetown : heph::conduit::Node<Georgetown> {
  std::string_view name = "georgetown";
  heph::conduit::QueuedInput<Vector3Stamped> murray{ this, "murray" };
  heph::conduit::QueuedInput<WrenchStamped> lena{ this, "lena" };

  static auto period() {
    return std::chrono::milliseconds(50);
  }

  auto operator()() {
    heph::log(heph::INFO, "georgetown", "murray", murray.getValue(), "lena", lena.getValue());

    return double{};
  }
};
}  // namespace mont_blanc

auto main() -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
    heph::conduit::NodeEngine engine{ {} };

    mont_blanc::Cordoba cordoba;

    mont_blanc::Lyon lyon;
    engine.addNode(lyon);
    mont_blanc::Freeport freeport;
    engine.addNode(freeport);
    mont_blanc::Medelin medellin;
    engine.addNode(medellin);
    mont_blanc::Portsmouth portsmouth;
    engine.addNode(portsmouth);
    mont_blanc::Delhi delhi;
    engine.addNode(delhi);

    lyon.amazon.connectTo(cordoba);

    mont_blanc::Hamburg hamburg;
    engine.addNode(hamburg);
    mont_blanc::Taipei taipei;
    engine.addNode(taipei);

    hamburg.tigris.connectTo(lyon);
    hamburg.ganges.connectTo(freeport);
    hamburg.nile.connectTo(medellin);
    hamburg.danube.connectTo(portsmouth);

    taipei.columbia.connectTo(delhi);

    mont_blanc::Osaka osaka;
    engine.addNode(osaka);

    osaka.parana.connectTo(hamburg);
    osaka.colorado.connectTo(taipei);
    osaka.columbia.connectTo(delhi);

    mont_blanc::Hebron hebron;
    engine.addNode(hebron);
    mont_blanc::Kingston kingston;
    engine.addNode(kingston);
    mont_blanc::Tripoli tripoli;
    engine.addNode(tripoli);

    tripoli.godavari.connectTo(osaka.godavari);
    tripoli.columbia.connectTo(delhi);

    mont_blanc::Mandalay mandalay;
    engine.addNode(mandalay);

    mandalay.danube.connectTo(portsmouth);
    mandalay.chenab.connectTo(hebron);
    mandalay.salween.connectTo(osaka.salween);
    mandalay.godavari.connectTo(osaka.godavari);
    mandalay.yamuna.connectTo(kingston);
    mandalay.loire.connectTo(tripoli);

    mont_blanc::Ponce ponce;
    engine.addNode(ponce);

    ponce.tagus.connectTo(mandalay.tagus);
    ponce.danube.connectTo(portsmouth);
    ponce.missouri.connectTo(mandalay.missouri);
    ponce.brazos.connectTo(mandalay.brazos);
    ponce.yamuna.connectTo(kingston);
    ponce.godavari.connectTo(osaka.godavari);
    ponce.loire.connectTo(tripoli);

    mont_blanc::Geneva geneva;
    engine.addNode(geneva);
    mont_blanc::Monaco monaco;
    engine.addNode(monaco);
    mont_blanc::Rotterdam rotterdam;
    engine.addNode(rotterdam);
    mont_blanc::Barcelona barcelona;
    engine.addNode(barcelona);

    geneva.parana.connectTo(hamburg);
    geneva.danube.connectTo(portsmouth);
    geneva.tagus.connectTo(mandalay.tagus);
    geneva.congo.connectTo(ponce.congo);

    monaco.congo.connectTo(ponce.congo);

    rotterdam.meckong.connectTo(ponce.meckong);
    barcelona.meckong.connectTo(ponce.meckong);

    ponce.ohio.connectTo(monaco);

    mont_blanc::Arequipa arequipa;
    mont_blanc::Georgetown georgetown;

    arequipa.arkansas.connectTo(geneva);
    georgetown.murray.connectTo(rotterdam);
    georgetown.lena.connectTo(barcelona);

    ponce.volga.connectTo(georgetown);

    engine.run();

  } catch (...) {
    fmt::println("unexcepted exception...");
  }
}
