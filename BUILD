# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_api_doc")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@rules_python//sphinxdocs:sphinx.bzl", "sphinx_build_binary", "sphinx_docs")
load("@rules_python//sphinxdocs:sphinx_docs_library.bzl", "sphinx_docs_library")

package(default_visibility = ["//visibility:public"])

exports_files([
    ".clangd",
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

############################
# Python
############################
compile_pip_requirements(
    name = "python_requirements",
    src = "requirements.in",
    requirements_txt = "requirements_lock.txt",
)

############################
# Documentation generation
############################

sphinx_docs_library(
    name = "sources",
    srcs = glob(["doc/*.rst"]) + ["//:README.rst"],
)

sphinx_docs_library(
    name = "examples",
    srcs = ["//modules/conduit:examples/mont_blanc.cpp"],
)

sphinx_build_binary(
    name = "sphinx",
    deps = [
        "@pypi-heph//breathe",
        "@pypi-heph//sphinx",
        "@pypi-heph//sphinx_book_theme",
        "@pypi-heph//sphinxcontrib_mermaid",
    ],
)

heph_cc_api_doc(
    name = "apidoc",
    targets = [
        "//modules/bag:bag",
        "//modules/cli:cli",
        "//modules/concurrency:concurrency",
        "//modules/conduit:conduit",
        "//modules/containers:containers",
        "//modules/containers_proto:containers_proto",
        "//modules/containers_reflection:containers_reflection",
        "//modules/format:format",
        "//modules/ipc:ipc",
        "//modules/net:net",
        "//modules/random:random",
        "//modules/serdes:serdes",
        "//modules/telemetry/log:log",
        "//modules/telemetry/metrics:metrics",
        "//modules/types:types",
        "//modules/types_proto:types_proto",
        "//modules/utils:utils",
        "//modules/websocket_bridge:websocket_bridge",
    ],
)

doc_deps = [
    ":examples",
    ":sources",
]

sphinx_docs(
    name = "docs-fast",
    config = "doc/conf.py",
    formats = [
        "html",
    ],
    sphinx = ":sphinx",
    deps = doc_deps,
)

sphinx_docs(
    name = "docs",
    config = "doc/conf.py",
    formats = [
        "html",
    ],
    sphinx = ":sphinx",
    deps = [
        ":apidoc",
    ] + doc_deps,
)
