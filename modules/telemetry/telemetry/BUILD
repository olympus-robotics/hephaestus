# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "telemetry",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/containers",
        "//modules/random",
        "@abseil-cpp//absl/log:absl_log",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
<<<<<<< HEAD
        "//modules/serdes",
=======
        "//modules/containers",
>>>>>>> 304e701d (improve)
        "//modules/utils",
        "@fmt",
        "@magic_enum",
        "@reflect-cpp",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "metric_sink_example",
    srcs = ["examples/metric_sink_example.cpp"],
    deps = [
        ":telemetry",
        "//modules/random",
    ],
)

heph_cc_binary(
    name = "log_sink_example",
    srcs = ["examples/log_sink_example.cpp"],
    deps = [
        ":telemetry",
        "//modules/cli",
        "//modules/random",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "metric_record_tests",
    srcs = ["tests/metric_record_tests.cpp"],
    deps = [
        ":telemetry",
        "//modules/random",
    ],
)

heph_cc_test(
    name = "log_tests",
    srcs = ["tests/log_tests.cpp"],
    deps = [
        ":telemetry",
        "//modules/random",
    ],
)

heph_cc_test(
    name = "log_tests_no_sink",
    srcs = ["tests/log_tests_no_sink.cpp"],
    deps = [
        ":telemetry",
        "//modules/random",
    ],
)
