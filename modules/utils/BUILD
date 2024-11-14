load("//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

# Generate version_impl.h
genrule(
    name = "version_impl",
    outs = ["src/version_impl.h"],
    cmd = "./$(location generate_version_file.sh) \"$@\"",
    local = True,
    tools = ["generate_version_file.sh"],
)

heph_cc_library(
    name = "utils",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]) + ["src/version_impl.h"],
    # extra_copts = INCLUDE_PATHS,
    includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":version_impl",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:log",
        "@abseil-cpp//absl/random:random",
        "@abseil-cpp//absl/time:time",
        "@com_github_ericniebler_range_v3//:range-v3",
        "@fmt",
        "@magic_enum",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "show_version",
    srcs = ["apps/show_version.cpp"],
    deps = [
        ":utils",
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
