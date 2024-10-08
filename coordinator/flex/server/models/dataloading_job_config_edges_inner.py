from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server import util


class DataloadingJobConfigEdgesInner(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, type_name=None, source_vertex=None, destination_vertex=None):  # noqa: E501
        """DataloadingJobConfigEdgesInner - a model defined in OpenAPI

        :param type_name: The type_name of this DataloadingJobConfigEdgesInner.  # noqa: E501
        :type type_name: str
        :param source_vertex: The source_vertex of this DataloadingJobConfigEdgesInner.  # noqa: E501
        :type source_vertex: str
        :param destination_vertex: The destination_vertex of this DataloadingJobConfigEdgesInner.  # noqa: E501
        :type destination_vertex: str
        """
        self.openapi_types = {
            'type_name': str,
            'source_vertex': str,
            'destination_vertex': str
        }

        self.attribute_map = {
            'type_name': 'type_name',
            'source_vertex': 'source_vertex',
            'destination_vertex': 'destination_vertex'
        }

        self._type_name = type_name
        self._source_vertex = source_vertex
        self._destination_vertex = destination_vertex

    @classmethod
    def from_dict(cls, dikt) -> 'DataloadingJobConfigEdgesInner':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The DataloadingJobConfig_edges_inner of this DataloadingJobConfigEdgesInner.  # noqa: E501
        :rtype: DataloadingJobConfigEdgesInner
        """
        return util.deserialize_model(dikt, cls)

    @property
    def type_name(self) -> str:
        """Gets the type_name of this DataloadingJobConfigEdgesInner.


        :return: The type_name of this DataloadingJobConfigEdgesInner.
        :rtype: str
        """
        return self._type_name

    @type_name.setter
    def type_name(self, type_name: str):
        """Sets the type_name of this DataloadingJobConfigEdgesInner.


        :param type_name: The type_name of this DataloadingJobConfigEdgesInner.
        :type type_name: str
        """

        self._type_name = type_name

    @property
    def source_vertex(self) -> str:
        """Gets the source_vertex of this DataloadingJobConfigEdgesInner.


        :return: The source_vertex of this DataloadingJobConfigEdgesInner.
        :rtype: str
        """
        return self._source_vertex

    @source_vertex.setter
    def source_vertex(self, source_vertex: str):
        """Sets the source_vertex of this DataloadingJobConfigEdgesInner.


        :param source_vertex: The source_vertex of this DataloadingJobConfigEdgesInner.
        :type source_vertex: str
        """

        self._source_vertex = source_vertex

    @property
    def destination_vertex(self) -> str:
        """Gets the destination_vertex of this DataloadingJobConfigEdgesInner.


        :return: The destination_vertex of this DataloadingJobConfigEdgesInner.
        :rtype: str
        """
        return self._destination_vertex

    @destination_vertex.setter
    def destination_vertex(self, destination_vertex: str):
        """Sets the destination_vertex of this DataloadingJobConfigEdgesInner.


        :param destination_vertex: The destination_vertex of this DataloadingJobConfigEdgesInner.
        :type destination_vertex: str
        """

        self._destination_vertex = destination_vertex
