# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

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
        "//modules/containers_reflection",
        "//modules/utils",
        "@fmt",
        "@magic_enum",
        "@reflect-cpp",
    ],
)

############
# Examples #
############

heph_cc_binary(
    name = "generic_formatter_example",
    srcs = [
        "examples/generic_formatter_example.cpp",
    ],
    deps = [
        ":format",
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

heph_cc_test(
    name = "formatter_collisions_tests",
    srcs = ["tests/formatter_collisions_tests.cpp"],
    deps = [
        ":format",
    ],
)

heph_cc_test(
    name = "types_formatter_tests",
    srcs = ["tests/types_formatter_tests.cpp"],
    deps = [
        ":format",
        "//modules/types",
    ],
)
