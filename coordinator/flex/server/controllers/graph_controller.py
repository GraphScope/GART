import connexion
from typing import Dict
from typing import Tuple
from typing import Union

from flex.server.models.change_graph_version_request import ChangeGraphVersionRequest  # noqa: E501
from flex.server.models.create_edge_type import CreateEdgeType  # noqa: E501
from flex.server.models.create_graph_by_pgql_request import CreateGraphByPgqlRequest  # noqa: E501
from flex.server.models.create_graph_by_yaml_request import CreateGraphByYamlRequest  # noqa: E501
from flex.server.models.create_graph_request import CreateGraphRequest  # noqa: E501
from flex.server.models.create_graph_response import CreateGraphResponse  # noqa: E501
from flex.server.models.create_graph_schema_request import CreateGraphSchemaRequest  # noqa: E501
from flex.server.models.create_vertex_type import CreateVertexType  # noqa: E501
from flex.server.models.error import Error  # noqa: E501
from flex.server.models.get_graph_response import GetGraphResponse  # noqa: E501
from flex.server.models.get_graph_schema_response import GetGraphSchemaResponse  # noqa: E501
from flex.server.models.get_graph_version_response import GetGraphVersionResponse  # noqa: E501
from flex.server import util

import etcd3
from urllib.parse import urlparse
import json
import yaml
import os
import time
import sys
import requests

GRAPH_ID = None

