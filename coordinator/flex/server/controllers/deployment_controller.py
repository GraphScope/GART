import connexion
from typing import Dict
from typing import Tuple
from typing import Union

from flex.server.models.error import Error  # noqa: E501
from flex.server.models.get_pod_log_response import GetPodLogResponse  # noqa: E501
from flex.server.models.get_resource_usage_response import GetResourceUsageResponse  # noqa: E501
from flex.server.models.get_storage_usage_response import GetStorageUsageResponse  # noqa: E501
from flex.server.models.running_deployment_info import RunningDeploymentInfo  # noqa: E501
from flex.server.models.running_deployment_status import RunningDeploymentStatus  # noqa: E501
from flex.server import util


def get_deployment_info():  # noqa: E501
    """get_deployment_info

    Deployment information # noqa: E501


    :rtype: Union[RunningDeploymentInfo, Tuple[RunningDeploymentInfo, int], Tuple[RunningDeploymentInfo, int, Dict[str, str]]
    """
    result_dict = {}
    result_dict["cluster_type"] = "KUBERNETES"
    with open ("/tmp/graph_schema_create_time.txt", "r") as f:
        result_dict["creation_time"] = f.read()
    result_dict["instance_name"] = "gart"
    result_dict["frontend"] = "Cypher/Gremlin"
    result_dict["engine"] = "gart"
    result_dict["storage"] = "MutableCSR"
    result_dict["version"] = "0.1.0"
    return (RunningDeploymentInfo.from_dict(result_dict), 200)


def get_deployment_pod_log(pod_name, component, from_cache):  # noqa: E501
    """get_deployment_pod_log

    [Deprecated] Get kubernetes pod&#39;s log # noqa: E501

    :param pod_name: 
    :type pod_name: str
    :param component: 
    :type component: str
    :param from_cache: 
    :type from_cache: bool

    :rtype: Union[GetPodLogResponse, Tuple[GetPodLogResponse, int], Tuple[GetPodLogResponse, int, Dict[str, str]]
    """
    return 'do some magic!'


def get_deployment_resource_usage():  # noqa: E501
    """get_deployment_resource_usage

    [Deprecated] Get resource usage(cpu/memory) of cluster # noqa: E501


    :rtype: Union[GetResourceUsageResponse, Tuple[GetResourceUsageResponse, int], Tuple[GetResourceUsageResponse, int, Dict[str, str]]
    """
    return 'do some magic!'


def get_deployment_status():  # noqa: E501
    """get_deployment_status

    Get deployment status of cluster # noqa: E501


    :rtype: Union[RunningDeploymentStatus, Tuple[RunningDeploymentStatus, int], Tuple[RunningDeploymentStatus, int, Dict[str, str]]
    """
    return 'do some magic!'


def get_storage_usage():  # noqa: E501
    """get_storage_usage

    [Deprecated] Get storage usage of Groot # noqa: E501


    :rtype: Union[GetStorageUsageResponse, Tuple[GetStorageUsageResponse, int], Tuple[GetStorageUsageResponse, int, Dict[str, str]]
    """
    return 'do some magic!'
