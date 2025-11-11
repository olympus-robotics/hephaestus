# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load(
    "@hephaestus//bazel:hephaestus.bzl",
    "heph_cc_binary",
    "heph_cc_library",
    "heph_cc_test",
)

package(default_visibility=["//visibility:public"])

heph_cc_library(
    name="containers",
    srcs=glob(
        [
            "src/**/*.cpp",
        ]
    ),
    hdrs=glob(
        [
            "include/**/*.h",
        ]
    ),
    implementation_deps=[],
    includes=["include"],
    visibility=["//visibility:public"],
    deps=[
        "//modules/utils",
        "@magic_enum",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name="example",
    srcs=["examples/blocking_queue_example.cpp"],
    deps=[
        ":containers",
        "@fmt",
    ],
)

#########
# Tests #
#########

heph_cc_test(
    name="blocking_queue_tests",
    srcs=["tests/blocking_queue_tests.cpp"],
    deps=[":containers"],
)

heph_cc_test(
    name="bit_flag_tests",
    srcs=["tests/bit_flag_tests.cpp"],
    deps=[":containers", "//modules/utils"],
)

heph_cc_test(
    name="intrusive_fifo_queue_tests",
    srcs=["tests/intrusive_fifo_queue_tests.cpp"],
    deps=[":containers"],
)
