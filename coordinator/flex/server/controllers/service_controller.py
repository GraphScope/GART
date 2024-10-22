import connexion
from typing import Dict
from typing import Tuple
from typing import Union

from flex.server.models.error import Error  # noqa: E501
from flex.server.models.service_status import ServiceStatus  # noqa: E501
from flex.server.models.start_service_request import StartServiceRequest  # noqa: E501
from flex.server import util

import os
from kubernetes import client, config

def get_external_ip_of_a_service(service_name='gremlin-service', namespace='default'):
    # Load the in-cluster configuration
    config.load_incluster_config()
    
    # Create an API client instance
    v1 = client.CoreV1Api()
    
    # Get the service details
    try:
        service = v1.read_namespaced_service(name=service_name, namespace=namespace)
        
        # Check for LoadBalancer Ingress
        if service.status.load_balancer.ingress:
            # Get the external IP address
            external_ips = [ingress.ip for ingress in service.status.load_balancer.ingress]
            return external_ips[0] if external_ips else None
        else:
            return None
    except client.exceptions.ApiException as e:
        print(f"Exception when reading service: {e}")
        return None

def get_service_status_by_id(graph_id):  # noqa: E501
    """get_service_status_by_id

    Get service status by graph ID # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[ServiceStatus, Tuple[ServiceStatus, int], Tuple[ServiceStatus, int, Dict[str, str]]
    """
    result_dict = {}
    k8s_namespace = os.getenv('NAME_SPACE', 'default')
    gremlin_service_name = os.getenv('GREMLIN_SERVICE_NAME', 'gremlin-service')
    gremlin_service_port = os.getenv('GIE_GREMLIN_PORT', '8182')    
    gremlin_service_ip = get_external_ip_of_a_service(gremlin_service_name, k8s_namespace)
    result_dict["graph_id"] = graph_id
    result_dict["status"] = "Running"
    result_dict["sdk_endpoints"] = {}
    result_dict["sdk_endpoints"]["gremlin"] = f"ws://{gremlin_service_ip}:{gremlin_service_port}/gremlin"
    return (ServiceStatus.from_dict(result_dict), 200)


def list_service_status():  # noqa: E501
    """list_service_status

    List all service status # noqa: E501


    :rtype: Union[List[ServiceStatus], Tuple[List[ServiceStatus], int], Tuple[List[ServiceStatus], int, Dict[str, str]]
    """
    result_dict = {}
    k8s_namespace = os.getenv('NAME_SPACE', 'default')
    gremlin_service_name = os.getenv('GREMLIN_SERVICE_NAME', 'gremlin-service')
    gremlin_service_port = os.getenv('GIE_GREMLIN_PORT', '8182')    
    gremlin_service_ip = get_external_ip_of_a_service(gremlin_service_name, k8s_namespace)
    try:
        with open("/tmp/graph_id.txt", "r") as f:
            graph_id = f.read()
    except:
        graph_id = "gart_graph"
    result_dict["graph_id"] = graph_id
    result_dict["status"] = "Running"
    result_dict["sdk_endpoints"] = {}
    result_dict["sdk_endpoints"]["gremlin"] = f"ws://{gremlin_service_ip}:{gremlin_service_port}/gremlin"
    return ([ServiceStatus.from_dict(result_dict)], 200)


def restart_service():  # noqa: E501
    """restart_service

    Restart current service # noqa: E501


    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'


def start_service(start_service_request=None):  # noqa: E501
    """start_service

    Start service # noqa: E501

    :param start_service_request: 
    :type start_service_request: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    if connexion.request.is_json:
        start_service_request = StartServiceRequest.from_dict(connexion.request.get_json())  # noqa: E501
    return 'do some magic!'


def stop_service():  # noqa: E501
    """stop_service

    Stop current service # noqa: E501


    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'
