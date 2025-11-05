# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "telemetry_loki_sink",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    implementation_deps = [
        "@cpr",
        "@fmt",
        "@reflect-cpp",
    ],
    deps=[
        "//modules/concurrency",
        "//modules/telemetry/telemetry",
        "@abseil-cpp//absl/base:thread_annotations",
        "@abseil-cpp//absl/synchronization:mutex",
    ],
)

############
# Examples #
############

heph_cc_binary(
    name = "loki_log_sink_example",
    srcs = ["examples/loki_log_sink_example.cpp"],
    deps = [
        ":telemetry_loki_sink",
        "//modules/cli",
    ],
)
