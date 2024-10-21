import connexion
from typing import Dict
from typing import Tuple
from typing import Union

from flex.server.models.error import Error  # noqa: E501
from flex.server.models.schema_mapping import SchemaMapping  # noqa: E501
from flex.server import util

import os
import requests
import json
import etcd3

def bind_datasource_in_batch(graph_id, schema_mapping):  # noqa: E501
    """bind_datasource_in_batch

    Bind data sources in batches # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param schema_mapping: 
    :type schema_mapping: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    with open("/tmp/graph_id.txt", "r") as f:
        existing_graph_id = f.read()
    if graph_id != existing_graph_id:
        return (f"Graph id {graph_id} not founded", 500)
    
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
        
    if not isinstance(schema_mapping, dict):
        schema_mapping = json.loads(schema_mapping)
        
    response = requests.post(
        f"{gart_controller_server}/submit-data-source",
        headers={"Content-Type": "application/json"},
        data=json.dumps({"schema": json.dumps(schema_mapping)}),
    )
    return (response.text, response.status_code)


def get_datasource_by_id(graph_id):  # noqa: E501
    """get_datasource_by_id

    Get data source by ID # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[SchemaMapping, Tuple[SchemaMapping, int], Tuple[SchemaMapping, int, Dict[str, str]]
    """
    with open("/tmp/graph_id.txt", "r") as f:
        existing_graph_id = f.read()
    if graph_id != existing_graph_id:
        return (f"Graph id {graph_id} not founded", 500)
    
    etcd_server = os.getenv("ETCD_SERVICE", "etcd")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")
    etcd_host = etcd_server.split("://")[1].split(":")[0]
    etcd_port = etcd_server.split(":")[2]
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    
    try:
        data_source_config, _ = etcd_client.get(etcd_prefix + "gart_data_source_json")
    except Exception as e:
        return "Failed to get data source: " + str(e), 500
    
    data_source_config = json.loads(data_source_config.decode("utf-8"))
    
    return (SchemaMapping.from_dict(data_source_config), 200)


def unbind_edge_datasource(graph_id, type_name, source_vertex_type, destination_vertex_type):  # noqa: E501
    """unbind_edge_datasource

    Unbind datas ource on an edge type # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param type_name: 
    :type type_name: str
    :param source_vertex_type: 
    :type source_vertex_type: str
    :param destination_vertex_type: 
    :type destination_vertex_type: str

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'


def unbind_vertex_datasource(graph_id, type_name):  # noqa: E501
    """unbind_vertex_datasource

    Unbind data source on a vertex type # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param type_name: 
    :type type_name: str

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'
