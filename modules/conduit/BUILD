# =================================================================================================
# Copyright (C) 2023-2025 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "conduit",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/cli",
        "//modules/utils",
        "//modules/telemetry/telemetry",
        "@fmt",
        "@liburing",
        "@stdexec"
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "example",
    srcs = ["examples/example.cpp"],
    deps = [
        ":conduit",
    ],
)

heph_cc_binary(
    name = "mont-blanc",
    srcs = ["examples/mont-blanc.cpp"],
    deps = [
        ":conduit",
    ],
)

#########
# Tests #
#########

