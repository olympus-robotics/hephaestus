# =================================================================================================
# Copyright (C) 2023-2025 HEPHAESTUS Contributors
# =================================================================================================

load(
    "@hephaestus//bazel:hephaestus.bzl",
    "heph_cc_binary",
    "heph_cc_library",
    "heph_cc_test",
)

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "net",
    srcs = glob(
        [
            "src/**/*.cpp",
            "include/**/detail/*.h",
        ],
    ),
    hdrs = glob(
        [
            "include/**/*.h",
        ],
        exclude = [
            "include/**/detail/**",
        ],
    ),
    implementation_deps = [
        "//modules/telemetry/telemetry",
        "//modules/utils",
        "@bluez",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "@liburing",
        "@stdexec",
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
        "//modules/telemetry/telemetry",
    ],
)

heph_cc_binary(
    name = "socketcan_ping_pong",
    srcs = ["examples/socketcan_ping_pong.cpp"],
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
        "//modules/telemetry/telemetry",
    ],
)

heph_cc_binary(
    name = "bt_server",
    srcs = ["examples/bt_server.cpp"],
    deps = [
        ":net",
        "//modules/cli",
        "//modules/telemetry/telemetry",
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
