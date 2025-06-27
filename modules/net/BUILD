# =================================================================================================
# Copyright (C) 2023-2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "net",
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
        "//modules/telemetry/telemetry",
        "//modules/utils",
        "@bluez",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "ping_pong",
    srcs = ["examples/ping_pong.cpp"],
    deps = [
        ":net",
        "//modules/cli",
    ],
)

heph_cc_binary(
    name = "bt_client",
    srcs = ["examples/bt_client.cpp"],
    deps = [
        ":net",
        "//modules/cli",
    ],
)

heph_cc_binary(
    name = "bt_server",
    srcs = ["examples/bt_server.cpp"],
    deps = [
        ":net",
        "//modules/cli",
    ],
)

#########
# Tests #
#########

NET_TEST_DEPS = [
    ":net",
]

heph_cc_test(
    name = "net_tests",
    srcs = ["tests/net_tests.cpp"],
    deps = NET_TEST_DEPS,
)
