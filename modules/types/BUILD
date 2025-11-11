# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@hephaestus//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "types",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    implementation_deps = [
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/random",
        "//modules/utils",
        "@fmt",
    ],
)

#########
# Tests #
#########

TYPES_TEST_DEPS = [
    ":types",
]

heph_cc_test(
    name = "bounds_tests",
    srcs = ["tests/bounds_tests.cpp"],
    deps = TYPES_TEST_DEPS,
)

heph_cc_test(
    name = "tests",
    srcs = ["tests/tests.cpp"],
    deps = TYPES_TEST_DEPS + [
        "//modules/random",
        "@abseil-cpp//absl/hash:hash_testing",
    ],
)

heph_cc_test(
    name = "uuid_v4_tests",
    srcs = ["tests/uuid_v4_tests.cpp"],
    deps = TYPES_TEST_DEPS,
)
