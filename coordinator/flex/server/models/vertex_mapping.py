from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.column_mapping import ColumnMapping
from flex.server import util

from flex.server.models.column_mapping import ColumnMapping  # noqa: E501

class VertexMapping(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, type_name=None, inputs=None, column_mappings=None):  # noqa: E501
        """VertexMapping - a model defined in OpenAPI

        :param type_name: The type_name of this VertexMapping.  # noqa: E501
        :type type_name: str
        :param inputs: The inputs of this VertexMapping.  # noqa: E501
        :type inputs: List[str]
        :param column_mappings: The column_mappings of this VertexMapping.  # noqa: E501
        :type column_mappings: List[ColumnMapping]
        """
        self.openapi_types = {
            'type_name': str,
            'inputs': List[str],
            'column_mappings': List[ColumnMapping]
        }

        self.attribute_map = {
            'type_name': 'type_name',
            'inputs': 'inputs',
            'column_mappings': 'column_mappings'
        }

        self._type_name = type_name
        self._inputs = inputs
        self._column_mappings = column_mappings

    @classmethod
    def from_dict(cls, dikt) -> 'VertexMapping':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The VertexMapping of this VertexMapping.  # noqa: E501
        :rtype: VertexMapping
        """
        return util.deserialize_model(dikt, cls)

    @property
    def type_name(self) -> str:
        """Gets the type_name of this VertexMapping.


        :return: The type_name of this VertexMapping.
        :rtype: str
        """
        return self._type_name

    @type_name.setter
    def type_name(self, type_name: str):
        """Sets the type_name of this VertexMapping.


        :param type_name: The type_name of this VertexMapping.
        :type type_name: str
        """
        if type_name is None:
            raise ValueError("Invalid value for `type_name`, must not be `None`")  # noqa: E501

        self._type_name = type_name

    @property
    def inputs(self) -> List[str]:
        """Gets the inputs of this VertexMapping.


        :return: The inputs of this VertexMapping.
        :rtype: List[str]
        """
        return self._inputs

    @inputs.setter
    def inputs(self, inputs: List[str]):
        """Sets the inputs of this VertexMapping.


        :param inputs: The inputs of this VertexMapping.
        :type inputs: List[str]
        """
        if inputs is None:
            raise ValueError("Invalid value for `inputs`, must not be `None`")  # noqa: E501

        self._inputs = inputs

    @property
    def column_mappings(self) -> List[ColumnMapping]:
        """Gets the column_mappings of this VertexMapping.


        :return: The column_mappings of this VertexMapping.
        :rtype: List[ColumnMapping]
        """
        return self._column_mappings

    @column_mappings.setter
    def column_mappings(self, column_mappings: List[ColumnMapping]):
        """Sets the column_mappings of this VertexMapping.


        :param column_mappings: The column_mappings of this VertexMapping.
        :type column_mappings: List[ColumnMapping]
        """

        self._column_mappings = column_mappings
