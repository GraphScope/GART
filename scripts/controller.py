#!/usr/bin/env python3

from flask import Flask, request, jsonify
import subprocess
import os
from kubernetes import client, config
import tempfile
import shutil
import etcd3
import time
import socket
import json
from datetime import datetime, timezone
import yaml

app = Flask(__name__)
port = int(os.getenv("CONTROLLER_FLASK_PORT", 5000))
previous_hosts_info = None
previous_read_epoch = None


@app.route("/submit-config", methods=["POST"])
def submit_config():
    if "file" in request.files:
        file = request.files["file"]
        if file.filename == "":
            return jsonify({"error": "No selected file"}), 400
        try:
            content = file.read()
        except Exception as e:
            return jsonify({"error": str(e)}), 400
    elif "schema" in request.form:
        content = request.form["schema"].encode("utf-8")
    else:
        return jsonify({"error": "No file part or config string in the request"}), 400

    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    if etcd_client.get(etcd_prefix + "gart_rg_mapping_yaml")[0] is not None:
        return "Config already exists", 500

    while True:
        try:
            etcd_client.put(etcd_prefix + "gart_rg_mapping_yaml", content)
            break
        except Exception as e:
            time.sleep(5)
    return "Config submitted", 200


@app.route("/submit-pgql-config", methods=["POST"])
def submit_pgql_config():
    if "file" in request.files:
        file = request.files["file"]
        if file.filename == "":
            return jsonify({"error": "No selected file"}), 400
        try:
            content = file.read()
        except Exception as e:
            return jsonify({"error": str(e)}), 400
    elif "schema" in request.form:
        content = request.form["schema"].encode("utf-8")
    else:
        return jsonify({"error": "No file part or config string in the request"}), 400

    with open("/tmp/pgql_config.sql", "wb") as f:
        f.write(content)
    subprocess.run(
        [
            "/bin/bash",
            "-c",
            "cd /workspace/gart/pgql/ && ./run.sh sql2yaml /tmp/pgql_config.sql /tmp/pgql_config.yaml",
        ]
    )
    with open("/tmp/pgql_config.yaml", "r") as f:
        yaml_content = f.read()
    yaml_content = yaml_content.replace("!!gart.pgql.GSchema", "")
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    if etcd_client.get(etcd_prefix + "gart_rg_mapping_yaml")[0] is not None:
        return "PGQL config already exists", 500

    while True:
        try:
            etcd_client.put(etcd_prefix + "gart_rg_mapping_yaml", yaml_content)
            break
        except Exception as e:
            time.sleep(5)
    return "PGQL config submitted", 200


@app.route("/submit-graph-schema", methods=["POST"])
def submit_graph_schema():
    graph_schema = request.json.get("schema").encode("utf-8")
    # graph_schema = request.form["schema"].encode('utf-8')
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    if etcd_client.get(etcd_prefix + "gart_graph_schema_json")[0] is not None:
        return "Graph schema already exists", 500

    try:
        etcd_client.put(etcd_prefix + "gart_graph_schema_json", graph_schema)
        return "Graph schema submitted", 200
    except Exception as e:
        return "Failed to submit graph schema: " + str(e), 500


@app.route("/submit-data-source", methods=["POST"])
def submit_data_source():
    data_source_config = request.json.get("schema").encode("utf-8")

    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    if etcd_client.get(etcd_prefix + "gart_data_source_json")[0] is not None:
        return "Data source already exists", 500

    try:
        etcd_client.put(etcd_prefix + "gart_data_source_json", data_source_config)
        return "Data source submitted", 200
    except Exception as e:
        return "Failed to submit data source: " + str(e), 500


