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
        "//modules/telemetry/metrics",
        "//modules/utils",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/synchronization:synchronization",
        "@fmt",
        "@reflect-cpp",
        "@stdexec",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "periodic_spinner",
    srcs = ["examples/periodic_spinner.cpp"],
    deps = [
        ":conduit",
        "//modules/concurrency",
        "//modules/utils",
        "@fmt",
        "@stdexec",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "basic_input_tests",
    srcs = ["tests/basic_input_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "input_tests",
    srcs = ["tests/input_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "output_tests",
    srcs = ["tests/output_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "stepper_tests",
    srcs = ["tests/stepper_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "node_tests",
    srcs = ["tests/node_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/concurrency",
        "//modules/telemetry/log",
        "//modules/types_proto",
        "@fmt",
        "@stdexec",
    ],
)

heph_cc_test(
    name = "graph_tests",
    srcs = ["tests/graph_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "distributed_graph_tests",
    srcs = ["tests/distributed_graph_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "zenoh_publisher_tests",
    srcs = ["tests/zenoh_publisher_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)

heph_cc_test(
    name = "zenoh_subscriber_tests",
    srcs = ["tests/zenoh_subscriber_tests.cpp"],
    deps = [
        ":conduit",
        "//modules/types_proto",
    ],
)
