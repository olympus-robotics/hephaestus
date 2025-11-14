# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "random",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "@fmt",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/error_handling",
        "//modules/utils",
        "@magic_enum",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "random_number_generator_tests",
    srcs = ["tests/random_number_generator_tests.cpp"],
    deps = [":random"],
)

heph_cc_test(
    name = "random_object_creator_tests",
    srcs = ["tests/random_object_creator_tests.cpp"],
    deps = [
        ":random",
        "//modules/utils",
    ],
)
