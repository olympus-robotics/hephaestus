# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:repositories.bzl", "binary_repositories", "cmake_repositories")

def _external_dependencies_impl(_ctx):
    binary_repositories()
    cmake_repositories()

external_dependencies = module_extension(
    implementation = _external_dependencies_impl,
)
