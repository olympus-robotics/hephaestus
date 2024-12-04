# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

config_setting(
    name = "clang_compiler",
    flag_values = {
        "@bazel_tools//tools/cpp:compiler": "clang",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "gcc_compiler",
    flag_values = {
        "@bazel_tools//tools/cpp:compiler": "gcc",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "dbg_compilation",
    values = {
        "compilation_mode": "dbg",
    },
)

config_setting(
    name = "opt_compilation",
    values = {
        "compilation_mode": "opt",
    },
)

config_setting(
    name = "fastbuild_compilation",
    values = {
        "compilation_mode": "fastbuild",
    },
)
