load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_proto_library")

PROTO_PATH = "/modules/examples/proto"

heph_cc_proto_library(
    name = "examples_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    strip_import_prefix = PROTO_PATH,
    visibility = ["//visibility:public"],
)
