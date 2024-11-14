"""
repositories.bzl - Bazel repository definitions for Hephaestus
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def bazel_repositories():
    """
    Downlad Bazel repositories.
    """
    RANGES_VERSION = "0.12.0"

    http_archive(
        name = "range-v3",
        urls = ["https://github.com/ericniebler/range-v3/archive/{version}.tar.gz".format(version = RANGES_VERSION)],
        strip_prefix = "range-v3-" + RANGES_VERSION,
        sha256 = "015adb2300a98edfceaf0725beec3337f542af4915cec4d0b89fa0886f4ba9cb",
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
