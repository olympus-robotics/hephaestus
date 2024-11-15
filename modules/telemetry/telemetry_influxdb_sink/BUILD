load("//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "telemetry_influxdb_sink",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/foreign_cc:influxdb",
        "//modules/concurrency",
        "//modules/random",
        "//modules/serdes",
        "//modules/telemetry/telemetry",
        "@fmt",
    ],
)

############
# Examples #
############

heph_cc_binary(
    name = "influxdb_measure_example",
    srcs = ["examples/influxdb_measure_example.cpp"],
    deps = [":telemetry_influxdb_sink"],
)
