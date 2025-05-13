# =================================================================================================
# Copyright (C) 2024 FILICS Contributors
# =================================================================================================

cc_library(
    name = "reflect-cpp",
    srcs = [
        "src/rfl/Generic.cpp",
        "src/yyjson.c",
    ] + glob([
        "src/rfl/generic/**/*.cpp",
        "src/rfl/internal/**/*.cpp",
        "src/rfl/parsing/**/*.cpp",
        "src/rfl/json/**/*.cpp",
        "src/rfl/yaml/**/*.cpp",
    ]),
    hdrs = [
        "include/rfl.hpp",
        "include/rfl/thirdparty/ctre.hpp",
        "include/rfl/thirdparty/yyjson.h",
    ] + glob([
        "include/rfl/*.hpp",
        "include/rfl/generic/**/*.hpp",
        "include/rfl/internal/**/*.hpp",
        "include/rfl/io/**/*.hpp",
        "include/rfl/parsing/**/*.hpp",
        "include/rfl/json/**/*.hpp",
        "include/rfl/yaml/**/*.hpp",
    ]),
    includes = [
        "include",
        "include/rfl/thirdparty/",
    ],
    visibility = ["//visibility:public"],
    deps = ["@yaml-cpp"],
)
