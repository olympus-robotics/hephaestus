package(default_visibility = ["//visibility:public"])

config_setting(
    name = "clang_compiler",
    flag_values = {
        "@bazel_tools//tools/cpp:compiler": "clang",
    },
    visibility = [":__subpackages__"],
)

config_setting(
    name = "gcc_compiler",
    flag_values = {
        "@bazel_tools//tools/cpp:compiler": "gcc",
    },
    visibility = [":__subpackages__"],
)
