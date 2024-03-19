// python_bindings.cc
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cstdint>
#include <future>
#include <string_view>

#include "fragment_builder.h"
#include "grape/utils/vertex_array.h"
#include "interfaces/fragment/gart_fragment.h"
#include "interfaces/fragment/iterator.h"
#include "interfaces/fragment/types.h"
#include "vineyard/common/util/json.h"

namespace py = pybind11;

PYBIND11_MODULE(local_gart, m) {
  m.doc() = "Local GART reader module created using pybind11";

  py::class_<FragmentBuilder>(m, "FragmentBuilder")
      .def(py::init<std::string, std::string, int>())
      .def("get_graph", &FragmentBuilder::get_graph);

  py::class_<gart::GartFragment<uint64_t, uint64_t>,
             std::shared_ptr<gart::GartFragment<uint64_t, uint64_t>>>(
      m, "GartFragment")
      .def(py::init<>())
      .def("build_graph", &gart::GartFragment<uint64_t, uint64_t>::Init)
      .def("vertex_label",
           &gart::GartFragment<uint64_t, uint64_t>::vertex_label)
      .def("vertex_label_num",
           &gart::GartFragment<uint64_t, uint64_t>::vertex_label_num)
      .def("edge_label_num",
           &gart::GartFragment<uint64_t, uint64_t>::edge_label_num)
      .def("vertex_property_num",
           &gart::GartFragment<uint64_t, uint64_t>::vertex_property_num)
      .def("edge_property_num",
           &gart::GartFragment<uint64_t, uint64_t>::edge_property_num)
      .def("inner_vertices",
           &gart::GartFragment<uint64_t, uint64_t>::InnerVertices)
      .def("get_id", &gart::GartFragment<uint64_t, uint64_t>::GetId)
      .def("get_vertices_num",
           py::overload_cast<>(
               &gart::GartFragment<uint64_t, uint64_t>::GetVerticesNum))
      .def("get_vertices_num",
           py::overload_cast<gart::label_id_t>(
               &gart::GartFragment<uint64_t, uint64_t>::GetVerticesNum))
      .def("get_edge_num",
           py::overload_cast<>(
               &gart::GartFragment<uint64_t, uint64_t>::GetEdgeNum))
      .def("get_vertex_label_name",
           &gart::GartFragment<uint64_t, uint64_t>::GetVertexLabelName)
      .def("get_vertex_label_id",
           &gart::GartFragment<uint64_t, uint64_t>::GetVertexLabelId)
      .def("get_edge_label_name",
           &gart::GartFragment<uint64_t, uint64_t>::GetEdgeLabelName)
      .def("get_edge_label_id",
           &gart::GartFragment<uint64_t, uint64_t>::GetEdgeLabelId)
      .def("get_vertex_prop_name",
           &gart::GartFragment<uint64_t, uint64_t>::GetVertexPropName)
      .def("get_vertex_prop_id",
           &gart::GartFragment<uint64_t, uint64_t>::GetVertexPropId)
      .def("get_edge_prop_name",
           &gart::GartFragment<uint64_t, uint64_t>::GetEdgePropName)
      .def("get_edge_prop_id",
           &gart::GartFragment<uint64_t, uint64_t>::GetEdgePropId)
      .def("get_vertex_prop_data_type",
           &gart::GartFragment<uint64_t, uint64_t>::GetVertexPropDataType)
      .def("get_edge_prop_data_type",
           &gart::GartFragment<uint64_t, uint64_t>::GetEdgePropDataType)
      .def("get_int_data",
           &gart::GartFragment<uint64_t, uint64_t>::GetData<int>)
      .def("get_float_data",
           &gart::GartFragment<uint64_t, uint64_t>::GetData<float>)
      .def("get_double_data",
           &gart::GartFragment<uint64_t, uint64_t>::GetData<double>)
      .def("get_string_data",
           &gart::GartFragment<uint64_t, uint64_t>::GetData<std::string_view>)
      .def("get_ulong_data",
           &gart::GartFragment<uint64_t, uint64_t>::GetData<uint64_t>)
      .def("get_long_data",
           &gart::GartFragment<uint64_t, uint64_t>::GetData<int64_t>)
      .def("get_inner_vertex",
           &gart::GartFragment<uint64_t, uint64_t>::GetInnerVertex)
      .def("gid2oid", &gart::GartFragment<uint64_t, uint64_t>::Gid2Oid)
      .def("oid2gid",
           [](gart::GartFragment<uint64_t, uint64_t>& self,
              gart::label_id_t label, const uint64_t& oid) {
             gart::vertex_t v;
             bool success = self.Oid2Gid(label, oid, v);
             return py::make_tuple(success, v);
           })
      .def("get_incoming_adjList",
           &gart::GartFragment<uint64_t, uint64_t>::GetIncomingAdjList)
      .def("get_outgoing_adjList",
           &gart::GartFragment<uint64_t, uint64_t>::GetOutgoingAdjList);

  py::class_<gart::VertexIterator>(m, "VertexIterator")
      .def(py::init<>())
      .def("valid", &gart::VertexIterator::valid)
      .def("next", &gart::VertexIterator::next)
      .def("vertex", &gart::VertexIterator::vertex);

  py::class_<gart::EdgeIterator>(m, "EdgeIterator")
      .def(py::init<seggraph::VegitoSegmentHeader*,
                    seggraph::VegitoEdgeBlockHeader*,
                    seggraph::EpochBlockHeader*, char*, size_t, size_t, size_t,
                    int*, char*, size_t>())
      .def("valid", &gart::EdgeIterator::valid)
      .def("next", &gart::EdgeIterator::next)
      .def("neighbor", &gart::EdgeIterator::neighbor)
      .def("get_int_data", &gart::EdgeIterator::get_data<int>)
      .def("get_float_data", &gart::EdgeIterator::get_data<float>)
      .def("get_string_data", &gart::EdgeIterator::get_data<std::string_view>)
      .def("get_ulong_data", &gart::EdgeIterator::get_data<uint64_t>)
      .def("get_long_data", &gart::EdgeIterator::get_data<int64_t>)
      .def("get_double_data", &gart::EdgeIterator::get_data<double>);

  py::class_<grape::Vertex<uint64_t>>(m, "Vertex")
      .def(py::init<>())
      .def(py::init<uint64_t>());
}
