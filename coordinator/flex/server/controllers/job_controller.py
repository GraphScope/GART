import connexion
from typing import Dict
from typing import Tuple
from typing import Union

from flex.server.models.create_dataloading_job_response import CreateDataloadingJobResponse  # noqa: E501
from flex.server.models.dataloading_job_config import DataloadingJobConfig  # noqa: E501
from flex.server.models.dataloading_mr_job_config import DataloadingMRJobConfig  # noqa: E501
from flex.server.models.error import Error  # noqa: E501
from flex.server.models.job_status import JobStatus  # noqa: E501
from flex.server import util

import requests
import os
import etcd3
from urllib.parse import urlparse
import json

RUNNING = None

def delete_job_by_id(job_id, delete_scheduler=None):  # noqa: E501
    """delete_job_by_id

    Delete job by ID # noqa: E501

    :param job_id: 
    :type job_id: str
    :param delete_scheduler: 
    :type delete_scheduler: bool

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'


def get_dataloading_job_config(graph_id, dataloading_job_config):  # noqa: E501
    """get_dataloading_job_config

    Post to get the data loading configuration for MapReduce Task # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param dataloading_job_config: 
    :type dataloading_job_config: dict | bytes

    :rtype: Union[DataloadingMRJobConfig, Tuple[DataloadingMRJobConfig, int], Tuple[DataloadingMRJobConfig, int, Dict[str, str]]
    """
    if connexion.request.is_json:
        dataloading_job_config = DataloadingJobConfig.from_dict(connexion.request.get_json())  # noqa: E501
    return 'do some magic!'


def get_job_by_id(job_id):  # noqa: E501
    """get_job_by_id

    Get job status by ID # noqa: E501

    :param job_id: 
    :type job_id: str

    :rtype: Union[JobStatus, Tuple[JobStatus, int], Tuple[JobStatus, int, Dict[str, str]]
    """
    global RUNNING
    result_dict = {}
    etcd_server = os.getenv("ETCD_SERVICE", "127.0.0.1:23790")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    parsed_url = urlparse(etcd_server)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    
    debezium_status_key = f"{etcd_prefix}debezium_request_is_sent"
    try:
        debezium_status, _ = etcd_client.get(debezium_status_key)
        if debezium_status == b"True":
            if RUNNING is None:
                RUNNING = "RUNNING"
            result_dict["status"] = RUNNING
            result_dict["id"] = "0"
            result_dict["type"] = "dataloading"
    except:
        return (JobStatus.from_dict(result_dict), 200)
        
    return (JobStatus.from_dict(result_dict), 200)
    
    
def list_jobs():  # noqa: E501
    """list_jobs

    List all jobs # noqa: E501


    :rtype: Union[List[JobStatus], Tuple[List[JobStatus], int], Tuple[List[JobStatus], int, Dict[str, str]]
    """
    global RUNNING
    result_dict = {}
    etcd_server = os.getenv("ETCD_SERVICE", "127.0.0.1:23790")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    parsed_url = urlparse(etcd_server)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    
    debezium_status_key = f"{etcd_prefix}debezium_request_is_sent"
    try:
        debezium_status, _ = etcd_client.get(debezium_status_key)
        if debezium_status == b"True":
            if RUNNING is None:
                RUNNING = "RUNNING"
            result_dict["status"] = RUNNING
            result_dict["id"] = "0"
            result_dict["type"] = "dataloading"
    except:
        return ([JobStatus.from_dict(result_dict)], 200)
        
    return ([JobStatus.from_dict(result_dict)], 200)


def pause_job(job_id):  # noqa: E501
    """pause_job

    Pause an existing job # noqa: E501

    :param job_id: 
    :type job_id: str

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    response = requests.post(f"{gart_controller_server}/control/pause")
    global RUNNING
    RUNNING = "PAUSED"
    return (response.text, response.status_code)


def resume_job(job_id):  # noqa: E501
    """resume_job

    Resume an existing job # noqa: E501

    :param job_id: 
    :type job_id: str

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    response = requests.post(f"{gart_controller_server}/control/resume")
    global RUNNING
    RUNNING = "RUNNING"
    return (response.text, response.status_code)


def submit_dataloading_job(graph_id, dataloading_job_config):  # noqa: E501
    """submit_dataloading_job

    Submit a dataloading job # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param dataloading_job_config: 
    :type dataloading_job_config: dict | bytes

    :rtype: Union[CreateDataloadingJobResponse, Tuple[CreateDataloadingJobResponse, int], Tuple[CreateDataloadingJobResponse, int, Dict[str, str]]
    """
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
        
    if not isinstance(dataloading_job_config, dict):
        dataloading_job_config = json.loads(dataloading_job_config)
        
    response = requests.post(
        f"{gart_controller_server}/submit-data-loading",
        headers={"Content-Type": "application/json"},
        data=json.dumps({"schema": json.dumps(dataloading_job_config)}),
    )
    
    result_dict = {}
    result_dict["job_id"] = "0"
    return (CreateDataloadingJobResponse.from_dict(result_dict), response.status_code)
