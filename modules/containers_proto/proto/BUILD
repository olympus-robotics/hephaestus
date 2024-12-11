# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_proto_library")

PROTO_MODULE = "modules/containers_proto/proto"

heph_cc_proto_library(
    name = "containers_gen_proto",
    srcs = glob([
        "**/*.proto",
    ]),
    module = PROTO_MODULE,
    visibility = ["//visibility:public"],
)
