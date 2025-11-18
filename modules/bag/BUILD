# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load(
    "@hephaestus//bazel:hephaestus.bzl",
    "heph_cc_binary",
    "heph_cc_library",
    "heph_cc_test",
)

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "bag",
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
        "//modules/telemetry/log",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/synchronization",
        "@magic_enum",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/ipc",
        "//modules/serdes",
        "@mcap",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "bag_player",
    srcs = ["apps/bag_player.cpp"],
    deps = [
        ":bag",
        "//modules/cli",
        "//modules/error_handling",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
        "@mcap",
    ],
)

heph_cc_binary(
    name = "bag_recorder",
    srcs = ["apps/bag_recorder.cpp"],
    deps = [
        ":bag",
        "//modules/cli",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "bag_tests",
    srcs = ["tests/tests.cpp"],
    deps = [
        ":bag",
        "//modules/ipc",
        "//modules/random",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@mcap",
    ],
)
