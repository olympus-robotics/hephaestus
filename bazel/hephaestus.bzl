# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

def heph_basic_copts():
    return [
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        "-Wshadow",  # warn the user if a variable declaration shadows one from a parent context
        "-Wnon-virtual-dtor",  # warn if a class with virtual functions has a non-virtual destructor.
        "-Wold-style-cast",  # warn for c-style casts
        "-Wcast-align",  # warn for potential performance problem casts
        "-Wunused",  # warn on anything being unused
        "-Woverloaded-virtual",  # warn if you overload (not override) a virtual function
        "-Wconversion",  # warn on type conversions that may lose data
        "-Wsign-conversion",  # warn on sign conversions
        "-Wnull-dereference",  # warn if a null dereference is detected
        "-Wdouble-promotion",  # warn if float is implicit promoted to double
        "-Wformat=2",  # warn on security issues around functions that format output (ie printf)
        "-Wimplicit-fallthrough",  # warn on statements that fallthrough without an explicit annotation
        "-Iexternal/abseil-cpp~",  # This is needed to avoid the error: file not found with <angled> include; use "quotes" instead
    ] + select({
        "@hephaestus//bazel:opt_compilation": ["-O3"],
        "//conditions:default": [],
    })

def heph_clang_copts():
    return [
        "-fcolor-diagnostics",
        "-fPIC",
    ]

def heph_gcc_copts():
    return [
        "-Wmisleading-indentation",
        "-Wduplicated-cond",
        "-Wduplicated-branches",
        "-Wlogical-op",
        "-Wuseless-cast",
        "-fdiagnostics-color=always",
    ]

def heph_copts():
    return select({
        "@hephaestus//bazel:clang_compiler": heph_clang_copts(),
        "@hephaestus//bazel:gcc_compiler": heph_gcc_copts(),
    }) + heph_basic_copts()

def heph_linkopts():
    return select({
        "@hephaestus//bazel:clang_compiler": ["--rtlib=compiler-rt"],
        "@hephaestus//bazel:gcc_compiler": [],
    })

# Create a filegroup containing all the headers file needed by the input target
def _cc_header_impl(ctx):
    headers = []
    if CcInfo in ctx.attr.target:
        cc_info = ctx.attr.target[CcInfo]
        headers = cc_info.compilation_context.headers
    return [DefaultInfo(files = headers)]

heph_cc_extract_headers = rule(
    attrs = {
        "target": attr.label(mandatory = True),
    },
    implementation = _cc_header_impl,
)

def heph_cc_library(
        name,
        hdrs,
        copts = [],
        linkopts = [],
        **kwargs):
    cc_library(
        name = name,
        hdrs = hdrs,
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        **kwargs
    )

    heph_cc_extract_headers(
        name = name + "_hdrs",
        target = name,
        visibility = kwargs.get("visibility"),
    )

def heph_cc_binary(
        copts = [],
        linkopts = [],
        **kwargs):
    cc_binary(
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        **kwargs
    )

def heph_cc_test(
        copts = [],
        linkopts = [],
        deps = [],
        **kwargs):
    cc_test(
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        deps = deps + [
            "@googletest//:gtest",
            "@googletest//:gtest_main",
        ],
        tags = ["no-clang-tidy"],  # NOTE: we need this to avoid all googletest issues
        **kwargs
    )

def heph_cc_proto_library(
        name,
        srcs,
        strip_import_prefix,
        deps = [],
        **kwargs):
    proto_library_name = name + "_gen_proto"
    proto_library(
        name = proto_library_name,
        srcs = srcs,
        strip_import_prefix = strip_import_prefix,
        deps = deps,
        **kwargs
    )

    cc_proto_library(
        name = name,
        tags = ["no-clang-tidy"],
        deps = [":{proto_library_name}".format(proto_library_name = proto_library_name)],
        **kwargs
    )

    native.config_setting(
        name = "dbg",
        values = {
            "compilation_mode": "dbg",
        },
    )
    native.config_setting(
        name = "opt",
        values = {
            "compilation_mode": "opt",
        },
    )
    native.config_setting(
        name = "fastbuild",
        values = {
            "compilation_mode": "fastbuild",
        },
    )