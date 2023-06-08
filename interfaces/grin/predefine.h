/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * @file predefine.h
 * @brief Pre-defined macros for storage features.
 * The macros are divided into several sections such as topology, partition,
 * and so on. 
 * In each section, the first part lists all available macros, and undefines
 * all GRIN_ASSUME_ macros by default.
 * After that is the MOST IMPORTANT part for storage implementors, i.e., the StorageSpecific area.
 * Storage implementors should turn ON/OFF the macros in this area based the features of the storage.
 * The final part is the rule part to handle dependencies between macros which should not be edited.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GRIN_INCLUDE_PREDEFINE_H_
#define GRIN_INCLUDE_PREDEFINE_H_

#include <stdbool.h>
#include <stddef.h>

/// Enumerates the directions of edges with respect to a certain vertex
typedef enum {
  IN = 0,     ///< incoming
  OUT = 1,    ///< outgoing
  BOTH = 2,   ///< incoming & outgoing
} GRIN_DIRECTION;

/// Enumerates the datatype supported in the storage
typedef enum {
  Undefined = 0,      ///< other unknown types
  Int32 = 1,          ///< int
  UInt32 = 2,         ///< unsigned int 
  Int64 = 3,          ///< long int
  UInt64 = 4,         ///< unsigned long int
  Float = 5,          ///< float
  Double = 6,         ///< double
  String = 7,         ///< string
  Date32 = 8,         ///< date
  Time32 = 9,         ///< Time32
  Timestamp64 = 10,   ///< Timestamp
} GRIN_DATATYPE;

/// Enumerates the error codes of grin
typedef enum {
  NO_ERROR = 0,               ///< success
  UNKNOWN_ERROR = 1,         ///< unknown error
  INVALID_VALUE = 2,         ///< invalid value
  UNKNOWN_DATATYPE = 3,      ///< unknown datatype
} GRIN_ERROR_CODE;

/* Section 1: Toplogy */

/** @name TopologyMacros
 * @brief Macros for basic graph topology features
 */
///@{
/** @ingroup TopologyMacros 
 * @brief The storage only support directed graphs.
 */
#define GRIN_ASSUME_HAS_DIRECTED_GRAPH

/** @ingroup TopologyMacros 
 * @brief The storage only support undirected graphs.
 */
#define GRIN_ASSUME_HAS_UNDIRECTED_GRAPH

/** @ingroup TopologyMacros 
 * @brief The storage only support graphs with single
 * edge between a pair of vertices.
 */
#define GRIN_ASSUME_HAS_MULTI_EDGE_GRAPH

/** @ingroup TopologyMacros 
 * @brief There is data on vertex. E.g., the PageRank value of a vertex.
 */
#define GRIN_WITH_VERTEX_DATA

/** @ingroup TopologyMacros
 * @brief There is data on edge. E.g., the weight of an edge.
*/
#define GRIN_WITH_EDGE_DATA

/** @ingroup TopologyMacros
 * @brief Enable the vertex list structure. 
 * The vertex list related APIs follow the design of GRIN List.
*/
#define GRIN_ENABLE_VERTEX_LIST

/** @ingroup TopologyMacros
 * @brief Enable the vertex list array-style retrieval. 
 * The vertex list related APIs follow the design of GRIN List.
*/
#define GRIN_ENABLE_VERTEX_LIST_ARRAY

/** @ingroup TopologyMacros
 * @brief Enable the vertex list iterator. 
 * The vertex list iterator related APIs follow the design of GRIN Iterator.
*/
#define GRIN_ENABLE_VERTEX_LIST_ITERATOR

/** @ingroup TopologyMacros
 * @brief Enable the edge list structure. 
 * The edge list related APIs follow the design of GRIN List.
*/
#define GRIN_ENABLE_EDGE_LIST

/** @ingroup TopologyMacros
 * @brief Enable the edge list array-style retrieval. 
 * The edge list related APIs follow the design of GRIN List.
*/
#define GRIN_ENABLE_EDGE_LIST_ARRAY

/** @ingroup TopologyMacros
 * @brief Enable the edge list iterator. 
 * The edge list iterator related APIs follow the design of GRIN Iterator.
*/
#define GRIN_ENABLE_EDGE_LIST_ITERATOR

/** @ingroup TopologyMacros
 * @brief Enable the adjacent list structure. 
 * The adjacent list related APIs follow the design of GRIN List.
*/
#define GRIN_ENABLE_ADJACENT_LIST

