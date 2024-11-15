load("//bazel:hephaestus.bzl", "heph_cc_library", "heph_cc_test")

package(default_visibility = ["//visibility:public"])

heph_cc_library(
    name = "telemetry",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/concurrency",
        "//modules/random",
        "//modules/serdes",
        "@fmt",
    ],
)

#########
# Tests #
#########

TELEMETRY_TEST_DEPS = [
    ":telemetry",
]

heph_cc_test(
    name = "metric_record_tests",
    srcs = ["tests/metric_record_tests.cpp"],
    deps = TELEMETRY_TEST_DEPS,
)
