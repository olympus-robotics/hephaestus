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
    includes = ["include"],
    visibility = ["//visibility:public"],
    implementation_deps = [
        "//modules/random",
    ],
    deps=[
        "//modules/serdes",
        "//modules/utils",
        "//modules/types",
        "//modules/types_proto/proto:types_gen_proto",

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
