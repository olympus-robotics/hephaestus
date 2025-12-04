# =================================================================================================
# Copyright (C) 2023-2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "concurrency",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/telemetry/log",
        "//modules/telemetry/metrics",
        "//modules/utils",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/containers",
        "//modules/error_handling",
        "@boost.container",
        "@liburing",
        "@stdexec",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "example",
    srcs = ["examples/spinner_example.cpp"],
    deps = [
        ":concurrency",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "clock_drift",
    srcs = ["examples/clock_drift.cpp"],
    deps = [
        ":concurrency",
        "//modules/cli",
        "//modules/telemetry/influxdb_sink",
        "//modules/telemetry/log",
        "//modules/telemetry/metrics",
        "//modules/utils",
        "@fmt",
        "@stdexec",
    ],
)

heph_cc_binary(
    name = "spinner_clock_drift",
    srcs = ["examples/spinner_clock_drift.cpp"],
    deps = [
        ":concurrency",
        "//modules/cli",
        "//modules/telemetry/influxdb_sink",
        "//modules/telemetry/log",
        "//modules/telemetry/metrics",
        "//modules/utils",
        "@fmt",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "io_ring_tests",
    srcs = ["tests/io_ring/io_ring_tests.cpp"],
    deps = [
        ":concurrency",
        "@liburing",
    ],
)

heph_cc_test(
    name = "context_tests",
    srcs = ["tests/context_tests.cpp"],
    deps = [
        ":concurrency",
        "@stdexec",
    ],
)

heph_cc_test(
    name = "any_sender_tests",
    srcs = ["tests/any_sender_tests.cpp"],
    deps = [":concurrency"],
)

heph_cc_test(
    name = "when_all_range_tests",
    srcs = ["tests/when_all_range_tests.cpp"],
    deps = [":concurrency"],
)

heph_cc_test(
    name = "queue_consumer_tests",
    srcs = ["tests/queue_consumer_tests.cpp"],
    deps = [
        ":concurrency",
        "//modules/random",
    ],
)

heph_cc_test(
    name = "spinner_state_machine_tests",
    srcs = ["tests/spinner_state_machine_tests.cpp"],
    deps = [":concurrency"],
)

heph_cc_test(
    name = "spinner_tests",
    srcs = ["tests/spinner_tests.cpp"],
    deps = [
        ":concurrency",
        "//modules/error_handling",
    ],
)

heph_cc_test(
    name = "spinners_manager_tests",
    srcs = ["tests/spinners_manager_tests.cpp"],
    deps = [
        ":concurrency",
    ],
)