/** @ingroup TopologyMacros
 * @brief Enable the adjacent list array-style retrieval. 
 * The adjacent list related APIs follow the design of GRIN List.
*/
#define GRIN_ENABLE_ADJACENT_LIST_ARRAY

/** @ingroup TopologyMacros
 * @brief Enable the adjacent list iterator. 
 * The adjacent list iterator related APIs follow the design of GRIN Iterator.
*/
#define GRIN_ENABLE_ADJACENT_LIST_ITERATOR
///@}


#ifndef GRIN_DOXYGEN_SKIP
// GRIN_DEFAULT_DISABLE
#undef GRIN_ASSUME_HAS_DIRECTED_GRAPH
#undef GRIN_ASSUME_HAS_UNDIRECTED_GRAPH
#undef GRIN_ASSUME_HAS_MULTI_EDGE_GRAPH
#undef GRIN_WITH_VERTEX_DATA
#undef GRIN_WITH_EDGE_DATA
#undef GRIN_ENABLE_VERTEX_LIST
#undef GRIN_ENABLE_VERTEX_LIST_ARRAY
#undef GRIN_ENABLE_VERTEX_LIST_ITERATOR
#undef GRIN_ENABLE_EDGE_LIST
#undef GRIN_ENABLE_EDGE_LIST_ARRAY
#undef GRIN_ENABLE_EDGE_LIST_ITERATOR
#undef GRIN_ENABLE_ADJACENT_LIST
#undef GRIN_ENABLE_ADJACENT_LIST_ARRAY
#undef GRIN_ENABLE_ADJACENT_LIST_ITERATOR
// GRIN_END

// GRIN_STORAGE_ENABLE
#define GRIN_ASSUME_HAS_DIRECTED_GRAPH
#define GRIN_ASSUME_HAS_UNDIRECTED_GRAPH
#define GRIN_ASSUME_HAS_MULTI_EDGE_GRAPH
#define GRIN_ENABLE_VERTEX_LIST
#define GRIN_ENABLE_VERTEX_LIST_ITERATOR
#define GRIN_ENABLE_ADJACENT_LIST
#define GRIN_ENABLE_ADJACENT_LIST_ITERATOR
// GRIN_END

// GRIN_FEATURE_DEPENDENCY
// GRIN_END

#endif  // GRIN_DOXYGEN_SKIP
/* End of Section 1 */

/* Section 2. Partition */

/** @name PartitionMacros
 * @brief Macros for partitioned graph features
 */
///@{
/** @ingroup PartitionMacros
 * @brief Enable partitioned graph. A partitioned graph usually contains
 * several fragments (i.e., local graphs) that are distributedly stored 
 * in a cluster. In GRIN, GRIN_GRAPH represents to a single fragment that can
 * be locally accessed.
 */
#define GRIN_ENABLE_GRAPH_PARTITION

/** @ingroup PartitionMacros
 * @brief The storage provides natural number IDs for partitions.
 * It follows the design of natural number ID trait in GRIN.
*/
#define GRIN_TRAIT_NATURAL_ID_FOR_PARTITION

/** @ingroup PartitionMacros
 * @brief The storage provides reference of vertex that can be
 * recognized in other partitions where the vertex also appears.
*/
#define GRIN_ENABLE_VERTEX_REF

/** @ingroup PartitionMacros
 * @brief The storage provides fast reference of vertex, which means
 * the vertex ref can be serialized into a int64 using grin_serialize_vertex_ref_as_int64
*/
#define GRIN_TRAIT_FAST_VERTEX_REF

/** @ingroup PartitionMacros
 * @brief The storage provides reference of edge that can be
 * recognized in other partitions where the edge also appears.
*/
#define GRIN_ENABLE_EDGE_REF
///@}



/** @name PartitionStrategyMacros
 * @brief Macros to define partition strategy assumptions, a partition strategy
 * can be seen as a combination of detail partition assumptions which are defined after
 * the strategies. Please refer to the documents for strategy details.
*/
///@{
/** @ingroup PartitionStrategyMacros
 * @brief The storage ONLY uses all-replicate partition strategy. This means the 
 * storage's replicate the graph among all partitions.
*/
#define GRIN_ASSUME_ALL_REPLICATE_PARTITION

/** @ingroup PartitionStrategyMacros
 * @brief The storage ONLY uses edge-cut partition strategy. This means the 
 * storage's entire partition strategy complies with edge-cut strategy 
 * definition in GRIN.
*/
#define GRIN_ASSUME_EDGE_CUT_PARTITION

