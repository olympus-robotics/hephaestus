# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library")

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
    implementation_deps = [
        "//modules/random",
        "//modules/serdes",
        "@fmt",
        "@influxdb",
    ],
    deps=[
        "//modules/concurrency",
        "//modules/telemetry/telemetry",
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
