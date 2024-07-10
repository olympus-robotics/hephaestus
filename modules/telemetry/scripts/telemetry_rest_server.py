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
        self.INFLUXDB_ORG = "filics"
        self.INFLUXDB_BUCKET = "filics"
        try:
            self._client = InfluxDBClient(self.INFLUXDB_URL, org=self.INFLUXDB_ORG, username=self.INFLUXDB_USER, password=self.INFLUXDB_PASSWORD)
            self._setup_influxdb()
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

    def _setup_influxdb(self):
        org_api = OrganizationsApi(self._client)
        bucket_api = BucketsApi(self._client)

        # Check if organization exists, create if it doesn't
        try:
            org = org_api.find_organizations(org=self.INFLUXDB_ORG)[0]
        except IndexError:
            org = org_api.create_organization(self.INFLUXDB_ORG)

        self._org_id = org.id

        # Check if bucket exists, create if it doesn't
        try:
            bucket_api.find_bucket_by_name(self.INFLUXDB_BUCKET)
        except Exception:
            bucket_api.create_bucket(bucket_name=self.INFLUXDB_BUCKET, org_id=self._org_id)

    def run(self):
        self._app.run(debug=True)

def main():
    client = InfluxDBRESTClient()
    client.run()

if __name__ == '__main__':
    main()
