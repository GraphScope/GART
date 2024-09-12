from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.base_edge_type_vertex_type_pair_relations_inner import BaseEdgeTypeVertexTypePairRelationsInner
from flex.server.models.create_property_meta import CreatePropertyMeta
from flex.server import util

from flex.server.models.base_edge_type_vertex_type_pair_relations_inner import BaseEdgeTypeVertexTypePairRelationsInner  # noqa: E501
from flex.server.models.create_property_meta import CreatePropertyMeta  # noqa: E501

class CreateEdgeType(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, type_name=None, vertex_type_pair_relations=None, directed=None, primary_keys=None, properties=None, description=None):  # noqa: E501
        """CreateEdgeType - a model defined in OpenAPI

        :param type_name: The type_name of this CreateEdgeType.  # noqa: E501
        :type type_name: str
        :param vertex_type_pair_relations: The vertex_type_pair_relations of this CreateEdgeType.  # noqa: E501
        :type vertex_type_pair_relations: List[BaseEdgeTypeVertexTypePairRelationsInner]
        :param directed: The directed of this CreateEdgeType.  # noqa: E501
        :type directed: bool
        :param primary_keys: The primary_keys of this CreateEdgeType.  # noqa: E501
        :type primary_keys: List[str]
        :param properties: The properties of this CreateEdgeType.  # noqa: E501
        :type properties: List[CreatePropertyMeta]
        :param description: The description of this CreateEdgeType.  # noqa: E501
        :type description: str
        """
        self.openapi_types = {
            'type_name': str,
            'vertex_type_pair_relations': List[BaseEdgeTypeVertexTypePairRelationsInner],
            'directed': bool,
            'primary_keys': List[str],
            'properties': List[CreatePropertyMeta],
            'description': str
        }

        self.attribute_map = {
            'type_name': 'type_name',
            'vertex_type_pair_relations': 'vertex_type_pair_relations',
            'directed': 'directed',
            'primary_keys': 'primary_keys',
            'properties': 'properties',
            'description': 'description'
        }

        self._type_name = type_name
        self._vertex_type_pair_relations = vertex_type_pair_relations
        self._directed = directed
        self._primary_keys = primary_keys
        self._properties = properties
        self._description = description

    @classmethod
    def from_dict(cls, dikt) -> 'CreateEdgeType':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The CreateEdgeType of this CreateEdgeType.  # noqa: E501
        :rtype: CreateEdgeType
        """
        return util.deserialize_model(dikt, cls)

    @property
    def type_name(self) -> str:
        """Gets the type_name of this CreateEdgeType.


        :return: The type_name of this CreateEdgeType.
        :rtype: str
        """
        return self._type_name

    @type_name.setter
    def type_name(self, type_name: str):
        """Sets the type_name of this CreateEdgeType.


        :param type_name: The type_name of this CreateEdgeType.
        :type type_name: str
        """
        if type_name is None:
            raise ValueError("Invalid value for `type_name`, must not be `None`")  # noqa: E501

        self._type_name = type_name

    @property
    def vertex_type_pair_relations(self) -> List[BaseEdgeTypeVertexTypePairRelationsInner]:
        """Gets the vertex_type_pair_relations of this CreateEdgeType.


        :return: The vertex_type_pair_relations of this CreateEdgeType.
        :rtype: List[BaseEdgeTypeVertexTypePairRelationsInner]
        """
        return self._vertex_type_pair_relations

    @vertex_type_pair_relations.setter
    def vertex_type_pair_relations(self, vertex_type_pair_relations: List[BaseEdgeTypeVertexTypePairRelationsInner]):
        """Sets the vertex_type_pair_relations of this CreateEdgeType.


        :param vertex_type_pair_relations: The vertex_type_pair_relations of this CreateEdgeType.
        :type vertex_type_pair_relations: List[BaseEdgeTypeVertexTypePairRelationsInner]
        """
        if vertex_type_pair_relations is None:
            raise ValueError("Invalid value for `vertex_type_pair_relations`, must not be `None`")  # noqa: E501

        self._vertex_type_pair_relations = vertex_type_pair_relations

    @property
    def directed(self) -> bool:
        """Gets the directed of this CreateEdgeType.


        :return: The directed of this CreateEdgeType.
        :rtype: bool
        """
        return self._directed

    @directed.setter
    def directed(self, directed: bool):
        """Sets the directed of this CreateEdgeType.


        :param directed: The directed of this CreateEdgeType.
        :type directed: bool
        """

        self._directed = directed

    @property
    def primary_keys(self) -> List[str]:
        """Gets the primary_keys of this CreateEdgeType.


        :return: The primary_keys of this CreateEdgeType.
        :rtype: List[str]
        """
        return self._primary_keys

    @primary_keys.setter
    def primary_keys(self, primary_keys: List[str]):
        """Sets the primary_keys of this CreateEdgeType.


        :param primary_keys: The primary_keys of this CreateEdgeType.
        :type primary_keys: List[str]
        """

        self._primary_keys = primary_keys

    @property
    def properties(self) -> List[CreatePropertyMeta]:
        """Gets the properties of this CreateEdgeType.


        :return: The properties of this CreateEdgeType.
        :rtype: List[CreatePropertyMeta]
        """
        return self._properties

    @properties.setter
    def properties(self, properties: List[CreatePropertyMeta]):
        """Sets the properties of this CreateEdgeType.


        :param properties: The properties of this CreateEdgeType.
        :type properties: List[CreatePropertyMeta]
        """

        self._properties = properties

    @property
    def description(self) -> str:
        """Gets the description of this CreateEdgeType.


        :return: The description of this CreateEdgeType.
        :rtype: str
        """
        return self._description

    @description.setter
    def description(self, description: str):
        """Sets the description of this CreateEdgeType.


        :param description: The description of this CreateEdgeType.
        :type description: str
        """

        self._description = description
