from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.edge_mapping import EdgeMapping
from flex.server.models.vertex_mapping import VertexMapping
from flex.server import util

from flex.server.models.edge_mapping import EdgeMapping  # noqa: E501
from flex.server.models.vertex_mapping import VertexMapping  # noqa: E501

class SchemaMapping(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, vertex_mappings=None, edge_mappings=None):  # noqa: E501
        """SchemaMapping - a model defined in OpenAPI

        :param vertex_mappings: The vertex_mappings of this SchemaMapping.  # noqa: E501
        :type vertex_mappings: List[VertexMapping]
        :param edge_mappings: The edge_mappings of this SchemaMapping.  # noqa: E501
        :type edge_mappings: List[EdgeMapping]
        """
        self.openapi_types = {
            'vertex_mappings': List[VertexMapping],
            'edge_mappings': List[EdgeMapping]
        }

        self.attribute_map = {
            'vertex_mappings': 'vertex_mappings',
            'edge_mappings': 'edge_mappings'
        }

        self._vertex_mappings = vertex_mappings
        self._edge_mappings = edge_mappings

    @classmethod
    def from_dict(cls, dikt) -> 'SchemaMapping':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The SchemaMapping of this SchemaMapping.  # noqa: E501
        :rtype: SchemaMapping
        """
        return util.deserialize_model(dikt, cls)

    @property
    def vertex_mappings(self) -> List[VertexMapping]:
        """Gets the vertex_mappings of this SchemaMapping.


        :return: The vertex_mappings of this SchemaMapping.
        :rtype: List[VertexMapping]
        """
        return self._vertex_mappings

    @vertex_mappings.setter
    def vertex_mappings(self, vertex_mappings: List[VertexMapping]):
        """Sets the vertex_mappings of this SchemaMapping.


        :param vertex_mappings: The vertex_mappings of this SchemaMapping.
        :type vertex_mappings: List[VertexMapping]
        """
        if vertex_mappings is None:
            raise ValueError("Invalid value for `vertex_mappings`, must not be `None`")  # noqa: E501

        self._vertex_mappings = vertex_mappings

    @property
    def edge_mappings(self) -> List[EdgeMapping]:
        """Gets the edge_mappings of this SchemaMapping.


        :return: The edge_mappings of this SchemaMapping.
        :rtype: List[EdgeMapping]
        """
        return self._edge_mappings

    @edge_mappings.setter
    def edge_mappings(self, edge_mappings: List[EdgeMapping]):
        """Sets the edge_mappings of this SchemaMapping.


        :param edge_mappings: The edge_mappings of this SchemaMapping.
        :type edge_mappings: List[EdgeMapping]
        """
        if edge_mappings is None:
            raise ValueError("Invalid value for `edge_mappings`, must not be `None`")  # noqa: E501

        self._edge_mappings = edge_mappings
