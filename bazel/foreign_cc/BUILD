load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
load(":cargo.bzl", "cargo_binary")

package(default_visibility = ["//visibility:public"])

# TODO(@fbrizzi): check how to build static libraries
DEFAULT_CACHE_ENTRIES = {
    "BUILD_SHARED_LIBS": "ON",
    "BUILD_TESTING": "OFF",
}

cmake(
    name = "mcap",
    cache_entries = DEFAULT_CACHE_ENTRIES,
    # MCAP_COMPRESSION_NO_LZ4
    # copts = [
    #     "-DMCAP_COMPRESSION_NO_LZ4",
    #     "-DMCAP_COMPRESSION_NO_ZSTD",
    # ],
    lib_source = "@mcap//:all_srcs",
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
    env = {
        "RUSTUP_HOME": "/usr/local/rustup",  # TODO(@fbrizzi) figure out how to read this from env var
        "RUSTC_WRAPPER": "sccache",
    },
    lib_source = "@zenoh-c//:all_srcs",
    out_include_dir = "include",
    out_shared_libs = ["libzenohc.so"],
    tags = ["local"],
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

cmake(
    name = "cpr",
    cache_entries = {
        "CPR_USE_SYSTEM_CURL": "ON",
        "BUILD_SHARED_LIBS": "ON",
        "curl_DIR": "${EXT_BUILD_DEPS}",
    },
    lib_source = "@cpr//:all_srcs",
    out_include_dir = "include",
    out_shared_libs = ["libcpr.so"],
    visibility = ["//visibility:public"],
    deps = [
        "@curl",
    ],
)

cmake(
    name = "influxdb",
    cache_entries = {
        "INFLUXCXX_WITH_BOOST": "OFF",
        "INFLUXCXX_TESTING": "OFF",
        "INFLUXCXX_SYSTEMTEST": "OFF",
        "cpr_DIR": "${EXT_BUILD_DEPS}/cpr/lib/cmake/cpr/",
    },
    lib_source = "@influxdb//:all_srcs",
    out_include_dir = "include",
    out_shared_libs = ["libInfluxDB.so"],
    visibility = ["//visibility:public"],
    deps = [
        ":cpr",
    ],
)
