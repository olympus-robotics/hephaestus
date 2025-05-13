load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_proto_library")

PROTO_MODULE = "modules/ipc/proto"

heph_cc_proto_library(
    name = "types_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    module = PROTO_MODULE,
    visibility = ["//visibility:public"],
    deps = ["@protobuf//:any_proto"],
)
