"""
repositories.bzl - Bazel repository definitions for Hephaestus
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

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

    ZENOH_VERSION = "release/1.0.0"
    http_archive(
        name = "zenoh-c",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/eclipse-zenoh/zenoh-c/archive/{version}.zip".format(version = ZENOH_VERSION)],
        strip_prefix = ("zenoh-c-" + ZENOH_VERSION).replace("/", "-"),
        sha256 = "7f86a341ff0c97da4c982f1a3934bc3f2555186a55d3098e9e347e6d2e232666",
    )

    http_archive(
        name = "zenoh-cpp",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/eclipse-zenoh/zenoh-cpp/archive/{version}.zip".format(version = ZENOH_VERSION)],
        strip_prefix = ("zenoh-cpp-" + ZENOH_VERSION).replace("/", "-"),
        sha256 = "864def7a7c3cec9caa7f7a59c02065278ae1328308fb0cb24cac99d1221ae586",
    )

    MCAP_VERSION = "ff7ef1b941f4eb17c87a31b3ed5a55627f3a4aee"
    http_archive(
        name = "mcap",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/olympus-robotics/mcap_builder/archive/{version}.zip".format(version = MCAP_VERSION)],
        strip_prefix = "mcap_builder-" + MCAP_VERSION,
        sha256 = "6203dff6903b717a5b153f2620343b7b43dc92b751f266d98ae588556a9cd9b7",
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
