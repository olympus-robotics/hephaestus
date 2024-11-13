package(
    default_visibility = ["//visibility:public"],
)

# MCAP expects users to build a source file that includes
# the library headers with MCAP_IMPLEMENTATION defined beforehand
# to actually build the library
cc_library(
    name = "mcap_lib",
    srcs = ["mcap_lib.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "@mcap",
        "@lz4",
        "@zstd",
    ],
)
