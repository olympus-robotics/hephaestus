# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "log",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/containers",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/synchronization:synchronization",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "@fmt",
        "@magic_enum",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "log_sink_example",
    srcs = ["examples/log_sink_example.cpp"],
    deps = [
        ":log",
        "//modules/cli",
        "//modules/utils",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "log_tests",
    srcs = ["tests/log_tests.cpp"],
    deps = [
        ":log",
        "//modules/containers",
        "@fmt",
    ],
)

heph_cc_test(
    name = "log_tests_no_sink",
    srcs = ["tests/log_tests_no_sink.cpp"],
    deps = [
        ":log",
    ],
)
