import unittest

from flask import json

from flex.server.models.error import Error  # noqa: E501
from flex.server.models.get_pod_log_response import GetPodLogResponse  # noqa: E501
from flex.server.models.get_resource_usage_response import GetResourceUsageResponse  # noqa: E501
from flex.server.models.get_storage_usage_response import GetStorageUsageResponse  # noqa: E501
from flex.server.models.running_deployment_info import RunningDeploymentInfo  # noqa: E501
from flex.server.models.running_deployment_status import RunningDeploymentStatus  # noqa: E501
from flex.server.test import BaseTestCase


class TestDeploymentController(BaseTestCase):
    """DeploymentController integration test stubs"""

    def test_get_deployment_info(self):
        """Test case for get_deployment_info

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/deployment',
            method='GET',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_deployment_pod_log(self):
        """Test case for get_deployment_pod_log

        
        """
        query_string = [('pod_name', 'pod_name_example'),
                        ('component', 'component_example'),
                        ('from_cache', True)]
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/deployment/log',
            method='GET',
            headers=headers,
            query_string=query_string)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_deployment_resource_usage(self):
        """Test case for get_deployment_resource_usage

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/deployment/resource/usage',
            method='GET',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_deployment_status(self):
        """Test case for get_deployment_status

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/deployment/status',
            method='GET',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_storage_usage(self):
        """Test case for get_storage_usage

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/deployment/storage/usage',
            method='GET',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))


if __name__ == '__main__':
    unittest.main()
