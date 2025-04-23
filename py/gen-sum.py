import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS
import datetime

bucket = "maindb"
org = "home"
token = "4nRZjT0EBB2QlgsiiXmlZms7WNQweT0cUkOFdPiyVZ_xBOm_eCgwkHJuLLrDU8iyXFs7XgnzgDgwG_EVDJoVEw=="
# Store the URL of your InfluxDB instance
url="http://192.168.8.10:8086"

client = influxdb_client.InfluxDBClient(
    url=url,
    token=token,
    org=org
)
# Get the write API
write_api = client.write_api(write_options=SYNCHRONOUS)

# Query script
query_api = client.query_api()
query = 'import "timezone"\
        option location = timezone.fixed(offset: 8h) \
    from(bucket: "maindb") |> range(start: today() )\
        |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")\
  |> filter(fn: (r) => r["topic"] == "/espepsolar/data-parsed")\
  |> filter(fn: (r) => r["_field"] == "bat_v" or r["_field"] == "ch_c")\
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")\
  |> map(fn: (r) => ({r with _value: r.bat_v * r.ch_c}))\
  |> aggregateWindow(every: 3m, fn: mean)'

sum_value = 0.0
result = query_api.query(org=org, query=query)
for table in result:
    print(table)
    for row in table.records:
        print (row.values["_time"])
        print (row.values["_value"])
        sum_value += row.values["_value"] * 0.05
        # Create a point to write to the database
        point = influxdb_client.Point("solar_generation") \
            .field("wat3", sum_value) \
            .time(row.values["_time"], influxdb_client.WritePrecision.NS)
        write_api.write(bucket=bucket, org=org, record=point)
        
