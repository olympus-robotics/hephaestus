# =================================================================================================
# Copyright (C) 2020 Benedek Thaler Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@bazel_skylib//rules:native_binary.bzl", "native_binary")
load("@rules_python//python/entry_points:py_console_script_binary.bzl", "py_console_script_binary")

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

filegroup(
    name = "clangd_config_file",
    srcs = ["//:.clangd"],
    visibility = ["//visibility:public"],
)

label_flag(
    name = "clangd_config",
    build_setting_default = ":clangd_config_file",
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
)

py_console_script_binary(
    name = "clangd_tidy",
    pkg = "@pypi-heph//clangd_tidy",
    script = "clangd-tidy",
)

label_flag(
    name = "clangd_tidy_bin",
    build_setting_default = ":clangd_tidy",
)

native_binary(
    name = "clangd_bin_file",
    src = "@llvm_toolchain_llvm//:bin/clangd",
    out = "clangd",
)

label_flag(
    name = "clangd_bin",
    build_setting_default = ":clangd_bin_file",
)

string_flag(
    name = "use_clangd",
    build_setting_default = "False",
    values = [
        "True",
        "False",
    ],
    visibility = ["//visibility:public"],
)
