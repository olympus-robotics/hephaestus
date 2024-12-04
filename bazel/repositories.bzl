# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def binary_repositories():
    """
    Download binary install repositories.
    """
    ZENOH_VERSION = "1.0.0"
    http_archive(
        name = "zenohc_builder",
        build_file = ":foreign_cc/BUILD.zenohc_builder",
        urls = ["https://github.com/olympus-robotics/zenohc_builder/archive/refs/tags/{version}.zip".format(version = ZENOH_VERSION)],
        strip_prefix = "zenohc_builder-{version}".format(version = ZENOH_VERSION),
        sha256 = "15605cf7fd83e390f82404c0545c247a2921302ef8f419559c8829e6b2c890d5",
    )

    MCAP_VERSION = "bebea860f68b278d6cccdb70e0ed299d2656af96"
    http_archive(
        name = "mcap",
        build_file = ":foreign_cc/BUILD.mcap",
        urls = ["https://github.com/foxglove/mcap/archive/{version}.zip".format(version = MCAP_VERSION)],
        strip_prefix = "mcap-" + MCAP_VERSION,
        sha256 = "ba970de5120059521fbc2a63eab54679432183dd358ba958fc75a3bcded95276",
    )

def cmake_repositories():
    """
    Downlad CMake repositories.
    """
    _ALL_CONTENT = """
filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)
"""

    CURL_VERSION = "curl-8_7_1"
    http_archive(
        name = "curl",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/curl/curl/archive/{version}.zip".format(version = CURL_VERSION)],
        strip_prefix = "curl-" + CURL_VERSION,
        sha256 = "1522999600630964adc3130eada47dc0be2a71908a36944b7aa78aad2b1006b7",
    )

    # TODO(@fbrizzi): Fix this instead of using my local fork. The problem are the files with non ASCII caracters.
    CPR_VERSION = "0db841abcdbf8dce8a1ae576069cf47ce046b95e"
    http_archive(
        name = "cpr",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/filippobrizzi/cpr/archive/{version}.zip".format(version = CPR_VERSION)],
        strip_prefix = "cpr-" + CPR_VERSION,
        sha256 = "f3b97147b778489659f7b8ed733059381e43462494ca79f906552be40fd31843",
    )

    INFLUXDB_VERSION = "8d8ab3b5ddc40b767a3ebf5eb1df76aa170dbca9"
    http_archive(
        name = "influxdb",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/offa/influxdb-cxx/archive/{version}.zip".format(version = INFLUXDB_VERSION)],
        strip_prefix = "influxdb-cxx-" + INFLUXDB_VERSION,
        sha256 = "c405a93119dd9ac1cbff3485fd497a88a57b1da8fe46b86a01f57bee7fdaa058",
    )
