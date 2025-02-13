# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

cc_library(
    name = "ws_protocol",
    srcs = glob([
        "cpp/foxglove-websocket/src/*.cpp",
    ]),
    hdrs = glob([
        "cpp/foxglove-websocket/include/**/*.hpp",
    ]),
    includes = ["cpp/foxglove-websocket/include"],
    visibility = ["//visibility:public"],
    deps = [
        "@websocketpp",
        "@openssl//:openssl",
    ],
    linkopts = ["-lssl", "-lcrypto"],  # Add this line to link OpenSSL libraries
)