@app.route("/submit-data-loading", methods=["POST"])
def submit_data_loading():
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    try:
        graph_schema, _ = etcd_client.get(etcd_prefix + "gart_graph_schema_json")
    except Exception as e:
        return "Failed to get graph schema: " + str(e), 500

    try:
        data_source_config, _ = etcd_client.get(etcd_prefix + "gart_data_source_json")
    except Exception as e:
        return "Failed to get data source: " + str(e), 500

    graph_schema = json.loads(graph_schema.decode("utf-8"))
    data_source_config = json.loads(data_source_config.decode("utf-8"))
    result_dict = {}
    result_dict["graph"] = graph_schema["name"]
    graph_schema = graph_schema["schema"]
    result_dict["loadingConfig"] = {}
    result_dict["loadingConfig"]["dataSource"] = "rdbms"
    result_dict["loadingConfig"]["method"] = "append"
    result_dict["loadingConfig"]["enableRowStore"] = False
    db_name = os.getenv("DB_NAME", "rdbms")
    result_dict["loadingConfig"]["database"] = db_name

    vertex_mappings_dict = {}
    vertex_types_list = []
    vertex_types_info = graph_schema["vertex_types"]
    for vertex_id in range(len(vertex_types_info)):
        vertex_type_element = {}
        vertex_type_element["type_name"] = vertex_types_info[vertex_id]["type_name"]
        vertex_type_element["primary_keys"] = vertex_types_info[vertex_id][
            "primary_keys"
        ]
        mappings_list = []
        for table_id in range(len(vertex_types_info)):
            if (
                vertex_type_element["type_name"]
                == data_source_config["vertex_mappings"][table_id]["type_name"]
            ):
                vertex_type_element["dataSourceName"] = data_source_config[
                    "vertex_mappings"
                ][table_id]["inputs"][0]
                pk_prop_name = vertex_types_info[vertex_id]["primary_keys"][0]
                column_mappings = data_source_config["vertex_mappings"][table_id][
                    "column_mappings"
                ]
                for column_id in range(len(column_mappings)):
                    mappings_element = {}
                    mappings_element["property"] = column_mappings[column_id][
                        "property"
                    ]
                    mappings_element["dataField"] = {}
                    mappings_element["dataField"]["name"] = column_mappings[column_id][
                        "column"
                    ]["name"]
                    mappings_list.append(mappings_element)
                    if pk_prop_name == column_mappings[column_id]["property"]:
                        vertex_type_element["idFieldName"] = column_mappings[column_id][
                            "column"
                        ]["name"]
                break
        vertex_type_element["mappings"] = mappings_list
        vertex_types_list.append(vertex_type_element)

    vertex_mappings_dict["vertex_types"] = vertex_types_list

    edge_mappings_dict = {}
    edge_types_list = []
    edge_types_info = graph_schema["edge_types"]
    for edge_id in range(len(edge_types_info)):
        edge_type_element = {}
        edge_type_element["undirected"] = not edge_types_info[edge_id].get(
            "directed", True
        )
        edge_type_element["type_pair"] = {}
        edge_type_element["type_pair"]["edge"] = edge_types_info[edge_id]["type_name"]
        edge_type_element["type_pair"]["source_vertex"] = edge_types_info[edge_id][
            "vertex_type_pair_relations"
        ][0]["source_vertex"]
        edge_type_element["type_pair"]["destination_vertex"] = edge_types_info[edge_id][
            "vertex_type_pair_relations"
        ][0]["destination_vertex"]
        for table_id in range(len(edge_types_info)):
            if (
                edge_type_element["type_pair"]["edge"]
                == data_source_config["edge_mappings"][table_id]["type_triplet"]["edge"]
            ):
                edge_type_element["dataSourceName"] = data_source_config[
                    "edge_mappings"
                ][table_id]["inputs"][0]
                edge_type_element["sourceVertexMappings"] = [
                    {
                        "dataField": {
                            "name": data_source_config["edge_mappings"][table_id][
                                "source_vertex_mappings"
                            ][0]["column"]["name"]
                        }
                    }
                ]
                edge_type_element["destinationVertexMappings"] = [
                    {
                        "dataField": {
                            "name": data_source_config["edge_mappings"][table_id][
                                "destination_vertex_mappings"
                            ][0]["column"]["name"]
                        }
                    }
                ]
                data_field_mappings_list = []
                column_mappings = data_source_config["edge_mappings"][table_id][
                    "column_mappings"
                ]
                for column_id in range(len(column_mappings)):
                    mappings_element = {}
                    mappings_element["property"] = column_mappings[column_id][
                        "property"
                    ]
                    mappings_element["dataField"] = {}
                    mappings_element["dataField"]["name"] = column_mappings[column_id][
                        "column"
                    ]["name"]
                    data_field_mappings_list.append(mappings_element)
                edge_type_element["dataFieldMappings"] = data_field_mappings_list
                break
        edge_types_list.append(edge_type_element)

    edge_mappings_dict["edge_types"] = edge_types_list

    result_dict["vertexMappings"] = vertex_mappings_dict
    result_dict["edgeMappings"] = edge_mappings_dict

    result_dict_str = yaml.dump(result_dict)

    try:
        etcd_client.put(etcd_prefix + "gart_rg_mapping_yaml", result_dict_str)
        return "Data loading config submitted", 200
    except Exception as e:
        return "Failed to submit data loading config: " + str(e), 500


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


@app.route("/get-graph-schema", methods=["GET"])
def get_graph_schema():
    subprocess.run(
        [
            "/bin/bash",
            "-c",
            "/workspace/gart/scripts/generate_schema_for_protal.py",
        ]
    )
    schema_file_path = "/tmp/graph_schema_for_portal.json"
    if not os.path.exists(schema_file_path):
        return "No graph schema found yet", 400
    with open(schema_file_path, "r") as f:
        schema = json.load(f)
    return json.dumps(schema), 200


