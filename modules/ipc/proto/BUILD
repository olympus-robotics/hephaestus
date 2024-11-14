load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

PROTO_PATH = "/modules/ipc/proto"

cc_proto_library(
    name = "types_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":types_proto"],
)

proto_library(
    name = "types_proto",
    srcs = ["hephaestus/ipc/zenoh/action_server/proto/types.proto"],
    strip_import_prefix = PROTO_PATH,
    deps = ["@protobuf//:any_proto"],
)
