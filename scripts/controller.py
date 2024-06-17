#!/usr/bin/env python3

from flask import Flask, request
import subprocess
import os
from kubernetes import client, config
import tempfile
import shutil
import etcd3
import time
from gremlin_python.driver.client import Client

app = Flask(__name__)
port = int(os.getenv("CONTROLLER_FLASK_PORT", 5000))
previous_hosts_info = None


@app.route("/control/pause", methods=["POST"])
def pause():
    subprocess.run(
        [
            "/bin/bash",
            "-c",
            "/workspace/gart/scripts/pause_resume_data_processing.sh pause",
        ]
    )
    return "Paused", 200


@app.route("/control/resume", methods=["POST"])
def resume():
    subprocess.run(
        [
            "/bin/bash",
            "-c",
            "/workspace/gart/scripts/pause_resume_data_processing.sh resume",
        ]
    )
    return "Resumed", 200


@app.route("/run-gae-task", methods=["POST"])
def run_gae_task():
    command = ""
    for key, value in request.form.items():
        command += f"--{key} {value} "
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith("http://"):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    command += f"--etcd_endpoint {etcd_server} "
    command += f"--meta_prefix {etcd_prefix} "
    return run_gae(command)

@app.route("/run-gie-task", methods=["POST"])
def run_gie_task():
    read_epoch = request.form.get("read_epoch", None)
    if read_epoch is None:
        return "read_epoch is required", 400
    query = request.form.get("query", None)
    if query is None:
        return "query is required", 400
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith("http://"):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    num_fragment = os.getenv("SUBGRAPH_NUM", 1)
    for idx in range(int(num_fragment)):
        schema_key = etcd_prefix + "gart_blob_m0" + f"_p{idx}" + f"_e{read_epoch}"
        while True:
            try:
                schema_str, _ = etcd_client.get(schema_key)
                if schema_str is not None:
                    break
                time.sleep(5)
            except Exception as e:
                time.sleep(5)
        pod_base_name = os.getenv("GIE_EXECUTOR_POD_BASE_NAME", "gart")
        pod_service_name = os.getenv("GIE_EXECUTOR_POD_SERVICE_NAME", "engine")
        pod_service_port = os.getenv("GIE_EXECUTOR_POD_SERVICE_PORT", 80)
        cmd = f"curl -X POST http://{pod_base_name}-{idx}.{pod_service_name}:{pod_service_port}/start-gie-executor -d 'read_epoch={read_epoch}&etcd_prefix={etcd_prefix}&etcd_endpoint={etcd_server}'"
        return cmd, 200
        launch_gie_executor_result = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, shell=True
        )
        print(launch_gie_executor_result.stdout + "\n" + launch_gie_executor_result.stderr)
    #TODO(wanglei): check if all gie executors are launched successfully
    time.sleep(5)
    gremlin_client = Client(f"ws://{etcd_server}/gremlin", "g")
    return gremlin_client.submit(query).all().result(), 200
        


def get_pod_ips(namespace, label_selector):
    config.load_incluster_config()
    v1 = client.CoreV1Api()

    result = []

    ret = v1.list_namespaced_pod(namespace, label_selector=label_selector)
    for pod in ret.items:
        result.append([pod.metadata.name, pod.status.pod_ip])

    return result


def get_tempdir():
    return os.path.join("/", tempfile.gettempprefix())


def mpi_resolve(gae_pod_name_ip):
    cmd = []
    env = {}
    mpi = None
    if "OPAL_PREFIX" in os.environ:
        mpi = os.path.expandvars("$OPAL_PREFIX/bin/mpirun")
    if mpi is None:
        if "OPAL_BINDIR" in os.environ:
            mpi = os.path.expandvars("$OPAL_BINDIR/mpirun")
    if mpi is None:
        mpi = shutil.which("mpirun")
    cmd.extend([mpi, "--allow-run-as-root", "--bind-to", "none"])
    cmd.extend(["-n", str(len(gae_pod_name_ip))])
    cmd.extend(["-host", ",".join([f"{name_ip[0]}:1" for name_ip in gae_pod_name_ip])])
    env["OMPI_MCA_btl_vader_single_copy_mechanism"] = "none"
    env["OMPI_MCA_orte_allowed_exit_without_sync"] = "1"
    env["OMPI_MCA_odls_base_sigkill_timeout"] = "0"
    env["OMPI_MCA_plm_rsh_agent"] = shutil.which("kube_ssh")
    return cmd, env


def run_gae(part_cmd):
    namespace = "gart"
    label_selector = "app=writer"
    global previous_hosts_info
    gae_pod_name_ip = get_pod_ips(namespace, label_selector)

    hosts = os.path.join(get_tempdir(), "hosts_of_nodes")
    with open(hosts, "w") as f:
        if previous_hosts_info is None or previous_hosts_info != gae_pod_name_ip:
            previous_hosts_info = gae_pod_name_ip
            for idx in range(len(gae_pod_name_ip)):
                f.write(f"{gae_pod_name_ip[idx][1]} {gae_pod_name_ip[idx][0]}\n")

    os.system("cat /tmp/hosts_of_nodes | sudo tee -a /etc/hosts")

    container = "analyzer"
    for idx in range(len(gae_pod_name_ip)):
        pod_name = gae_pod_name_ip[idx][0]
        os.system(
            f"kubectl cp {hosts} {namespace}/{pod_name}:/tmp/hosts_of_nodes -c {container} --retries=5"
        )

    cmd, mpi_env = mpi_resolve(gae_pod_name_ip)
    gae_bin_path = "/workspace/gart/build/apps/run_gart_app"
    cmd.extend([gae_bin_path])
    cmd.extend(part_cmd.split())
    print("Analytical engine launching command: " + " ".join(cmd))
    env = os.environ.copy()
    env.update(mpi_env)
    gae_result = subprocess.run(
        cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    return gae_result.stdout + " " + gae_result.stderr, 200


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=port)
