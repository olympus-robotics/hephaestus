# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

cc_library(
    name = "zenoh-c",
    srcs = ["lib/libzenohc.a"],
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    defines = [
        "ZENOH_WITH_UNSTABLE_API",
        "ZENOHCXX_ZENOHC",
    ],
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