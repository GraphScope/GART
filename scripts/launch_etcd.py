import argparse
from urllib.parse import urlparse
import requests
import time
import subprocess
import socket
import sys


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch ETCD with a a given endpoint",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--etcd_endpoint", help="Etcd endpoint")
    return parser


def check_port(host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind((host, port))
            return True
        except socket.error as e:
            print(f"Port {port} is already in use: {e}")
            return False


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()
    etcd_endpoint = args.etcd_endpoint

    if not etcd_endpoint.startswith(("http://", "https://")):
        etcd_endpoint = "http://" + etcd_endpoint
    parsed_url = urlparse(etcd_endpoint)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port

    if not check_port(etcd_host, etcd_port):
        print(0)
        sys.exit(0)

    etcd_peer_port = etcd_port + 1

    while etcd_peer_port < 65535:
        if check_port(etcd_host, etcd_peer_port):
            break
        etcd_peer_port += 1

    if etcd_peer_port == 65535:
        print(0)
        sys.exit(0)

    etcd_command = [
        "etcd",
        "--listen-client-urls",
        f"http://{etcd_host}:{etcd_port}",
        "--advertise-client-urls",
        f"http://{etcd_host}:{etcd_port}",
        "--listen-peer-urls",
        f"http://{etcd_host}:{etcd_peer_port}",
        "--initial-cluster",
        f"default=http://{etcd_host}:{etcd_peer_port}",
        "--initial-advertise-peer-urls",
        f"http://{etcd_host}:{etcd_peer_port}",
        "--data-dir",
        "default.etcd",
    ]

    etcd_process = subprocess.Popen(
        etcd_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )

    etcd_health_url = etcd_endpoint + "/health"
    server_up = False
    max_retries = 10
    retry_interval_seconds = 2

    status = 0

    for _ in range(max_retries):
        try:
            response = requests.get(etcd_health_url)
            if response.status_code == 200 and response.json().get("health") == "true":
                server_up = True
                status = 1
                break
        except requests.exceptions.RequestException as e:
            time.sleep(retry_interval_seconds)

    if not server_up:
        etcd_process.terminate()  # Or .kill() if terminate does not work

    print(status)
