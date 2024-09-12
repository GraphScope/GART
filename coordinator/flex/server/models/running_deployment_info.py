from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server import util


class RunningDeploymentInfo(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, instance_name=None, cluster_type=None, version=None, creation_time=None, frontend=None, engine=None, storage=None):  # noqa: E501
        """RunningDeploymentInfo - a model defined in OpenAPI

        :param instance_name: The instance_name of this RunningDeploymentInfo.  # noqa: E501
        :type instance_name: str
        :param cluster_type: The cluster_type of this RunningDeploymentInfo.  # noqa: E501
        :type cluster_type: str
        :param version: The version of this RunningDeploymentInfo.  # noqa: E501
        :type version: str
        :param creation_time: The creation_time of this RunningDeploymentInfo.  # noqa: E501
        :type creation_time: str
        :param frontend: The frontend of this RunningDeploymentInfo.  # noqa: E501
        :type frontend: str
        :param engine: The engine of this RunningDeploymentInfo.  # noqa: E501
        :type engine: str
        :param storage: The storage of this RunningDeploymentInfo.  # noqa: E501
        :type storage: str
        """
        self.openapi_types = {
            'instance_name': str,
            'cluster_type': str,
            'version': str,
            'creation_time': str,
            'frontend': str,
            'engine': str,
            'storage': str
        }

        self.attribute_map = {
            'instance_name': 'instance_name',
            'cluster_type': 'cluster_type',
            'version': 'version',
            'creation_time': 'creation_time',
            'frontend': 'frontend',
            'engine': 'engine',
            'storage': 'storage'
        }

        self._instance_name = instance_name
        self._cluster_type = cluster_type
        self._version = version
        self._creation_time = creation_time
        self._frontend = frontend
        self._engine = engine
        self._storage = storage

    @classmethod
    def from_dict(cls, dikt) -> 'RunningDeploymentInfo':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The RunningDeploymentInfo of this RunningDeploymentInfo.  # noqa: E501
        :rtype: RunningDeploymentInfo
        """
        return util.deserialize_model(dikt, cls)

    @property
    def instance_name(self) -> str:
        """Gets the instance_name of this RunningDeploymentInfo.


        :return: The instance_name of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._instance_name

    @instance_name.setter
    def instance_name(self, instance_name: str):
        """Sets the instance_name of this RunningDeploymentInfo.


        :param instance_name: The instance_name of this RunningDeploymentInfo.
        :type instance_name: str
        """
        if instance_name is None:
            raise ValueError("Invalid value for `instance_name`, must not be `None`")  # noqa: E501

        self._instance_name = instance_name

    @property
    def cluster_type(self) -> str:
        """Gets the cluster_type of this RunningDeploymentInfo.


        :return: The cluster_type of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._cluster_type

    @cluster_type.setter
    def cluster_type(self, cluster_type: str):
        """Sets the cluster_type of this RunningDeploymentInfo.


        :param cluster_type: The cluster_type of this RunningDeploymentInfo.
        :type cluster_type: str
        """
        allowed_values = ["HOSTS", "KUBERNETES"]  # noqa: E501
        if cluster_type not in allowed_values:
            raise ValueError(
                "Invalid value for `cluster_type` ({0}), must be one of {1}"
                .format(cluster_type, allowed_values)
            )

        self._cluster_type = cluster_type

    @property
    def version(self) -> str:
        """Gets the version of this RunningDeploymentInfo.


        :return: The version of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._version

    @version.setter
    def version(self, version: str):
        """Sets the version of this RunningDeploymentInfo.


        :param version: The version of this RunningDeploymentInfo.
        :type version: str
        """
        if version is None:
            raise ValueError("Invalid value for `version`, must not be `None`")  # noqa: E501

        self._version = version

    @property
    def creation_time(self) -> str:
        """Gets the creation_time of this RunningDeploymentInfo.


        :return: The creation_time of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._creation_time

    @creation_time.setter
    def creation_time(self, creation_time: str):
        """Sets the creation_time of this RunningDeploymentInfo.


        :param creation_time: The creation_time of this RunningDeploymentInfo.
        :type creation_time: str
        """
        if creation_time is None:
            raise ValueError("Invalid value for `creation_time`, must not be `None`")  # noqa: E501

        self._creation_time = creation_time

    @property
    def frontend(self) -> str:
        """Gets the frontend of this RunningDeploymentInfo.


        :return: The frontend of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._frontend

    @frontend.setter
    def frontend(self, frontend: str):
        """Sets the frontend of this RunningDeploymentInfo.


        :param frontend: The frontend of this RunningDeploymentInfo.
        :type frontend: str
        """
        allowed_values = ["Cypher/Gremlin", "AnalyticalApps"]  # noqa: E501
        if frontend not in allowed_values:
            raise ValueError(
                "Invalid value for `frontend` ({0}), must be one of {1}"
                .format(frontend, allowed_values)
            )

        self._frontend = frontend

    @property
    def engine(self) -> str:
        """Gets the engine of this RunningDeploymentInfo.


        :return: The engine of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._engine

    @engine.setter
    def engine(self, engine: str):
        """Sets the engine of this RunningDeploymentInfo.


        :param engine: The engine of this RunningDeploymentInfo.
        :type engine: str
        """
        allowed_values = ["Hiactor", "Gaia"]  # noqa: E501
        if engine not in allowed_values:
            raise ValueError(
                "Invalid value for `engine` ({0}), must be one of {1}"
                .format(engine, allowed_values)
            )

        self._engine = engine

    @property
    def storage(self) -> str:
        """Gets the storage of this RunningDeploymentInfo.


        :return: The storage of this RunningDeploymentInfo.
        :rtype: str
        """
        return self._storage

    @storage.setter
    def storage(self, storage: str):
        """Sets the storage of this RunningDeploymentInfo.


        :param storage: The storage of this RunningDeploymentInfo.
        :type storage: str
        """
        allowed_values = ["MutableCSR", "MutablePersistent"]  # noqa: E501
        if storage not in allowed_values:
            raise ValueError(
                "Invalid value for `storage` ({0}), must be one of {1}"
                .format(storage, allowed_values)
            )

        self._storage = storage