def get_graph_schema():
    property_data_type_mapping = {}

    property_data_type_mapping["int"] = {"primitive_type": "DT_SIGNED_INT32"}
    property_data_type_mapping["integer"] = {"primitive_type": "DT_SIGNED_INT32"}
    property_data_type_mapping["bigint"] = {"primitive_type": "DT_SIGNED_INT64"}
    property_data_type_mapping["float"] = {"primitive_type": "DT_FLOAT"}
    property_data_type_mapping["double precision"] = {"primitive_type": "DT_FLOAT"}
    property_data_type_mapping["varchar"] = {"string": {"long_text": ""}}
    property_data_type_mapping["character varying"] = {"string": {"long_text": ""}}
    property_data_type_mapping["text"] = {"string": {"long_text": ""}}

    etcd_server = os.getenv("ETCD_SERVICE", "127.0.0.1:23790")
    if not etcd_server.startswith(("http://", "https://")):
        etcd_server = f"http://{etcd_server}"
    parsed_url = urlparse(etcd_server)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")

    result_dict = {}

    rg_mapping_key = etcd_prefix + "gart_rg_mapping_yaml"

    try_max_times = 3
    try_times = 0
    while try_times < try_max_times:
        try:
            rg_mapping_str, _ = etcd_client.get(rg_mapping_key)
            if rg_mapping_str is not None:
                rg_mapping_str = rg_mapping_str.decode("utf-8")
                break
            try_times += 1
            time.sleep(2)
        except Exception as e:
            try_times += 1
            time.sleep(2)

    if try_times == try_max_times:
        return result_dict

    rg_mapping = yaml.load(rg_mapping_str, Loader=yaml.SafeLoader)

    table_schema_key = etcd_prefix + "gart_table_schema"
    try_times = 0
    while try_times < try_max_times:
        try:
            table_schema_str, _ = etcd_client.get(table_schema_key)
            if table_schema_str is not None:
                table_schema_str = table_schema_str.decode("utf-8")
                break
            try_times += 1
            time.sleep(2)
        except Exception as e:
            try_times += 1
            time.sleep(2)

    if try_times == try_max_times:
        return result_dict

    table_schema = json.loads(table_schema_str)

    result_dict["name"] = GRAPH_ID
    result_dict["id"] = GRAPH_ID
    vertex_types_array = []
    edge_types_array = []
    schema_dict = {}

    vertex_types = rg_mapping["vertexMappings"]["vertex_types"]
    vertex_type_number = len(vertex_types)

    for idx in range(vertex_type_number):
        vertex_type_dict = {}
        vertex_type_dict["type_name"] = vertex_types[idx]["type_name"]
        table_name = vertex_types[idx]["dataSourceName"]
        property_mappings = vertex_types[idx]["mappings"]
        properties_array = []
        property_info_array = table_schema[table_name]
        for prop_idx in range(len(property_mappings)):
            property_dict = {}
            property_dict["property_name"] = property_mappings[prop_idx]["property"]
            table_cloumn_name = property_mappings[prop_idx]["dataField"]["name"]
            for i in range(len(property_info_array)):
                if property_info_array[i][0] == table_cloumn_name:
                    if property_info_array[i][1].startswith("varchar"):
                        property_dict["property_type"] = property_data_type_mapping[
                            "varchar"
                        ]
                    else:
                        property_dict["property_type"] = property_data_type_mapping.get(
                            property_info_array[i][1],
                            property_data_type_mapping["varchar"],
                        )
                    break
            properties_array.append(property_dict)
        vertex_type_dict["properties"] = properties_array
        vertex_types_array.append(vertex_type_dict)

    edge_types = rg_mapping["edgeMappings"]["edge_types"]
    edge_type_number = len(edge_types)
    for idx in range(edge_type_number):
        edge_type_dict = {}
        edge_type_dict["type_name"] = edge_types[idx]["type_pair"]["edge"]
        vertex_type_pair_relation = {}
        vertex_type_pair_relation["source_vertex"] = edge_types[idx]["type_pair"][
            "source_vertex"
        ]
        vertex_type_pair_relation["destination_vertex"] = edge_types[idx]["type_pair"][
            "destination_vertex"
        ]
        # FIXME: hard code to MANY_TO_MANY
        vertex_type_pair_relation["relation"] = "MANY_TO_MANY"
        edge_type_dict["vertex_type_pair_relations"] = [vertex_type_pair_relation]
        edge_type_dict["directed"] = not edge_types[idx]["type_pair"].get(
            "undirected", False
        )
        properties_array = []
        property_mappings = edge_types[idx]["dataFieldMappings"]
        table_name = edge_types[idx]["dataSourceName"]
        property_info_array = table_schema[table_name]
        for prop_idx in range(len(property_mappings)):
            property_dict = {}
            property_dict["property_name"] = property_mappings[prop_idx]["property"]
            table_cloumn_name = property_mappings[prop_idx]["dataField"]["name"]
            for i in range(len(property_info_array)):
                if property_info_array[i][0] == table_cloumn_name:
                    if property_info_array[i][1].startswith("varchar"):
                        property_dict["property_type"] = property_data_type_mapping[
                            "varchar"
                        ]
                    else:
                        property_dict["property_type"] = property_data_type_mapping.get(
                            property_info_array[i][1],
                            property_data_type_mapping["varchar"],
                        )
                    break
            properties_array.append(property_dict)
        edge_type_dict["properties"] = properties_array
        edge_types_array.append(edge_type_dict)

    schema_dict["vertex_types"] = vertex_types_array
    schema_dict["edge_types"] = edge_types_array
    result_dict["schema"] = schema_dict

    return result_dict


