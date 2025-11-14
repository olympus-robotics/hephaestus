# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load(
    "@hephaestus//bazel:hephaestus.bzl",
    "heph_cc_binary",
    "heph_cc_library",
    "heph_cc_test",
)

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "examples",
    srcs = glob(
        [
            "src/**/*.cpp",
        ],
    ),
    hdrs = glob(
        [
            "include/**/*.h",
        ],
    ),
    implementation_deps = [
        "//modules/ipc",
        "//modules/random",
        "//modules/telemetry/log",
        "//modules/types",
        "@abseil-cpp//absl/container:raw_hash_map",
        "@abseil-cpp//absl/status:status",
        "@abseil-cpp//absl/status:statusor",
        "@protobuf",
        "@zstd",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/examples/proto:examples_gen_proto",
        "//modules/serdes",
        "//modules/utils",
        "@eigen",
        "@fmt",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "mcap_reader",
    srcs = ["examples/mcap_reader.cpp"],
    deps = [
        "@fmt",
        "@mcap",
    ],
)

heph_cc_binary(
    name = "mcap_writer",
    srcs = ["examples/mcap_writer.cpp"],
    deps = [
        ":examples",
        "//modules/bag",
        "//modules/ipc",
        "//modules/serdes",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "serdes_examples",
    srcs = ["examples/serdes_examples.cpp"],
    deps = [
        ":examples",
    ],
)

heph_cc_binary(
    name = "zenoh_pub",
    srcs = [
        "examples/zenoh_program_options.h",
        "examples/zenoh_pub.cpp",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_sub",
    srcs = [
        "examples/zenoh_program_options.h",
        "examples/zenoh_sub.cpp",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "zenoh_service_client",
    srcs = [
        "examples/zenoh_program_options.h",
        "examples/zenoh_service_client.cpp",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_service_server",
    srcs = [
        "examples/zenoh_program_options.h",
        "examples/zenoh_service_server.cpp",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_string_service_client",
    srcs = [
        "examples/zenoh_program_options.h",
        "examples/zenoh_string_service_client.cpp",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "zenoh_string_service_server",
    srcs = [
        "examples/zenoh_program_options.h",
        "examples/zenoh_string_service_server.cpp",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_action_server",
    srcs = [
        "examples/zenoh_action_server.cpp",
        "examples/zenoh_program_options.h",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
    ],
)

heph_cc_binary(
    name = "zenoh_action_server_client",
    srcs = [
        "examples/zenoh_action_server_client.cpp",
        "examples/zenoh_program_options.h",
    ],
    deps = [
        ":examples",
        "//modules/cli",
        "//modules/ipc",
        "//modules/telemetry/log",
        "//modules/utils",
        "@fmt",
        "@magic_enum",
    ],
)

#########
# Tests #
#########

EXAMPLES_TEST_DEPS = [
    ":examples",
    "//modules/random",
    "//modules/ipc",
]

heph_cc_test(
    name = "zenoh_tests",
    srcs = [
        "tests/helpers.h",
        "tests/zenoh_tests.cpp",
    ],
    deps = EXAMPLES_TEST_DEPS,
)

heph_cc_test(
    name = "types_proto_tests",
    srcs = [
        "tests/helpers.h",
        "tests/types_proto_tests.cpp",
    ],
    deps = EXAMPLES_TEST_DEPS,
)
