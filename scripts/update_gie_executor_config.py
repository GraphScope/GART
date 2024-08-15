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

is_running = False


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
    with open("/tmp/read_epoch", "w") as f:
        f.write(read_epoch)
    etcd_endpoint = request.form.get("etcd_endpoint", None)
    if etcd_endpoint is None:
        return "etcd_endpoint is required", 400
    etcd_prefix = request.form.get("etcd_prefix", "gart_meta_")
    if etcd_prefix is None:
        return "etcd_prefix is required", 400
    global is_running
    if not is_running:
        is_running = True
        cmd = f"/home/graphscope/GART/scripts/launch_gie_executor.sh {etcd_endpoint} {etcd_prefix} {read_epoch}"
        subprocess.run(
            [
                "/bin/bash",
                "-c",
                cmd,
            ]
        )
        return "Executor launching sucessfully", 200
    else:
        return "Executor is already running", 200


port = int(os.getenv("GIE_EXECUTOR_FLASK_PORT", 5000))
app.run(host="0.0.0.0", port=port)
