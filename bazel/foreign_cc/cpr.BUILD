# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

cc_library(
    name = "cpr",
    hdrs = glob(["include/cpr/**/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    srcs = glob(["cpr/**/*.cpp"]),
    deps = ["@curl"],
)
