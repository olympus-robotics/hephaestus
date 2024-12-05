# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "telemetry",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "//modules/random",
        "//modules/serdes",
        "@abseil-cpp//absl/base:log",
        "@abseil-cpp//absl/log:absl_log",
        "@fmt",
        "@magic_enum",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "metric_sink_example",
    srcs = ["examples/metric_sink_example.cpp"],
    deps = [
        ":telemetry",
    ],
)

heph_cc_binary(
    name = "log_sink_example",
    srcs = ["examples/log_sink_example.cpp"],
    deps = [
        ":telemetry",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "metric_record_tests",
    srcs = ["tests/metric_record_tests.cpp"],
    deps = [
        ":telemetry",
    ],
)

heph_cc_test(
    name = "log_tests",
    srcs = ["tests/log_tests.cpp"],
    deps = [
        ":telemetry",
    ],
)
