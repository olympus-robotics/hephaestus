# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:repositories.bzl", "foreign_cc_repositories")

def _external_dependencies_impl(_ctx):
    foreign_cc_repositories()

external_dependencies = module_extension(
    implementation = _external_dependencies_impl,
)
