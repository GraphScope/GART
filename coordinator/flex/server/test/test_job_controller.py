import unittest

from flask import json

from flex.server.models.create_dataloading_job_response import CreateDataloadingJobResponse  # noqa: E501
from flex.server.models.dataloading_job_config import DataloadingJobConfig  # noqa: E501
from flex.server.models.dataloading_mr_job_config import DataloadingMRJobConfig  # noqa: E501
from flex.server.models.error import Error  # noqa: E501
from flex.server.models.job_status import JobStatus  # noqa: E501
from flex.server.test import BaseTestCase


class TestJobController(BaseTestCase):
    """JobController integration test stubs"""

    def test_delete_job_by_id(self):
        """Test case for delete_job_by_id

        
        """
        query_string = [('delete_scheduler', True)]
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/job/{job_id}'.format(job_id='job_id_example'),
            method='DELETE',
            headers=headers,
            query_string=query_string)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_dataloading_job_config(self):
        """Test case for get_dataloading_job_config

        
        """
        dataloading_job_config = {"schedule":"schedule","loading_config":{"format":{"metadata":{"key":""},"type":"type"},"import_option":"overwrite"},"vertices":[{"type_name":"type_name"},{"type_name":"type_name"}],"repeat":"once","edges":[{"type_name":"type_name","source_vertex":"source_vertex","destination_vertex":"destination_vertex"},{"type_name":"type_name","source_vertex":"source_vertex","destination_vertex":"destination_vertex"}]}
        headers = { 
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        response = self.client.open(
            '/api/v1/graph/{graph_id}/dataloading/config'.format(graph_id='graph_id_example'),
            method='POST',
            headers=headers,
            data=json.dumps(dataloading_job_config),
            content_type='application/json')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_job_by_id(self):
        """Test case for get_job_by_id

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/job/{job_id}'.format(job_id='job_id_example'),
            method='GET',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_list_jobs(self):
        """Test case for list_jobs

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/job',
            method='GET',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_pause_job(self):
        """Test case for pause_job

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/job/{job_id}/pause'.format(job_id='job_id_example'),
            method='POST',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_resume_job(self):
        """Test case for resume_job

        
        """
        headers = { 
            'Accept': 'application/json',
        }
        response = self.client.open(
            '/api/v1/job/{job_id}/resume'.format(job_id='job_id_example'),
            method='POST',
            headers=headers)
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_submit_dataloading_job(self):
        """Test case for submit_dataloading_job

        
        """
        dataloading_job_config = {"schedule":"schedule","loading_config":{"format":{"metadata":{"key":""},"type":"type"},"import_option":"overwrite"},"vertices":[{"type_name":"type_name"},{"type_name":"type_name"}],"repeat":"once","edges":[{"type_name":"type_name","source_vertex":"source_vertex","destination_vertex":"destination_vertex"},{"type_name":"type_name","source_vertex":"source_vertex","destination_vertex":"destination_vertex"}]}
        headers = { 
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        response = self.client.open(
            '/api/v1/graph/{graph_id}/dataloading'.format(graph_id='graph_id_example'),
            method='POST',
            headers=headers,
            data=json.dumps(dataloading_job_config),
            content_type='application/json')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))


if __name__ == '__main__':
    unittest.main()
