import argparse
import json
import sys
import yaml
import time
from urllib.parse import urlparse
import requests

import etcd3
import paramiko


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch GART in distributed mode with a given configuration",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--config_file", help="Configuration file path")
    parser.add_argument("--rg_mapping_file", help="RG Mapping file path")
    return parser


def is_etcd_running(host, port):
    """Check if etcd is running by sending a request to the member list API."""
    try:
        response = requests.get(f"http://{host}:{port}/health")
        return response.status_code == 200 and response.json().get("health") == "true"
    except requests.exceptions.RequestException:
        return False


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()

    unset = False

    if not isinstance(args.config_file, str) or len(args.config_file) == 0:
        print("Please specify the GART configuration with --config_file")
        unset = True

    if not isinstance(args.rg_mapping_file, str) or len(args.rg_mapping_file) == 0:
        print("Please specify the RG Mapping file with --rg_mapping_file")
        unset = True

    if unset:
        sys.exit(1)

    with open(args.config_file, "r", encoding="UTF-8") as f:
        config = json.load(f)

    with open(args.rg_mapping_file, "r", encoding="UTF-8") as f:
        rg_mapping = yaml.safe_load(f)

    v6d_socket = config["v6d_socket"]
    v6d_size = config["v6d_size"]
    etcd_endpoint = config["etcd_endpoint"]
    if not etcd_endpoint.startswith(("http://", "https://")):
        etcd_endpoint = "http://" + etcd_endpoint
    parsed_url = urlparse(etcd_endpoint)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    if is_etcd_running(etcd_host, etcd_port):
        print("Etcd is already running.")
    else:
        etcd_command = f"nohup etcd --listen-client-urls http://{etcd_host}:{etcd_port} --advertise-client-urls http://{etcd_host}:{etcd_port} --listen-peer-urls http://{etcd_host}:{etcd_port+1} --initial-cluster default=http://{etcd_host}:{etcd_port+1} --initial-advertise-peer-urls http://{etcd_host}:{etcd_port+1} --data-dir default.etcd >etcd.log 2>&1 &"
        ssh.connect(etcd_host)
        ssh.exec_command(etcd_command)

        etcd_up = False
        max_retries = 10
        retry_interval_seconds = 2

        for _ in range(max_retries):
            if is_etcd_running(etcd_host, etcd_port):
                etcd_up = True
                ssh.close()
                break
            time.sleep(retry_interval_seconds)

        if not etcd_up:
            print("Etcd service did not start within expected time frame.")
            ssh.close()
            sys.exit(1)
        else:
            print("Etcd service is up and running.")
            ssh.close()

    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    etcd_prefix = config["etcd_prefix"]

    etcd_client.put(etcd_prefix + "capturer_is_up", "False")
    etcd_client.put(etcd_prefix + "converter_is_up", "False")

    total_subgraph_num = config["total_subgraph_num"]
    for idx in range(total_subgraph_num):
        etcd_client.put(etcd_prefix + f"writer_{idx}_is_up", "False")

    etcd_client.put(
        etcd_prefix + "gart_rg_mapping_yaml", yaml.dump(rg_mapping, sort_keys=False)
    )

    # Launch the capturer
    capturer_host = config["capturer_host"]
    ssh.connect(capturer_host)

    gart_bin_path = config["gart_bin_path"]
    db_type = config["db_type"]
    db_host = config["db_host"]
    db_port = config["db_port"]
    db_user = config["db_user"]
    db_password = config["db_password"]
    db_name = config["db_name"]
    kafka_server = config["kafka_server"]
    enable_bulkload = config["enable_bulkload"]
    kafka_path = config["kafka_path"]

    def check_status(role):
        # Poll etcd status
        timeout = 120  # Maximum time to wait in seconds
        check_interval = 5  # Check every 5 seconds
        time_elapsed = 0
        while time_elapsed < timeout:
            if (
                etcd_client.get(etcd_prefix + f"{role}_is_up")[0].decode("utf-8")
                == "True"
            ):
                return True
            else:
                print(f"Waiting for {role} to be up...")
                time.sleep(check_interval)
                time_elapsed += check_interval

    # Command to start capturer in the background
    start_capturer_cmd = f"export KAFKA_HOME={kafka_path}; cd {gart_bin_path}/; nohup {gart_bin_path}/gart --db-host {db_host} --db-port {db_port} --db-name {db_name} --db-type {db_type} -u {db_user} -p {db_password} --v6d-sock {v6d_socket} --v6d-size {v6d_size} -e {etcd_endpoint} --etcd-prefix {etcd_prefix} --kafka-server {kafka_server} --subgraph-num {total_subgraph_num} --enable-bulkload {enable_bulkload} --rg-from-etcd 1 --role capturer >capturer.log 2>&1 &"
    ssh.exec_command(start_capturer_cmd)

    capturer_status = check_status("capturer")
    if not capturer_status:
        print("Capturer failed to start.")
        ssh.close()
        sys.exit(1)
    else:
        print("Capturer is up and running.")
        ssh.close()

    # Launch the converter
    converter_host = config["converter_host"]
    ssh.connect(converter_host)
    start_converter_cmd = f"cd {gart_bin_path}/; nohup {gart_bin_path}/gart --db-host {db_host} --db-port {db_port} --db-name {db_name} --db-type {db_type} -u {db_user} -p {db_password} --v6d-sock {v6d_socket} --v6d-size {v6d_size} -e {etcd_endpoint} --etcd-prefix {etcd_prefix} --kafka-server {kafka_server} --subgraph-num {total_subgraph_num} --enable-bulkload {enable_bulkload} --role converter >converter.log 2>&1 &"

    ssh.exec_command(start_converter_cmd)

    converter_status = check_status("converter")
    if not converter_status:
        print("Converter failed to start.")
        ssh.close()
        sys.exit(1)
    else:
        print("Converter is up and running.")
        ssh.close()

    # Launch the writers
    writer_hosts = config["writer_hosts"]
    for idx in range(len(writer_hosts)):
        writer_host = writer_hosts[idx]["host"]
        subgraph_id = writer_hosts[idx]["subgraph_id"]
        ssh.connect(writer_host)
        start_writer_cmd = f"cd {gart_bin_path}/; nohup {gart_bin_path}/gart --db-host {db_host} --db-port {db_port} --db-name {db_name} --db-type {db_type} -u {db_user} -p {db_password} --v6d-sock {v6d_socket} --v6d-size {v6d_size} -e {etcd_endpoint} --etcd-prefix {etcd_prefix} --kafka-server {kafka_server} --subgraph-num {total_subgraph_num} --enable-bulkload {enable_bulkload} --role writer --start-slot {subgraph_id} --num-slot 1 >writer_{subgraph_id}.log 2>&1 &"
        ssh.exec_command(start_writer_cmd)

        writer_status = check_status(f"writer_{idx}")
        if not writer_status:
            print(f"Writer {idx} failed to start.")
            ssh.close()
            sys.exit(1)
        else:
            print(f"Writer {idx} is up and running.")
            ssh.close()