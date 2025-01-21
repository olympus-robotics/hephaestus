# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "format",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/utils",
        "@fmt",
        "@reflect-cpp",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "generic_formatter_tests",
    srcs = ["tests/generic_formatter_tests.cpp"],
    deps = [
        ":format",
        "//modules/types",
        "//modules/types_proto",
    ],
)
