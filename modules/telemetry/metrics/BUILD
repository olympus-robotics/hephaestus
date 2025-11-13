# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "metrics",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/containers",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
        "@reflect-cpp",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "metric_sink_example",
    srcs = ["examples/metric_sink_example.cpp"],
    deps = [
        ":metrics",
        "//modules/random",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "metric_record_tests",
    srcs = ["tests/metric_record_tests.cpp"],
    deps = [
        ":metrics",
        "//modules/random",
    ],
)
