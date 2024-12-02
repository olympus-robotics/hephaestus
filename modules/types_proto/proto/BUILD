load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_proto_library")

PROTO_PATH = "/modules/types_proto/proto"

heph_cc_proto_library(
    name = "types_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    strip_import_prefix = PROTO_PATH,
    visibility = ["//visibility:public"],
)
