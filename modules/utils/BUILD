# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "utils",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:log",
        "@abseil-cpp//absl/random:random",
        "@abseil-cpp//absl/time:time",
        "@fmt",
        "@magic_enum",
        "@range-v3//:range-v3",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name = "concepts_tests",
    srcs = ["tests/concepts_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "exception_tests",
    srcs = ["tests/exception_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "file_tests",
    srcs = ["tests/file_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "format_tests",
    srcs = ["tests/format_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "string_literal_tests",
    srcs = ["tests/string_literal_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "string_utils_tests",
    srcs = ["tests/string_utils_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "watchdog_tests",
    srcs = ["tests/watchdog_tests.cpp"],
    deps = [":utils"],
)

heph_cc_test(
    name = "stop_watch_tests",
    srcs = ["tests/stop_watch_tests.cpp"],
    deps = [":utils"],
)
