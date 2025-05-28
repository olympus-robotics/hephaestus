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

# ghcr.io/olympus-robotics/x86_64/hephaestus-dev:latest
platform(
    name = "docker_image_platform",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
        "@bazel_tools//tools/cpp:clang",
    ],
    exec_properties = {
        "OSFamily": "Linux",
        "dockerNetwork": "off",
        "container-image": "docker://ghcr.io/olympus-robotics/x86_64/hephaestus-dev:latest",
    },
)
