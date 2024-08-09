# README: telemetry

## Brief

telemetry provides functionality to log time series structured data.

## Detailed description

See `examples/example.cpp` for how to instantiate and use telemetry.

## InfluxDB
You can easily try logging data in InfluxDB by going in the `influxdb` folder and run:
```
$ docker compose up
```

you can then run `./bin/hephaestus_telemetry_example` to start populating the database.

By vising [http://localhost:8087/](http://localhost:8087/) from your browser you can actually see the data being written.
