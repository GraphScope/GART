#!/usr/bin/env python3

import sys

server_id = sys.argv[1]
server_num = sys.argv[2]
rpc_port = sys.argv[3]
namespace = sys.argv[4]
pod_base_name = sys.argv[5]
engine_service_name = sys.argv[6]
engine_port = sys.argv[7]


with open("/home/graphscope/gie-executor-config.properties", "w") as f:
    f.write("rpc.port: " + rpc_port + "\n")
    f.write("server.id: " + server_id + "\n")
    f.write("server.size: " + server_num + "\n")
    f.write("pegasus.worker.num: 1\n")
    f.write("graph.type: VINEYARD\n")
    f.write("graph.vineyard.object.id = 20906017567569673\n")
    network_servers = ""
    for idx in range(int(server_num)):
        service_name = (
            pod_base_name
            + "-"
            + str(idx)
            + "."
            + engine_service_name
            + "."
            + namespace
            + ".svc.cluster.local"
        )
        network_servers += service_name + ":" + engine_port + ","
    network_servers = network_servers[:-1]
    f.write("network.servers: " + network_servers + "\n")
