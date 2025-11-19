# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "test_utils",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/error_handling",
        "//modules/random",
        "//modules/telemetry/log",
        "//modules/utils",
        "@googletest//:gtest",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)
