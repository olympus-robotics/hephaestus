load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")

PROTO_PATH = "/modules/types_proto/proto"

proto_library(
    name = "numeric_value_proto",
    srcs = ["hephaestus/types/proto/numeric_value.proto"],
    strip_import_prefix = PROTO_PATH,
)

proto_library(
    name = "bounds_proto",
    srcs = ["hephaestus/types/proto/bounds.proto"],
    strip_import_prefix = PROTO_PATH,
    deps = [":numeric_value_proto"],
)

proto_library(
    name = "dummy_type_proto",
    srcs = ["hephaestus/types/proto/dummy_type.proto"],
    strip_import_prefix = PROTO_PATH,
)

cc_proto_library(
    name = "numeric_value_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":numeric_value_proto"],
)

cc_proto_library(
    name = "bounds_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":bounds_proto"],
)

cc_proto_library(
    name = "dummy_type_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":dummy_type_proto"],
)