@app.route("/get-all-available-read-epochs", methods=["GET"])
def get_all_available_read_epochs():
    all_epochs = get_all_available_read_epochs_internal()[0]
    all_epochs.reverse()
    return json.dumps(all_epochs), 200


@app.route("/get-read-epoch-by-timestamp", methods=["POST"])
def get_read_epoch_by_timestamp():
    time_string = request.form.get("timestamp", None)
    if time_string is None:
        return "timestamp is required", 400
    # we assume the format of timestamp is like "2021-09-01 00:00:00"
    time_format = "%Y-%m-%d %H:%M:%S"
    dt = datetime.strptime(time_string, time_format)
    # the datetime is in UTC, convert it to local time zone
    dt = dt.astimezone(timezone.utc)
    unix_time = int(dt.timestamp())
    epoch_unix_time_pairs = get_all_available_read_epochs_internal()[1]
    # iterate through the list of epoch_unix_time pairs from end to start
    for (
        epoch,
        previous_unix_time_epoch,
        unix_time_epoch,
        num_vertices,
        num_edges,
    ) in reversed(epoch_unix_time_pairs):
        if unix_time_epoch <= unix_time:
            converted_time = datetime.fromtimestamp(unix_time_epoch)
            # convert time into local time zone
            converted_time = converted_time.replace(tzinfo=timezone.utc).astimezone(
                tz=None
            )
            formatted_time = converted_time.strftime("%Y-%m-%d %H:%M:%S")
            previous_formatted_time = "-"
            if epoch != 0:
                converted_time = datetime.fromtimestamp(previous_unix_time_epoch)
                # convert time into local time zone
                converted_time = converted_time.replace(tzinfo=timezone.utc).astimezone(
                    tz=None
                )
                previous_formatted_time = converted_time.strftime("%Y-%m-%d %H:%M:%S")
            result = {
                "version_id": str(epoch),
                "begin_time": previous_formatted_time,
                "end_time": formatted_time,
                "num_vertices": num_vertices,
                "num_edges": num_edges,
            }
            return json.dumps(result), 200
    return json.dumps({}), 200


@app.route("/get-graph-current-version", methods=["GET"])
def get_graph_current_version():
    if previous_read_epoch is None:
        return json.dumps({}), 200
    all_epochs = get_all_available_read_epochs_internal()[0]
    result = {}
    for idx in range(len(all_epochs)):
        if all_epochs[idx][0] == int(previous_read_epoch):
            begin_time = all_epochs[idx][1]
            end_time = all_epochs[idx][2]
            num_vertices = all_epochs[idx][3]
            num_edges = all_epochs[idx][4]
            result = {
                "version_id": str(previous_read_epoch),
                "begin_time": begin_time,
                "end_time": end_time,
                "num_vertices": num_vertices,
                "num_edges": num_edges,
            }
    return json.dumps(result), 200


@app.route("/run-gae-task", methods=["POST"])
def run_gae_task():
    command = ""
    data = request.json
    algorithm_name = data.get("algorithm_name")
    graph_version = data.get("graph_version")
    latest_epoch = get_latest_read_epoch()
    if int(graph_version) > latest_epoch:
        return "Invalid read epoch", 400
    if latest_epoch == 2**64 - 1:
        return "No available read epoch", 400
    command += f"--app_name {algorithm_name} "
    command += f"--read_epoch {graph_version} "
    for key, value in data.items():
        if key not in ["algorithm_name", "graph_version"]:
            command += f"--{key} {value} "
    # for key, value in request.form.items():
    #    command += f"--{key} {value} "
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    command += f"--etcd_endpoint {etcd_server} "
    command += f"--meta_prefix {etcd_prefix} "
    return run_gae(command)


