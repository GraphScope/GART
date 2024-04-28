#!/usr/bin/env python3

import argparse
import json
import sys
import yaml
import time
from urllib.parse import urlparse
import requests
import socket
import os

import etcd3

import subprocess

def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch GART in Kubernetes cluster",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--num_subgraphs", default=1, help="Number of subgraphs")
    parser.add_argument("--etcd_prefix", default="gart_meta_", help="Etcd prefix")
    parser.add_argument("--rg_mapping_file_path", help="RGMapping file path")
    return parser

def is_etcd_running(host, port):
    """Check if etcd is running by sending a request to the member list API."""
    try:
        response = requests.get(f"http://{host}:{port}/health", timeout=1)
        return response.status_code == 200 and response.json().get("health") == "true"
    except requests.exceptions.RequestException:
        return False

if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()
    
    gart_home = os.getenv("GART_HOME")
    
    if False:
        launch_etcd_pod_deployment_cmd = f"kubectl apply -f {gart_home}/k8s/etcd-deployment.yaml"
        process = subprocess.run(launch_etcd_pod_deployment_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        if process.returncode != 0:
            print("Start Etcd Deployment Error:\n", process.stderr)
            sys.exit(1)
        else:
            print("Etcd Deployment info: ", process.stdout)
            
        launch_etcd_pod_service_cmd = f"kubectl apply -f {gart_home}/k8s/etcd-service.yaml"
        process = subprocess.run(launch_etcd_pod_service_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        if process.returncode != 0:
            print("Start Etcd Service Error:\n", process.stderr)
            sys.exit(1)
        else:
            print("Etcd Service info: ", process.stdout)
    
    etcd_host="localhost"
    etcd_port=12379
    #FIXME: read port from yaml file
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    etcd_prefix = args.etcd_prefix
    
    if not is_etcd_running(etcd_host, etcd_port):
        print("Etcd is not running")
        sys.exit(1)
    
    etcd_client.put(etcd_prefix + "capturer_is_up", "False")
    etcd_client.put(etcd_prefix + "converter_is_up", "False")
    
    num_subgraphs = int(args.num_subgraphs)
    for idx in range(num_subgraphs):
        etcd_client.put(etcd_prefix + f"writer_{idx}_is_up", "False")

    with open(args.rg_mapping_file_path, "r", encoding="UTF-8") as f:
        rg_mapping = yaml.safe_load(f)
        
    etcd_client.put(
        etcd_prefix + "gart_rg_mapping_yaml", yaml.dump(rg_mapping, sort_keys=False)
    )

