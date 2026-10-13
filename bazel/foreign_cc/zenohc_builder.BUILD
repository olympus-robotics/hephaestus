# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

cc_library(
    name = "zenoh-c",
    # @platforms is only a dev dependency here, hence the bazel_tools condition
    srcs = select({
        "@bazel_tools//src/conditions:linux_aarch64": ["lib/aarch64/libzenohc.a"],
        "//conditions:default": ["lib/libzenohc.a"],
    }),
    hdrs = glob(["include/**/*.h"]),
    defines = [
        "ZENOH_WITH_UNSTABLE_API",
        "Z_FEATURE_UNSTABLE_API",
        "ZENOHCXX_ZENOHC",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "zenoh-cpp",
    hdrs = glob([
        "include/**/*.hxx",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [":zenoh-c"],
)
