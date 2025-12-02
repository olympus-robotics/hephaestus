# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load(
    "@hephaestus//bazel:hephaestus.bzl",
    "heph_cc_binary",
    "heph_cc_library",
    "heph_cc_test",
)

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "error_handling",
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
        "//modules/telemetry/log",
        "//modules/utils",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@fmt",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "panic_tests",
    srcs = ["tests/panic_tests.cpp"],
    deps = [
        ":error_handling",
        "//modules/telemetry/log",
    ],
)
