#!/usr/bin/env python3

import etcd3
import sys
from urllib.parse import urlparse
import time

etcd_endpoint = sys.argv[1]
etcd_prefix = sys.argv[2]

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
        
with open('/etc/gie-graph-schema.json', 'w') as f:
    f.write(schema_str)
