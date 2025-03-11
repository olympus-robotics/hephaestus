# =================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "websocket_bridge",
    srcs = glob([
        "src/ipc/*.cpp",
        "src/websocket_bridge/*.cpp",
        "src/websocket_bridge/utils/*.cpp",
    ]),
    hdrs = glob([
        "include/hephaestus/ipc/*.h",
        "include/hephaestus/websocket_bridge/*.h",
        "include/hephaestus/websocket_bridge/utils/*.h",
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
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@boringssl//:ssl",
        "@fmt",
        "@reflect-cpp",
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
    name = "example_client__services",
    srcs = ["examples/example_client__services.cpp"],
    deps = [
        ":websocket_bridge",
        "@bazel_tools//tools/cpp/runfiles",
    ],
)

heph_cc_binary(
    name = "example_client__topics",
    srcs = ["examples/example_client__topics.cpp"],
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
    name = "bridge_config_tests",
    srcs = [
        "tests/bridge_config_tests.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "bridge_state_tests",
    srcs = [
        "tests/bridge_state_tests.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "ipc_entity_manager_tests",
    srcs = [
        "tests/ipc_entity_manager_tests.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "protobuf_serdes_tests",
    srcs = [
        "tests/protobuf_serdes_tests.cpp",
    ],
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)
