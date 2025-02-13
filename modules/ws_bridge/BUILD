# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "ws_bridge",
    srcs = glob([
        "src/*.cpp",
    ]),
    hdrs = glob([
        "include/hephaestus/ws_bridge/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/ipc",
        "//modules/cli",
        "//modules/concurrency",
        "//modules/containers",
        "//modules/serdes",
        "//modules/telemetry/telemetry",
        "//modules/utils",
        "@fmt",
        "@nlohmann_json//:json",
        "@zenohc_builder//:zenoh-c",
        "@zenohc_builder//:zenoh-cpp",
        "@websocketpp",
        "@ws_protocol",
        "@openssl//:openssl",
    ],
    linkopts = ["-lssl", "-lcrypto"],  # Add this line to link OpenSSL libraries
)

############
# Binaries #
############

heph_cc_binary(
    name = "ws_bridge_app",
    srcs = ["apps/ws_bridge_app.cpp"],
    deps = [
        ":ws_bridge",
    ],
)

#########
# Tests #
#########

IPC_TEST_DEPS = [
    ":ws_bridge",
]

heph_cc_test(
    name = "tests",
    srcs = [
        "tests/tests.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)