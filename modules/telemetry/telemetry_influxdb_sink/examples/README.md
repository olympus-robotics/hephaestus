# README: telemetry_sink

## Brief

telemetry_sink is where all the telemetry sinks are.


## InfluxDB

`examples/influxdb_measure_example.cpp` shows how to instantiate an Influxdb sink and send data to it.

For an end-to-end testing you can easily start the InfluxDB + Grafana backend by going in the `influxdb-backend` folder and run:
```
$ docker compose up
```
to generate the data run: `./bin/hephaestus_telemetry_sink_influxdb_measure_example`.

By vising [http://localhost:8087/](http://localhost:8087/) from your browser you can actually see the data being written to the database, while Grafana is at [http://localhost:3002/](http://localhost:3002/).
