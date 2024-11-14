"""
extensions.bzl - Bazel extensions for Hephaestus
"""

load("//bazel:repositories.bzl", "bazel_repositories", "cmake_repositories")

def _external_dependencies_impl(_ctx):
    bazel_repositories()
    cmake_repositories()

external_dependencies = module_extension(
    implementation = _external_dependencies_impl,
)
