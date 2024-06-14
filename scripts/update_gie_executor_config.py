#!/usr/bin/env python3

import sys
import os
from flask import Flask, request
import subprocess

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
    
app = Flask(__name__)

@app.route("/start-gie-executor", methods=["POST"])
def start_gie_executor():
    read_epoch = request.form.get("read_epoch", None)
    if read_epoch is None:
        return "read_epoch is required", 400
    # set the environment variable `READ_EPOCH` and make it visible to the linux command
    os.environ["READ_EPOCH"] = read_epoch
    cmd = "cd /home/graphscope/GraphScope/interactive_engine/executor/assembly/grin_gart && ./target/release/grin_executor ../../../assembly/src/conf/graphscope/log4rs.yml /home/graphscope/gie-executor-config.properties"
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, shell=True)
    return result.stdout + "\n" + result.stderr, 200
    

port = int(os.getenv("GIE_EXECUTOR_FLASK_PORT", 5000))
app.run(host="0.0.0.0", port=port)
