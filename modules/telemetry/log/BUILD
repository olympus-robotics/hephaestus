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
        "@abseil-cpp//absl/log:absl_log",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/utils",
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
        "//modules/random",
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
        "//modules/random",
    ],
)

heph_cc_test(
    name = "log_tests_no_sink",
    srcs = ["tests/log_tests_no_sink.cpp"],
    deps = [
        ":log",
        "//modules/random",
    ],
)
