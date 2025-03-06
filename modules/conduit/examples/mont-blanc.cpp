//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <random>
#include <string_view>
#include <thread>

#include "hephaestus/conduit/context.h"
#include "hephaestus/conduit/node_operation.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"

// This is adapting the mont-blanc example from here https:/...

namespace mont_blanc {
struct Image {
  std::int32_t width;
  std::int32_t height;
};
auto format_as(Image image) {
  return fmt::format("Image(width={}, height={})", image.width, image.height);
}

struct PointCloud2 {};
auto format_as(PointCloud2) {
  return fmt::format("PointCloud2()");
}
struct LaserScan {};
auto format_as(LaserScan) {
  return fmt::format("LaserScan()");
}
struct Quaternion {};
auto format_as(Quaternion) {
  return fmt::format("Quaternion()");
}
struct Vector3 {};
auto format_as(Vector3) {
  return fmt::format("Vector3()");
}
struct Pose {};
auto format_as(Pose) {
  return fmt::format("Pose()");
}
struct Twist {};
auto format_as(Twist) {
  return fmt::format("Twist()");
}
struct TwistWithCovarianceStamped {};
auto format_as(TwistWithCovarianceStamped) {
  return fmt::format("TwistWithCovarianceStamped()");
}
struct Vector3Stamped {};
auto format_as(Vector3Stamped) {
  return fmt::format("Vector3Stamped()");
}
struct WrenchStamped {};
auto format_as(WrenchStamped) {
  return fmt::format("WrenchStamped()");
}

struct Cordoba : heph::conduit::NodeOperation<Cordoba, float> {
  std::string_view name = "cordoba";
  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(100) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() -> float {
    return distribution(gen);
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_real_distribution<float> distribution{ 0.0f, 10.0f };
};

struct Lyon : heph::conduit::NodeOperation<Lyon, float> {
  std::string_view name = "lyon";
  heph::conduit::QueuedInput<float> amazon{ this, "/amazon" };

  auto trigger(heph::conduit::Context& /*context*/) {
    return amazon.await();
  }

