from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server import util


class CreateGraphByYamlRequest(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, name=None, description=None, _schema=None):  # noqa: E501
        """CreateGraphByYamlRequest - a model defined in OpenAPI

        :param name: The name of this CreateGraphByYamlRequest.  # noqa: E501
        :type name: str
        :param description: The description of this CreateGraphByYamlRequest.  # noqa: E501
        :type description: str
        :param _schema: The _schema of this CreateGraphByYamlRequest.  # noqa: E501
        :type _schema: str
        """
        self.openapi_types = {
            'name': str,
            'description': str,
            '_schema': str
        }

        self.attribute_map = {
            'name': 'name',
            'description': 'description',
            '_schema': 'schema'
        }

        self._name = name
        self._description = description
        self.__schema = _schema

    @classmethod
    def from_dict(cls, dikt) -> 'CreateGraphByYamlRequest':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The CreateGraphByYamlRequest of this CreateGraphByYamlRequest.  # noqa: E501
        :rtype: CreateGraphByYamlRequest
        """
        return util.deserialize_model(dikt, cls)

    @property
    def name(self) -> str:
        """Gets the name of this CreateGraphByYamlRequest.


        :return: The name of this CreateGraphByYamlRequest.
        :rtype: str
        """
        return self._name

    @name.setter
    def name(self, name: str):
        """Sets the name of this CreateGraphByYamlRequest.


        :param name: The name of this CreateGraphByYamlRequest.
        :type name: str
        """

        self._name = name

    @property
    def description(self) -> str:
        """Gets the description of this CreateGraphByYamlRequest.


        :return: The description of this CreateGraphByYamlRequest.
        :rtype: str
        """
        return self._description

    @description.setter
    def description(self, description: str):
        """Sets the description of this CreateGraphByYamlRequest.


        :param description: The description of this CreateGraphByYamlRequest.
        :type description: str
        """

        self._description = description

    @property
    def _schema(self) -> str:
        """Gets the _schema of this CreateGraphByYamlRequest.


        :return: The _schema of this CreateGraphByYamlRequest.
        :rtype: str
        """
        return self.__schema

    @_schema.setter
    def _schema(self, _schema: str):
        """Sets the _schema of this CreateGraphByYamlRequest.


        :param _schema: The _schema of this CreateGraphByYamlRequest.
        :type _schema: str
        """

        self.__schema = _schema
