import grpc
import msgpack
import json

from gart.archieve import OutArchive
from gart.digraph import DiGraph
from gart.local_graph import LocalDiGraph

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
        self._known_version = -1

        self.local_deployment = False

        if (
            "localhost" in service_port
            or "127.0.0.1" in service_port
            or "::1" in service_port
        ):
            self.local_deployment = True

    def get_latest_version(self):
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.LATEST_GRAPH_VERSION, args="")
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        version = arc.get_size()
        self._known_version = version
        return version

    def get_graph(self, version):
        if version > self._known_version:
            latest_version = self.get_latest_version()
            self._known_version = latest_version
            if version > latest_version:
                raise ValueError(f"Version {version} does not exist")
        if self.local_deployment:
            response_iterator = self.stub.getData(
                pb2.Request(op=pb2.CONNECT_INFO, args="")
            )
            total_response = bytes()
            for response in response_iterator:
                total_response += response.result
            arc = OutArchive(total_response)
            result = msgpack.unpackb(arc.get_bytes(), use_list=False)
            etcd_endpoint = result["etcd_endpoint"]
            meta_prefix = result["meta_prefix"]
            return LocalDiGraph(etcd_endpoint, meta_prefix, version)
        return DiGraph(version, self.stub)

    def run_sssp(self, graph, source_node, weight=None):
        if weight is None:
            n = (source_node, "")
        else:
            n = (source_node, weight)
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.RUN_GAE_SSSP, args=arg, version=graph._version)
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        try:
            arc = OutArchive(total_response)
            tmp = msgpack.unpackb(arc.get_bytes(), use_list=False)
            result = {(d["label_id"], d["oid"]): d["distance"] for d in tmp}
            return result
        except TypeError:
            raise ValueError("Failed to execute GAE SSSP")
