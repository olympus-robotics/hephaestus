# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "containers_reflection",
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
        "//modules/utils",
        "@reflect-cpp",
    ],
)

##############
# Tests      #
##############

heph_cc_test(
    name = "containers_reflection_tests",
    srcs = ["tests/containers_reflection_tests.cpp"],
    deps = [
        ":containers_reflection",
    ],
)