/** @ingroup PartitionStrategyMacros
 * @brief The storage ONLY uses edge-cut partition & edges only follow src strategy. 
 * This means the storage's entire partition strategy complies with edge-cut strategy 
 * definition in GRIN, and edges are partitioned to the partition of the source vertex.
*/
#define GRIN_ASSUME_EDGE_CUT_FOLLOW_SRC_PARTITION


/** @ingroup PartitionStrategyMacros
 * @brief The storage ONLY uses edge-cut partition & edges only follow dst strategy. 
 * This means the storage's entire partition strategy complies with edge-cut strategy 
 * definition in GRIN, and edges are partitioned to the partition of the destination vertex.
*/
#define GRIN_ASSUME_EDGE_CUT_FOLLOW_DST_PARTITION


/** @ingroup PartitionStrategyMacros
 * @brief The storage ONLY uses vertex-cut partition strategy. This means the 
 * storage's entire partition strategy complies with vertex-cut strategy 
 * definition in GRIN.
*/
#define GRIN_ASSUME_VERTEX_CUT_PARTITION
///@}

/** @name PartitionAssumptionMacros
 * @brief Macros to define detailed partition assumptions with respect to the
 * concept of local complete. Please refer to the documents for the meaning of
 * local complete.
*/
///@{
/** @ingroup PartitionAssumptionMacros
 * @brief Assume the vertex data are only stored together with master vertices.
*/
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_DATA

/** @ingroup PartitionAssumptionMacros
 * @brief Assume the vertex data are replicated on both master and mirror vertices.
*/
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_DATA

/** @ingroup PartitionAssumptionMacros
 * @brief Assume the edge data are only stored together with master edges.
*/
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_DATA

/** @ingroup PartitionAssumptionMacros
 * @brief Assume the edge data are replicated on both master and mirror edges.
*/
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_DATA
///@}

/** @name TraitMirrorPartitionMacros
 * @brief Macros for storage that provides the partition list where the mirror
 * vertices are located. This trait is usually enabled by storages using vertex-cut
 * partition strategy.
*/
///@{
/** @ingroup TraitMirrorPartitionMacros
 * @brief The storage provides the partition list where the mirror
 * vertices are located of a local master vertex.
*/
#define GRIN_TRAIT_MASTER_VERTEX_MIRROR_PARTITION_LIST

/** @ingroup TraitMirrorPartitionMacros
 * @brief The storage provides the partition list where the mirror
 * vertices are located of a local mirror vertex
*/
#define GRIN_TRAIT_MIRROR_VERTEX_MIRROR_PARTITION_LIST

/** @ingroup TraitMirrorPartitionMacros
 * @brief The storage provides the partition list where the mirror
 * edges are located of a local master edge
*/
#define GRIN_TRAIT_MASTER_EDGE_MIRROR_PARTITION_LIST

/** @ingroup TraitMirrorPartitionMacros
 * @brief The storage provides the partition list where the mirror
 * edges are located of a local mirror edge
*/
#define GRIN_TRAIT_MIRROR_EDGE_MIRROR_PARTITION_LIST
///@}

/** @name TraitFilterMacros
 * @brief Macros for storage that provides filtering ability of partitions for structures
 * like vertex list or adjacent list. This trait is usually enabled for efficient graph traversal.
*/
///@{
/** @ingroup TraitFilterMacros
 * @brief The storage provides a filtering predicate of master vertices
 * for vertex list iterator. That means, the caller can use the predicate
 * to make a master-only vertex list iterator from the original iterator.
*/
#define GRIN_TRAIT_SELECT_MASTER_FOR_VERTEX_LIST

/** @ingroup TraitFilterMacros
 * @brief The storage provides a filtering predicate of single partition vertices
 * for vertex list iterator. That means, the caller can use the predicate
 * to make a single-partition vertex list iterator from the original iterator.
*/
#define GRIN_TRAIT_SELECT_PARTITION_FOR_VERTEX_LIST

/** @ingroup TraitFilterMacros
 * @brief The storage provides a filtering predicate of master edges
 * for edge list iterator. That means, the caller can use the predicate
 * to make a master-only edge list iterator from the original iterator.
*/
#define GRIN_TRAIT_SELECT_MASTER_FOR_EDGE_LIST

/** @ingroup TraitFilterMacros
 * @brief The storage provides a filtering predicate of single partition edges
 * for edge list iterator. That means, the caller can use the predicate
 * to make a single-partition edge list iterator from the original iterator.
*/
#define GRIN_TRAIT_SELECT_PARTITION_FOR_EDGE_LIST

