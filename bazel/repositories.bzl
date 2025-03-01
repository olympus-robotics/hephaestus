# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def foreign_cc_repositories():
    ZENOH_VERSION = "1.2.1"
    http_archive(
        name = "zenohc_builder",
        build_file = ":foreign_cc/zenohc_builder.BUILD",
        urls = ["https://github.com/olympus-robotics/zenohc_builder/archive/refs/tags/{version}.zip".format(version = ZENOH_VERSION)],
        strip_prefix = "zenohc_builder-{version}".format(version = ZENOH_VERSION),
        sha256 = "b6015958b7924f721b76f688a6ae4b8c55548192d43de4be3ab0a15af0e3b46f",
    )

    MCAP_VERSION = "bebea860f68b278d6cccdb70e0ed299d2656af96"
    http_archive(
        name = "mcap",
        build_file = ":foreign_cc/mcap.BUILD",
        urls = ["https://github.com/foxglove/mcap/archive/{version}.zip".format(version = MCAP_VERSION)],
        strip_prefix = "mcap-" + MCAP_VERSION,
        sha256 = "ba970de5120059521fbc2a63eab54679432183dd358ba958fc75a3bcded95276",
    )

    CPR_VERSION = "1.11.1"
    http_archive(
        name = "cpr",
        build_file = ":foreign_cc/cpr.BUILD",
        patches = ["//bazel/foreign_cc:cpr.patch"],
        urls = ["https://github.com/libcpr/cpr/archive/refs/tags/{version}.tar.gz".format(version = CPR_VERSION)],
        strip_prefix = "cpr-" + CPR_VERSION,
        sha256 = "e84b8ef348f41072609f53aab05bdaab24bf5916c62d99651dfbeaf282a8e0a2",
    )

    INFLUXDB_VERSION = "2b8a87cfbb2b0a8c01a38212fade79553c8f75c8"
    http_archive(
        name = "influxdb",
        build_file = ":foreign_cc/influxdb.BUILD",
        urls = ["https://github.com/offa/influxdb-cxx/archive/{version}.zip".format(version = INFLUXDB_VERSION)],
        strip_prefix = "influxdb-cxx-" + INFLUXDB_VERSION,
        sha256 = "374f5cff1cabc5ce8ff0d4b227475e2221c038b668a797fd9ef117bee43ead07",
    )

    REFLECT_CPP_VERSION = "0.16.0"
    http_archive(
        name = "reflect-cpp",
        build_file = "//bazel/foreign_cc:reflect_cpp.BUILD",
        urls = ["https://github.com/olympus-robotics/reflect-cpp/archive/refs/tags/v{version}-cmake-3.22.1.zip".format(version = REFLECT_CPP_VERSION)],
        strip_prefix = "reflect-cpp-" + REFLECT_CPP_VERSION + "-cmake-3.22.1",
        sha256 = "63fd52189f7df9c5648c5479e7af9377271a18f65a30a1a45fe160f8e0cf787a",
    )

    WS_PROTOCOL_VERSION = "1.4.0-dev"
    WS_PROTOCOL_TAG = "releases/cpp/v" + WS_PROTOCOL_VERSION
    http_archive(
        name = "ws_protocol",
        build_file = ":foreign_cc/ws_protocol.BUILD",
        urls = ["https://github.com/mfehr/ws-protocol/archive/refs/tags/{tag}.zip".format(tag = WS_PROTOCOL_TAG)],
        strip_prefix = "ws-protocol-releases-cpp-v" + WS_PROTOCOL_VERSION,
        sha256 = "c13877cbef4a9e8c48e6104ae05e2081534d34755aebf48ab50f4568f4760026",
    )