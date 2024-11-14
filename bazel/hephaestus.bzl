"""
hephaestus.bzl - Bazel build rules for Hephaestus.
"""

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
        extra_copts = [],
        extra_linkopts = [],
        **kwargs):
    cc_library(
        copts = heph_copts() + extra_copts,
        linkopts = heph_linkopts() + extra_linkopts,
        **kwargs
    )

def heph_cc_binary(
        extra_copts = [],
        extra_linkopts = [],
        **kwargs):
    cc_binary(
        copts = heph_copts() + extra_copts,
        linkopts = heph_linkopts() + extra_linkopts,
        **kwargs
    )

def heph_cc_test(
        extra_copts = [],
        extra_linkopts = [],
        deps = [],
        **kwargs):
    cc_test(
        copts = heph_copts() + extra_copts,
        linkopts = heph_linkopts() + extra_linkopts,
        deps = deps + [
            "@googletest//:gtest",
            "@googletest//:gtest_main",
        ],
        **kwargs
    )
