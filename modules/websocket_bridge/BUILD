# =================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "websocket_bridge",
    srcs = glob([
        "src/utils/*.cpp",
        "src/ipc/*.cpp",
        "src/websocket_bridge/*.cpp",
    ]),
    hdrs = glob([
        "include/hephaestus/websocket_bridge/*.h",
        "include/hephaestus/ipc/*.h",
        "include/hephaestus/utils/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/cli",
        "//modules/concurrency",
        "//modules/containers",
        "//modules/ipc",
        "//modules/serdes",
        "//modules/telemetry/telemetry",
        "//modules/types_proto",
        "//modules/types",
        "//modules/utils",
        "@fmt",
        "@boringssl//:ssl",
        "@reflect-cpp",
        "@websocketpp",
        "@ws_protocol",
        "@zenohc_builder//:zenoh-c",
        "@zenohc_builder//:zenoh-cpp",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "app",
    srcs = ["apps/bridge_app.cpp"],
    deps = [
        ":websocket_bridge",
        "@bazel_tools//tools/cpp/runfiles",
    ],
)

heph_cc_binary(
    name = "test_client__services",
    srcs = ["examples/test_client__services.cpp"],
    deps = [
        ":websocket_bridge",
        "@bazel_tools//tools/cpp/runfiles",
    ],
)

heph_cc_binary(
    name = "test_client__topics",
    srcs = ["examples/test_client__topics.cpp"],
    deps = [
        ":websocket_bridge",
        "@bazel_tools//tools/cpp/runfiles",
    ],
)

#########
# Tests #
#########

IPC_TEST_DEPS = [
    ":websocket_bridge",
]

heph_cc_test(
    name = "test_bridge_config",
    srcs = [
        "tests/test_bridge_config.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "test_ipc_graph",
    srcs = [
        "tests/test_ipc_graph.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "test_bridge_state",
    srcs = [
        "tests/test_bridge_state.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "test_ipc_interface",
    srcs = [
        "tests/test_ipc_interface.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "test_protobuf_serdes",
    srcs = [
        "tests/test_protobuf_serdes.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)
