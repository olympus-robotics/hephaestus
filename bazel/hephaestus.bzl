"""
hephaestus.bzl - Bazel build rules for Hephaestus.
"""

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

HEPH_COPTS = [
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Werror",
]

HEPH_CLANG_COPTS = HEPH_COPTS + [
    "-Wshadow",  # warn the user if a variable declaration shadows one from a parent context
    "-Wnon-virtual-dtor",  # warn if a class with virtual functions has a non-virtual destructor.
    "-Wold-style-cast",  # warn for c-style casts
    "-Wcast-align",  # warn for potential performance problem casts
    "-Wunused",  # warn on anything being unused
    "-Woverloaded-virtual",  # warn if you overload (not override) a virtual function
    "-Wpedantic",  # warn if non-standard C++ is used
    "-Wconversion",  # warn on type conversions that may lose data
    "-Wsign-conversion",  # warn on sign conversions
    "-Wnull-dereference",  # warn if a null dereference is detected
    "-Wdouble-promotion",  # warn if float is implicit promoted to double
    "-Wformat=2",  # warn on security issues around functions that format output (ie printf)
    "-Wimplicit-fallthrough",  # warn on statements that fallthrough without an explicit annotation
]

HEPH_GCC_COPTS = HEPH_COPTS + HEPH_CLANG_COPTS + [
    "-Wmisleading-indentation",
    "-Wduplicated-cond",
    "-Wduplicated-branches",
    "-Wlogical-op",
    "-Wuseless-cast",
]

HEPH_DEFAULT_COPTS = select({
    "//heph:clang_compiler": HEPH_CLANG_COPTS,
    "//heph:gcc_compiler": HEPH_GCC_COPTS,
})

def heph_copts():
    return (
        [
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-fcolor-diagnostics",  # this won't work for GCC, needs to check
            "-DEIGEN_AVOID_STL_ARRAY",
            "-Wno-sign-compare",
            "-ftemplate-depth=900",
            "-pthread",
            "-std=c++20",
            "-Iexternal/abseil-cpp~",
        ]
    )

def heph_linkopts():
    # If dbg add -rdynamic
    return (
        [
        ]
    )

def heph_test_linkopts():
    return (
        [
            "-lpthread",
            "-lm",
            "-lgtest_main",
            "-lstdc++",
        ]
    )

def heph_cc_test(
        extra_copts = [],
        extra_linkopts = [],
        **kwargs):
    cc_test(
        copts = HEPH_DEFAULT_COPTS + extra_copts,
        linkopts = linkopts + extra_linkopts,
        **kwargs
    )

def heph_cc_binary(
        extra_copts = [],
        extra_linkopts = [],
        **kwargs):
    cc_binary(
        copts = copts + extra_copts,
        linkopts = linkopts + extra_linkopts,
        **kwargs
    )

def heph_cc_library(
        extra_copts = [],
        extra_linkopts = [],
        **kwargs):
    cc_library(
        copts = HEPH_DEFAULT_COPTS + extra_copts,
        linkopts = linkopts + extra_linkopts,
        **kwargs
    )
