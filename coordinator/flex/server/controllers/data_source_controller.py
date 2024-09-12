import connexion
from typing import Dict
from typing import Tuple
from typing import Union

from flex.server.models.error import Error  # noqa: E501
from flex.server.models.schema_mapping import SchemaMapping  # noqa: E501
from flex.server import util


def bind_datasource_in_batch(graph_id, schema_mapping):  # noqa: E501
    """bind_datasource_in_batch

    Bind data sources in batches # noqa: E501

    :param graph_id: 
    :type graph_id: str
    :param schema_mapping: 
    :type schema_mapping: dict | bytes

    :rtype: Union[str, Tuple[str, int], Tuple[str, int, Dict[str, str]]
    """
    if connexion.request.is_json:
        schema_mapping = SchemaMapping.from_dict(connexion.request.get_json())  # noqa: E501
    return 'do some magic!'


def get_datasource_by_id(graph_id):  # noqa: E501
    """get_datasource_by_id

    Get data source by ID # noqa: E501

    :param graph_id: 
    :type graph_id: str

    :rtype: Union[SchemaMapping, Tuple[SchemaMapping, int], Tuple[SchemaMapping, int, Dict[str, str]]
    """
    return 'do some magic!'


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