/** @ingroup TraitFilterMacros
 * @brief The storage provides a filtering predicate of master neighbors
 * for adjacent list iterator. That means, the caller can use the predicate
 * to make a master-only adjacent list iterator from the original iterator.
*/
#define GRIN_TRAIT_SELECT_MASTER_NEIGHBOR_FOR_ADJACENT_LIST

/** @ingroup TraitFilterMacros
 * @brief The storage provides a filtering predicate of single-partition vertices
 * for adjacent list iterator. That means, the caller can use the predicate
 * to make a single-partition adjacent list iterator from the original iterator.
*/
#define GRIN_TRAIT_SELECT_NEIGHBOR_PARTITION_FOR_ADJACENT_LIST
///@}

#ifndef GRIN_DOXYGEN_SKIP 
// GRIN_DEFAULT_DISABLE
#undef GRIN_ENABLE_GRAPH_PARTITION
#undef GRIN_TRAIT_NATURAL_ID_FOR_PARTITION
#undef GRIN_ENABLE_VERTEX_REF
#undef GRIN_TRAIT_FAST_VERTEX_REF
#undef GRIN_ENABLE_EDGE_REF
#undef GRIN_ASSUME_ALL_REPLICATE_PARTITION
#undef GRIN_ASSUME_EDGE_CUT_PARTITION
#undef GRIN_ASSUME_EDGE_CUT_FOLLOW_SRC_PARTITION
#undef GRIN_ASSUME_EDGE_CUT_FOLLOW_DST_PARTITION
#undef GRIN_ASSUME_VERTEX_CUT_PARTITION
#undef GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_DATA
#undef GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_DATA
#undef GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_DATA
#undef GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_DATA
#undef GRIN_TRAIT_MASTER_VERTEX_MIRROR_PARTITION_LIST
#undef GRIN_TRAIT_MIRROR_VERTEX_MIRROR_PARTITION_LIST
#undef GRIN_TRAIT_MASTER_EDGE_MIRROR_PARTITION_LIST
#undef GRIN_TRAIT_MIRROR_EDGE_MIRROR_PARTITION_LIST
#undef GRIN_TRAIT_SELECT_MASTER_FOR_VERTEX_LIST
#undef GRIN_TRAIT_SELECT_PARTITION_FOR_VERTEX_LIST
#undef GRIN_TRAIT_SELECT_MASTER_FOR_EDGE_LIST
#undef GRIN_TRAIT_SELECT_PARTITION_FOR_EDGE_LIST
#undef GRIN_TRAIT_SELECT_MASTER_NEIGHBOR_FOR_ADJACENT_LIST
#undef GRIN_TRAIT_SELECT_NEIGHBOR_PARTITION_FOR_ADJACENT_LIST
// GRIN_END

// GRIN_STORAGE_ENABLE
#define GRIN_ENABLE_GRAPH_PARTITION
#define GRIN_TRAIT_NATURAL_ID_FOR_PARTITION
#define GRIN_ENABLE_VERTEX_REF
#define GRIN_TRAIT_FAST_VERTEX_REF
#define GRIN_ASSUME_EDGE_CUT_PARTITION
#define GRIN_TRAIT_SELECT_MASTER_FOR_VERTEX_LIST
// GRIN_END

// GRIN_FEATURE_DEPENDENCY
#ifdef GRIN_ASSUME_ALL_REPLICATE_PARTITION
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_DATA
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_DATA
#endif

#ifdef GRIN_ASSUME_EDGE_CUT_PARTITION
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_DATA
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_DATA
#endif

#ifdef GRIN_ASSUME_EDGE_CUT_FOLLOW_SRC_PARTITION
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_DATA
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_DATA
#endif

#ifdef GRIN_ASSUME_EDGE_CUT_FOLLOW_DST_PARTITION
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_DATA
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_DATA
#endif

#ifdef GRIN_ASSUME_VERTEX_CUT_PARTITION
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_DATA
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_DATA
#define GRIN_TRAIT_MASTER_VERTEX_MIRROR_PARTITION_LIST
#endif
// GRIN_END

#endif // GRIN_DOXY_SKIP
/* End of Section 2 */

/* Section 3. Property */

/** @name PropertyMacros
 * @brief Macros for basic property graph features
 */
///@{
/** @ingroup PropertyMacros
 * @brief Enable the pure data structure Row
*/
#define GRIN_ENABLE_ROW

/** @ingroup PropertyMacros
 * @brief This trait is used to indicate the storage can return a pointer to the
 * value of a property.
*/
#define GRIN_TRAIT_CONST_VALUE_PTR

