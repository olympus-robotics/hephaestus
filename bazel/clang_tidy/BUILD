# =================================================================================================
# Copyright (C) 2020 Benedek Thaler Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@bazel_skylib//rules:native_binary.bzl", "native_binary")

sh_binary(
    name = "clang_tidy_wrapper",
    srcs = [":run_clang_tidy.sh"],
    data = [":clang_tidy_config"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "clang_tidy_config_file",
    srcs = ["//:.clang-tidy"],
    visibility = ["//visibility:public"],
)

label_flag(
    name = "clang_tidy_config",
    build_setting_default = ":clang_tidy_config_file",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "clang_tidy_additional_deps_default",
    srcs = [],
)

label_flag(
    name = "clang_tidy_additional_deps",
    build_setting_default = ":clang_tidy_additional_deps_default",
    visibility = ["//visibility:public"],
)

native_binary(
    name = "clang_tidy_bin_file",
    src = "@llvm_toolchain_llvm//:bin/clang-tidy",
    out = "clang_tidy",
)

label_flag(
    name = "clang_tidy_bin",
    build_setting_default = ":clang_tidy_bin_file",
    visibility = ["//visibility:public"],
)
