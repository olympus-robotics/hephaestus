# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "ipc",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/cli",
        "//modules/concurrency",
        "//modules/containers",
        "//modules/ipc/proto:types_gen_proto",
        "//modules/serdes",
        "//modules/telemetry/telemetry",
        "//modules/utils",
        "@fmt",
        "@nlohmann_json//:json",
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
    ],
)

heph_cc_binary(
    name = "zenoh_string_service_client",
    srcs = ["apps/zenoh_string_service_client.cpp"],
    deps = [
        ":ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_topic_echo",
    srcs = ["apps/zenoh_topic_echo.cpp"],
    deps = [
        ":ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_topic_list",
    srcs = ["apps/zenoh_topic_list.cpp"],
    deps = [
        ":ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_ipc_graph",
    srcs = [
        "apps/zenoh_ipc_graph.cpp",
    ],
    deps = [
        ":ipc",
    ],
)

#########
# Tests #
#########

IPC_TEST_DEPS = [
    ":ipc",
    "//modules/random",
    "//modules/types",
    "//modules/types_proto",
]

heph_cc_test(
    name = "action_server_tests",
    srcs = [
        "tests/action_server_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "dynamic_subscriber_tests",
    srcs = [
        "tests/dynamic_subscriber_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "pollable_action_server_tests",
    srcs = [
        "tests/pollable_action_server_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "pub_sub_tests",
    srcs = [
        "tests/pub_sub_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "service_tests",
    srcs = [
        "tests/service_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "topic_filter_tests",
    srcs = [
        "tests/topic_filter_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "liveliness_tests",
    srcs = [
        "tests/liveliness_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "config_tests",
    srcs = [
        "tests/config_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "session_tests",
    srcs = [
        "tests/session_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "topic_database_tests",
    srcs = [
        "tests/topic_database_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "ipc_graph_tests",
    srcs = [
        "tests/ipc_graph_tests.cpp",
    ],
    flaky = True,
    tags = ["exclusive"],
    deps = IPC_TEST_DEPS,
)