def create_edge_type(graph_id, create_edge_type=None):  # noqa: E501
    """create_edge_type

    Create a edge type # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param create_edge_type: 
    :type create_edge_type: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    if connexion.request.is_json:
        create_edge_type = CreateEdgeType.from_dict(connexion.request.get_json())  # noqa: E501
    return 'do some magic!'


def create_graph(create_graph_request):  # noqa: E501
    """create_graph

    Create a new graph # noqa: E501

    :param create_graph_request: 
    :type create_graph_request: dict | bytes

    :rtype: Union[CreateGraphResponse, Tuple[CreateGraphResponse, int], Tuple[CreateGraphResponse, int, Dict[str, str]]
    """
    result_dict = {}
    if isinstance(create_graph_request, dict):
        result_dict["graph_id"] = create_graph_request["name"]
    else:
        create_graph_request = json.loads(create_graph_request)
        result_dict["graph_id"] = create_graph_request["name"]
    global GRAPH_ID
    GRAPH_ID = result_dict["graph_id"]
    with open("/tmp/graph_id.txt", "w") as f:
        f.write(GRAPH_ID)
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
         
    response = requests.post(
        f"{gart_controller_server}/submit-graph-schema",
        headers={"Content-Type": "application/json"},
        data=json.dumps({"schema": json.dumps(create_graph_request)}),
    )
    return (CreateGraphResponse.from_dict(result_dict), response.status_code)


def create_graph_by_pgql(create_graph_by_pgql_request):  # noqa: E501
    """create_graph_by_pgql

    Create a new graph by providing pgql # noqa: E501

    :param create_graph_by_pgql_request: 
    :type create_graph_by_pgql_request: dict | bytes

    :rtype: Union[CreateGraphResponse, Tuple[CreateGraphResponse, int], Tuple[CreateGraphResponse, int, Dict[str, str]]
    """
    result_dict = {}
    if isinstance(create_graph_by_pgql_request, dict):
        result_dict["graph_id"] = create_graph_by_pgql_request["name"]
    else:
        create_graph_by_pgql_request = json.loads(create_graph_by_pgql_request)
        result_dict["graph_id"] = create_graph_by_pgql_request["name"]
    global GRAPH_ID
    GRAPH_ID = result_dict["graph_id"]
    with open("/tmp/graph_id.txt", "w") as f:
        f.write(GRAPH_ID)
    create_graph_by_pgql_request = create_graph_by_pgql_request["schema"]
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    response = requests.post(
        f"{gart_controller_server}/submit-pgql-config",
        data={"schema": create_graph_by_pgql_request},
    )
    return (CreateGraphResponse.from_dict(result_dict), response.status_code)


def create_graph_by_yaml(create_graph_by_yaml_request):  # noqa: E501
    """create_graph_by_yaml

    Create a new graph by providing yaml # noqa: E501

    :param create_graph_by_yaml_request: 
    :type create_graph_by_yaml_request: dict | bytes

    :rtype: Union[CreateGraphResponse, Tuple[CreateGraphResponse, int], Tuple[CreateGraphResponse, int, Dict[str, str]]
    """
    result_dict = {}
    if isinstance(create_graph_by_yaml_request, dict):
        result_dict["graph_id"] = create_graph_by_yaml_request["name"]
    else:
        create_graph_by_yaml_request = json.loads(create_graph_by_yaml_request)
        result_dict["graph_id"] = create_graph_by_yaml_request["name"]
    global GRAPH_ID
    GRAPH_ID = result_dict["graph_id"]
    with open("/tmp/graph_id.txt", "w") as f:
        f.write(GRAPH_ID)
    create_graph_request_yaml = create_graph_by_yaml_request["schema"]
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    response = requests.post(
        f"{gart_controller_server}/submit-config",
        data={"schema": create_graph_request_yaml},
    )
    return (CreateGraphResponse.from_dict(result_dict), response.status_code)


def create_vertex_type(graph_id, create_vertex_type):  # noqa: E501
    """create_vertex_type

    Create a vertex type # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param create_vertex_type: 
    :type create_vertex_type: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    if connexion.request.is_json:
        create_vertex_type = CreateVertexType.from_dict(connexion.request.get_json())  # noqa: E501
    return 'do some magic!'


def delete_edge_type_by_name(graph_id, type_name, source_vertex_type, destination_vertex_type):  # noqa: E501
    """delete_edge_type_by_name

    Delete edge type by name # noqa: E501

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


def delete_graph_by_id(graph_id):  # noqa: E501
    """delete_graph_by_id

    Delete graph by ID # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'


def delete_vertex_type_by_name(graph_id, type_name):  # noqa: E501
    """delete_vertex_type_by_name

    Delete vertex type by name # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param type_name: 
    :type type_name: str

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    return 'do some magic!'


def get_graph_all_available_versions(graph_id):  # noqa: E501
    """get_graph_all_available_versions

    Get all available versions for a specific graph # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[List[GetGraphVersionResponse], Tuple[List[GetGraphVersionResponse], int], Tuple[List[GetGraphVersionResponse], int, Dict[str, str]]
    """
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    response = requests.get(f"{gart_controller_server}/get-all-available-read-epochs")
    result = []
    all_versions = response.json()
    for idx in range(len(all_versions)):
        result_dict = {}
        result_dict["version_id"] = str(all_versions[idx][0])
        result_dict["begin_time"] = str(all_versions[idx][1])
        result_dict["end_time"] = str(all_versions[idx][2])
        result_dict["num_vertices"] = str(all_versions[idx][3])
        result_dict["num_edges"] = str(all_versions[idx][4])
        result.append(GetGraphVersionResponse.from_dict(result_dict))
    return (result, 200)


def get_graph_by_id(graph_id):  # noqa: E501
    """get_graph_by_id

    Get graph by ID # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[GetGraphResponse, Tuple[GetGraphResponse, int], Tuple[GetGraphResponse, int, Dict[str, str]]
    """
    result_dict = get_graph_schema()
    if not result_dict:
        return (GetGraphResponse.from_dict(result_dict), 500)
    result_dict["id"] = graph_id
    result_dict["name"] = graph_id
    return (GetGraphResponse.from_dict(result_dict), 200)


def get_graph_version_by_timestamp(graph_id, timestamp):  # noqa: E501
    """get_graph_version_by_timestamp

    Get the latest version by providing a timestamp # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param timestamp: 
    :type timestamp: str

    :rtype: Union[GetGraphVersionResponse, Tuple[GetGraphVersionResponse, int], Tuple[GetGraphVersionResponse, int, Dict[str, str]]
    """
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    response = requests.post(
        f"{gart_controller_server}/get-read-epoch-by-timestamp",
        data={"timestamp": timestamp},
    )
    return (GetGraphVersionResponse.from_dict(response.json()), 200)


def get_schema_by_id(graph_id):  # noqa: E501
    """get_schema_by_id

    Get graph schema by ID # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[GetGraphSchemaResponse, Tuple[GetGraphSchemaResponse, int], Tuple[GetGraphSchemaResponse, int, Dict[str, str]]
    """
    result_dict = get_graph_schema()

    if result_dict:
        return (GetGraphSchemaResponse.from_dict(result_dict["schema"]), 200)
    else:
        return (GetGraphSchemaResponse.from_dict(result_dict["schema"]), 500)


def import_schema_by_id(graph_id, create_graph_schema_request):  # noqa: E501
    """import_schema_by_id

    Import graph schema # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param create_graph_schema_request: 
    :type create_graph_schema_request: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    if connexion.request.is_json:
        create_graph_schema_request = CreateGraphSchemaRequest.from_dict(connexion.request.get_json())  # noqa: E501
    return 'do some magic!'


def list_graphs():  # noqa: E501
    """list_graphs

    List all graphs # noqa: E501


    :rtype: Union[List[GetGraphResponse], Tuple[List[GetGraphResponse], int], Tuple[List[GetGraphResponse], int, Dict[str, str]]
    """
    result_dict = get_graph_schema()
    if not result_dict:
        return ([GetGraphResponse.from_dict(result_dict)], 500)
    return ([GetGraphResponse.from_dict(result_dict)], 200)


def read_graph_by_a_given_version(graph_id, change_graph_version_request):  # noqa: E501
    """read_graph_by_a_given_version

    Read graph by a given version # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param change_graph_version_request: 
    :type change_graph_version_request: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    gart_controller_server = os.getenv("GART_CONTROLLER_SERVER", "127.0.0.1:8080")
    if not gart_controller_server.startswith(("http://", "https://")):
        gart_controller_server = f"http://{gart_controller_server}"
    graph_version = change_graph_version_request["version_id"]
    response = requests.post(
        f"{gart_controller_server}/change-read-epoch",
        data={"read_epoch": graph_version},
    )
    return (response.text, response.status_code)
