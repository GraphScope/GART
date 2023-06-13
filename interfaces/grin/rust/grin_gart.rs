
#[doc = "< incoming"]
pub const GRIN_DIRECTION_IN: GrinDirection = 0;
#[doc = "< outgoing"]
pub const GRIN_DIRECTION_OUT: GrinDirection = 1;
#[doc = "< incoming & outgoing"]
pub const GRIN_DIRECTION_BOTH: GrinDirection = 2;
#[doc = " Enumerates the directions of edges with respect to a certain vertex"]
pub type GrinDirection = u32;
#[doc = "< other unknown types"]
pub const GRIN_DATATYPE_UNDEFINED: GrinDatatype = 0;
#[doc = "< int"]
pub const GRIN_DATATYPE_INT32: GrinDatatype = 1;
#[doc = "< unsigned int"]
pub const GRIN_DATATYPE_UINT32: GrinDatatype = 2;
#[doc = "< long int"]
pub const GRIN_DATATYPE_INT64: GrinDatatype = 3;
#[doc = "< unsigned long int"]
pub const GRIN_DATATYPE_UINT64: GrinDatatype = 4;
#[doc = "< float"]
pub const GRIN_DATATYPE_FLOAT: GrinDatatype = 5;
#[doc = "< double"]
pub const GRIN_DATATYPE_DOUBLE: GrinDatatype = 6;
#[doc = "< string"]
pub const GRIN_DATATYPE_STRING: GrinDatatype = 7;
#[doc = "< date"]
pub const GRIN_DATATYPE_DATE32: GrinDatatype = 8;
#[doc = "< Time32"]
pub const GRIN_DATATYPE_TIME32: GrinDatatype = 9;
#[doc = "< Timestamp"]
pub const GRIN_DATATYPE_TIMESTAMP64: GrinDatatype = 10;
#[doc = " Enumerates the datatype supported in the storage"]
pub type GrinDatatype = u32;
#[doc = "< success"]
pub const GRIN_ERROR_CODE_NO_ERROR: GrinErrorCode = 0;
#[doc = "< unknown error"]
pub const GRIN_ERROR_CODE_UNKNOWN_ERROR: GrinErrorCode = 1;
#[doc = "< invalid value"]
pub const GRIN_ERROR_CODE_INVALID_VALUE: GrinErrorCode = 2;
#[doc = "< unknown datatype"]
pub const GRIN_ERROR_CODE_UNKNOWN_DATATYPE: GrinErrorCode = 3;
#[doc = " Enumerates the error codes of grin"]
pub type GrinErrorCode = u32;
#[doc = "@}"]
pub type GrinGraph = *mut ::std::os::raw::c_void;
pub type GrinVertex = u64;
pub type GrinEdge = *mut ::std::os::raw::c_void;
pub type GrinVertexList = *mut ::std::os::raw::c_void;
pub type GrinVertexListIterator = *mut ::std::os::raw::c_void;
pub type GrinAdjacentList = *mut ::std::os::raw::c_void;
pub type GrinAdjacentListIterator = *mut ::std::os::raw::c_void;
pub type GrinPartitionedGraph = *mut ::std::os::raw::c_void;
pub type GrinPartition = u32;
pub type GrinPartitionList = *mut ::std::os::raw::c_void;
pub type GrinPartitionId = u32;
pub type GrinVertexRef = i64;
pub type GrinVertexType = u32;
pub type GrinVertexTypeList = *mut ::std::os::raw::c_void;
pub type GrinVertexProperty = u64;
pub type GrinVertexPropertyList = *mut ::std::os::raw::c_void;
pub type GrinVertexTypeId = u32;
pub type GrinVertexPropertyId = u32;
pub type GrinEdgeType = u32;
pub type GrinEdgeTypeList = *mut ::std::os::raw::c_void;
pub type GrinVevType = *mut ::std::os::raw::c_void;
pub type GrinVevTypeList = *mut ::std::os::raw::c_void;
pub type GrinEdgeProperty = u64;
pub type GrinEdgePropertyList = *mut ::std::os::raw::c_void;
pub type GrinEdgeTypeId = u32;
pub type GrinEdgePropertyId = u32;
pub type GrinRow = *mut ::std::os::raw::c_void;
extern "C" {
    #[cfg(feature = "grin_enable_adjacent_list")]
    #[allow(unused)]
    pub fn grin_destroy_adjacent_list(arg1: GrinGraph, arg2: GrinAdjacentList);

    #[cfg(feature = "grin_enable_adjacent_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_adjacent_list_begin(
        arg1: GrinGraph,
        arg2: GrinAdjacentList,
    ) -> GrinAdjacentListIterator;

    #[cfg(feature = "grin_enable_adjacent_list_iterator")]
    #[allow(unused)]
    pub fn grin_destroy_adjacent_list_iter(arg1: GrinGraph, arg2: GrinAdjacentListIterator);

    #[cfg(feature = "grin_enable_adjacent_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_next_adjacent_list_iter(arg1: GrinGraph, arg2: GrinAdjacentListIterator);

    #[cfg(feature = "grin_enable_adjacent_list_iterator")]
    #[allow(unused)]
    pub fn grin_is_adjacent_list_end(arg1: GrinGraph, arg2: GrinAdjacentListIterator) -> bool;

    #[cfg(feature = "grin_enable_adjacent_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_neighbor_from_adjacent_list_iter(
        arg1: GrinGraph,
        arg2: GrinAdjacentListIterator,
    ) -> GrinVertex;

    #[cfg(feature = "grin_enable_adjacent_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_edge_from_adjacent_list_iter(
        arg1: GrinGraph,
        arg2: GrinAdjacentListIterator,
    ) -> GrinEdge;

    #[allow(unused)]
    pub fn grin_get_graph_from_storage(
        arg1: i32,
        arg2: *mut *mut ::std::os::raw::c_char,
    ) -> GrinGraph;

    #[allow(unused)]
    pub fn grin_destroy_graph(arg1: GrinGraph);

    #[cfg(all(feature = "grin_assume_has_directed_graph", feature = "grin_assume_has_undirected_graph"))]
    #[allow(unused)]
    pub fn grin_is_directed(arg1: GrinGraph) -> bool;

    #[cfg(feature = "grin_assume_has_multi_edge_graph")]
    #[allow(unused)]
    pub fn grin_is_multigraph(arg1: GrinGraph) -> bool;

    #[allow(unused)]
    pub fn grin_destroy_vertex(arg1: GrinGraph, arg2: GrinVertex);

    #[allow(unused)]
    pub fn grin_equal_vertex(arg1: GrinGraph, arg2: GrinVertex, arg3: GrinVertex) -> bool;

    #[allow(unused)]
    pub fn grin_destroy_edge(arg1: GrinGraph, arg2: GrinEdge);

    #[allow(unused)]
    pub fn grin_get_src_vertex_from_edge(arg1: GrinGraph, arg2: GrinEdge) -> GrinVertex;

    #[allow(unused)]
    pub fn grin_get_dst_vertex_from_edge(arg1: GrinGraph, arg2: GrinEdge) -> GrinVertex;

    #[cfg(feature = "grin_enable_vertex_list")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_list(arg1: GrinGraph, arg2: GrinVertexList);

    #[cfg(feature = "grin_enable_vertex_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_vertex_list_begin(
        arg1: GrinGraph,
        arg2: GrinVertexList,
    ) -> GrinVertexListIterator;

    #[cfg(feature = "grin_enable_vertex_list_iterator")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_list_iter(arg1: GrinGraph, arg2: GrinVertexListIterator);

    #[cfg(feature = "grin_enable_vertex_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_next_vertex_list_iter(arg1: GrinGraph, arg2: GrinVertexListIterator);

    #[cfg(feature = "grin_enable_vertex_list_iterator")]
    #[allow(unused)]
    pub fn grin_is_vertex_list_end(arg1: GrinGraph, arg2: GrinVertexListIterator) -> bool;

    #[cfg(feature = "grin_enable_vertex_list_iterator")]
    #[allow(unused)]
    pub fn grin_get_vertex_from_iter(
        arg1: GrinGraph,
        arg2: GrinVertexListIterator,
    ) -> GrinVertex;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_partitioned_graph_from_storage(
        arg1: i32,
        arg2: *mut *mut ::std::os::raw::c_char,
    ) -> GrinPartitionedGraph;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_destroy_partitioned_graph(arg1: GrinPartitionedGraph);

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_total_partitions_number(arg1: GrinPartitionedGraph) -> usize;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_local_partition_list(arg1: GrinPartitionedGraph) -> GrinPartitionList;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_destroy_partition_list(arg1: GrinPartitionedGraph, arg2: GrinPartitionList);

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_create_partition_list(arg1: GrinPartitionedGraph) -> GrinPartitionList;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_insert_partition_to_list(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartitionList,
        arg3: GrinPartition,
    ) -> bool;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_partition_list_size(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartitionList,
    ) -> usize;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_partition_from_list(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartitionList,
        arg3: usize,
    ) -> GrinPartition;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_equal_partition(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartition,
        arg3: GrinPartition,
    ) -> bool;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_destroy_partition(arg1: GrinPartitionedGraph, arg2: GrinPartition);

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_partition_info(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartition,
    ) -> *const ::std::os::raw::c_void;

    #[cfg(feature = "grin_enable_graph_partition")]
    #[allow(unused)]
    pub fn grin_get_local_graph_by_partition(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartition,
    ) -> GrinGraph;

    #[cfg(feature = "grin_trait_natural_id_for_partition")]
    #[allow(unused)]
    pub fn grin_get_partition_by_id(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartitionId,
    ) -> GrinPartition;

    #[cfg(feature = "grin_trait_natural_id_for_partition")]
    #[allow(unused)]
    pub fn grin_get_partition_id(
        arg1: GrinPartitionedGraph,
        arg2: GrinPartition,
    ) -> GrinPartitionId;

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_get_vertex_ref_by_vertex(arg1: GrinGraph, arg2: GrinVertex) -> GrinVertexRef;

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_ref(arg1: GrinGraph, arg2: GrinVertexRef);

    #[doc = " @brief get the local vertex from the vertex ref\n if the vertex ref is not regconized, a null vertex is returned\n @param GrinGraph the graph\n @param GrinVertexRef the vertex ref"]
    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_get_vertex_from_vertex_ref(arg1: GrinGraph, arg2: GrinVertexRef) -> GrinVertex;

    #[doc = " @brief get the master partition of a vertex ref.\n Some storage can still provide the master partition of the vertex ref,\n even if the vertex ref can NOT be recognized locally.\n @param GrinGraph the graph\n @param GrinVertexRef the vertex ref"]
    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_get_master_partition_from_vertex_ref(
        arg1: GrinGraph,
        arg2: GrinVertexRef,
    ) -> GrinPartition;

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_serialize_vertex_ref(
        arg1: GrinGraph,
        arg2: GrinVertexRef,
    ) -> *const ::std::os::raw::c_char;

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_destroy_serialized_vertex_ref(
        arg1: GrinGraph,
        arg2: *const ::std::os::raw::c_char,
    );

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_deserialize_to_vertex_ref(
        arg1: GrinGraph,
        arg2: *const ::std::os::raw::c_char,
    ) -> GrinVertexRef;

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_is_master_vertex(arg1: GrinGraph, arg2: GrinVertex) -> bool;

    #[cfg(feature = "grin_enable_vertex_ref")]
    #[allow(unused)]
    pub fn grin_is_mirror_vertex(arg1: GrinGraph, arg2: GrinVertex) -> bool;

    #[allow(unused)]
    pub fn grin_serialize_vertex_ref_as_int64(
        arg1: GrinGraph,
        arg2: GrinVertexRef,
    ) -> i64;

    #[cfg(feature = "grin_trait_fast_vertex_ref")]
    #[allow(unused)]
    pub fn grin_deserialize_int64_to_vertex_ref(
        arg1: GrinGraph,
        arg2: i64,
    ) -> GrinVertexRef;

    #[cfg(all(feature = "grin_trait_select_master_for_vertex_list", feature = "grin_with_vertex_property"))]
    #[allow(unused)]
    pub fn grin_get_vertex_list_by_type_select_master(
        arg1: GrinGraph,
        arg2: GrinVertexType,
    ) -> GrinVertexList;

    #[cfg(all(feature = "grin_trait_select_master_for_vertex_list", feature = "grin_with_vertex_property"))]
    #[allow(unused)]
    pub fn grin_get_vertex_list_by_type_select_mirror(
        arg1: GrinGraph,
        arg2: GrinVertexType,
    ) -> GrinVertexList;

    #[allow(unused)]
    pub fn grin_destroy_string_value(arg1: GrinGraph, arg2: *const ::std::os::raw::c_char);

    #[doc = " @brief get the vertex property name\n @param GrinGraph the graph\n @param GrinVertexProperty the vertex property"]
    #[cfg(feature = "grin_with_vertex_property_name")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_name(
        arg1: GrinGraph,
        arg2: GrinVertexType,
        arg3: GrinVertexProperty,
    ) -> *const ::std::os::raw::c_char;

    #[doc = " @brief get the vertex property with a given name under a specific vertex type\n @param GrinGraph the graph\n @param GrinVertexType the specific vertex type\n @param name the name"]
    #[cfg(feature = "grin_with_vertex_property_name")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_by_name(
        arg1: GrinGraph,
        arg2: GrinVertexType,
        name: *const ::std::os::raw::c_char,
    ) -> GrinVertexProperty;

    #[doc = " @brief get all the vertex properties with a given name\n @param GrinGraph the graph\n @param name the name"]
    #[cfg(feature = "grin_with_vertex_property_name")]
    #[allow(unused)]
    pub fn grin_get_vertex_properties_by_name(
        arg1: GrinGraph,
        name: *const ::std::os::raw::c_char,
    ) -> GrinVertexPropertyList;

    #[doc = " @brief get the edge property name\n @param GrinGraph the graph\n @param GrinEdgeProperty the edge property"]
    #[cfg(feature = "grin_with_edge_property_name")]
    #[allow(unused)]
    pub fn grin_get_edge_property_name(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
        arg3: GrinEdgeProperty,
    ) -> *const ::std::os::raw::c_char;

    #[doc = " @brief get the edge property with a given name under a specific edge type\n @param GrinGraph the graph\n @param GrinEdgeType the specific edge type\n @param name the name"]
    #[cfg(feature = "grin_with_edge_property_name")]
    #[allow(unused)]
    pub fn grin_get_edge_property_by_name(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
        name: *const ::std::os::raw::c_char,
    ) -> GrinEdgeProperty;

    #[doc = " @brief get all the edge properties with a given name\n @param GrinGraph the graph\n @param name the name"]
    #[cfg(feature = "grin_with_edge_property_name")]
    #[allow(unused)]
    pub fn grin_get_edge_properties_by_name(
        arg1: GrinGraph,
        name: *const ::std::os::raw::c_char,
    ) -> GrinEdgePropertyList;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_equal_vertex_property(
        arg1: GrinGraph,
        arg2: GrinVertexProperty,
        arg3: GrinVertexProperty,
    ) -> bool;

    #[doc = " @brief destroy vertex property\n @param GrinVertexProperty vertex property"]
    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_property(arg1: GrinGraph, arg2: GrinVertexProperty);

    #[doc = " @brief get property data type\n @param GrinVertexProperty vertex property"]
    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_datatype(
        arg1: GrinGraph,
        arg2: GrinVertexProperty,
    ) -> GrinDatatype;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_int32(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_uint32(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> u32;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_int64(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> i64;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_uint64(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> u64;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_float(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> f32;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_double(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> f64;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_string(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> *const ::std::os::raw::c_char;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_date32(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_time32(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_vertex_property_value_of_timestamp64(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> i64;

    #[doc = " @brief get the vertex type that the property is bound to\n @param GrinVertexProperty vertex property"]
    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_from_property(
        arg1: GrinGraph,
        arg2: GrinVertexProperty,
    ) -> GrinVertexType;

    #[cfg(all(feature = "grin_with_vertex_property", feature = "grin_trait_const_value_ptr"))]
    #[allow(unused)]
    pub fn grin_get_vertex_property_value(
        arg1: GrinGraph,
        arg2: GrinVertex,
        arg3: GrinVertexProperty,
    ) -> *const ::std::os::raw::c_void;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_equal_edge_property(
        arg1: GrinGraph,
        arg2: GrinEdgeProperty,
        arg3: GrinEdgeProperty,
    ) -> bool;

    #[doc = " @brief destroy edge property\n @param GrinEdgeProperty edge property"]
    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_destroy_edge_property(arg1: GrinGraph, arg2: GrinEdgeProperty);

    #[doc = " @brief get property data type\n @param GrinEdgeProperty edge property"]
    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_datatype(
        arg1: GrinGraph,
        arg2: GrinEdgeProperty,
    ) -> GrinDatatype;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_int32(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_uint32(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> u32;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_int64(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> i64;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_uint64(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> u64;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_float(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> f32;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_double(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> f64;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_string(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> *const ::std::os::raw::c_char;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_date32(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_time32(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_edge_property_value_of_timestamp64(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> i64;

    #[doc = " @brief get the edge type that the property is bound to\n @param GrinEdgeProperty edge property"]
    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_type_from_property(
        arg1: GrinGraph,
        arg2: GrinEdgeProperty,
    ) -> GrinEdgeType;

    #[cfg(all(feature = "grin_with_edge_property", feature = "grin_trait_const_value_ptr"))]
    #[allow(unused)]
    pub fn grin_get_edge_property_value(
        arg1: GrinGraph,
        arg2: GrinEdge,
        arg3: GrinEdgeProperty,
    ) -> *const ::std::os::raw::c_void;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_list_by_type(
        arg1: GrinGraph,
        arg2: GrinVertexType,
    ) -> GrinVertexPropertyList;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_list_size(
        arg1: GrinGraph,
        arg2: GrinVertexPropertyList,
    ) -> usize;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_from_list(
        arg1: GrinGraph,
        arg2: GrinVertexPropertyList,
        arg3: usize,
    ) -> GrinVertexProperty;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_create_vertex_property_list(arg1: GrinGraph) -> GrinVertexPropertyList;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_property_list(arg1: GrinGraph, arg2: GrinVertexPropertyList);

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_insert_vertex_property_to_list(
        arg1: GrinGraph,
        arg2: GrinVertexPropertyList,
        arg3: GrinVertexProperty,
    ) -> bool;

    #[cfg(feature = "grin_trait_natural_id_for_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_by_id(
        arg1: GrinGraph,
        arg2: GrinVertexType,
        arg3: GrinVertexPropertyId,
    ) -> GrinVertexProperty;

    #[doc = " We must specify the vertex type here, because the vertex property id is unique only under a specific vertex type"]
    #[cfg(feature = "grin_trait_natural_id_for_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_property_id(
        arg1: GrinGraph,
        arg2: GrinVertexType,
        arg3: GrinVertexProperty,
    ) -> GrinVertexPropertyId;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_list_by_type(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
    ) -> GrinEdgePropertyList;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_list_size(
        arg1: GrinGraph,
        arg2: GrinEdgePropertyList,
    ) -> usize;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_from_list(
        arg1: GrinGraph,
        arg2: GrinEdgePropertyList,
        arg3: usize,
    ) -> GrinEdgeProperty;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_create_edge_property_list(arg1: GrinGraph) -> GrinEdgePropertyList;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_destroy_edge_property_list(arg1: GrinGraph, arg2: GrinEdgePropertyList);

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_insert_edge_property_to_list(
        arg1: GrinGraph,
        arg2: GrinEdgePropertyList,
        arg3: GrinEdgeProperty,
    ) -> bool;

    #[cfg(feature = "grin_trait_natural_id_for_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_by_id(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
        arg3: GrinEdgePropertyId,
    ) -> GrinEdgeProperty;

    #[doc = " We must specify the edge type here, because the edge property id is unique only under a specific edge type"]
    #[cfg(feature = "grin_trait_natural_id_for_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_property_id(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
        arg3: GrinEdgeProperty,
    ) -> GrinEdgePropertyId;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_destroy_row(arg1: GrinGraph, arg2: GrinRow);

    #[allow(unused)]
    pub fn grin_get_int32_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_uint32_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> u32;

    #[allow(unused)]
    pub fn grin_get_int64_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> i64;

    #[allow(unused)]
    pub fn grin_get_uint64_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> u64;

    #[allow(unused)]
    pub fn grin_get_float_from_row(arg1: GrinGraph, arg2: GrinRow, arg3: usize) -> f32;

    #[allow(unused)]
    pub fn grin_get_double_from_row(arg1: GrinGraph, arg2: GrinRow, arg3: usize) -> f64;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_get_string_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> *const ::std::os::raw::c_char;

    #[allow(unused)]
    pub fn grin_get_date32_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_time32_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> i32;

    #[allow(unused)]
    pub fn grin_get_timestamp64_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: usize,
    ) -> i64;

    #[doc = " @brief create a row, usually to get vertex/edge by primary keys"]
    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_create_row(arg1: GrinGraph) -> GrinRow;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_int32_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: i32,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_uint32_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: u32,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_int64_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: i64,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_uint64_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: u64,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_float_to_row(arg1: GrinGraph, arg2: GrinRow, arg3: f32) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_double_to_row(arg1: GrinGraph, arg2: GrinRow, arg3: f64) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_string_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: *const ::std::os::raw::c_char,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_date32_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: i32,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_time32_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: i32,
    ) -> bool;

    #[cfg(feature = "grin_enable_row")]
    #[allow(unused)]
    pub fn grin_insert_timestamp64_to_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: i64,
    ) -> bool;

    #[doc = " @brief the value of a property from row by its position in row"]
    #[cfg(all(feature = "grin_enable_row", feature = "grin_trait_const_value_ptr"))]
    #[allow(unused)]
    pub fn grin_get_value_from_row(
        arg1: GrinGraph,
        arg2: GrinRow,
        arg3: GrinDatatype,
        arg4: usize,
    ) -> *const ::std::os::raw::c_void;

    #[doc = " @brief get vertex row directly from the graph, this API only works for row store system\n @param GrinGraph the graph\n @param GrinVertex the vertex which is the row index"]
    #[cfg(all(feature = "grin_with_vertex_property", feature = "grin_enable_row"))]
    #[allow(unused)]
    pub fn grin_get_vertex_row(arg1: GrinGraph, arg2: GrinVertex) -> GrinRow;

    #[doc = " @brief get edge row directly from the graph, this API only works for row store system\n @param GrinGraph the graph\n @param GrinEdge the edge which is the row index"]
    #[cfg(all(feature = "grin_with_edge_property", feature = "grin_enable_row"))]
    #[allow(unused)]
    pub fn grin_get_edge_row(arg1: GrinGraph, arg2: GrinEdge) -> GrinRow;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_num_by_type(arg1: GrinGraph, arg2: GrinVertexType) -> usize;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_num_by_type(arg1: GrinGraph, arg2: GrinEdgeType) -> usize;

    #[cfg(all(feature = "grin_enable_vertex_list", feature = "grin_with_vertex_property"))]
    #[allow(unused)]
    pub fn grin_get_vertex_list_by_type(
        arg1: GrinGraph,
        arg2: GrinVertexType,
    ) -> GrinVertexList;

    #[cfg(all(feature = "grin_enable_adjacent_list", feature = "grin_with_edge_property"))]
    #[allow(unused)]
    pub fn grin_get_adjacent_list_by_edge_type(
        arg1: GrinGraph,
        arg2: GrinDirection,
        arg3: GrinVertex,
        arg4: GrinEdgeType,
    ) -> GrinAdjacentList;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_equal_vertex_type(
        arg1: GrinGraph,
        arg2: GrinVertexType,
        arg3: GrinVertexType,
    ) -> bool;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_type(arg1: GrinGraph, arg2: GrinVertex) -> GrinVertexType;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_type(arg1: GrinGraph, arg2: GrinVertexType);

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_list(arg1: GrinGraph) -> GrinVertexTypeList;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_destroy_vertex_type_list(arg1: GrinGraph, arg2: GrinVertexTypeList);

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_create_vertex_type_list(arg1: GrinGraph) -> GrinVertexTypeList;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_insert_vertex_type_to_list(
        arg1: GrinGraph,
        arg2: GrinVertexTypeList,
        arg3: GrinVertexType,
    ) -> bool;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_list_size(arg1: GrinGraph, arg2: GrinVertexTypeList) -> usize;

    #[cfg(feature = "grin_with_vertex_property")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_from_list(
        arg1: GrinGraph,
        arg2: GrinVertexTypeList,
        arg3: usize,
    ) -> GrinVertexType;

    #[cfg(feature = "grin_with_vertex_type_name")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_name(
        arg1: GrinGraph,
        arg2: GrinVertexType,
    ) -> *const ::std::os::raw::c_char;

    #[cfg(feature = "grin_with_vertex_type_name")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_by_name(
        arg1: GrinGraph,
        arg2: *const ::std::os::raw::c_char,
    ) -> GrinVertexType;

    #[cfg(feature = "grin_trait_natural_id_for_vertex_type")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_id(arg1: GrinGraph, arg2: GrinVertexType)
        -> GrinVertexTypeId;

    #[cfg(feature = "grin_trait_natural_id_for_vertex_type")]
    #[allow(unused)]
    pub fn grin_get_vertex_type_by_id(
        arg1: GrinGraph,
        arg2: GrinVertexTypeId,
    ) -> GrinVertexType;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_equal_edge_type(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
        arg3: GrinEdgeType,
    ) -> bool;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_type(arg1: GrinGraph, arg2: GrinEdge) -> GrinEdgeType;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_destroy_edge_type(arg1: GrinGraph, arg2: GrinEdgeType);

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_type_list(arg1: GrinGraph) -> GrinEdgeTypeList;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_destroy_edge_type_list(arg1: GrinGraph, arg2: GrinEdgeTypeList);

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_create_edge_type_list(arg1: GrinGraph) -> GrinEdgeTypeList;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_insert_edge_type_to_list(
        arg1: GrinGraph,
        arg2: GrinEdgeTypeList,
        arg3: GrinEdgeType,
    ) -> bool;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_type_list_size(arg1: GrinGraph, arg2: GrinEdgeTypeList) -> usize;

    #[cfg(feature = "grin_with_edge_property")]
    #[allow(unused)]
    pub fn grin_get_edge_type_from_list(
        arg1: GrinGraph,
        arg2: GrinEdgeTypeList,
        arg3: usize,
    ) -> GrinEdgeType;

    #[cfg(feature = "grin_with_edge_type_name")]
    #[allow(unused)]
    pub fn grin_get_edge_type_name(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
    ) -> *const ::std::os::raw::c_char;

    #[cfg(feature = "grin_with_edge_type_name")]
    #[allow(unused)]
    pub fn grin_get_edge_type_by_name(
        arg1: GrinGraph,
        arg2: *const ::std::os::raw::c_char,
    ) -> GrinEdgeType;

    #[cfg(feature = "grin_trait_natural_id_for_edge_type")]
    #[allow(unused)]
    pub fn grin_get_edge_type_id(arg1: GrinGraph, arg2: GrinEdgeType) -> GrinEdgeTypeId;

    #[cfg(feature = "grin_trait_natural_id_for_edge_type")]
    #[allow(unused)]
    pub fn grin_get_edge_type_by_id(arg1: GrinGraph, arg2: GrinEdgeTypeId) -> GrinEdgeType;

    #[doc = " @brief  the src vertex type list"]
    #[cfg(all(feature = "grin_with_vertex_property", feature = "grin_with_edge_property"))]
    #[allow(unused)]
    pub fn grin_get_src_types_by_edge_type(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
    ) -> GrinVertexTypeList;

    #[doc = " @brief get the dst vertex type list"]
    #[cfg(all(feature = "grin_with_vertex_property", feature = "grin_with_edge_property"))]
    #[allow(unused)]
    pub fn grin_get_dst_types_by_edge_type(
        arg1: GrinGraph,
        arg2: GrinEdgeType,
    ) -> GrinVertexTypeList;

    #[doc = " @brief get the edge type list related to a given pair of vertex types"]
    #[cfg(all(feature = "grin_with_vertex_property", feature = "grin_with_edge_property"))]
    #[allow(unused)]
    pub fn grin_get_edge_types_by_vertex_type_pair(
        arg1: GrinGraph,
        arg2: GrinVertexType,
        arg3: GrinVertexType,
    ) -> GrinEdgeTypeList;


    #[allow(unused)]
    pub fn grin_get_last_error_code() -> GrinErrorCode;

}

 pub const GRIN_NULL_DATATYPE: GrinDatatype = GRIN_DATATYPE_UNDEFINED;

 pub const GRIN_NULL_GRAPH: GrinGraph = std::ptr::null_mut();

 pub const GRIN_NULL_VERTEX: GrinVertex = u64::MAX;

 pub const GRIN_NULL_EDGE: GrinEdge = std::ptr::null_mut();

 pub const GRIN_NULL_LIST: *mut ::std::os::raw::c_void = std::ptr::null_mut();

 pub const GRIN_NULL_LIST_ITERATOR: *mut ::std::os::raw::c_void = std::ptr::null_mut();

 pub const GRIN_NULL_PARTITION: GrinPartition = u32::MAX;

 pub const GRIN_NULL_VERTEX_REF: GrinVertexRef = -1;

 pub const GRIN_NULL_VERTEX_TYPE: GrinVertexType = u32::MAX;

 pub const GRIN_NULL_EDGE_TYPE: GrinEdgeType = u32::MAX;

 pub const GRIN_NULL_VERTEX_PROPERTY: GrinVertexProperty = u64::MAX;

 pub const GRIN_NULL_EDGE_PROPERTY: GrinEdgeProperty = u64::MAX;

 pub const GRIN_NULL_ROW: GrinRow = std::ptr::null_mut();

 pub const GRIN_NULL_NATURAL_ID: u32 = u32::MAX;

 pub const GRIN_NULL_SIZE: u32 = u32::MAX;

