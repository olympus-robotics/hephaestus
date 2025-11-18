# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "loki_sink",
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
        "//modules/error_handling",
        "//modules/utils",
        "@fmt",
        "@magic_enum",
        "@reflect-cpp",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "//modules/telemetry/log",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@cpr",
    ],
)

############
# Examples #
############

heph_cc_binary(
    name = "loki_log_sink_example",
    srcs = ["examples/loki_log_sink_example.cpp"],
    deps = [
        ":loki_sink",
        "//modules/telemetry/log",
        "//modules/utils",
    ],
)
