load("//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "types_proto",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/random",
        "//modules/serdes",
        "//modules/types",
        "//modules/types_proto/proto:types_gen_cc_proto",
    ],
)

#########
# Tests #
#########

TYPES_PROTO_TEST_DEPS = [
    ":types_proto",
]

heph_cc_test(
    name = "serialization_tests",
    srcs = ["tests/serialization_tests.cpp"],
    deps = TYPES_PROTO_TEST_DEPS,
)
