from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.column_mapping import ColumnMapping
from flex.server.models.edge_mapping_type_triplet import EdgeMappingTypeTriplet
from flex.server import util

from flex.server.models.column_mapping import ColumnMapping  # noqa: E501
from flex.server.models.edge_mapping_type_triplet import EdgeMappingTypeTriplet  # noqa: E501

class EdgeMapping(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, type_triplet=None, inputs=None, source_vertex_mappings=None, destination_vertex_mappings=None, column_mappings=None):  # noqa: E501
        """EdgeMapping - a model defined in OpenAPI

        :param type_triplet: The type_triplet of this EdgeMapping.  # noqa: E501
        :type type_triplet: EdgeMappingTypeTriplet
        :param inputs: The inputs of this EdgeMapping.  # noqa: E501
        :type inputs: List[str]
        :param source_vertex_mappings: The source_vertex_mappings of this EdgeMapping.  # noqa: E501
        :type source_vertex_mappings: List[ColumnMapping]
        :param destination_vertex_mappings: The destination_vertex_mappings of this EdgeMapping.  # noqa: E501
        :type destination_vertex_mappings: List[ColumnMapping]
        :param column_mappings: The column_mappings of this EdgeMapping.  # noqa: E501
        :type column_mappings: List[ColumnMapping]
        """
        self.openapi_types = {
            'type_triplet': EdgeMappingTypeTriplet,
            'inputs': List[str],
            'source_vertex_mappings': List[ColumnMapping],
            'destination_vertex_mappings': List[ColumnMapping],
            'column_mappings': List[ColumnMapping]
        }

        self.attribute_map = {
            'type_triplet': 'type_triplet',
            'inputs': 'inputs',
            'source_vertex_mappings': 'source_vertex_mappings',
            'destination_vertex_mappings': 'destination_vertex_mappings',
            'column_mappings': 'column_mappings'
        }

        self._type_triplet = type_triplet
        self._inputs = inputs
        self._source_vertex_mappings = source_vertex_mappings
        self._destination_vertex_mappings = destination_vertex_mappings
        self._column_mappings = column_mappings

    @classmethod
    def from_dict(cls, dikt) -> 'EdgeMapping':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The EdgeMapping of this EdgeMapping.  # noqa: E501
        :rtype: EdgeMapping
        """
        return util.deserialize_model(dikt, cls)

    @property
    def type_triplet(self) -> EdgeMappingTypeTriplet:
        """Gets the type_triplet of this EdgeMapping.


        :return: The type_triplet of this EdgeMapping.
        :rtype: EdgeMappingTypeTriplet
        """
        return self._type_triplet

    @type_triplet.setter
    def type_triplet(self, type_triplet: EdgeMappingTypeTriplet):
        """Sets the type_triplet of this EdgeMapping.


        :param type_triplet: The type_triplet of this EdgeMapping.
        :type type_triplet: EdgeMappingTypeTriplet
        """
        if type_triplet is None:
            raise ValueError("Invalid value for `type_triplet`, must not be `None`")  # noqa: E501

        self._type_triplet = type_triplet

    @property
    def inputs(self) -> List[str]:
        """Gets the inputs of this EdgeMapping.


        :return: The inputs of this EdgeMapping.
        :rtype: List[str]
        """
        return self._inputs

    @inputs.setter
    def inputs(self, inputs: List[str]):
        """Sets the inputs of this EdgeMapping.


        :param inputs: The inputs of this EdgeMapping.
        :type inputs: List[str]
        """
        if inputs is None:
            raise ValueError("Invalid value for `inputs`, must not be `None`")  # noqa: E501

        self._inputs = inputs

    @property
    def source_vertex_mappings(self) -> List[ColumnMapping]:
        """Gets the source_vertex_mappings of this EdgeMapping.


        :return: The source_vertex_mappings of this EdgeMapping.
        :rtype: List[ColumnMapping]
        """
        return self._source_vertex_mappings

    @source_vertex_mappings.setter
    def source_vertex_mappings(self, source_vertex_mappings: List[ColumnMapping]):
        """Sets the source_vertex_mappings of this EdgeMapping.


        :param source_vertex_mappings: The source_vertex_mappings of this EdgeMapping.
        :type source_vertex_mappings: List[ColumnMapping]
        """

        self._source_vertex_mappings = source_vertex_mappings

    @property
    def destination_vertex_mappings(self) -> List[ColumnMapping]:
        """Gets the destination_vertex_mappings of this EdgeMapping.


        :return: The destination_vertex_mappings of this EdgeMapping.
        :rtype: List[ColumnMapping]
        """
        return self._destination_vertex_mappings

    @destination_vertex_mappings.setter
    def destination_vertex_mappings(self, destination_vertex_mappings: List[ColumnMapping]):
        """Sets the destination_vertex_mappings of this EdgeMapping.


        :param destination_vertex_mappings: The destination_vertex_mappings of this EdgeMapping.
        :type destination_vertex_mappings: List[ColumnMapping]
        """

        self._destination_vertex_mappings = destination_vertex_mappings

    @property
    def column_mappings(self) -> List[ColumnMapping]:
        """Gets the column_mappings of this EdgeMapping.


        :return: The column_mappings of this EdgeMapping.
        :rtype: List[ColumnMapping]
        """
        return self._column_mappings

    @column_mappings.setter
    def column_mappings(self, column_mappings: List[ColumnMapping]):
        """Sets the column_mappings of this EdgeMapping.


        :param column_mappings: The column_mappings of this EdgeMapping.
        :type column_mappings: List[ColumnMapping]
        """

        self._column_mappings = column_mappings
