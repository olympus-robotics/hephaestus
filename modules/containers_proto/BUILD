# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "containers_proto",
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
        "//modules/containers_proto/proto:containers_gen_proto",
        "//modules/serdes",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "bitflag_serialization_tests",
    srcs = ["tests/bitflag_serialization_tests.cpp"],
    deps = [":containers_proto"],
)
