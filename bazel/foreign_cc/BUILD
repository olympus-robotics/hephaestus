# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(default_visibility = ["//visibility:public"])

DEFAULT_CACHE_ENTRIES = {
    "BUILD_TESTING": "OFF",
    "BUILD_SHARED_LIBS": "OFF",
}

cmake(
    name = "curl",
    cache_entries = {
        "BUILD_CURL_EXE": "OFF",
        "BUILD_STATIC_LIBS": "ON",
        "CURL_ENABLE_SSL": "OFF",
    } | DEFAULT_CACHE_ENTRIES,
    lib_source = "@curl//:all_srcs",
    out_include_dir = "include",
    out_static_libs = ["libcurl.a"],
    visibility = ["//visibility:public"],
)

cmake(
    name = "cpr",
    cache_entries = {
        "CPR_USE_SYSTEM_CURL": "ON",
        "CPR_ENABLE_SSL": "OFF",
        "CMAKE_PREFIX_PATH": "${EXT_BUILD_DEPS}/curl/",
    } | DEFAULT_CACHE_ENTRIES,
    lib_source = "@cpr//:all_srcs",
    out_include_dir = "include",
    out_static_libs = ["libcpr.a"],
    visibility = ["//visibility:public"],
    deps = [":curl"],
)

cmake(
    name = "influxdb",
    cache_entries = {
        "INFLUXCXX_WITH_BOOST": "OFF",
        "INFLUXCXX_TESTING": "OFF",
        "INFLUXCXX_SYSTEMTEST": "OFF",
        "cpr_DIR": "${EXT_BUILD_DEPS}/cpr/lib/cmake/cpr/",
    } | DEFAULT_CACHE_ENTRIES,
    lib_source = "@influxdb//:all_srcs",
    out_include_dir = "include",
    out_static_libs = ["libInfluxDB.a"],
    visibility = ["//visibility:public"],
    deps = [
        ":cpr",
    ],
)
