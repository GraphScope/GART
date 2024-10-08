from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server import util


class DataloadingJobConfigLoadingConfigFormat(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, type=None, metadata=None):  # noqa: E501
        """DataloadingJobConfigLoadingConfigFormat - a model defined in OpenAPI

        :param type: The type of this DataloadingJobConfigLoadingConfigFormat.  # noqa: E501
        :type type: str
        :param metadata: The metadata of this DataloadingJobConfigLoadingConfigFormat.  # noqa: E501
        :type metadata: Dict[str, object]
        """
        self.openapi_types = {
            'type': str,
            'metadata': Dict[str, object]
        }

        self.attribute_map = {
            'type': 'type',
            'metadata': 'metadata'
        }

        self._type = type
        self._metadata = metadata

    @classmethod
    def from_dict(cls, dikt) -> 'DataloadingJobConfigLoadingConfigFormat':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The DataloadingJobConfig_loading_config_format of this DataloadingJobConfigLoadingConfigFormat.  # noqa: E501
        :rtype: DataloadingJobConfigLoadingConfigFormat
        """
        return util.deserialize_model(dikt, cls)

    @property
    def type(self) -> str:
        """Gets the type of this DataloadingJobConfigLoadingConfigFormat.


        :return: The type of this DataloadingJobConfigLoadingConfigFormat.
        :rtype: str
        """
        return self._type

    @type.setter
    def type(self, type: str):
        """Sets the type of this DataloadingJobConfigLoadingConfigFormat.


        :param type: The type of this DataloadingJobConfigLoadingConfigFormat.
        :type type: str
        """

        self._type = type

    @property
    def metadata(self) -> Dict[str, object]:
        """Gets the metadata of this DataloadingJobConfigLoadingConfigFormat.


        :return: The metadata of this DataloadingJobConfigLoadingConfigFormat.
        :rtype: Dict[str, object]
        """
        return self._metadata

    @metadata.setter
    def metadata(self, metadata: Dict[str, object]):
        """Sets the metadata of this DataloadingJobConfigLoadingConfigFormat.


        :param metadata: The metadata of this DataloadingJobConfigLoadingConfigFormat.
        :type metadata: Dict[str, object]
        """

        self._metadata = metadata
