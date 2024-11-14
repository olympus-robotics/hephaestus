load("//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "bag",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    defines = [
        # TODO(@fbrizzi): check how to specify this defines in the mcap library to be automatically picked here.
        "MCAP_COMPRESSION_NO_LZ4",
        "MCAP_COMPRESSION_NO_ZSTD",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/foreign_cc:mcap",
        "//modules/containers",
        "//modules/ipc",
        "//modules/random",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:log",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "bag_player",
    srcs = ["apps/bag_player.cpp"],
    deps = [
        ":bag",
    ],
)

heph_cc_binary(
    name = "bag_recorder",
    srcs = ["apps/bag_recorder.cpp"],
    deps = [
        ":bag",
    ],
)

heph_cc_binary(
    name = "bag_example",
    srcs = ["examples/example.cpp"],
    deps = [
        ":bag",
    ],
)

#########
# Tests #
#########

BAG_TEST_DEPS = [
    ":bag",
    "//modules/types",
    "//modules/types_proto",
]

heph_cc_test(
    name = "bag_tests",
    srcs = ["tests/tests.cpp"],
    deps = BAG_TEST_DEPS,
)
