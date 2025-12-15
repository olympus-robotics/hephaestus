# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@doxygen//:doxygen.bzl", "doxygen")
load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_buf//buf:defs.bzl", "buf_lint_test")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_python//sphinxdocs:sphinx.bzl", "sphinx_build_binary", "sphinx_docs")
load("@rules_python//sphinxdocs:sphinx_docs_library.bzl", "sphinx_docs_library")

def heph_basic_copts():
    return [
        "-g3",  # Enable debugging symbols
        "-fno-omit-frame-pointer",  # Increase debugging experience without sacrificing too much performance
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
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
        "-Wno-nullability-extension",
        "-Wno-private-header",
        "-Iexternal/abseil-cpp~",  # This is needed to avoid the error: file not found with <angled> include; use "quotes" instead
    ] + select({
        "@hephaestus//bazel:opt_compilation": ["-O3"],
        "//conditions:default": [],
    })

def heph_clang_copts():
    return [
        "-fcolor-diagnostics",
        "-fPIC",
        "-Wshadow",  # warn the user if a variable declaration shadows one from a parent context
    ]

def heph_gcc_copts():
    return [
        "-Wmisleading-indentation",
        "-Wduplicated-cond",
        "-Wduplicated-branches",
        "-Wlogical-op",
        "-Wuseless-cast",
        "-fdiagnostics-color=always",
        "-fPIC",
        "-Wshadow",
    ]

def heph_copts():
    return select({
        "@hephaestus//bazel:clang_compiler": heph_clang_copts(),
        "@hephaestus//bazel:gcc_compiler": heph_gcc_copts(),
    }) + heph_basic_copts()

def heph_linkopts():
    return ["-g3"] + select({
        "@hephaestus//bazel:clang_compiler": [],
        "@hephaestus//bazel:gcc_compiler": [],
    }) + select({
        "@hephaestus//bazel:dbg_compilation": ["-rdynamic"],
        "@hephaestus//bazel:fastbuild_compilation": ["-rdynamic"],
        "//conditions:default": [],
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
        deps = [],
        implementation_deps = [],
        **kwargs):
    cc_library(
        name = name,
        hdrs = hdrs,
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        deps = deps,
        implementation_deps = implementation_deps,
        **kwargs
    )

    heph_cc_extract_headers(
        name = name + "_hdrs",
        target = name,
        visibility = kwargs.get("visibility"),
    )

def _cc_lib_header_impl(ctx):
    headers = []
    if CcInfo in ctx.attr.target:
        cc_info = ctx.attr.target[CcInfo]
        headers = depset(cc_info.compilation_context.direct_headers)
    return [DefaultInfo(files = headers)]

heph_cc_extract_library_headers = rule(
    attrs = {
        "target": attr.label(mandatory = True),
    },
    implementation = _cc_lib_header_impl,
)

def _heph_generate_api_overview(ctx):
    outfiles = []
    names = []
    for target in ctx.attr.targets:
        name = target.label.name.split(":")[-1]
        out = ctx.actions.declare_file("doc/api/" + name + ".rst")
        content = """
================
{name}
================

""".format(name = name)
        if CcInfo in target:
            cc_info = target[CcInfo]
            headers = cc_info.compilation_context.direct_headers
            for header in headers:
                header_name = "/".join(header.short_path.split("/")[3:])

                content += """

.. doxygenfile:: {header_name}
   :sections: briefdescription detaileddescription enum func protected-func protected-static-func protected-type prototype innernamespace innerclass public-type public-attrib public-func public-static-attrib public-static-func related typedef var

""".format(header_name = header_name)

        ctx.actions.write(
            output = out,
            content = content,
        )
        outfiles += [out]
        names += [name]

    out = ctx.actions.declare_file("doc/api.rst")
    content = """
.. _api_overview:

=============
API Reference
=============

.. toctree::
   :maxdepth: 1

"""
    for name in names:
        content += """   api/{name}
""".format(name = name)

    ctx.actions.write(
        output = out,
        content = content,
    )
    return [DefaultInfo(files = depset([out] + outfiles))]

heph_generate_api_overview = rule(
    attrs = {
        "targets": attr.label_list(mandatory = True),
    },
    implementation = _heph_generate_api_overview,
)

def _get_strip_inc_paths(targets = []):
    strip_inc_paths = []
    for target in targets:
        name = target.split(":")[-1]
        strip_inc_paths += ["./modules/" + name + "/include"]
    return strip_inc_paths

def heph_cc_api_doc(
        name,
        targets = []):
    doxygen(
        name = "doxygen",
        project_name = "hephaestus",
        generate_xml = True,
        generate_html = False,
        xml_output = "doc/doxygen/xml",
        hide_undoc_classes = True,
        hide_undoc_members = True,
        hide_undoc_namespaces = True,
        directory_graph = False,
        extract_private = False,
        full_path_names = True,
        force_local_includes = True,
        strip_from_inc_path = _get_strip_inc_paths(targets),
        exclude_patterns = ["*/external/*"],
        exclude_symbols = ["internal", "detail"],
        file_patterns = ["*h"],
        outs = ["doc/doxygen/xml"],
        srcs = [target + "_hdrs" for target in targets],
    )

    heph_generate_api_overview(name = "api_overview", targets = targets)

    sphinx_docs_library(
        name = name,
        srcs = [":api_overview", ":doxygen"],
    )

def heph_cc_binary(
        copts = [],
        linkopts = [],
        **kwargs):
    cc_binary(
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        env = {
            # Leak detection currently doesn't work due to zenoh
            "ASAN_OPTIONS": "detect_leaks=0",
            "UBSAN_OPTIONS": "print_stacktrace=1:halt_on_error=1",
        },
        malloc = "@tcmalloc//tcmalloc",
        **kwargs
    )

def heph_cc_test(
        copts = [],
        linkopts = [],
        deps = [],
        tags = [],
        env = {},
        size = "small",
        **kwargs):
    merged_env = dict(env)
    merged_env.update({
        # Leak detection currently doesn't work due to zenoh
        "ASAN_OPTIONS": "detect_leaks=0",
        "UBSAN_OPTIONS": "print_stacktrace=1:halt_on_error=1",
        "RUST_BACKTRACE": "full",
    })
    cc_test(
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        deps = deps + [
            "@googletest//:gtest",
            "@hephaestus//modules/test_utils",
        ],
        env = merged_env,
        tags = tags + ["no-dwyu"],  # TODO (@graeter,@vittorioromeo) once test_utils are cleared out revisit
        size = size,
        **kwargs
    )

def heph_proto_library(
        name,
        srcs,
        module,
        deps = [],
        **kwargs):
    proto_library(
        name = name,
        srcs = srcs,
        strip_import_prefix = "/" + module,
        deps = deps,
        **kwargs
    )

    buf_lint_test(
        name = name + "_lint",
        targets = [":" + name],
        config = "@@//:buf.yaml",
        module = module,
        **kwargs
    )

def heph_cc_proto_library(
        name,
        srcs,
        module,
        deps = [],
        **kwargs):
    proto_library_name = name + "_gen_proto"
    heph_proto_library(
        name = proto_library_name,
        srcs = srcs,
        module = module,
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
