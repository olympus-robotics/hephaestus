# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

genrule(
    name = "mcap_src",
    outs = ["cpp/mcap/mcap.cpp"],
    cmd = "OUT=$$(pwd)/$@;" +
          """cat <<EOL > $$OUT
#define MCAP_IMPLEMENTATION
#include "mcap/mcap.hpp"
EOL
""",
)

cc_library(
    name = "mcap",
    srcs = ["cpp/mcap/mcap.cpp"],
    hdrs = glob([
        "cpp/mcap/include/**/*.hpp",
        "cpp/mcap/include/**/*.inl",
    ]),
    defines = [
        "MCAP_COMPRESSION_NO_LZ4",
        "MCAP_COMPRESSION_NO_ZSTD",
    ],
    includes = ["cpp/mcap/include"],
    visibility = ["//visibility:public"],
    deps = [":mcap_src"],
)
