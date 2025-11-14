# =================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "websocket_bridge",
    srcs = glob([
        "src/websocket_bridge/*.cpp",
        "src/websocket_bridge/utils/*.cpp",
        "src/websocket_bridge/ipc/*.cpp",
    ]),
    hdrs = glob([
        "include/hephaestus/websocket_bridge/*.h",
        "include/hephaestus/websocket_bridge/utils/*.h",
        "include/hephaestus/websocket_bridge/ipc/*.h",
    ]),
    implementation_deps = [
        "//modules/error_handling",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/strings:strings",
        "@magic_enum",
        "@reflect-cpp",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/ipc",
        "//modules/serdes",
        "//modules/telemetry/log",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/log:log",
        "@abseil-cpp//absl/synchronization:synchronization",
        "@fmt",
        "@nlohmann_json//:json",
        "@protobuf",
        "@ws_protocol",
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
        "//modules/cli",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
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

heph_cc_test(
    name = "bridge_config_tests",
    srcs = [
        "tests/bridge_config_tests.cpp",
    ],
    deps = [":websocket_bridge"],
)

heph_cc_test(
    name = "bridge_state_tests",
    srcs = [
        "tests/bridge_state_tests.cpp",
    ],
    tags = ["exclusive"],
    deps = [":websocket_bridge"],
)

heph_cc_test(
    name = "ipc_entity_manager_tests",
    srcs = [
        "tests/ipc_entity_manager_tests.cpp",
    ],
    tags = ["exclusive"],
    deps = [":websocket_bridge"] + [
        "//modules/ipc",
        "//modules/random",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "@fmt",
    ],
)

heph_cc_test(
    name = "protobuf_serdes_tests",
    srcs = [
        "tests/protobuf_serdes_tests.cpp",
    ],
    deps = [":websocket_bridge"],
)