@app.route("/change-read-epoch", methods=["POST"])
def change_read_epoch():
    read_epoch = request.form.get("read_epoch", None)
    if read_epoch is None:
        return "read_epoch is required", 400
    latest_epoch = get_latest_read_epoch()
    if int(read_epoch) > latest_epoch:
        return "Invalid read epoch", 400
    if latest_epoch == 2**64 - 1:
        return "No available read epoch", 400
    global previous_read_epoch
    if previous_read_epoch is None or previous_read_epoch != read_epoch:
        etcd_server = os.getenv("ETCD_SERVICE", "etcd")
        if not etcd_server.startswith(("http://", "https://")):
            etcd_server = f"http://{etcd_server}"
        etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
        etcd_host = etcd_server.split("://")[1].split(":")[0]
        etcd_port = etcd_server.split(":")[2]
        etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
        num_fragment = os.getenv("SUBGRAPH_NUM", "1")
        pod_base_name = os.getenv("GIE_EXECUTOR_POD_BASE_NAME", "gart")
        pod_service_name = os.getenv("GIE_EXECUTOR_POD_SERVICE_NAME", "engine")
        pod_service_port = os.getenv("GIE_EXECUTOR_POD_SERVICE_PORT", "80")
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

            cmd = f"curl -X POST http://{pod_base_name}-{idx}.{pod_service_name}:{pod_service_port}/start-gie-executor -d 'read_epoch={read_epoch}&etcd_prefix={etcd_prefix}&etcd_endpoint={etcd_server}'"

            launch_gie_executor_result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                shell=True,
            )
            print(
                launch_gie_executor_result.stdout
                + "\n"
                + launch_gie_executor_result.stderr
            )
        # check if all gie executors are launched successfully
        executors_status = [0] * int(num_fragment)
        gie_executor_rpc_port = os.getenv("GIE_EXECUTOR_RPC_PORT", "8000")
        while True:
            if all(executors_status):
                break
            for idx in range(int(num_fragment)):
                if executors_status[idx]:
                    continue
                if check_host_port(
                    f"{pod_base_name}-{idx}.{pod_service_name}",
                    int(gie_executor_rpc_port),
                ):
                    executors_status[idx] = 1
                else:
                    time.sleep(0.5)
        # check if gie frontend is launched successfully
        gie_frontend_service_name = os.getenv("GIE_FRONTEND_SERVICE_NAME", "frontend")
        gie_frontend_service_port = os.getenv("GIE_FRONTEND_SERVIVE_PORT", "8182")
        while True:
            if check_host_port(
                gie_frontend_service_name, int(gie_frontend_service_port)
            ):
                break
            time.sleep(0.5)
        # make sure change read epoch successfully
        previous_read_epoch = read_epoch
    return f"Read version changed to version {read_epoch}", 200


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


def check_host_port(host, port):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1)
        sock.connect((host, port))
        sock.close()
        return True
    except socket.error as e:
        print(f"Error: {e} on {host}:{port}")
        return False


def get_all_available_read_epochs_internal():
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    num_fragment = os.getenv("SUBGRAPH_NUM", "1")
    latest_read_epoch = get_latest_read_epoch()
    if latest_read_epoch == 2**64 - 1:
        return [[], []]
    available_epochs = []
    available_epochs_internal = []
    for epoch in range(latest_read_epoch + 1):
        latest_timestamp = None
        previous_timestamp = "-"
        num_vertices = 0
        num_edges = 0
        for frag_id in range(int(num_fragment)):
            schema_key = etcd_prefix + "gart_blob_m0" + f"_p{frag_id}" + f"_e{epoch}"
            schema_str, _ = etcd_client.get(schema_key)
            schema = json.loads(schema_str)
            unix_timestamp = schema["timestamp"]
            num_vertices += schema["total_vertex_num"]
            num_edges += schema["total_edge_num"]
            if latest_timestamp is None or unix_timestamp > latest_timestamp:
                latest_timestamp = unix_timestamp
        converted_time = datetime.fromtimestamp(latest_timestamp)
        # convert time into local time zone
        converted_time = converted_time.replace(tzinfo=timezone.utc).astimezone(tz=None)
        formatted_time = converted_time.strftime("%Y-%m-%d %H:%M:%S")
        available_epochs.append(
            [epoch, previous_timestamp, formatted_time, num_vertices, num_edges]
        )
        available_epochs_internal.append(
            [epoch, previous_timestamp, latest_timestamp, num_vertices, num_edges]
        )

    for epoch in range(latest_read_epoch, 0, -1):
        available_epochs[epoch][1] = available_epochs[epoch - 1][2]
        available_epochs_internal[epoch][1] = available_epochs_internal[epoch - 1][2]

    return [available_epochs, available_epochs_internal]


def get_latest_read_epoch():
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    num_fragment = os.getenv("SUBGRAPH_NUM", "1")
    latest_epoch = 2**64 - 1
    for idx in range(int(num_fragment)):
        etcd_key = etcd_prefix + "gart_latest_epoch_p" + str(idx)
        try:
            etcd_value, _ = etcd_client.get(etcd_key)
            if etcd_value is None:
                etcd_value = 2**64 - 1
            if latest_epoch > int(etcd_value):
                latest_epoch = int(etcd_value)
        except Exception as e:
            print(f"Error: {e}")

    return latest_epoch


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=port)
