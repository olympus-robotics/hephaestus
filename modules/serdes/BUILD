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
    implementation_deps = [
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/error_handling",
        "//modules/utils",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@fmt",
        "@magic_enum",
        "@nlohmann_json//:json",
        "@protobuf//:json_util",  # json_util.h
        "@protobuf//:protobuf_headers",  # descriptor.h
        "@protobuf//:protobuf_lite",  # map.h
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
        "@nlohmann_json//:json",
    ],
)

heph_cc_test(
    name = "type_info_tests",
    srcs = [
        "tests/test_proto_conversion.h",
        "tests/type_info_tests.cpp",
    ],
    deps = [
        ":serdes",
        ":test_gen_proto",
        "//modules/random",
        "//modules/utils",
    ],
)
