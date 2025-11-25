# =================================================================================================
# Copyright (C) 2023-2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "conduit",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob(
        [
            "include/**/*.h",
        ],
    ),
    implementation_deps = [
        "//modules/telemetry/metrics",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "//modules/containers",
        "//modules/error_handling",
        "//modules/ipc",
        "//modules/net",
        "//modules/serdes",
        "//modules/telemetry/log",
        "//modules/types_proto",  #TODO(@heller) do we really want that?
        "//modules/utils",
        "@fmt",
        "@stdexec",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "conduit_clock_drift",
    srcs = ["examples/conduit_clock_drift.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
        "//modules/telemetry/influxdb_sink",
        "//modules/telemetry/log",
        "//modules/telemetry/metrics",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "mont_blanc",
    srcs = ["examples/mont_blanc.cpp"],
    deps = [
        ":conduit",
        "//modules/format",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
        "@stdexec",
    ],
)

heph_cc_binary(
    name = "conduit_graph",
    srcs = ["examples/conduit_graph.cpp"],
    deps = [
        ":conduit",
        "//modules/telemetry/log",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "conduit_publisher",
    srcs = ["examples/conduit_publisher.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
        "//modules/net",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "conduit_subscriber",
    srcs = ["examples/conduit_subscriber.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
        "//modules/format",
        "//modules/net",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "conduit_input_publisher",
    srcs = ["examples/conduit_input_publisher.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
        "//modules/net",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "conduit_input_subscriber",
    srcs = ["examples/conduit_input_subscriber.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
        "//modules/format",
        "//modules/net",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@fmt",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "node_tests",
    srcs = ["tests/node_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/concurrency",
        "//modules/telemetry/log",
        "@fmt",
        "@stdexec",
    ],
)

heph_cc_test(
    name = "input_output_tests",
    srcs = ["tests/input_output_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/concurrency",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "@fmt",
        "@stdexec",
    ],
)

heph_cc_test(
    name = "zenoh_nodes_tests",
    srcs = ["tests/zenoh_nodes_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "remote_publisher_tests",
    srcs = ["tests/remote_publisher_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/error_handling",
        "//modules/net",
        "//modules/telemetry/log",
        "//modules/types",
        "//modules/types_proto",
        "//modules/utils",
        "@stdexec",
    ],
)
