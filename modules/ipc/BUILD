load("//bazel:hephaestus.bzl", "heph_cc_binary", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "ipc",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    defines = [
        "ZENOH_WITH_UNSTABLE_API",
        "ZENOHCXX_ZENOHC",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/foreign_cc:zenoh-c",
        "//bazel/foreign_cc:zenoh-cpp",
        "//modules/cli",
        "//modules/concurrency",
        "//modules/containers",
        "//modules/ipc/proto:types_cc_proto",
        "//modules/serdes",
        "//modules/utils",
        # "@abseil-cpp//absl/container:raw_hash_map",
        "@abseil-cpp//absl/log:log",
        # "@abseil-cpp//absl/status:status",
        # "@abseil-cpp//absl/status:statusor",
        "@fmt",
        "@nlohmann_json//:json",
        # "@range-v3",
    ],
)

############
# Binaries #
############

heph_cc_binary(
    name = "zenoh_node_list",
    srcs = ["apps/zenoh_node_list.cpp"],
    deps = [
        ":ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_string_service",
    srcs = ["apps/zenoh_string_service.cpp"],
    deps = [
        ":ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_topic_echo",
    srcs = ["apps/zenoh_topic_echo.cpp"],
    deps = [
        ":ipc",
    ],
)

heph_cc_binary(
    name = "zenoh_topic_list",
    srcs = ["apps/zenoh_topic_list.cpp"],
    deps = [
        ":ipc",
    ],
)

#########
# Tests #
#########

IPC_TEST_DEPS = [
    ":ipc",
    "//modules/random",
    "//modules/types",
    "//modules/types_proto",
]

heph_cc_test(
    name = "action_server_tests",
    srcs = [
        "tests/action_server_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "dynamic_subscriber_tests",
    srcs = [
        "tests/dynamic_subscriber_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "pub_sub_tests",
    srcs = [
        "tests/pub_sub_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "service_tests",
    srcs = [
        "tests/service_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)

heph_cc_test(
    name = "topic_filter_tests",
    srcs = [
        "tests/topic_filter_tests.cpp",
    ],
    deps = IPC_TEST_DEPS,
)
