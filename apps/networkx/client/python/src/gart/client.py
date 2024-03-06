import grpc
import msgpack
import json

from gart.archieve import OutArchive
from gart.digraph import DiGraph

import gart.proto.types_pb2 as pb2
import gart.proto.types_pb2_grpc as pb2_grpc


class Client:
    def __init__(self, service_port):
        # Increase the maximum message size the client can receive
        channel_options = [
            (
                "grpc.max_receive_message_length",
                1024 * 1024 * 100 + 1024 * 2024,
            )  # 101MB
        ]
        channel = grpc.insecure_channel(service_port, options=channel_options)
        self.stub = pb2_grpc.QueryGraphServiceStub(channel)

    def get_latest_version(self):
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.LATEST_GRAPH_VERSION, args="")
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return arc.get_size()

    def get_graph(self, version):
        return DiGraph(version, self.stub)
