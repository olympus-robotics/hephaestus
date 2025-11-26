# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "test_utils",
    srcs = glob(
        [
            "src/**/*.cpp",
        ],
    ),
    hdrs = glob(
        [
            "include/**/*.h",
        ],
    ),
    implementation_deps = [
        "//modules/random",
        "//modules/telemetry/log",
        "//modules/telemetry/metrics",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/error_handling",
        "//modules/utils",
        "@googletest//:gtest",
    ],
)

heph_cc_test(
    name = "test_utils_tests",
    srcs = glob([
        "tests/**/*.cpp",
    ]),
    deps = [
    ],
)
