import argparse
from urllib.parse import urlparse
import requests
import time
import subprocess


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch ETCD with a a given endpoint",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--etcd_endpoint", help="Etcd endpoint")
    return parser


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()
    etcd_endpoint = args.etcd_endpoint

    if not etcd_endpoint.startswith(("http://", "https://")):
        etcd_endpoint = "http://" + etcd_endpoint
    parsed_url = urlparse(etcd_endpoint)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port

    etcd_command = [
        "etcd",
        "--listen-client-urls",
        f"http://{etcd_host}:{etcd_port}",
        "--advertise-client-urls",
        f"http://{etcd_host}:{etcd_port}",
        "--listen-peer-urls",
        f"http://{etcd_host}:{etcd_port+1}",
        "--initial-cluster",
        f"default=http://{etcd_host}:{etcd_port+1}",
        "--initial-advertise-peer-urls",
        f"http://{etcd_host}:{etcd_port+1}",
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
