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

import sys
import grpc
from archieve import OutArchive

sys.path.insert(1, "../proto")

import types_pb2 as pb2
import types_pb2_grpc as pb2_grpc

channel = grpc.insecure_channel('localhost:50051')
stub = pb2_grpc.QueryGraphServiceStub(channel)
response = stub.getData(pb2.Request(op=pb2.NODE_NUM, args=""))
arc = OutArchive(response.result)
print(arc.get_size())