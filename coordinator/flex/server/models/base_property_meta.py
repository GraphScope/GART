from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.gs_data_type import GSDataType
from flex.server import util

from flex.server.models.gs_data_type import GSDataType  # noqa: E501

class BasePropertyMeta(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, property_name=None, property_type=None, nullable=None, default_value=None, description=None):  # noqa: E501
        """BasePropertyMeta - a model defined in OpenAPI

        :param property_name: The property_name of this BasePropertyMeta.  # noqa: E501
        :type property_name: str
        :param property_type: The property_type of this BasePropertyMeta.  # noqa: E501
        :type property_type: GSDataType
        :param nullable: The nullable of this BasePropertyMeta.  # noqa: E501
        :type nullable: bool
        :param default_value: The default_value of this BasePropertyMeta.  # noqa: E501
        :type default_value: object
        :param description: The description of this BasePropertyMeta.  # noqa: E501
        :type description: str
        """
        self.openapi_types = {
            'property_name': str,
            'property_type': GSDataType,
            'nullable': bool,
            'default_value': object,
            'description': str
        }

        self.attribute_map = {
            'property_name': 'property_name',
            'property_type': 'property_type',
            'nullable': 'nullable',
            'default_value': 'default_value',
            'description': 'description'
        }

        self._property_name = property_name
        self._property_type = property_type
        self._nullable = nullable
        self._default_value = default_value
        self._description = description

    @classmethod
    def from_dict(cls, dikt) -> 'BasePropertyMeta':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The BasePropertyMeta of this BasePropertyMeta.  # noqa: E501
        :rtype: BasePropertyMeta
        """
        return util.deserialize_model(dikt, cls)

    @property
    def property_name(self) -> str:
        """Gets the property_name of this BasePropertyMeta.


        :return: The property_name of this BasePropertyMeta.
        :rtype: str
        """
        return self._property_name

    @property_name.setter
    def property_name(self, property_name: str):
        """Sets the property_name of this BasePropertyMeta.


        :param property_name: The property_name of this BasePropertyMeta.
        :type property_name: str
        """
        if property_name is None:
            raise ValueError("Invalid value for `property_name`, must not be `None`")  # noqa: E501

        self._property_name = property_name

    @property
    def property_type(self) -> GSDataType:
        """Gets the property_type of this BasePropertyMeta.


        :return: The property_type of this BasePropertyMeta.
        :rtype: GSDataType
        """
        return self._property_type

    @property_type.setter
    def property_type(self, property_type: GSDataType):
        """Sets the property_type of this BasePropertyMeta.


        :param property_type: The property_type of this BasePropertyMeta.
        :type property_type: GSDataType
        """
        if property_type is None:
            raise ValueError("Invalid value for `property_type`, must not be `None`")  # noqa: E501

        self._property_type = property_type

    @property
    def nullable(self) -> bool:
        """Gets the nullable of this BasePropertyMeta.


        :return: The nullable of this BasePropertyMeta.
        :rtype: bool
        """
        return self._nullable

    @nullable.setter
    def nullable(self, nullable: bool):
        """Sets the nullable of this BasePropertyMeta.


        :param nullable: The nullable of this BasePropertyMeta.
        :type nullable: bool
        """

        self._nullable = nullable

    @property
    def default_value(self) -> object:
        """Gets the default_value of this BasePropertyMeta.


        :return: The default_value of this BasePropertyMeta.
        :rtype: object
        """
        return self._default_value

    @default_value.setter
    def default_value(self, default_value: object):
        """Sets the default_value of this BasePropertyMeta.


        :param default_value: The default_value of this BasePropertyMeta.
        :type default_value: object
        """

        self._default_value = default_value

    @property
    def description(self) -> str:
        """Gets the description of this BasePropertyMeta.


        :return: The description of this BasePropertyMeta.
        :rtype: str
        """
        return self._description

    @description.setter
    def description(self, description: str):
        """Sets the description of this BasePropertyMeta.


        :param description: The description of this BasePropertyMeta.
        :type description: str
        """

        self._description = description
