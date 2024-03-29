#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2023 Alibaba Group Holding Limited. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import grpc
from gart.archieve import OutArchive
import msgpack
import json

from gart.proto import types_pb2 as pb2
from gart.proto import types_pb2_grpc as pb2_grpc

channel = grpc.insecure_channel("localhost:50051")
stub = pb2_grpc.QueryGraphServiceStub(channel)
# response = stub.getData(pb2.Request(op=pb2.NODE_NUM, args=""))
# arc = OutArchive(response.result)
# print(arc.get_size())

response_iterator = stub.getData(pb2.Request(op=pb2.NODES, args=""))
total_result = bytes()
idx = 0
for response in response_iterator:
    print("idx : " + str(idx))
    idx += 1
    total_result += response.result
    # print(type(response.result))
    # arc = OutArchive(response.result)
    # result = msgpack.unpackb(arc.get_bytes(), use_list=False)
    # print(result)
    # break
arc = OutArchive(total_result)
result = msgpack.unpackb(arc.get_bytes(), use_list=False)
# print(result)

exit()
v_label = 0
v_oid = 0

v = (v_label, v_oid)
arg = json.dumps(v).encode("utf-8", errors="ignore")
response = stub.getData(pb2.Request(op=pb2.SUCCS_BY_NODE, args=arg))
arc = OutArchive(response.result)
result = msgpack.unpackb(arc.get_bytes(), use_list=False)
# print(result)


response = stub.getData(pb2.Request(op=pb2.NODE_DATA, args=arg))
print(response.result)
arc = OutArchive(response.result)
result = msgpack.unpackb(arc.get_bytes(), use_list=False)
print("node data:")
print(result)

response = stub.getData(pb2.Request(op=pb2.NODES, args=""))
# Serialize the response to bytes
response_bytes = response.SerializeToString()

# Get the size of the serialized response
response_size = len(response_bytes)

print(f"The size of the gRPC response is: {response_size} bytes")
arc = OutArchive(response.result)
result = msgpack.unpackb(arc.get_bytes(), use_list=False)
# print(result)

src_label = 4
src_oid = 933
src = (src_label, src_oid)
dst_label = 0
dst_oid = 1226
dst = (dst_label, dst_oid)
edge = (src, dst)
arg = json.dumps(src).encode("utf-8", errors="ignore")
response = stub.getData(pb2.Request(op=pb2.SUCCS_BY_NODE, args=arg))
arc = OutArchive(response.result)
result = msgpack.loads(arc.get_bytes(), use_list=False)
print("neighbors:")
print(type(result))

response = stub.getData(pb2.Request(op=pb2.SUCC_ATTR_BY_NODE, args=arg))
arc = OutArchive(response.result)
result = msgpack.loads(arc.get_bytes(), use_list=False)
print("edge prop:")
print(type(result[-1]))

arg = json.dumps(edge).encode("utf-8", errors="ignore")
response = stub.getData(pb2.Request(op=pb2.EDGE_DATA, args=arg))
arc = OutArchive(response.result)
result = msgpack.loads(arc.get_bytes(), use_list=False)
print("edge prop:")
print(result)
