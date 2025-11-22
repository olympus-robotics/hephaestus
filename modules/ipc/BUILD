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
    name = "ipc",
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
        "//modules/utils",
        "@abseil-cpp//absl/strings:strings",
        "@nlohmann_json//:json",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/cli",
        "//modules/concurrency",
        "//modules/error_handling",
        "//modules/ipc/proto:types_gen_proto",
        "//modules/random",
        "//modules/serdes",
        "//modules/telemetry/log",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization:synchronization",
        "@fmt",
        "@magic_enum",
        "@zenohc_builder//:zenoh-c",
        "@zenohc_builder//:zenoh-cpp",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "zenoh_node_list",
    srcs = ["apps/zenoh_node_list.cpp"],
    deps = [
        ":ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "zenoh_string_service_client",
    srcs = ["apps/zenoh_string_service_client.cpp"],
    deps = [
        ":ipc",
        "//modules/cli",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "zenoh_topic_echo",
    srcs = ["apps/zenoh_topic_echo.cpp"],
    deps = [
        ":ipc",
        "//modules/cli",
        "//modules/error_handling",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
        "@nlohmann_json//:json",
    ],
)

heph_cc_binary(
    name = "zenoh_topic_list",
    srcs = ["apps/zenoh_topic_list.cpp"],
    deps = [
        ":ipc",
        "//modules/cli",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "zenoh_ipc_graph",
    srcs = [
        "apps/zenoh_ipc_graph.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/cli",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "action_server_tests",
    srcs = [
        "tests/action_server_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/random",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_test(
    name = "dynamic_subscriber_tests",
    srcs = [
        "tests/dynamic_subscriber_tests.cpp",
    ],
    deps = [":ipc"],
)

heph_cc_test(
    name = "pollable_action_server_tests",
    srcs = [
        "tests/pollable_action_server_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/error_handling",
        "//modules/random",
        "//modules/types",
        "//modules/types_proto",
        "@fmt",
    ],
)

heph_cc_test(
    name = "pub_sub_tests",
    srcs = [
        "tests/pub_sub_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/error_handling",
        "//modules/random",
        "//modules/serdes",
        "//modules/types",
        "//modules/types_proto",
        "@fmt",
    ],
)

heph_cc_test(
    name = "service_tests",
    srcs = [
        "tests/service_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/random",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "@fmt",
    ],
)

heph_cc_test(
    name = "topic_filter_tests",
    srcs = [
        "tests/topic_filter_tests.cpp",
    ],
    deps = [":ipc"],
)

heph_cc_test(
    name = "liveliness_tests",
    srcs = [
        "tests/liveliness_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/random",
        "@fmt",
        "@zenohc_builder//:zenoh-cpp",
    ],
)

heph_cc_test(
    name = "config_tests",
    srcs = [
        "tests/config_tests.cpp",
    ],
    deps = [
        ":ipc",
        "@nlohmann_json//:json",
    ],
)

heph_cc_test(
    name = "session_tests",
    srcs = [
        "tests/session_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/random",
    ],
)

heph_cc_test(
    name = "topic_database_tests",
    srcs = [
        "tests/topic_database_tests.cpp",
    ],
    deps = [
        ":ipc",
        "//modules/random",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "@fmt",
    ],
)

heph_cc_test(
    name = "ipc_graph_tests",
    srcs = [
        "tests/ipc_graph_tests.cpp",
    ],
    flaky = True,
    tags = ["exclusive"],
    deps = [
        ":ipc",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
    ],
)