/** @ingroup PropertyMacros
 * @brief There are properties bound to vertices. When vertices are typed, vertex
 * properties are bound to vertex types, according to the definition of vertex type.
*/
#define GRIN_WITH_VERTEX_PROPERTY

/** @ingroup PropertyMacros
 * @brief There are property names for vertex properties. The relationship between property
 * name and properties is one-to-many, because properties bound to different vertex/edge
 * types are distinguished even they may share the same property name. Please refer to
 * the design of Property for details.
*/
#define GRIN_WITH_VERTEX_PROPERTY_NAME

/** @ingroup PropertyMacros
 * @brief There are unique names for each vertex type.
*/
#define GRIN_WITH_VERTEX_TYPE_NAME

/** @ingroup PropertyMacros
 * @brief The storage provides natural number IDs for vertex types.
 * It follows the design of natural ID trait in GRIN.
*/
#define GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_TYPE

/** @ingroup PropertyMacros
 * @brief There are primary keys for vertices. Vertex primary keys is
 * a set of vertex properties whose values can distinguish vertices. When vertices are
 * typed, each vertex type has its own primary keys which distinguishes the vertices of
 * that type. 
 * 
 * With primary keys, one can get the vertex from the graph or a certain type
 * by providing the values of the primary keys. The macro is unset if GRIN_WITH_VERTEX_PROPERTY
 * is NOT defined, in which case, one can use ORIGINAL_ID when vertices have
 * no properties.
*/
#define GRIN_ENABLE_VERTEX_PRIMARY_KEYS

/** @ingroup PropertyMacros
 * @brief The storage provides natural number IDs for properties bound to
 * a certain vertex type.
 * It follows the design of natural ID trait in GRIN.
*/
#define GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_PROPERTY


/** @ingroup PropertyMacros
 * @brief There are properties bound to edges. When edges are typed, edge
 * properties are bound to edge types, according to the definition of edge type.
*/
#define GRIN_WITH_EDGE_PROPERTY

/** @ingroup PropertyMacros
 * @brief There are property names for edge properties. The relationship between property
 * name and properties is one-to-many, because properties bound to different vertex/edge
 * types are distinguished even they may share the same property name. Please refer to
 * the design of Property for details.
*/
#define GRIN_WITH_EDGE_PROPERTY_NAME

/** @ingroup PropertyMacros
 * @brief There are unique names for each edge type.
*/
#define GRIN_WITH_EDGE_TYPE_NAME

/** @ingroup PropertyMacros
 * @brief The storage provides natural number IDs for edge types.
 * It follows the design of natural ID trait in GRIN.
*/
#define GRIN_TRAIT_NATURAL_ID_FOR_EDGE_TYPE

/** @ingroup PropertyMacros
 * @brief There are primary keys for edges. Edge primary keys is
 * a set of edge properties whose values can distinguish edges. When edges are
 * typed, each edge type has its own primary keys which distinguishes the edges of
 * that type. 
 * 
 * With primary keys, one can get the edge from the graph or a certain type
 * by providing the values of the primary keys. The macro is unset if GRIN_WITH_EDGE_PROPERTY
 * is NOT defined.
*/
#define GRIN_ENABLE_EDGE_PRIMARY_KEYS

/** @ingroup PropertyMacros
 * @brief The storage provides natural number IDs for properties bound to
 * a certain edge type.
 * It follows the design of natural ID trait in GRIN.
*/
#define GRIN_TRAIT_NATURAL_ID_FOR_EDGE_PROPERTY
///@}

/** @name TraitFilterTypeMacros
 * @brief Macros of traits to filter vertex/edge type for
 * structures like vertex list and adjacent list.
 */
///@{
/** @ingroup TraitFilterTypeMacros
 * @brief The storage provides a filtering predicate of single-type vertices
 * for vertex list iterator. That means, the caller can use the predicate
 * to make a vertex list iterator for a certain type of vertices from the 
 * original iterator.
*/
#define GRIN_TRAIT_SELECT_TYPE_FOR_VERTEX_LIST

/** @ingroup TraitFilterTypeMacros
 * @brief The storage provides a filtering predicate of single-type edges
 * for edge list iterator. That means, the caller can use the predicate
 * to make an edge list iterator for a certain type of edges from the 
 * original iterator.
*/
#define GRIN_TRAIT_SELECT_TYPE_FOR_EDGE_LIST

