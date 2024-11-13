"""
repositories.bzl - Bazel repository definitions for Hephaestus
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def init_repositories():
    """
    Initialize the repositories.
    """
    RANGES_COMMIT = "a81477931a8aa2ad025c6bda0609f38e09e4d7ec"

    http_archive(
        name = "range-v3",
        urls = ["https://github.com/ericniebler/range-v3/archive/{commit}.tar.gz".format(commit = RANGES_COMMIT)],
        strip_prefix = "range-v3-" + RANGES_COMMIT,
        sha256 = "612b5d89f58a578240b28a1304ffb0d085686ebe0137adf175ed0e3382b7ed58",
    )

    ZENOH_VERSION = "1.0.0.9"

    _ALL_CONTENT = """
filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)
"""

    http_archive(
        name = "zenoh-c",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/eclipse-zenoh/zenoh-c/archive/{version}.zip".format(version = ZENOH_VERSION)],
        strip_prefix = "zenoh-c-" + ZENOH_VERSION,
        sha256 = "23f2cbf44465663dfc4df056f3b66e294e5f702a4b3647d8a50a8995ac8154b4",
    )

    http_archive(
        name = "zenoh-cpp",
        build_file_content = _ALL_CONTENT,
        urls = ["https://github.com/eclipse-zenoh/zenoh-cpp/archive/{version}.zip".format(version = ZENOH_VERSION)],
        strip_prefix = "zenoh-cpp-" + ZENOH_VERSION,
        sha256 = "251fff971f100e7f8c523171860526281b9fc30ff78fc9eb7ddadc31fdbf7d28",
    )
