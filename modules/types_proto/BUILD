# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

heph_cc_library(
    name = "types_proto",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
        "//modules/random",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/serdes",
        "//modules/types",
        "//modules/types_proto/proto:types_gen_proto",
        "//modules/utils",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "serialization_tests",
    srcs = ["tests/serialization_tests.cpp"],
    deps = [
        ":types_proto",
    ],
)
