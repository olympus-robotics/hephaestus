"""
extensions.bzl - Bazel extensions for Hephaestus
"""

load("//bazel:repositories.bzl", "init_repositories")

def _non_module_dependencies_impl(_ctx):
    init_repositories()

non_module_dependencies = module_extension(
    implementation = _non_module_dependencies_impl,
)