  auto operator()(float f) -> float {
    heph::log(heph::INFO, "lyon", "/amazon", f);
    return f;
  }
};

struct Freeport : heph::conduit::NodeOperation<Freeport, std::int64_t> {
  std::string_view name = "freeport";
  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(50) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() -> std::int64_t {
    return distribution(gen);
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int64_t> distribution{ 0, 10 };
};

struct Medelin : heph::conduit::NodeOperation<Medelin, std::int32_t> {
  std::string_view name = "medelin";
  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(10) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() -> std::int32_t {
    return distribution(gen);
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int32_t> distribution{ 0, 10 };
};

struct Portsmouth : heph::conduit::NodeOperation<Portsmouth, std::string> {
  std::string_view name = "portsmouth";
  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(200) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() -> std::string {
    return fmt::format("0x{:x}{:x}{:x}{:x}", distribution(gen), distribution(gen), distribution(gen),
                       distribution(gen));
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int32_t> distribution{ 0, 10 };
};

struct Delhi : heph::conduit::NodeOperation<Delhi, Image> {
  std::string_view name = "delhi";
  std::chrono::steady_clock::duration delay{ std::chrono::seconds(1) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() -> Image {
    return { .width = distribution(gen), .height = distribution(gen) };
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_int_distribution<std::int32_t> distribution{ 0, 10 };
};

struct Hamburg : heph::conduit::NodeOperation<Hamburg, std::string> {
  std::string_view name = "hamburg";

  heph::conduit::QueuedInput<float> tigris{ this, "tigris" };
  heph::conduit::QueuedInput<std::int64_t, 2> ganges{ this, "ganges" };
  heph::conduit::QueuedInput<std::int32_t, 10> nile{ this, "nile" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };

  auto trigger(heph::conduit::Context& /*context*/) {
    return stdexec::when_all(tigris.just(), ganges.just(), nile.just(), danube.await());
  }

  auto operator()(std::optional<float> t, std::optional<std::int64_t> g, std::optional<std::int32_t> n,
                  std::string d) -> std::string {
    heph::log(heph::INFO, "hamburg", "/tigris", t, "/ganges", g, "/nile", n, "/danube", d);

    tigris_last_value = t.value_or(tigris_last_value);
    ganges_last_value = g.value_or(ganges_last_value);
    nile_last_value = n.value_or(nile_last_value);

    return fmt::format("hamburg/parana:{}", d);
  }
  float tigris_last_value;
  std::int64_t ganges_last_value;
  std::int32_t nile_last_value;
};

struct Taipei : heph::conduit::NodeOperation<Taipei, Image> {
  std::string_view name = "taipei";
  heph::conduit::QueuedInput<Image> columbia{ this, "columbia" };

  auto trigger(heph::conduit::Context& /*context*/) {
    return columbia.await();
  }

  auto operator()(Image image) {
    return image;
  }
};

struct Osaka : heph::conduit::NodeOperation<Osaka> {
  std::string_view name = "osaka";
  heph::conduit::QueuedInput<std::string> parana{ this, "parana" };
  heph::conduit::QueuedInput<Image> colorado{ this, "colorado" };
  heph::conduit::QueuedInput<Image> columbia{ this, "columbia" };

  heph::conduit::Output<PointCloud2> salween{ this, "salween" };
  heph::conduit::Output<LaserScan> godavari{ this, "godawari" };

  auto trigger(heph::conduit::Context& /*context*/) {
    return stdexec::when_all(parana.just(), colorado.await(), columbia.just());
  }

  auto operator()(heph::conduit::Context& context, std::optional<std::string> s, Image,
                  std::optional<Image>) {
    heph::log(heph::INFO, "osaka", "/parana", s);
    return stdexec::when_all(salween.setValue(context, PointCloud2{}),
                             godavari.setValue(context, LaserScan{}));
  }
};

struct Hebron : heph::conduit::NodeOperation<Hebron, Quaternion> {
  std::string_view name = "hebron";

  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(100) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() {
    return Quaternion{};
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_real_distribution<float> distribution{ 0.0f, 10.0f };
};

struct Kingston : heph::conduit::NodeOperation<Kingston, Vector3> {
  std::string_view name = "kingston";

  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(100) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()() {
    return Vector3{};
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_real_distribution<float> distribution{ 0.0f, 10.0f };
};

struct Tripoli : heph::conduit::NodeOperation<Tripoli, PointCloud2> {
  std::string_view name = "hebron";

  heph::conduit::QueuedInput<LaserScan> godavari{ this, "godavari" };
  heph::conduit::QueuedInput<Image> columbia{ this, "columbia" };

  auto trigger(heph::conduit::Context& /*context*/) {
    return stdexec::when_all(godavari.await(), columbia.just());
  }

  auto operator()(LaserScan /*scan*/, std::optional<Image> /*image*/) {
    return PointCloud2{};
  }

  std::random_device rd;
  std::mt19937 gen{ rd() };
  std::uniform_real_distribution<float> distribution{ 0.0f, 10.0f };
};

struct Mandalay : heph::conduit::NodeOperation<Mandalay> {
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

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
  }

  auto operator()(heph::conduit::Context& context) {
    heph::log(heph::INFO, "mandalay",
              //
              "danube", danube.getValue(),
              //
              "chenab", chenab.getValue(),
              //
              "salween", salween.getValue(),
              //
              "godavari", godavari.getValue(),
              //
              "yamuna", yamuna.getValue(),
              //
              "loire", loire.getValue()
              //
    );
    return stdexec::when_all(tagus.setValue(context, Pose{}), missouri.setValue(context, Image{}),
                             brazos.setValue(context, PointCloud2{}));
  }
};

struct Ponce : heph::conduit::NodeOperation<Ponce> {
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

  auto trigger(heph::conduit::Context& /*context*/) {
    return stdexec::when_all(brazos.await());
  }
  auto operator()(heph::conduit::Context& context, PointCloud2 cloud) {
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
    return stdexec::when_all(congo.setValue(context, Twist{}),
                             meckong.setValue(context, TwistWithCovarianceStamped{}));
  }
};

struct Geneva : heph::conduit::NodeOperation<Geneva, std::string> {
  std::string_view name = "geneva";
  heph::conduit::QueuedInput<std::string> parana{ this, "parana" };
  heph::conduit::QueuedInput<std::string> danube{ this, "danube" };
  heph::conduit::QueuedInput<Pose> tagus{ this, "tagus" };
  heph::conduit::QueuedInput<Twist> congo{ this, "congo" };

  auto trigger(heph::conduit::Context&) {
    return parana.await();
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

struct Monaco : heph::conduit::NodeOperation<Monaco, float> {
  std::string_view name = "monaco";
  heph::conduit::QueuedInput<Twist> congo{ this, "congo" };

  auto trigger(heph::conduit::Context&) {
    return congo.await();
  }

  auto operator()(Twist) {
    return float{};
  }
};

struct Rotterdam : heph::conduit::NodeOperation<Rotterdam, Vector3Stamped> {
  std::string_view name = "rotterdam";
  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger(heph::conduit::Context&) {
    return meckong.await();
  }

  auto operator()(TwistWithCovarianceStamped) {
    return Vector3Stamped{};
  }
};

struct Barcelona : heph::conduit::NodeOperation<Barcelona, WrenchStamped> {
  std::string_view name = "barcelona";
  heph::conduit::QueuedInput<TwistWithCovarianceStamped> meckong{ this, "meckong" };

  auto trigger(heph::conduit::Context&) {
    return meckong.await();
  }

  auto operator()(TwistWithCovarianceStamped) {
    return WrenchStamped{};
  }
};

struct Arequipa : heph::conduit::NodeOperation<Arequipa> {
  std::string_view name = "arequipa";
  heph::conduit::QueuedInput<std::string> arkansas{ this, "arkansas" };

  auto trigger(heph::conduit::Context&) {
    return arkansas.await();
  }

  auto operator()(std::string) {
  }
};

struct Georgetown : heph::conduit::NodeOperation<Georgetown, double> {
  std::string_view name = "georgetown";
  heph::conduit::QueuedInput<Vector3Stamped> murray{ this, "murray" };
  heph::conduit::QueuedInput<WrenchStamped> lena{ this, "lena" };

  std::chrono::steady_clock::duration delay{ std::chrono::milliseconds(50) };

  auto trigger(heph::conduit::Context& context) {
    return context.scheduleAfter(delay);
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

    mont_blanc::Cordoba cordoba;

    mont_blanc::Lyon lyon;
    mont_blanc::Freeport freeport;
    mont_blanc::Medelin medellin;
    mont_blanc::Portsmouth portsmouth;
    mont_blanc::Delhi delhi;

    cordoba.connectTo(lyon.amazon);

    mont_blanc::Hamburg hamburg;
    mont_blanc::Taipei taipei;

    lyon.connectTo(hamburg.tigris);
    freeport.connectTo(hamburg.ganges);
    medellin.connectTo(hamburg.nile);
    portsmouth.connectTo(hamburg.danube);

    delhi.connectTo(taipei.columbia);

    mont_blanc::Osaka osaka;

    hamburg.connectTo(osaka.parana);
    taipei.connectTo(osaka.colorado);
    delhi.connectTo(osaka.columbia);

    mont_blanc::Hebron hebron;
    mont_blanc::Kingston kingston;
    mont_blanc::Tripoli tripoli;

    osaka.godavari.connectTo(tripoli.godavari);
    delhi.connectTo(tripoli.columbia);

    mont_blanc::Mandalay mandalay;
    portsmouth.connectTo(mandalay.danube);
    hebron.connectTo(mandalay.chenab);
    osaka.salween.connectTo(mandalay.salween);
    osaka.godavari.connectTo(mandalay.godavari);
    kingston.connectTo(mandalay.yamuna);
    tripoli.connectTo(mandalay.loire);

    mont_blanc::Ponce ponce;
    mandalay.tagus.connectTo(ponce.tagus);
    portsmouth.connectTo(ponce.danube);
    mandalay.missouri.connectTo(ponce.missouri);
    mandalay.brazos.connectTo(ponce.brazos);
    kingston.connectTo(ponce.yamuna);
    osaka.godavari.connectTo(ponce.godavari);
    tripoli.connectTo(ponce.loire);

    mont_blanc::Geneva geneva;
    mont_blanc::Monaco monaco;
    mont_blanc::Rotterdam rotterdam;
    mont_blanc::Barcelona barcelona;

    hamburg.connectTo(geneva.parana);
    portsmouth.connectTo(geneva.danube);
    mandalay.tagus.connectTo(geneva.tagus);
    ponce.congo.connectTo(geneva.congo);
    ponce.congo.connectTo(monaco.congo);
    ponce.meckong.connectTo(rotterdam.meckong);
    ponce.meckong.connectTo(barcelona.meckong);

    monaco.connectTo(ponce.ohio);

    mont_blanc::Arequipa arequipa;
    mont_blanc::Georgetown georgetown;

    geneva.connectTo(arequipa.arkansas);
    rotterdam.connectTo(georgetown.murray);
    barcelona.connectTo(georgetown.lena);

    georgetown.connectTo(ponce.volga);

    heph::conduit::DataflowGraph g;
    // heph::conduit::Context context;
    heph::conduit::Context context{ { .time_scale_factor = 1.0 } };
    osaka.run(g, context);

    // Run for 10 seconds.
    std::thread stopper{ [&context] {
      std::this_thread::sleep_for(std::chrono::seconds(10));
      context.requestStop();
    } };

    context.run();
    stopper.join();

    fmt::println("{}", g.toDot());
  } catch (...) {
    fmt::println("unexcepted exception...");
  }
}
