# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_proto_library")

PROTO_PATH = "/modules/containers_proto/proto"

heph_cc_proto_library(
    name = "containers_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    strip_import_prefix = PROTO_PATH,
    visibility = ["//visibility:public"],
)
