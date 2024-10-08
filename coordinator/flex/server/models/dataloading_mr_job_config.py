from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server import util


class DataloadingMRJobConfig(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, config=None):  # noqa: E501
        """DataloadingMRJobConfig - a model defined in OpenAPI

        :param config: The config of this DataloadingMRJobConfig.  # noqa: E501
        :type config: str
        """
        self.openapi_types = {
            'config': str
        }

        self.attribute_map = {
            'config': 'config'
        }

        self._config = config

    @classmethod
    def from_dict(cls, dikt) -> 'DataloadingMRJobConfig':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The DataloadingMRJobConfig of this DataloadingMRJobConfig.  # noqa: E501
        :rtype: DataloadingMRJobConfig
        """
        return util.deserialize_model(dikt, cls)

    @property
    def config(self) -> str:
        """Gets the config of this DataloadingMRJobConfig.


        :return: The config of this DataloadingMRJobConfig.
        :rtype: str
        """
        return self._config

    @config.setter
    def config(self, config: str):
        """Sets the config of this DataloadingMRJobConfig.


        :param config: The config of this DataloadingMRJobConfig.
        :type config: str
        """
        if config is None:
            raise ValueError("Invalid value for `config`, must not be `None`")  # noqa: E501

        self._config = config
