# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "websocket_bridge",
    srcs = glob([
        "src/*.cpp",
    ]),
    hdrs = glob([
        "include/hephaestus/websocket_bridge/*.h",
    ]),
    includes = ["include"],
    linkopts = [
        "-lssl",
        "-lcrypto",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/cli",
        "//modules/concurrency",
        "//modules/containers",
        "//modules/ipc",
        "//modules/serdes",
        "//modules/telemetry/telemetry",
        "//modules/utils",
        "@fmt",
        "@openssl",
        "@websocketpp",
        "@ws_protocol",
        "@yaml-cpp",
        "@zenohc_builder//:zenoh-c",
        "@zenohc_builder//:zenoh-cpp",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "websocket_bridge_app",
    srcs = ["apps/websocket_bridge_app.cpp"],
    deps = [
        ":websocket_bridge",
    ],
)

#########
# Tests #
#########

IPC_TEST_DEPS = [
    ":websocket_bridge",
]

heph_cc_test(
    name = "test_config",
    srcs = [
        "tests/test_config.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)
