"""
hephaestus.bzl - Bazel build rules for Hephaestus.
"""

load("@aspect_rules_lint//lint:clang_tidy.bzl", "lint_clang_tidy_aspect")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

def heph_basic_copts():
    return ([
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
    ])

def heph_clang_copts():
    return heph_basic_copts() + [
        "-fcolor-diagnostics",
    ]

def heph_gcc_copts():
    return heph_basic_copts() + [
        "-Wmisleading-indentation",
        "-Wduplicated-cond",
        "-Wduplicated-branches",
        "-Wlogical-op",
        "-Wuseless-cast",
        "-fdiagnostics-color=always",
    ]

def heph_copts():
    return select({
        "//:clang_compiler": heph_clang_copts(),
        "//:gcc_compiler": heph_gcc_copts(),
    })

def heph_linkopts():
    return ([])

def heph_cc_library(
        copts = [],
        linkopts = [],
        **kwargs):
    cc_library(
        copts = heph_copts() + copts,
        linkopts = heph_linkopts() + linkopts,
        **kwargs
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
        **kwargs
    )

clang_tidy = lint_clang_tidy_aspect(
    binary = "@@//:clang_tidy",
    configs = ["@@//:clang_tidy_config"],
    lint_target_headers = True,
    angle_includes_are_system = False,
    verbose = False,
)
