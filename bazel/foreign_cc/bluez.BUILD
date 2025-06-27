# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

cc_library(
    name = "bluez",
    srcs = glob([
        "lib/**/*.c",
        "lib/**/*.h",
    ]),
    hdrs = glob(["lib/**/*.h"]),
    include_prefix = "bluetooth",
    includes = ["lib"],
    strip_include_prefix = "lib",
    visibility = ["//visibility:public"],
)
