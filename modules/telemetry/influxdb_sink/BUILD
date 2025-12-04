# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "influxdb_sink",
    srcs = glob(
        [
            "src/**/*.cpp",
        ],
    ),
    hdrs = glob(
        [
            "include/**/*.h",
        ],
    ),
    implementation_deps = [
        "//modules/telemetry/log",
        "@abseil-cpp//absl/strings:strings",
        "@fmt",
        "@influxdb",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "//modules/telemetry/metrics",
    ],
)

############
# Examples #
############

heph_cc_binary(
    name = "influxdb_measure_example",
    srcs = ["examples/influxdb_measure_example.cpp"],
    deps = [
        ":influxdb_sink",
        "//modules/concurrency",
        "//modules/random",
        "//modules/telemetry/log",
        "//modules/telemetry/metrics",
        "//modules/utils",
        "@fmt",
    ],
)
