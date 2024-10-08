from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.base_edge_type_vertex_type_pair_relations_inner_x_csr_params import BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams
from flex.server import util

from flex.server.models.base_edge_type_vertex_type_pair_relations_inner_x_csr_params import BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams  # noqa: E501

class BaseEdgeTypeVertexTypePairRelationsInner(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, source_vertex=None, destination_vertex=None, relation=None, x_csr_params=None):  # noqa: E501
        """BaseEdgeTypeVertexTypePairRelationsInner - a model defined in OpenAPI

        :param source_vertex: The source_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.  # noqa: E501
        :type source_vertex: str
        :param destination_vertex: The destination_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.  # noqa: E501
        :type destination_vertex: str
        :param relation: The relation of this BaseEdgeTypeVertexTypePairRelationsInner.  # noqa: E501
        :type relation: str
        :param x_csr_params: The x_csr_params of this BaseEdgeTypeVertexTypePairRelationsInner.  # noqa: E501
        :type x_csr_params: BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams
        """
        self.openapi_types = {
            'source_vertex': str,
            'destination_vertex': str,
            'relation': str,
            'x_csr_params': BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams
        }

        self.attribute_map = {
            'source_vertex': 'source_vertex',
            'destination_vertex': 'destination_vertex',
            'relation': 'relation',
            'x_csr_params': 'x_csr_params'
        }

        self._source_vertex = source_vertex
        self._destination_vertex = destination_vertex
        self._relation = relation
        self._x_csr_params = x_csr_params

    @classmethod
    def from_dict(cls, dikt) -> 'BaseEdgeTypeVertexTypePairRelationsInner':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The BaseEdgeType_vertex_type_pair_relations_inner of this BaseEdgeTypeVertexTypePairRelationsInner.  # noqa: E501
        :rtype: BaseEdgeTypeVertexTypePairRelationsInner
        """
        return util.deserialize_model(dikt, cls)

    @property
    def source_vertex(self) -> str:
        """Gets the source_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.


        :return: The source_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.
        :rtype: str
        """
        return self._source_vertex

    @source_vertex.setter
    def source_vertex(self, source_vertex: str):
        """Sets the source_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.


        :param source_vertex: The source_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.
        :type source_vertex: str
        """
        if source_vertex is None:
            raise ValueError("Invalid value for `source_vertex`, must not be `None`")  # noqa: E501

        self._source_vertex = source_vertex

    @property
    def destination_vertex(self) -> str:
        """Gets the destination_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.


        :return: The destination_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.
        :rtype: str
        """
        return self._destination_vertex

    @destination_vertex.setter
    def destination_vertex(self, destination_vertex: str):
        """Sets the destination_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.


        :param destination_vertex: The destination_vertex of this BaseEdgeTypeVertexTypePairRelationsInner.
        :type destination_vertex: str
        """
        if destination_vertex is None:
            raise ValueError("Invalid value for `destination_vertex`, must not be `None`")  # noqa: E501

        self._destination_vertex = destination_vertex

    @property
    def relation(self) -> str:
        """Gets the relation of this BaseEdgeTypeVertexTypePairRelationsInner.


        :return: The relation of this BaseEdgeTypeVertexTypePairRelationsInner.
        :rtype: str
        """
        return self._relation

    @relation.setter
    def relation(self, relation: str):
        """Sets the relation of this BaseEdgeTypeVertexTypePairRelationsInner.


        :param relation: The relation of this BaseEdgeTypeVertexTypePairRelationsInner.
        :type relation: str
        """
        allowed_values = ["MANY_TO_MANY", "ONE_TO_MANY", "MANY_TO_ONE", "ONE_TO_ONE"]  # noqa: E501
        if relation not in allowed_values:
            raise ValueError(
                "Invalid value for `relation` ({0}), must be one of {1}"
                .format(relation, allowed_values)
            )

        self._relation = relation

    @property
    def x_csr_params(self) -> BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams:
        """Gets the x_csr_params of this BaseEdgeTypeVertexTypePairRelationsInner.


        :return: The x_csr_params of this BaseEdgeTypeVertexTypePairRelationsInner.
        :rtype: BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams
        """
        return self._x_csr_params

    @x_csr_params.setter
    def x_csr_params(self, x_csr_params: BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams):
        """Sets the x_csr_params of this BaseEdgeTypeVertexTypePairRelationsInner.


        :param x_csr_params: The x_csr_params of this BaseEdgeTypeVertexTypePairRelationsInner.
        :type x_csr_params: BaseEdgeTypeVertexTypePairRelationsInnerXCsrParams
        """

        self._x_csr_params = x_csr_params