/** @ingroup TraitFilterTypeMacros
 * @brief The storage provides a filtering predicate of single-type neighbors
 * for adjacent list iterator. That means, the caller can use the predicate
 * to make an adjacent list iterator of neighbors with a certain type from 
 * the original iterator.
*/
#define GRIN_TRAIT_SELECT_NEIGHBOR_TYPE_FOR_ADJACENT_LIST

/** @ingroup TraitFilterTypeMacros
 * @brief The storage provides a filtering predicate of single-type edges
 * for adjacent list iterator. That means, the caller can use the predicate
 * to make an adjacent list iterator of edges with a certain type from 
 * the original iterator.
*/
#define GRIN_TRAIT_SELECT_EDGE_TYPE_FOR_ADJACENT_LIST

/** @ingroup TraitFilterTypeMacros
 * @brief The storage provides specific relationship description for each
 * vertex-edge-vertex type traid. This means further optimizations can be
 * applied by the callers for vev traid under certain relationships, such as
 * one-to-one, one-to-many, or many-to-one.
*/
#define GRIN_TRAIT_SPECIFIC_VEV_RELATION
///@}


/** @name PropetyAssumptionMacros
 * @brief Macros of assumptions for property local complete, and particularly define
 * the by type local complete assumptions for hybrid partiton strategy.
 */
///@{
/** @ingroup PropetyAssumptionMacros
 * @brief Assume full property values of a vertex are ONLY stored with master vertices. 
*/
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_PROPERTY

/** @ingroup PropetyAssumptionMacros
 * @brief Assume full property values of a vertex are replicated with master and mirror vertices. 
*/
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_PROPERTY

/** @ingroup PropetyAssumptionMacros
 * @brief Assume full property values of a vertex are split among master and mirror vertices. 
*/
#define GRIN_ASSUME_SPLIT_MASTER_MIRROR_PARTITION_FOR_VERTEX_PROPERTY

/** @ingroup PropetyAssumptionMacros
 * @brief Assume full property values of an edge are ONLY stored with master edges. 
*/
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_PROPERTY

/** @ingroup PropetyAssumptionMacros
 * @brief Assume full property values of an edge are replicated with master and mirror edges. 
*/
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY

/** @ingroup PropetyAssumptionMacros
 * @brief Assume full property values of an edge are split among master and mirror edges. 
*/
#define GRIN_ASSUME_SPLIT_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY
///@}

#ifndef GRIN_DOXYGEN_SKIP
// GRIN_DEFAULT_DISABLE
#undef GRIN_ENABLE_ROW
#undef GRIN_TRAIT_CONST_VALUE_PTR
#undef GRIN_WITH_VERTEX_PROPERTY
#undef GRIN_WITH_VERTEX_PROPERTY_NAME
#undef GRIN_WITH_VERTEX_TYPE_NAME
#undef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_TYPE
#undef GRIN_ENABLE_VERTEX_PRIMARY_KEYS
#undef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_PROPERTY
#undef GRIN_WITH_EDGE_PROPERTY
#undef GRIN_WITH_EDGE_PROPERTY_NAME
#undef GRIN_WITH_EDGE_TYPE_NAME
#undef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_TYPE
#undef GRIN_ENABLE_EDGE_PRIMARY_KEYS
#undef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_PROPERTY
#undef GRIN_TRAIT_SELECT_TYPE_FOR_VERTEX_LIST
#undef GRIN_TRAIT_SELECT_TYPE_FOR_EDGE_LIST
#undef GRIN_TRAIT_SELECT_NEIGHBOR_TYPE_FOR_ADJACENT_LIST
#undef GRIN_TRAIT_SELECT_EDGE_TYPE_FOR_ADJACENT_LIST
#undef GRIN_TRAIT_SPECIFIC_VEV_RELATION
#undef GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_PROPERTY
#undef GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_PROPERTY
#undef GRIN_ASSUME_SPLIT_MASTER_MIRROR_PARTITION_FOR_VERTEX_PROPERTY
#undef GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_PROPERTY
#undef GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY
#undef GRIN_ASSUME_SPLIT_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY
// GRIN_END

