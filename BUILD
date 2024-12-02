# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

package(default_visibility = ["//visibility:public"])

exports_files([
    ".clang-tidy",
    "buf.yaml",
    ".clang-format",
])

alias(
    name = "format",
    actual = "//bazel/tools:format",
)

alias(
    name = "format.check",
    actual = "//bazel/tools:format.check",
)

refresh_compile_commands(
    name = "refresh_compile_commands",
    targets = {
        "//modules/...": "",
    },
)
