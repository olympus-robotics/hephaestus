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
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/containers",
        "//modules/telemetry/telemetry",
        "//modules/utils",
        "@liburing",
        "@magic_enum",
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
    ],
)

heph_cc_binary(
    name = "clock_drift",
    srcs = ["examples/clock_drift.cpp"],
    deps = [
        ":concurrency",
        "//modules/cli",
        "//modules/telemetry/telemetry_influxdb_sink",
    ],
)

heph_cc_binary(
    name = "spinner_clock_drift",
    srcs = ["examples/spinner_clock_drift.cpp"],
    deps = [
        ":concurrency",
        "//modules/cli",
        "//modules/telemetry/telemetry_influxdb_sink",
    ],
)

#########
# Tests #
#########

CONCURRENCY_TEST_DEPS = [
    ":concurrency",
    "//modules/random",
]

heph_cc_test(
    name = "io_ring_tests",
    srcs = ["tests/io_ring/io_ring_tests.cpp"],
    deps = CONCURRENCY_TEST_DEPS,
)

heph_cc_test(
    name = "context_tests",
    srcs = ["tests/context_tests.cpp"],
    deps = CONCURRENCY_TEST_DEPS,
)

heph_cc_test(
    name = "queue_consumer_tests",
    srcs = ["tests/queue_consumer_tests.cpp"],
    deps = CONCURRENCY_TEST_DEPS,
)

heph_cc_test(
    name = "spinner_state_machine_tests",
    srcs = ["tests/spinner_state_machine_tests.cpp"],
    deps = CONCURRENCY_TEST_DEPS,
)

heph_cc_test(
    name = "spinner_tests",
    srcs = ["tests/spinner_tests.cpp"],
    deps = CONCURRENCY_TEST_DEPS,
)

heph_cc_test(
    name = "spinners_manager_tests",
    srcs = ["tests/spinners_manager_tests.cpp"],
    deps = CONCURRENCY_TEST_DEPS,
)
