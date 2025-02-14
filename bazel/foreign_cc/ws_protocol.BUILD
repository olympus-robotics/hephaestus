# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

cc_library(
    name = "ws_protocol",
    srcs = glob([
        "cpp/foxglove-websocket/src/*.cpp",
    ]),
    hdrs = glob(["cpp/foxglove-websocket/include/foxglove/websocket/*.hpp"]),
    includes = ["cpp/foxglove-websocket/include"],
    visibility = ["//visibility:public"],
    deps = [
        "@nlohmann_json//:json",
        "@boringssl//:ssl",
        "@websocketpp",
    ],
)