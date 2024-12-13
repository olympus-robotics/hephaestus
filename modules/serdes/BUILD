# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_proto_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "serdes",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/utils",
        "@abseil-cpp//absl/status:status",
        "@abseil-cpp//absl/status:statusor",
        "@fmt",
        "@magic_enum",
        "@nlohmann_json//:json",
        "@protobuf",
        "@range-v3//:range-v3",
    ],
)

#########
# Tests #
#########

PROTO_MODULE = "modules/serdes/tests"

heph_cc_proto_library(
    name = "test_gen_proto",
    srcs = ["tests/test_user_type.proto"],
    module = PROTO_MODULE,
)

heph_cc_test(
    name = "tests",
    srcs = [
        "tests/test_proto_conversion.h",
        "tests/tests.cpp",
    ],
    deps = [
        ":serdes",
        ":test_gen_proto",
    ],
)
