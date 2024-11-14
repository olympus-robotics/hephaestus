load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")

PROTO_PATH = "/modules/examples/proto"

proto_library(
    name = "examples_proto",
    srcs = [
        "hephaestus/examples/types/proto/geometry.proto",
        "hephaestus/examples/types/proto/pose.proto",
        "hephaestus/examples/types/proto/sample.proto",
    ],
    strip_import_prefix = PROTO_PATH,
)

cc_proto_library(
    name = "examples_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":examples_proto"],
)
