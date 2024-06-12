#!/usr/bin/env python3

import etcd3
import sys
from urllib.parse import urlparse
import time

etcd_endpoint = sys.argv[1]
etcd_prefix = sys.argv[2]
namespace = sys.argv[3]
pod_base_name = sys.argv[4]
gremlin_port = sys.argv[5]
server_size = sys.argv[6]
rpc_service_name = sys.argv[7]
rpc_service_port = sys.argv[8]

server_size = int(server_size)

if not etcd_endpoint.startswith(("http://", "https://")):
    etcd_endpoint = "http://" + etcd_endpoint
parsed_url = urlparse(etcd_endpoint)
etcd_host = parsed_url.netloc.split(":")[0]
etcd_port = parsed_url.port
etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

schema_key = etcd_prefix + "gart_gie_schema_p0"

while True:
    try:
        schema_str, _ = etcd_client.get(schema_key)
        if schema_str is not None:
            schema_str = schema_str.decode("utf-8")
            break
        time.sleep(5)
    except Exception as e:
        time.sleep(5)

with open("/home/graphscope/gie-graph-schema.json", "w") as f:
    f.write(schema_str)

with open("/home/graphscope/gie-frontend-config.properties", "w") as f:
    f.write("pegasus.worker.num: 2\n")
    f.write("pegasus.batch.size: 1024\n")
    f.write("pegasus.timeout: 240000\n")
    f.write("pegasus.output.capacity: 16\n")
    f.write("graph.schema: /home/graphscope/gie-graph-schema.json\n")
    f.write("neo4j.bolt.server.disabled = NEO4J_DISABLED\n")
    f.write("neo4j.bolt.server.port = FRONTEND_CYPHER_PORT\n")
    f.write("gremlin.server.port = " + gremlin_port + "\n")
    pegasus_hosts = ""
    for idx in range(server_size):
        service_name = (
            pod_base_name
            + "-"
            + str(idx)
            + "."
            + rpc_service_name
            + "."
            + namespace
            + ".svc.cluster.local"
        )
        pegasus_hosts += service_name + ":" + rpc_service_port + ","
    pegasus_hosts = pegasus_hosts[:-1]
    f.write("pegasus.hosts: " + pegasus_hosts + "\n")
