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
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "//modules/ipc",
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
        "//modules/telemetry/telemetry_influxdb_sink",
    ],
)

heph_cc_binary(
    name = "mont_blanc",
    srcs = ["examples/mont_blanc.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
    ],
)

heph_cc_binary(
    name = "conduit_graph",
    srcs = ["examples/conduit_graph.cpp"],
    deps = [
        ":conduit",
        "//modules/cli",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "node_tests",
    srcs = ["tests/node_tests.cpp"],
    deps = [":conduit"],
)

heph_cc_test(
    name = "input_output_tests",
    srcs = ["tests/input_output_tests.cpp"],
    deps = [":conduit"],
)

heph_cc_test(
    name = "zenoh_nodes_tests",
    srcs = ["tests/zenoh_nodes_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types",
        "//modules/types_proto",
    ],
)
