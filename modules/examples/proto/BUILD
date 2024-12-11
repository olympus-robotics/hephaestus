load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_proto_library")

PROTO_MODULE = "modules/examples/proto"

heph_cc_proto_library(
    name = "examples_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    module = PROTO_MODULE,
    visibility = ["//visibility:public"],
)
