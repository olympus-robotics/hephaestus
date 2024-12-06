genrule(
    name = "influxdb_export_header",
    outs = ["include/InfluxDB/influxdb_export.h"],
    cmd = "OUT=$$(pwd)/$@;" +
          """cat <<EOL > $$OUT
#pragma once
#define INFLUXDB_EXPORT
""",
)

cc_library(
    name = "influxdb",
    srcs = [
        "src/HTTP.cxx",
        "src/HTTP.h",
        "src/InfluxDB.cxx",
        "src/InfluxDBBuilder.cxx",
        "src/InfluxDBFactory.cxx",
        "src/LineProtocol.cxx",
        "src/LineProtocol.h",
        "src/NoBoostSupport.cxx",
        "src/BoostSupport.h",
        "src/Point.cxx",
        "src/Proxy.cxx",
        "src/UriParser.h",
    ],
    hdrs = glob(["include/**/*.h"]) + ["include/InfluxDB/influxdb_export.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = ["@cpr", ":influxdb_export_header"],
)
