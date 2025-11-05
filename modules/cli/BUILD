# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "cli",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/utils",
        "@abseil-cpp//absl/strings:ascii",
        "@fmt",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "example",
    srcs = ["examples/example.cpp"],
    deps = [
        ":cli",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "program_options_tests",
    srcs = ["tests/program_options_tests.cpp"],
    deps = [
        ":cli",
    ],
)