// GRIN_STORAGE_ENABLE
#define GRIN_ENABLE_ROW
#define GRIN_TRAIT_CONST_VALUE_PTR
#define GRIN_WITH_VERTEX_PROPERTY
#define GRIN_WITH_VERTEX_PROPERTY_NAME
#define GRIN_WITH_VERTEX_TYPE_NAME
#define GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_TYPE
#define GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_PROPERTY
#define GRIN_WITH_EDGE_PROPERTY
#define GRIN_WITH_EDGE_PROPERTY_NAME
#define GRIN_WITH_EDGE_TYPE_NAME
#define GRIN_TRAIT_NATURAL_ID_FOR_EDGE_TYPE
#define GRIN_TRAIT_NATURAL_ID_FOR_EDGE_PROPERTY
#define GRIN_TRAIT_SELECT_TYPE_FOR_VERTEX_LIST
#define GRIN_TRAIT_SELECT_EDGE_TYPE_FOR_ADJACENT_LIST
// GRIN_END

// GRIN_FEATURE_DEPENDENCY
#ifdef GRIN_ASSUME_ALL_REPLICATE_PARTITION
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_PROPERTY
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY
#endif

#ifdef GRIN_ASSUME_EDGE_CUT_PARTITION
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_PROPERTY
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY
#endif

#ifdef GRIN_ASSUME_EDGE_CUT_FOLLOW_SRC_PARTITION
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_PROPERTY
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_PROPERTY
#endif

#ifdef GRIN_ASSUME_EDGE_CUT_FOLLOW_DST_PARTITION
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_VERTEX_PROPERTY
#define GRIN_ASSUME_MASTER_ONLY_PARTITION_FOR_EDGE_PROPERTY
#endif

#ifdef GRIN_ASSUME_VERTEX_CUT_PARTITION
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_VERTEX_PROPERTY
#define GRIN_ASSUME_REPLICATE_MASTER_MIRROR_PARTITION_FOR_EDGE_PROPERTY
#endif

#ifdef GRIN_ENABLE_VERTEX_PRIMARY_KEYS
#define GRIN_ENABLE_ROW
#endif

#ifdef GRIN_ENABLE_EDGE_PRIMARY_KEYS
#define GRIN_ENABLE_ROW
#endif
// GRIN_END

#endif // GRIN_DOXY_SKIP
/* End of Section 3 */

/* Section 4. Index */
/** @name IndexLabelMacros
 * @brief Macros for label features
 */
///@{
/** @ingroup IndexLabelMacros
 * @brief Enable vertex label on graph. 
*/
#define GRIN_WITH_VERTEX_LABEL

/** @ingroup IndexLabelMacros
 * @brief Enable edge label on graph. 
*/
#define GRIN_WITH_EDGE_LABEL
///@}

/** @name IndexOrderMacros
 * @brief Macros for ordering features.
 * Please refer to the order section in the documents for details.
 */
///@{
/** @ingroup IndexOrderMacros
 * @brief assume all vertex list are sorted.
 * We will expend the assumption to support master/mirror or
 * by type in the future if needed.
*/
#define GRIN_ASSUME_ALL_VERTEX_LIST_SORTED
///@}

/** @name IndexOIDMacros
 * @brief Macros for label features
 */
///@{
/** @ingroup IndexOIDMacros
 * @brief There is original ID of type int64 for each vertex
 * This facilitates queries starting from a specific vertex,
 * since one can get the vertex handler directly using its original ID.
 */
#define GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_INT64

/** @ingroup IndexOIDMacros
 * @brief There is original ID of type string for each vertex
 * This facilitates queries starting from a specific vertex,
 * since one can get the vertex handler directly using its original ID.
 */
#define GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_STRING
///@}

#ifndef GRIN_DOXYGEN_SKIP
// GRIN_DEFAULT_DISABLE
#undef GRIN_WITH_VERTEX_LABEL
#undef GRIN_WITH_EDGE_LABEL
#undef GRIN_ASSUME_ALL_VERTEX_LIST_SORTED
#undef GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_INT64
#undef GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_STRING
// GRIN_END

// GRIN_STORAGE_ENABLE
// #define GRIN_ASSUME_ALL_VERTEX_LIST_SORTED
#define GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_INT64
// GRIN_END

// GRIN_FEATURE_DEPENDENCY
// GRIN_END

#endif  // GRIN_DOXYGEN_SKIP
/* End of Section 4 */

/** @name NullValues
 * Macros for Null(invalid) values
 */
