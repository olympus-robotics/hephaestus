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
  explicit Cordoba(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Cordoba>{ engine } {
  }

  static auto name() -> std::string_view {
    return "cordoba";
  }

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
  explicit Lyon(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Lyon>{ engine } {
  }

  static auto name() -> std::string_view {
    return "lyon";
  }

  heph::conduit::QueuedInput<float> amazon{ this, "amazon" };

  auto trigger() {
    return amazon.get();
  }

  auto operator()(float f) -> float {
    heph::log(heph::INFO, "lyon", "amazon", f);
    return f;
  }
};

struct Freeport : heph::conduit::Node<Freeport> {
  explicit Freeport(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Freeport>{ engine } {
  }

  static auto name() -> std::string_view {
    return "freeport";
  }

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
  explicit Medelin(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Medelin>{ engine } {
  }

  static auto name() -> std::string_view {
    return "medelin";
  }

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
  explicit Portsmouth(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Portsmouth>{ engine } {
  }

  static auto name() -> std::string_view {
    return "portsmouth";
  }

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
  explicit Delhi(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Delhi>{ engine } {
  }

  static auto name() -> std::string_view {
    return "delhi";
  }

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
  explicit Hamburg(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Hamburg>{ engine } {
  }

  static auto name() -> std::string_view {
    return "hamburg";
  }

  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<float, InputPolicyT> tigris{ this, "tigris" };
  heph::conduit::QueuedInput<std::int64_t, InputPolicyT> ganges{ this, "ganges" };
  heph::conduit::QueuedInput<std::int32_t, InputPolicyT> nile{ this, "nile" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };

  auto trigger() {
    return stdexec::when_all(tigris.get(), ganges.get(), nile.get(), danube.get());
  }

  auto operator()(std::optional<float> t, std::optional<std::int64_t> g, std::optional<std::int32_t> n,
                  std::string d) -> std::string {
    heph::log(heph::INFO, "hamburg", "tigris", t, "ganges", g, "nile", n, "danube", d);

    return fmt::format("hamburg/parana:{}", d);
  }
};

struct Taipei : heph::conduit::Node<Taipei> {
  explicit Taipei(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Taipei>{ engine } {
  }

  static auto name() -> std::string_view {
    return "taipei";
  }

  heph::conduit::QueuedInput<Image> columbia{ this, "columbia" };

  auto trigger() {
    return columbia.get();
  }

  auto operator()(Image image) {
    return image;
  }
};

struct Osaka : heph::conduit::Node<Osaka> {
  explicit Osaka(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Osaka>{ engine } {
  }

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

  auto trigger() {
    return stdexec::when_all(parana.get(), colorado.get(), columbia.get());
  }

  auto operator()(heph::conduit::NodeEngine& engine, std::optional<std::string> s, Image /**/,
                  std::optional<Image> /**/) {
    heph::log(heph::INFO, "osaka", "parana", s);
    return stdexec::when_all(salween.setValue(engine, PointCloud2{}), godavari.setValue(engine, LaserScan{}));
  }
};

struct Hebron : heph::conduit::Node<Hebron> {
  explicit Hebron(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Hebron>{ engine } {
  }

  static auto name() -> std::string_view {
    return "hebron";
  }

  static auto period() {
    return std::chrono::milliseconds(100);
  }

  auto operator()() {
    return Quaternion{};
  }
};

struct Kingston : heph::conduit::Node<Kingston> {
  explicit Kingston(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Kingston>{ engine } {
  }

  static auto name() -> std::string_view {
    return "kingston";
  }

  static auto period() {
    return std::chrono::milliseconds(100);
  }

  auto operator()() {
    return Vector3{};
  }
};

struct Tripoli : heph::conduit::Node<Tripoli> {
  explicit Tripoli(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Tripoli>{ engine } {
  }

  static auto name() -> std::string_view {
    return "tripoli";
  }

  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<Image, heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                               heph::conduit::SetMethod::OVERWRITE>>
      columbia{ this, "columbia" };

  auto trigger() {
    return stdexec::when_all(godavari.get(), columbia.get());
  }

  auto operator()(LaserScan /*scan*/, std::optional<Image> /*image*/) {
    return PointCloud2{};
  }
};

struct Mandalay : heph::conduit::Node<Mandalay> {
  explicit Mandalay(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Mandalay>{ engine } {
  }

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
  explicit Ponce(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Ponce>{ engine } {
  }

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

  auto trigger() {
    return brazos.get();
  }

  auto operator()(heph::conduit::NodeEngine& engine, PointCloud2 cloud) {
    heph::log(heph::INFO, "ponce",
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
  explicit Geneva(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Geneva>{ engine } {
  }

  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::POLL,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  static auto name() -> std::string_view {
    return "geneva";
  }

  heph::conduit::QueuedInput<std::string> parana{ this, "parana" };
  heph::conduit::QueuedInput<std::string, InputPolicyT> danube{ this, "danube" };
  heph::conduit::QueuedInput<Pose, InputPolicyT> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<Twist, InputPolicyT> congo{ this, "congo" };

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
  explicit Monaco(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Monaco>{ engine } {
  }

  static auto name() -> std::string_view {
    return "monaco";
  }

  heph::conduit::QueuedInput<Twist> congo{ this, "congo" };

  auto trigger() {
    return congo.get();
  }

  auto operator()(Twist /**/) {
    return float{};
  }
};

struct Rotterdam : heph::conduit::Node<Rotterdam> {
  explicit Rotterdam(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Rotterdam>{ engine } {
  }

  static auto name() -> std::string_view {
    return "rotterdam";
  }

  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger() {
    return meckong.get();
  }

  auto operator()(TwistWithCovarianceStamped /**/) {
    return Vector3Stamped{};
  }
};

struct Barcelona : heph::conduit::Node<Barcelona> {
  explicit Barcelona(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Barcelona>{ engine } {
  }

  static auto name() -> std::string_view {
    return "barcelona";
  }

  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger() {
    return meckong.get();
  }

  auto operator()(TwistWithCovarianceStamped /**/) {
    return WrenchStamped{};
  }
};

struct Arequipa : heph::conduit::Node<Arequipa> {
  explicit Arequipa(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Arequipa>{ engine } {
  }

  static auto name() -> std::string_view {
    return "arequipa";
  }

  heph::conduit::QueuedInput<std::string> arkansas{ this, "arkansas" };

  auto trigger() {
    return arkansas.get();
  }

  auto operator()(std::string const& /**/) {
  }
};

struct Georgetown : heph::conduit::Node<Georgetown> {
  explicit Georgetown(heph::conduit::NodeEngine& engine) : heph::conduit::Node<Georgetown>{ engine } {
  }

  static auto name() -> std::string_view {
    return "georgetown";
  }
  using InputPolicyT = heph::conduit::InputPolicy<1, heph::conduit::RetrievalMethod::BLOCK,
                                                  heph::conduit::SetMethod::OVERWRITE>;

  heph::conduit::QueuedInput<Vector3Stamped, InputPolicyT> murray{ this, "murray" };
  heph::conduit::QueuedInput<WrenchStamped, InputPolicyT> lena{ this, "lena" };

  static auto period() {
    return std::chrono::milliseconds(50);
  }

  auto trigger() {
    return stdexec::when_all(murray.get(), lena.get());
  }

  auto operator()(Vector3Stamped m, WrenchStamped l) {
    heph::log(heph::INFO, "georgetown", "murray", m, "lena", l);

    return double{};
  }
};
}  // namespace mont_blanc

auto main() -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());
    heph::conduit::NodeEngine engine{ {} };

    mont_blanc::Cordoba cordoba{ engine };

    mont_blanc::Lyon lyon{ engine };
    mont_blanc::Freeport freeport{ engine };
    mont_blanc::Medelin medellin{ engine };
    mont_blanc::Portsmouth portsmouth{ engine };
    mont_blanc::Delhi delhi{ engine };

    lyon.amazon.connectTo(cordoba);

    mont_blanc::Hamburg hamburg{ engine };
    mont_blanc::Taipei taipei{ engine };

    hamburg.tigris.connectTo(lyon);
    hamburg.ganges.connectTo(freeport);
    hamburg.nile.connectTo(medellin);
    hamburg.danube.connectTo(portsmouth);

    taipei.columbia.connectTo(delhi);

    mont_blanc::Osaka osaka{ engine };

    osaka.parana.connectTo(hamburg);
    osaka.colorado.connectTo(taipei);
    osaka.columbia.connectTo(delhi);

    mont_blanc::Hebron hebron{ engine };
    mont_blanc::Kingston kingston{ engine };
    mont_blanc::Tripoli tripoli{ engine };

    tripoli.godavari.connectTo(osaka.godavari);
    tripoli.columbia.connectTo(delhi);

    mont_blanc::Mandalay mandalay{ engine };

    mandalay.danube.connectTo(portsmouth);
    mandalay.chenab.connectTo(hebron);
    mandalay.salween.connectTo(osaka.salween);
    mandalay.godavari.connectTo(osaka.godavari);
    mandalay.yamuna.connectTo(kingston);
    mandalay.loire.connectTo(tripoli);

    mont_blanc::Ponce ponce{ engine };

    ponce.tagus.connectTo(mandalay.tagus);
    ponce.danube.connectTo(portsmouth);
    ponce.missouri.connectTo(mandalay.missouri);
    ponce.brazos.connectTo(mandalay.brazos);
    ponce.yamuna.connectTo(kingston);
    ponce.godavari.connectTo(osaka.godavari);
    ponce.loire.connectTo(tripoli);

    mont_blanc::Geneva geneva{ engine };
    mont_blanc::Monaco monaco{ engine };
    mont_blanc::Rotterdam rotterdam{ engine };
    mont_blanc::Barcelona barcelona{ engine };

    geneva.parana.connectTo(hamburg);
    geneva.danube.connectTo(portsmouth);
    geneva.tagus.connectTo(mandalay.tagus);
    geneva.congo.connectTo(ponce.congo);

    monaco.congo.connectTo(ponce.congo);

    rotterdam.meckong.connectTo(ponce.meckong);
    barcelona.meckong.connectTo(ponce.meckong);

    ponce.ohio.connectTo(monaco);

    mont_blanc::Arequipa arequipa{ engine };
    mont_blanc::Georgetown georgetown{ engine };

    arequipa.arkansas.connectTo(geneva);
    georgetown.murray.connectTo(rotterdam);
    georgetown.lena.connectTo(barcelona);

    ponce.volga.connectTo(georgetown);

    engine.run();

  } catch (...) {
    fmt::println("unexcepted exception...");
  }
}
