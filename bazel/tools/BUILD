# =================================================================================================
# Copyright (C) 2020 Benedek Thaler Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@aspect_rules_lint//format:defs.bzl", "format_multirun")

package(default_visibility = ["//visibility:public"])

alias(
    name = "buf",
    actual = "@rules_buf_toolchains//:buf",
)

format_multirun(
    name = "format",
    c = "@llvm_toolchain_llvm//:bin/clang-format",
    cc = "@llvm_toolchain_llvm//:bin/clang-format",
    protocol_buffer = ":buf",
    starlark = "@buildifier_prebuilt//:buildifier",
)
