from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from flex.server.models.base_model import Model
from flex.server.models.parameter import Parameter
from flex.server import util

from flex.server.models.parameter import Parameter  # noqa: E501

class GetStoredProcResponse(Model):
    """NOTE: This class is auto generated by OpenAPI Generator (https://openapi-generator.tech).

    Do not edit the class manually.
    """

    def __init__(self, name=None, description=None, type=None, query=None, id=None, library=None, params=None, returns=None, bound_graph=None, runnable=None):  # noqa: E501
        """GetStoredProcResponse - a model defined in OpenAPI

        :param name: The name of this GetStoredProcResponse.  # noqa: E501
        :type name: str
        :param description: The description of this GetStoredProcResponse.  # noqa: E501
        :type description: str
        :param type: The type of this GetStoredProcResponse.  # noqa: E501
        :type type: str
        :param query: The query of this GetStoredProcResponse.  # noqa: E501
        :type query: str
        :param id: The id of this GetStoredProcResponse.  # noqa: E501
        :type id: str
        :param library: The library of this GetStoredProcResponse.  # noqa: E501
        :type library: str
        :param params: The params of this GetStoredProcResponse.  # noqa: E501
        :type params: List[Parameter]
        :param returns: The returns of this GetStoredProcResponse.  # noqa: E501
        :type returns: List[Parameter]
        :param bound_graph: The bound_graph of this GetStoredProcResponse.  # noqa: E501
        :type bound_graph: str
        :param runnable: The runnable of this GetStoredProcResponse.  # noqa: E501
        :type runnable: bool
        """
        self.openapi_types = {
            'name': str,
            'description': str,
            'type': str,
            'query': str,
            'id': str,
            'library': str,
            'params': List[Parameter],
            'returns': List[Parameter],
            'bound_graph': str,
            'runnable': bool
        }

        self.attribute_map = {
            'name': 'name',
            'description': 'description',
            'type': 'type',
            'query': 'query',
            'id': 'id',
            'library': 'library',
            'params': 'params',
            'returns': 'returns',
            'bound_graph': 'bound_graph',
            'runnable': 'runnable'
        }

        self._name = name
        self._description = description
        self._type = type
        self._query = query
        self._id = id
        self._library = library
        self._params = params
        self._returns = returns
        self._bound_graph = bound_graph
        self._runnable = runnable

    @classmethod
    def from_dict(cls, dikt) -> 'GetStoredProcResponse':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The GetStoredProcResponse of this GetStoredProcResponse.  # noqa: E501
        :rtype: GetStoredProcResponse
        """
        return util.deserialize_model(dikt, cls)

    @property
    def name(self) -> str:
        """Gets the name of this GetStoredProcResponse.


        :return: The name of this GetStoredProcResponse.
        :rtype: str
        """
        return self._name

    @name.setter
    def name(self, name: str):
        """Sets the name of this GetStoredProcResponse.


        :param name: The name of this GetStoredProcResponse.
        :type name: str
        """
        if name is None:
            raise ValueError("Invalid value for `name`, must not be `None`")  # noqa: E501

        self._name = name

    @property
    def description(self) -> str:
        """Gets the description of this GetStoredProcResponse.


        :return: The description of this GetStoredProcResponse.
        :rtype: str
        """
        return self._description

    @description.setter
    def description(self, description: str):
        """Sets the description of this GetStoredProcResponse.


        :param description: The description of this GetStoredProcResponse.
        :type description: str
        """

        self._description = description

    @property
    def type(self) -> str:
        """Gets the type of this GetStoredProcResponse.


        :return: The type of this GetStoredProcResponse.
        :rtype: str
        """
        return self._type

    @type.setter
    def type(self, type: str):
        """Sets the type of this GetStoredProcResponse.


        :param type: The type of this GetStoredProcResponse.
        :type type: str
        """
        allowed_values = ["cpp", "cypher"]  # noqa: E501
        if type not in allowed_values:
            raise ValueError(
                "Invalid value for `type` ({0}), must be one of {1}"
                .format(type, allowed_values)
            )

        self._type = type

    @property
    def query(self) -> str:
        """Gets the query of this GetStoredProcResponse.


        :return: The query of this GetStoredProcResponse.
        :rtype: str
        """
        return self._query

    @query.setter
    def query(self, query: str):
        """Sets the query of this GetStoredProcResponse.


        :param query: The query of this GetStoredProcResponse.
        :type query: str
        """
        if query is None:
            raise ValueError("Invalid value for `query`, must not be `None`")  # noqa: E501

        self._query = query

    @property
    def id(self) -> str:
        """Gets the id of this GetStoredProcResponse.


        :return: The id of this GetStoredProcResponse.
        :rtype: str
        """
        return self._id

    @id.setter
    def id(self, id: str):
        """Sets the id of this GetStoredProcResponse.


        :param id: The id of this GetStoredProcResponse.
        :type id: str
        """
        if id is None:
            raise ValueError("Invalid value for `id`, must not be `None`")  # noqa: E501

        self._id = id

    @property
    def library(self) -> str:
        """Gets the library of this GetStoredProcResponse.


        :return: The library of this GetStoredProcResponse.
        :rtype: str
        """
        return self._library

    @library.setter
    def library(self, library: str):
        """Sets the library of this GetStoredProcResponse.


        :param library: The library of this GetStoredProcResponse.
        :type library: str
        """
        if library is None:
            raise ValueError("Invalid value for `library`, must not be `None`")  # noqa: E501

        self._library = library

    @property
    def params(self) -> List[Parameter]:
        """Gets the params of this GetStoredProcResponse.


        :return: The params of this GetStoredProcResponse.
        :rtype: List[Parameter]
        """
        return self._params

    @params.setter
    def params(self, params: List[Parameter]):
        """Sets the params of this GetStoredProcResponse.


        :param params: The params of this GetStoredProcResponse.
        :type params: List[Parameter]
        """
        if params is None:
            raise ValueError("Invalid value for `params`, must not be `None`")  # noqa: E501

        self._params = params

    @property
    def returns(self) -> List[Parameter]:
        """Gets the returns of this GetStoredProcResponse.


        :return: The returns of this GetStoredProcResponse.
        :rtype: List[Parameter]
        """
        return self._returns

    @returns.setter
    def returns(self, returns: List[Parameter]):
        """Sets the returns of this GetStoredProcResponse.


        :param returns: The returns of this GetStoredProcResponse.
        :type returns: List[Parameter]
        """
        if returns is None:
            raise ValueError("Invalid value for `returns`, must not be `None`")  # noqa: E501

        self._returns = returns

    @property
    def bound_graph(self) -> str:
        """Gets the bound_graph of this GetStoredProcResponse.


        :return: The bound_graph of this GetStoredProcResponse.
        :rtype: str
        """
        return self._bound_graph

    @bound_graph.setter
    def bound_graph(self, bound_graph: str):
        """Sets the bound_graph of this GetStoredProcResponse.


        :param bound_graph: The bound_graph of this GetStoredProcResponse.
        :type bound_graph: str
        """
        if bound_graph is None:
            raise ValueError("Invalid value for `bound_graph`, must not be `None`")  # noqa: E501

        self._bound_graph = bound_graph

    @property
    def runnable(self) -> bool:
        """Gets the runnable of this GetStoredProcResponse.


        :return: The runnable of this GetStoredProcResponse.
        :rtype: bool
        """
        return self._runnable

    @runnable.setter
    def runnable(self, runnable: bool):
        """Sets the runnable of this GetStoredProcResponse.


        :param runnable: The runnable of this GetStoredProcResponse.
        :type runnable: bool
        """
        if runnable is None:
            raise ValueError("Invalid value for `runnable`, must not be `None`")  # noqa: E501

        self._runnable = runnable