///@{
/** @brief Null graph (invalid return value) */
#define GRIN_NULL_GRAPH NULL
/** @brief Non-existing vertex (invalid return value) */
#define GRIN_NULL_VERTEX (unsigned long long int)~0
/** @brief Non-existing edge (invalid return value) */
#define GRIN_NULL_EDGE NULL
/** @brief Null list of any kind (invalid return value) */
#define GRIN_NULL_LIST NULL
/** @brief Null list iterator of any kind (invalid return value) */
#define GRIN_NULL_LIST_ITERATOR NULL
/** @brief Non-existing partition (invalid return value) */
#define GRIN_NULL_PARTITION (unsigned)~0
/** @brief Null vertex reference (invalid return value) */
#define GRIN_NULL_VERTEX_REF -1
/** @brief Null edge reference (invalid return value) */
#define GRIN_NULL_EDGE_REF NULL
/** @brief Non-existing vertex type (invalid return value) */
#define GRIN_NULL_VERTEX_TYPE (unsigned)~0
/** @brief Non-existing edge type (invalid return value) */
#define GRIN_NULL_EDGE_TYPE (unsigned)~0
/** @brief Non-existing vertex property (invalid return value) */
#define GRIN_NULL_VERTEX_PROPERTY (unsigned long long int)~0
/** @brief Non-existing vertex property (invalid return value) */
#define GRIN_NULL_EDGE_PROPERTY (unsigned long long int)~0
/** @brief Null row (invalid return value) */
#define GRIN_NULL_ROW NULL
/** @brief Null natural id of any kind (invalid return value) */
#define GRIN_NULL_NATURAL_ID (unsigned)~0
/** @brief Null size (invalid return value) */
#define GRIN_NULL_SIZE (unsigned)~0
/** @breif Null name (invalid return value) */
#define GRIN_NULL_NAME NULL
///@}


/* Define the handlers using typedef */
typedef void* GRIN_GRAPH;                      
typedef unsigned long long int GRIN_VERTEX;                     
typedef void* GRIN_EDGE;                       

#ifdef GRIN_WITH_VERTEX_DATA
typedef void* GRIN_VERTEX_DATA;                 
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST
typedef void* GRIN_VERTEX_LIST;                 
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST_ITERATOR
typedef void* GRIN_VERTEX_LIST_ITERATOR;         
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST
typedef void* GRIN_ADJACENT_LIST;               
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST_ITERATOR
typedef void* GRIN_ADJACENT_LIST_ITERATOR;       
#endif

#ifdef GRIN_WITH_EDGE_DATA
typedef void* GRIN_EDGE_DATA;                   
#endif

#ifdef GRIN_ENABLE_EDGE_LIST
typedef void* GRIN_EDGE_LIST;                   
#endif

#ifdef GRIN_ENABLE_EDGE_LIST_ITERATOR
typedef void* GRIN_EDGE_LIST_ITERATOR;           
#endif

#ifdef GRIN_ENABLE_GRAPH_PARTITION
typedef void* GRIN_PARTITIONED_GRAPH;
typedef unsigned GRIN_PARTITION;
typedef void* GRIN_PARTITION_LIST;
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_PARTITION
typedef unsigned GRIN_PARTITION_ID;
#endif

#ifdef GRIN_ENABLE_VERTEX_REF
typedef long long int GRIN_VERTEX_REF;
#endif

#ifdef GRIN_ENABLE_EDGE_REF
typedef void* GRIN_EDGE_REF;
#endif


#ifdef GRIN_WITH_VERTEX_PROPERTY
typedef unsigned GRIN_VERTEX_TYPE;
typedef void* GRIN_VERTEX_TYPE_LIST;
typedef unsigned long long int GRIN_VERTEX_PROPERTY;
typedef void* GRIN_VERTEX_PROPERTY_LIST;
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_TYPE
typedef unsigned GRIN_VERTEX_TYPE_ID;
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_PROPERTY
typedef unsigned GRIN_VERTEX_PROPERTY_ID;
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY
typedef unsigned GRIN_EDGE_TYPE;
typedef void* GRIN_EDGE_TYPE_LIST;
typedef void* GRIN_VEV_TYPE;
typedef void* GRIN_VEV_TYPE_LIST;
typedef unsigned long long int GRIN_EDGE_PROPERTY;
typedef void* GRIN_EDGE_PROPERTY_LIST;
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_TYPE
typedef unsigned GRIN_EDGE_TYPE_ID;
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_PROPERTY
typedef unsigned GRIN_EDGE_PROPERTY_ID;
#endif

#ifdef GRIN_ENABLE_ROW
typedef void* GRIN_ROW;
#endif

#if defined(GRIN_WITH_VERTEX_LABEL) || defined(GRIN_WITH_EDGE_LABEL)
typedef void* GRIN_LABEL;
typedef void* GRIN_LABEL_LIST;
#endif

#endif  // GRIN_INCLUDE_PREDEFINE_H_

#ifdef __cplusplus
}
#endif