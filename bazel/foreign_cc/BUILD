load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(default_visibility = ["//visibility:public"])

# TODO(@fbrizzi): check how to build static libraries
DEFAULT_CACHE_ENTRIES = {
    "BUILD_SHARED_LIBS": "ON",
    "BUILD_TESTING": "OFF",
}

cmake(
    name = "mcap",
    cache_entries = DEFAULT_CACHE_ENTRIES,
    lib_source = "@com_olympus_robotics_mcap//:all_srcs",
    out_include_dir = "include",
    out_shared_libs = ["libmcap.so"],
    visibility = ["//visibility:public"],
)

cmake(
    name = "zenoh-c",
    cache_entries = {
        "ZENOHC_CARGO_CHANNEL": "+stable",
        "ZENOHC_BUILD_WITH_SHARED_MEMORY": "TRUE",
        "ZENOHC_BUILD_WITH_UNSTABLE_API": "TRUE",
    },
    lib_source = "@zenoh-c//:all_srcs",
    out_include_dir = "include",
    out_shared_libs = ["libzenohc.so"],
    # tags = ["local"],  # have to break the sandbox to build ring@0.17.8
    visibility = ["//visibility:public"],
)

cmake(
    name = "zenoh-cpp",
    cache_entries = {
        "Z_FEATURE_UNSTABLE_API": "1",
        "ZENOHCXX_EXAMPLES_PROTOBUF": "OFF",
        "zenohc_DIR": "${EXT_BUILD_DEPS}/zenoh-c/lib/cmake/zenohc/",
    },
    lib_source = "@zenoh-cpp//:all_srcs",
    out_headers_only = True,
    out_include_dir = "include",
    visibility = ["//visibility:public"],
    deps = [
        ":zenoh-c",
    ],
)
