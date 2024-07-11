from flask import Flask, request, jsonify
import json
# https://docs.influxdata.com/influxdb/v2/api-guide/client-libraries/python/
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from influxdb_client.client.bucket_api import BucketsApi
from dateutil.parser import parse as time_parse
from influxdb_client.client.organizations_api import OrganizationsApi

class InfluxDBRESTClient:
    def __init__(self):
        # InfluxDB configuration
        self.INFLUXDB_URL = "http://localhost:8087"
        self.INFLUXDB_USER = "admin"
        self.INFLUXDB_PASSWORD = "adminadmin"
        self.INFLUXDB_ORG = "olympus-robotics"
        self.INFLUXDB_BUCKET = "hephaestus"

        try:
            self._client = InfluxDBClient(self.INFLUXDB_URL, org=self.INFLUXDB_ORG, username=self.INFLUXDB_USER, password=self.INFLUXDB_PASSWORD)
        except Exception as e:
            print(f"Failed to connect to InfluxDB: {str(e)}")
            exit(1)

        self._write_api = self._client.write_api(write_options=SYNCHRONOUS)

        self._app = Flask(__name__)
        self._app.add_url_rule('/', 'write_telemetry', self._write_telemetry, methods=['POST'])

    def _write_telemetry(self):
        data = request.json

        if not data:
            return jsonify({"error": "No data provided"}), 400

        timestamp = time_parse(data["logTimestamp"]).timestamp()
        timestamp_ns = int(timestamp * 1e9)
        fields = json.loads(data["jsonValues"])
        try:
            # Create a Point object
            point_dict = {
                "measurement": data["component"],
                "tags": {"tag": data["tag"]},
                "fields": fields,
                "time": timestamp_ns,
            }
            print(point_dict)
            point = Point.from_dict(point_dict, write_precision=WritePrecision.NS)

            # Write the point to InfluxDB
            self._write_api.write(bucket=self.INFLUXDB_BUCKET, record=point)
            return jsonify({"message": "Data successfully written to InfluxDB"}), 200

        except Exception as e:
            return jsonify({"error": str(e)}), 500

    def run(self):
        self._app.run(debug=True)

def main():
    client = InfluxDBRESTClient()
    client.run()

if __name__ == '__main__':
    main()
