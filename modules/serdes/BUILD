load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

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
        "@abseil-cpp//absl/log:log",
        "@abseil-cpp//absl/status:status",
        "@abseil-cpp//absl/status:statusor",
        "@com_github_ericniebler_range_v3//:range-v3",
        "@fmt",
        "@magic_enum",
        "@nlohmann_json//:json",
        "@protobuf",
    ],
)

#########
# Tests #
#########

PROTO_PATH = "/modules/serdes/tests"

proto_library(
    name = "test_proto",
    srcs = ["tests/test_user_type.proto"],
    strip_import_prefix = PROTO_PATH,
)

cc_proto_library(
    name = "test_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":test_proto"],
)

heph_cc_test(
    name = "tests",
    srcs = [
        "tests/test_proto_conversion.h",
        "tests/tests.cpp",
    ],
    deps = [
        ":serdes",
        ":test_cc_proto",
    ],
)
