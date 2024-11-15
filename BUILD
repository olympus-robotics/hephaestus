# load("@rules_pkg//:pkg.bzl", "pkg_deb", "pkg_tar")

load("@bazel_skylib//rules:native_binary.bzl", "native_binary")

package(default_visibility = ["//visibility:public"])

config_setting(
    name = "clang_compiler",
    flag_values = {
        "@bazel_tools//tools/cpp:compiler": "clang",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "gcc_compiler",
    flag_values = {
        "@bazel_tools//tools/cpp:compiler": "gcc",
    },
    visibility = ["//visibility:public"],
)

filegroup(
    name = "clang_tidy_config",
    srcs = [".clang-tidy"],
    visibility = ["//visibility:public"],
)

native_binary(
    name = "clang_tidy",
    src = "@llvm_toolchain_llvm//:bin/clang-tidy",
    out = "clang_tidy",
)

# pkg_tar(
#     name = "bazel-bin",
#     srcs = ["//modules/examples"],
#     mode = "0755",
#     package_dir = "/install",
#     strip_prefix = "/src",
# )

# pkg_tar(
#     name = "debian-data",
#     extension = "tar.gz",
#     deps = [
#         ":bazel-bin",
#     ],
# )

# pkg_deb(
#     name = "bazel-debian",
#     architecture = "amd64",
#     built_using = "unzip (6.0.1)",
#     data = ":debian-data",
#     depends = [
#         "zlib1g-dev",
#         "unzip",
#     ],
#     description_file = "debian/description",
#     homepage = "http://bazel.build",
#     maintainer = "The Bazel Authors <bazel-dev@googlegroups.com>",
#     package = "bazel",
#     version = "0.1.1",
# )
