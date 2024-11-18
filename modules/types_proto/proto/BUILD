load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")

PROTO_PATH = "/modules/types_proto/proto"

proto_library(
    name = "types_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    strip_import_prefix = PROTO_PATH,
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "types_gen_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":types_gen_proto"],
)
