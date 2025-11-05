# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "bag",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/containers",
        "//modules/random",
        "//modules/telemetry/telemetry",
        "@zstd",
    ]
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
    ],
)

heph_cc_binary(
    name = "bag_recorder",
    srcs = ["apps/bag_recorder.cpp"],
    deps = [
        ":bag",
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
        "//modules/types",
        "//modules/types_proto",
    ],
)
