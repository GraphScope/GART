#include <stdio.h>

#include "grape/grape.h"
#include "grape/util.h"

#include "grin/src/predefine.h"
#include "interfaces/fragment/gart_fragment.h"

#include "../include/common/error.h"
#include "../include/index/label.h"
#include "../include/index/order.h"
#include "../include/index/original_id.h"
#include "../include/partition/partition.h"
#include "../include/partition/reference.h"
#include "../include/partition/topology.h"
#include "../include/property/partition.h"
#include "../include/property/primarykey.h"
#include "../include/property/property.h"
#include "../include/property/propertylist.h"
#include "../include/property/row.h"
#include "../include/property/topology.h"
#include "../include/property/type.h"
#include "../include/topology/adjacentlist.h"
#include "../include/topology/edgelist.h"
#include "../include/topology/structure.h"
#include "../include/topology/vertexlist.h"

GRIN_GRAPH get_graph(int argc, char** argv) {
#ifdef GRIN_ENABLE_GRAPH_PARTITION
  GRIN_PARTITIONED_GRAPH pg =
      grin_get_partitioned_graph_from_storage(argc - 1, &(argv[0]));
  GRIN_PARTITION_LIST local_partitions = grin_get_local_partition_list(pg);
  GRIN_PARTITION partition =
      grin_get_partition_from_list(pg, local_partitions, 0);
  GRIN_PARTITION_ID partition_id = grin_get_partition_id(pg, partition);
  GRIN_PARTITION p1 = grin_get_partition_by_id(pg, partition_id);
  if (!grin_equal_partition(pg, partition, p1)) {
    printf("partition not match\n");
  }
  grin_destroy_partition(pg, p1);
  GRIN_GRAPH g = grin_get_local_graph_by_partition(pg, partition);
  grin_destroy_partition(pg, partition);
  grin_destroy_partition_list(pg, local_partitions);
  grin_destroy_partitioned_graph(pg);
#else
  GRIN_GRAPH g = grin_get_graph_from_storage(argc - 1, &(argv[1]));
#endif
  return g;
}

#ifdef GRIN_ENABLE_GRAPH_PARTITION
GRIN_PARTITION get_partition(int argc, char** argv) {
  GRIN_PARTITIONED_GRAPH pg =
      grin_get_partitioned_graph_from_storage(argc - 1, &(argv[0]));
  GRIN_PARTITION_LIST local_partitions = grin_get_local_partition_list(pg);
  GRIN_PARTITION partition =
      grin_get_partition_from_list(pg, local_partitions, 0);
  grin_destroy_partition_list(pg, local_partitions);
  grin_destroy_partitioned_graph(pg);
  return partition;
}
#endif

#ifdef GRIN_ENABLE_GRAPH_PARTITION
GRIN_PARTITIONED_GRAPH get_partitioend_graph(int argc, char** argv) {
  GRIN_PARTITIONED_GRAPH pg =
      grin_get_partitioned_graph_from_storage(argc - 1, &(argv[1]));
  return pg;
}
#endif

GRIN_VERTEX_TYPE get_one_vertex_type(GRIN_GRAPH g) {
  GRIN_VERTEX_TYPE_LIST vtl = grin_get_vertex_type_list(g);
  GRIN_VERTEX_TYPE vt = grin_get_vertex_type_from_list(g, vtl, 0);
  grin_destroy_vertex_type_list(g, vtl);
  return vt;
}

GRIN_EDGE_TYPE get_one_edge_type(GRIN_GRAPH g) {
  GRIN_EDGE_TYPE_LIST etl = grin_get_edge_type_list(g);
  GRIN_EDGE_TYPE et = grin_get_edge_type_from_list(g, etl, 0);
  grin_destroy_edge_type_list(g, etl);
  return et;
}

GRIN_VERTEX get_one_vertex(GRIN_GRAPH g) {
  GRIN_VERTEX_LIST vl = grin_get_vertex_list(g);
#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
  GRIN_VERTEX v = grin_get_vertex_from_list(g, vl, 0);
#else
  GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, vl);
  GRIN_VERTEX v = grin_get_vertex_from_iter(g, vli);
  grin_destroy_vertex_list_iter(g, vli);
#endif
  grin_destroy_vertex_list(g, vl);
  return v;
}

GRIN_VERTEX get_vertex_marco(GRIN_GRAPH g) {
  GRIN_VERTEX_LIST vl = grin_get_vertex_list(g);
#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
  GRIN_VERTEX v = grin_get_vertex_from_list(g, vl, 3);
#else
  GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, vl);
  for (int i = 0; i < 3; ++i) {
    grin_get_next_vertex_list_iter(g, vli);
  }
  GRIN_VERTEX v = grin_get_vertex_from_iter(g, vli);
  grin_destroy_vertex_list_iter(g, vli);
#endif
  grin_destroy_vertex_list(g, vl);
  return v;
}

void test_property_type(int argc, char** argv) {
  printf("+++++++++++++++++++++ Test property/type +++++++++++++++++++++\n");

  GRIN_GRAPH g = get_graph(argc, argv);

  printf("------------ Vertex Type ------------\n");
  GRIN_VERTEX_TYPE_LIST vtl = grin_get_vertex_type_list(g);
  size_t vtl_size = grin_get_vertex_type_list_size(g, vtl);
  printf("vertex type list size: %zu\n", vtl_size);

  for (size_t i = 0; i < vtl_size; ++i) {
    printf("------------ Iterate the %zu-th vertex type ------------\n", i);
    GRIN_VERTEX_TYPE vt = grin_get_vertex_type_from_list(g, vtl, i);
#ifdef GRIN_WITH_VERTEX_TYPE_NAME
    const char* vt_name = grin_get_vertex_type_name(g, vt);
    printf("vertex type name: %s\n", vt_name);
    GRIN_VERTEX_TYPE vt0 = grin_get_vertex_type_by_name(g, vt_name);
    // grin_destroy_name(g, vt_name);
    if (!grin_equal_vertex_type(g, vt, vt0)) {
      printf("vertex type name not match\n");
    }
    grin_destroy_vertex_type(g, vt0);
#endif
#ifdef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_TYPE
    printf("vertex type id: %u\n", grin_get_vertex_type_id(g, vt));
    GRIN_VERTEX_TYPE vt1 =
        grin_get_vertex_type_by_id(g, grin_get_vertex_type_id(g, vt));
    if (!grin_equal_vertex_type(g, vt, vt1)) {
      printf("vertex type id not match\n");
    }
    grin_destroy_vertex_type(g, vt1);
#endif
  }
  grin_destroy_vertex_type_list(g, vtl);

  printf(
      "------------ Create a vertex type list of one type \"person\" "
      "------------\n");
  GRIN_VERTEX_TYPE_LIST vtl2 = grin_create_vertex_type_list(g);
#ifdef GRIN_WITH_VERTEX_TYPE_NAME
  GRIN_VERTEX_TYPE vt2_w = grin_get_vertex_type_by_name(g, "knows");
  if (vt2_w == GRIN_NULL_VERTEX_TYPE) {
    printf("(Correct) vertex type of knows does not exists\n");
  }
  GRIN_VERTEX_TYPE vt2 = grin_get_vertex_type_by_name(g, "person");
  if (vt2 == GRIN_NULL_VERTEX_TYPE) {
    printf("(Wrong) vertex type of person can not be found\n");
  } else {
    const char* vt2_name = grin_get_vertex_type_name(g, vt2);
    printf("vertex type name: %s\n", vt2_name);
    // grin_destroy_name(g, vt2_name);
  }
#else
  GRIN_VERTEX_TYPE vt2 = get_one_vertex_type(g);
#endif
  grin_insert_vertex_type_to_list(g, vtl2, vt2);
  size_t vtl2_size = grin_get_vertex_type_list_size(g, vtl2);
  printf("created vertex type list size: %zu\n", vtl2_size);
  GRIN_VERTEX_TYPE vt3 = grin_get_vertex_type_from_list(g, vtl2, 0);
  if (!grin_equal_vertex_type(g, vt2, vt3)) {
    printf("vertex type not match\n");
  }
  grin_destroy_vertex_type(g, vt2);
  grin_destroy_vertex_type(g, vt3);
  grin_destroy_vertex_type_list(g, vtl2);

  // edge
  printf("------------ Edge Type ------------\n");
  GRIN_EDGE_TYPE_LIST etl = grin_get_edge_type_list(g);
  size_t etl_size = grin_get_edge_type_list_size(g, etl);
  printf("edge type list size: %zu\n", etl_size);

  for (size_t i = 0; i < etl_size; ++i) {
    printf("------------ Iterate the %zu-th edge type ------------\n", i);
    GRIN_EDGE_TYPE et = grin_get_edge_type_from_list(g, etl, i);
#ifdef GRIN_WITH_EDGE_TYPE_NAME
    const char* et_name = grin_get_edge_type_name(g, et);
    printf("edge type name: %s\n", et_name);
    GRIN_EDGE_TYPE et0 = grin_get_edge_type_by_name(g, et_name);
    // grin_destroy_name(g, et_name);
    if (!grin_equal_edge_type(g, et, et0)) {
      printf("edge type name not match\n");
    }
    grin_destroy_edge_type(g, et0);
#endif
#ifdef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_TYPE
    printf("edge type id: %u\n", grin_get_edge_type_id(g, et));
    GRIN_EDGE_TYPE et1 =
        grin_get_edge_type_by_id(g, grin_get_edge_type_id(g, et));
    if (!grin_equal_edge_type(g, et, et1)) {
      printf("edge type id not match\n");
    }
    grin_destroy_edge_type(g, et1);
#endif
    // relation
    GRIN_VERTEX_TYPE_LIST src_vtl = grin_get_src_types_by_edge_type(g, et);
    size_t src_vtl_size = grin_get_vertex_type_list_size(g, src_vtl);
    printf("source vertex type list size: %zu\n", src_vtl_size);

    GRIN_VERTEX_TYPE_LIST dst_vtl = grin_get_dst_types_by_edge_type(g, et);
    size_t dst_vtl_size = grin_get_vertex_type_list_size(g, dst_vtl);
    printf("destination vertex type list size: %zu\n", dst_vtl_size);

    if (src_vtl_size != dst_vtl_size) {
      printf("source and destination vertex type list size not match\n");
    }
    for (size_t j = 0; j < src_vtl_size; ++j) {
      GRIN_VERTEX_TYPE src_vt = grin_get_vertex_type_from_list(g, src_vtl, j);
      GRIN_VERTEX_TYPE dst_vt = grin_get_vertex_type_from_list(g, dst_vtl, j);
      const char* src_vt_name = grin_get_vertex_type_name(g, src_vt);
      const char* dst_vt_name = grin_get_vertex_type_name(g, dst_vt);
      const char* et_name = grin_get_edge_type_name(g, et);
      printf("edge type name: %s-%s-%s\n", src_vt_name, et_name, dst_vt_name);
      // grin_destroy_name(g, src_vt_name);
      // grin_destroy_name(g, dst_vt_name);
      // grin_destroy_name(g, et_name);
      grin_destroy_vertex_type(g, src_vt);
      grin_destroy_vertex_type(g, dst_vt);
    }
    grin_destroy_vertex_type_list(g, src_vtl);
    grin_destroy_vertex_type_list(g, dst_vtl);
  }
  grin_destroy_edge_type_list(g, etl);

  printf(
      "------------ Create an edge type list of one type \"created\" "
      "------------\n");
  GRIN_EDGE_TYPE_LIST etl2 = grin_create_edge_type_list(g);
#ifdef GRIN_WITH_EDGE_TYPE_NAME
  GRIN_EDGE_TYPE et2_w = grin_get_edge_type_by_name(g, "person");
  if (et2_w == GRIN_NULL_EDGE_TYPE) {
    printf("(Correct) edge type of person does not exists\n");
  }
  GRIN_EDGE_TYPE et2 = grin_get_edge_type_by_name(g, "person_knows_person");
  if (et2 == GRIN_NULL_EDGE_TYPE) {
    printf("(Wrong) edge type of person_knows_person can not be found\n");
  } else {
    const char* et2_name = grin_get_edge_type_name(g, et2);
    printf("edge type name: %s\n", et2_name);
    // grin_destroy_name(g, et2_name);
  }
#else
  GRIN_EDGE_TYPE et2 = get_one_edge_type(g);
#endif
  grin_insert_edge_type_to_list(g, etl2, et2);
  size_t etl2_size = grin_get_edge_type_list_size(g, etl2);
  printf("created edge type list size: %zu\n", etl2_size);
  GRIN_EDGE_TYPE et3 = grin_get_edge_type_from_list(g, etl2, 0);
  if (!grin_equal_edge_type(g, et2, et3)) {
    printf("edge type not match\n");
  }
  grin_destroy_edge_type(g, et2);
  grin_destroy_edge_type(g, et3);
  grin_destroy_edge_type_list(g, etl2);

  grin_destroy_graph(g);
}

void test_property_topology(int argc, char** argv) {
  printf(
      "+++++++++++++++++++++ Test property/topology +++++++++++++++++++++\n");
  GRIN_GRAPH g = get_graph(argc, argv);
  GRIN_VERTEX_TYPE vt = get_one_vertex_type(g);
  GRIN_EDGE_TYPE et = get_one_edge_type(g);
  const char* vt_name = grin_get_vertex_type_name(g, vt);
  const char* et_name = grin_get_edge_type_name(g, et);

#ifdef GRIN_ENABLE_VERTEX_LIST
  GRIN_VERTEX_LIST vl = grin_get_vertex_list(g);

#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
  size_t vl_size = grin_get_vertex_list_size(g, vl);
  printf("vertex list size: %zu\n", vl_size);
#endif

#ifdef GRIN_TRAIT_SELECT_TYPE_FOR_VERTEX_LIST
  GRIN_VERTEX_LIST typed_vl = grin_select_type_for_vertex_list(g, vt, vl);
  size_t typed_vnum = grin_get_vertex_num_by_type(g, vt);

#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
  size_t typed_vl_size = grin_get_vertex_list_size(g, typed_vl);
  printf("vertex number under type: %zu %zu\n", typed_vl_size, typed_vnum);

  for (size_t j = 0; j < typed_vl_size; ++j) {
    GRIN_VERTEX v = grin_get_vertex_from_list(g, typed_vl, j);
    GRIN_VERTEX_TYPE v_type = grin_get_vertex_type(g, v);
    if (!grin_equal_vertex_type(g, v_type, vt)) {
      printf("vertex type not match\n");
    }
    grin_destroy_vertex_type(g, v_type);
    grin_destroy_vertex(g, v);
  }
#else
  GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, typed_vl);
  size_t typed_vl_size = 0;
  while (grin_is_vertex_list_end(g, vli) == false) {
    ++typed_vl_size;
    GRIN_VERTEX v = grin_get_vertex_from_iter(g, vli);
    GRIN_VERTEX_TYPE v_type = grin_get_vertex_type(g, v);
    if (!grin_equal_vertex_type(g, v_type, vt)) {
      printf("vertex type not match\n");
    }
    grin_destroy_vertex_type(g, v_type);
    grin_destroy_vertex(g, v);
    grin_get_next_vertex_list_iter(g, vli);
  }
  printf("vertex number under type: %zu %zu\n", typed_vl_size, typed_vnum);
  grin_destroy_vertex_list_iter(g, vli);
#endif

  grin_destroy_vertex_list(g, typed_vl);
#endif
  grin_destroy_vertex_list(g, vl);
#endif
/*
#ifdef GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_INT64
  GRIN_DATATYPE dt = grin_get_vertex_original_id_datatype(g);
  if (dt == Int64) {
    long long int v0id = 4;
    GRIN_VERTEX v0 = grin_get_vertex_by_original_id_of_int64(g, v0id);
    if (v0 == GRIN_NULL_VERTEX) {
      printf("(Wrong) vertex of id %lld can not be found\n", v0id);
    } else {
      printf("vertex of original id %lld found\n", v0id);
      long long int oid0 = grin_get_vertex_original_id_of_int64(g, v0);
      printf("get vertex original id: %lld\n", oid0);
    }
    grin_destroy_vertex(g, v0);
  } else {
    printf("(Wrong) vertex original id type not int64\n");
  }
#endif
*/
#ifdef GRIN_ENABLE_EDGE_LIST
  GRIN_EDGE_LIST el = grin_get_edge_list(g);
#ifdef GRIN_ENABLE_EDGE_LIST_ARRAY
  size_t el_size = grin_get_edge_list_size(g, el);
  printf("edge list size: %zu\n", el_size);
#endif

#ifdef GRIN_TRAIT_SELECT_TYPE_FOR_EDGE_LIST
  GRIN_EDGE_LIST typed_el = grin_select_type_for_edge_list(g, et, el);
  size_t typed_enum = grin_get_edge_num_by_type(g, et);

#ifdef GRIN_ENABLE_EDGE_LIST_ARRAY
  size_t typed_el_size = grin_get_edge_list_size(g, typed_el);
  printf("edge number under type: %zu %zu\n", typed_el_size, typed_enum);

  for (size_t j = 0; j < typed_el_size; ++j) {
    GRIN_EDGE e = grin_get_edge_from_list(g, typed_el, j);
    GRIN_EDGE_TYPE e_type = grin_get_edge_type(g, e);
    if (!grin_equal_edge_type(g, e_type, et)) {
      printf("edge type not match\n");
    }
    grin_destroy_edge_type(g, e_type);
    grin_destroy_edge(g, e);
  }
#else
  GRIN_EDGE_LIST_ITERATOR eli = grin_get_edge_list_begin(g, typed_el);
  size_t typed_el_size = 0;
  while (grin_is_edge_list_end(g, eli) == false) {
    ++typed_el_size;
    GRIN_EDGE e = grin_get_edge_from_iter(g, eli);
    GRIN_EDGE_TYPE e_type = grin_get_edge_type(g, e);
    if (!grin_equal_edge_type(g, e_type, et)) {
      printf("edge type not match\n");
    }
    grin_destroy_edge_type(g, e_type);
    grin_destroy_edge(g, e);
    grin_get_next_edge_list_iter(g, eli);
  }
  printf("edge number under type: %zu %zu\n", typed_el_size, typed_enum);
  grin_destroy_edge_list_iter(g, eli);
#endif

  grin_destroy_edge_list(g, typed_el);
#endif
  grin_destroy_edge_list(g, el);
#endif

  // grin_destroy_name(g, vt_name);
  // grin_destroy_name(g, et_name);
  grin_destroy_vertex_type(g, vt);
  grin_destroy_edge_type(g, et);
  grin_destroy_graph(g);
}

void test_property_vertex_property_value(int argc, char** argv) {
  printf("------------ Test Vertex property value ------------\n");
  GRIN_GRAPH g = get_graph(argc, argv);

  GRIN_VERTEX_TYPE_LIST vtl = grin_get_vertex_type_list(g);
  size_t vtl_size = grin_get_vertex_type_list_size(g, vtl);
  for (size_t vt_index = 0; vt_index < vtl_size; ++vt_index) {
    GRIN_VERTEX_TYPE vt = grin_get_vertex_type_from_list(g, vtl, vt_index);

    GRIN_VERTEX_PROPERTY_LIST vpl =
        grin_get_vertex_property_list_by_type(g, vt);

#ifdef GRIN_TRAIT_SELECT_MASTER_FOR_VERTEX_LIST
    GRIN_VERTEX_LIST all_vl = grin_get_vertex_list(g);
    GRIN_VERTEX_LIST vl = grin_select_master_for_vertex_list(g, all_vl);
#else
    GRIN_VERTEX_LIST vl = grin_get_vertex_list(g);
#endif
    GRIN_VERTEX_LIST typed_vl = grin_select_type_for_vertex_list(g, vt, vl);
#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
    size_t typed_vl_size = grin_get_vertex_list_size(g, typed_vl);
#else
    size_t typed_vl_size = grin_get_vertex_num_by_type(g, vt);
#endif
    size_t vpl_size = grin_get_vertex_property_list_size(g, vpl);
    printf("vertex list size: %zu vertex property list size: %zu\n",
           typed_vl_size, vpl_size);

#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
    for (size_t i = 0; i < typed_vl_size; ++i) {
      GRIN_VERTEX v = grin_get_vertex_from_list(g, typed_vl, i);
#else
    GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, typed_vl);
    size_t i = 0;
    while (grin_is_vertex_list_end(g, vli) == 0) {
      GRIN_VERTEX v = grin_get_vertex_from_iter(g, vli);
#endif
      GRIN_ROW row = grin_get_vertex_row(g, v);
      for (size_t j = 0; j < vpl_size; ++j) {
        GRIN_VERTEX_PROPERTY vp = grin_get_vertex_property_from_list(g, vpl, j);
        GRIN_VERTEX_TYPE vt1 = grin_get_vertex_type_from_property(g, vp);
        if (!grin_equal_vertex_type(g, vt, vt1)) {
          printf("vertex type not match by property\n");
        }
        grin_destroy_vertex_type(g, vt1);
#ifdef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_PROPERTY
        unsigned int id = grin_get_vertex_property_id(g, vt, vp);
        GRIN_VERTEX_PROPERTY vp1 = grin_get_vertex_property_by_id(g, vt, id);
        if (!grin_equal_vertex_property(g, vp, vp1)) {
          printf("vertex property not match by id\n");
        }
        grin_destroy_vertex_property(g, vp1);
#else
        unsigned int id = ~0;
#endif

#ifdef GRIN_WITH_VERTEX_PROPERTY_NAME
        const char* vp_name = grin_get_vertex_property_name(g, vt, vp);
        GRIN_VERTEX_PROPERTY vp2 =
            grin_get_vertex_property_by_name(g, vt, vp_name);
        if (!grin_equal_vertex_property(g, vp, vp2)) {
          printf("vertex property not match by name\n");
        }
#else
        const char* vp_name = "unknown";
#endif
        GRIN_DATATYPE dt = grin_get_vertex_property_datatype(g, vp);
#ifdef GRIN_TRAIT_CONST_VALUE_PTR
        const void* pv = grin_get_vertex_property_value(g, v, vp);
        if (grin_get_last_error_code() == NO_ERROR) {
          printf("(Correct) no error\n");
        } else {
          printf("(Wrong) error code: %d\n", grin_get_last_error_code());
        }
        const void* rv = grin_get_value_from_row(g, row, dt, j);
        if (dt == Int64) {
          printf("vp_id %u v%zu %s value int64: %ld %ld\n", id, i, vp_name,
                 *((long int*) pv), *((long int*) rv));
        } else if (dt == String) {
          printf("vp_id %u v%zu %s value string: %s %s\n", id, i, vp_name,
                 (char*) pv, (char*) rv);
        } else if (dt == Int32) {
          printf("vp_id %u v%zu %s value int32: %d %d\n", id, i, vp_name,
                 *((int*) pv), *((int*) rv));
        }
        // grin_destroy_value(g, dt, pv);
        // grin_destroy_value(g, dt, rv);
#else
        if (dt == Int64) {
          long long int pv = grin_get_vertex_property_value_of_int64(g, v, vp);
          if (grin_get_last_error_code() == NO_ERROR) {
            printf("(Correct) no error\n");
          } else {
            printf("(Wrong) error code: %d\n", grin_get_last_error_code());
          }
          long long int rv = grin_get_int64_from_row(g, row, j);
          printf("vp_id %u v%zu %s value: %lld %lld\n", id, i, vp_name, pv, rv);
        } else if (dt == String) {
          const char* pv = grin_get_vertex_property_value_of_string(g, v, vp);
          if (grin_get_last_error_code() == NO_ERROR) {
            printf("(Correct) no error\n");
          } else {
            printf("(Wrong) error code: %d\n", grin_get_last_error_code());
          }
          const char* rv = grin_get_string_from_row(g, row, j);
          printf("vp_id %u v%zu %s value: %s %s\n", id, i, vp_name, pv, rv);
          grin_destroy_string_value(g, pv);
          grin_destroy_string_value(g, rv);
        }
#endif
        grin_destroy_vertex_property(g, vp);
      }
      grin_destroy_row(g, row);
      grin_destroy_vertex(g, v);
#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
    }
#else
      grin_get_next_vertex_list_iter(g, vli);
      ++i;
    }
    grin_destroy_vertex_list_iter(g, vli);
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_PROPERTY
    GRIN_VERTEX_PROPERTY vp3 = grin_get_vertex_property_by_id(g, vt, vpl_size);
    if (vp3 == GRIN_NULL_VERTEX_PROPERTY) {
      printf("(Correct) vertex property of id %zu does not exist\n", vpl_size);
    } else {
      printf("(Wrong) vertex property of id %zu exists\n", vpl_size);
      grin_destroy_vertex_property(g, vp3);
    }
#endif

#ifdef GRIN_WITH_VERTEX_PROPERTY_NAME
    GRIN_VERTEX_PROPERTY vp4 =
        grin_get_vertex_property_by_name(g, vt, "unknown");
    if (vp4 == GRIN_NULL_VERTEX_PROPERTY) {
      printf("(Correct) vertex property of name \"unknown\" does not exist\n");
    } else {
      printf("(Wrong) vertex property of name \"unknown\" exists\n");
      grin_destroy_vertex_property(g, vp4);
    }

    GRIN_VERTEX_PROPERTY_LIST vpl1 =
        grin_get_vertex_properties_by_name(g, "unknown");
    if (vpl1 == GRIN_NULL_LIST) {
      printf(
          "(Correct) vertex properties of name \"unknown\" does not exist\n");
    } else {
      printf("(Wrong) vertex properties of name \"unknown\" exists\n");
      grin_destroy_vertex_property_list(g, vpl1);
    }

    GRIN_VERTEX_PROPERTY_LIST vpl2 =
        grin_get_vertex_properties_by_name(g, "person_name");
    if (vpl2 == GRIN_NULL_LIST) {
      printf(
          "(Wrong) vertex properties of name \"person_name\" does not exist\n");
    } else {
      printf("(Correct) vertex properties of name \"person_name\" exists\n");
      size_t vpl2_size = grin_get_vertex_property_list_size(g, vpl2);
      for (size_t i = 0; i < vpl2_size; ++i) {
        GRIN_VERTEX_PROPERTY vp5 =
            grin_get_vertex_property_from_list(g, vpl2, i);
        GRIN_VERTEX_TYPE vt5 = grin_get_vertex_type_from_property(g, vp5);
        const char* vp5_name = grin_get_vertex_property_name(g, vt5, vp5);
        const char* vt5_name = grin_get_vertex_type_name(g, vt5);
        printf("vertex type name: %s, vertex property name: %s\n", vt5_name,
               vp5_name);
        grin_destroy_vertex_property(g, vp5);
        grin_destroy_vertex_type(g, vt5);
        // grin_destroy_name(g, vt5_name);
        // grin_destroy_name(g, vp5_name);
      }
      grin_destroy_vertex_property_list(g, vpl2);
    }
#endif

    grin_destroy_vertex_list(g, typed_vl);
    grin_destroy_vertex_list(g, vl);
    grin_destroy_vertex_property_list(g, vpl);
  }
  grin_destroy_vertex_type_list(g, vtl);
  grin_destroy_graph(g);
}

void test_property_edge_property_value(int argc, char** argv) {
  printf("------------ Test Edge property value ------------\n");
  GRIN_GRAPH g = get_graph(argc, argv);
  // edge
  GRIN_VERTEX v = get_vertex_marco(g);
  GRIN_VERTEX_TYPE vt = grin_get_vertex_type(g, v);
  GRIN_ADJACENT_LIST al = grin_get_adjacent_list(g, OUT, v);
#ifdef GRIN_ENABLE_ADJACENT_LIST_ARRAY
  printf("adjacent list size: %zu\n", grin_get_adjacent_list_size(g, al));
#endif

  GRIN_EDGE_TYPE_LIST etl = grin_get_edge_type_list(g);
  size_t etl_size = grin_get_edge_type_list_size(g, etl);
  printf("edge type list size: %zu\n", etl_size);

  for (size_t i = 0; i < etl_size; ++i) {
    GRIN_EDGE_TYPE et = grin_get_edge_type_from_list(g, etl, i);
    GRIN_EDGE_PROPERTY_LIST epl = grin_get_edge_property_list_by_type(g, et);
    size_t epl_size = grin_get_edge_property_list_size(g, epl);
    printf("edge property list size: %zu\n", epl_size);

#ifdef GRIN_TRAIT_SELECT_EDGE_TYPE_FOR_ADJACENT_LIST
    GRIN_ADJACENT_LIST al1 = grin_select_edge_type_for_adjacent_list(g, et, al);

#ifdef GRIN_ENABLE_ADJACENT_LIST_ARRAY
    size_t al1_size = grin_get_adjacent_list_size(g, al1);
    printf("selected adjacent list size: %zu\n", al1_size);
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST_ARRAY
    for (size_t j = 0; j < al1_size; ++j) {
      GRIN_EDGE e = grin_get_edge_from_adjacent_list(g, al1, j);
#else
    GRIN_ADJACENT_LIST_ITERATOR ali = grin_get_adjacent_list_begin(g, al1);
    size_t j = 0;
    while (grin_is_adjacent_list_end(g, ali) == false) {
      GRIN_EDGE e = grin_get_edge_from_adjacent_list_iter(g, ali);
#endif
      GRIN_EDGE_TYPE et1 = grin_get_edge_type(g, e);
      if (!grin_equal_edge_type(g, et, et1)) {
        printf("edge type does not match\n");
      }

      GRIN_ROW row = grin_get_edge_row(g, e);
      for (size_t k = 0; k < epl_size; ++k) {
        GRIN_EDGE_PROPERTY ep = grin_get_edge_property_from_list(g, epl, k);
        GRIN_EDGE_TYPE et2 = grin_get_edge_type_from_property(g, ep);
        if (!grin_equal_edge_type(g, et, et2)) {
          printf("edge type does not match\n");
        }
        grin_destroy_edge_type(g, et2);

        const char* ep_name = grin_get_edge_property_name(g, et, ep);
        printf("edge property name: %s\n", ep_name);

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_PROPERTY
        unsigned int id = grin_get_edge_property_id(g, et, ep);
        GRIN_EDGE_PROPERTY ep1 = grin_get_edge_property_by_id(g, et, id);
        if (!grin_equal_edge_property(g, ep, ep1)) {
          printf("edge property not match by id\n");
        }
        grin_destroy_edge_property(g, ep1);
#else
        unsigned int id = ~0;
#endif
        GRIN_DATATYPE dt = grin_get_edge_property_datatype(g, ep);
#ifdef GRIN_TRAIT_CONST_VALUE_PTR
        const void* pv = grin_get_edge_property_value(g, e, ep);
        const void* rv = grin_get_value_from_row(g, row, dt, k);

        if (dt == Int64) {
          printf("ep_id %u e%zu %s value: %ld %ld\n", id, j, ep_name,
                 *((long int*) pv), *((long int*) rv));
        } else if (dt == String) {
          printf("ep_id %u e%zu %s value: %s %s\n", id, j, ep_name, (char*) pv,
                 (char*) rv);
        } else if (dt == Double) {
          printf("ep_id %u e%zu %s value: %f %f\n", id, j, ep_name,
                 *((double*) pv), *((double*) rv));
        }
        grin_destroy_edge_property(g, ep);
        // grin_destroy_name(g, ep_name);
        // grin_destroy_value(g, dt, pv);
        // grin_destroy_value(g, dt, rv);
#else
        if (dt == Int64) {
          long long int pv = grin_get_edge_property_value_of_int64(g, e, ep);
          long long int rv = grin_get_int64_from_row(g, row, k);
          printf("ep_id %u e%zu %s value: %lld %lld\n", id, j, ep_name, pv, rv);
        } else if (dt == String) {
          const char* pv = grin_get_edge_property_value_of_string(g, e, ep);
          const char* rv = grin_get_string_from_row(g, row, k);
          printf("ep_id %u e%zu %s value: %s %s\n", id, j, ep_name, pv, rv);
          grin_destroy_string_value(g, pv);
          grin_destroy_string_value(g, rv);
        } else if (dt == Double) {
          double pv = grin_get_edge_property_value_of_double(g, e, ep);
          double rv = grin_get_double_from_row(g, row, k);
          printf("ep_id %u e%zu %s value: %f %f\n", id, j, ep_name, pv, rv);
        }
#endif
        // grin_destroy_name(g, ep_name);
        // grin_destroy_value(g, dt, pv);
        // grin_destroy_value(g, dt, rv);
      }

      grin_destroy_row(g, row);
      grin_destroy_edge_type(g, et1);
      grin_destroy_edge(g, e);

#ifdef GRIN_ENABLE_ADJACENT_LIST_ARRAY
    }
#else
      grin_get_next_adjacent_list_iter(g, ali);
      ++j;
    }
    grin_destroy_adjacent_list_iter(g, ali);
#endif

    grin_destroy_adjacent_list(g, al1);
#endif

    for (size_t j = 0; j < epl_size; ++j) {
      GRIN_EDGE_PROPERTY ep = grin_get_edge_property_from_list(g, epl, j);
      GRIN_EDGE_TYPE et1 = grin_get_edge_type_from_property(g, ep);
      if (!grin_equal_edge_type(g, et, et1)) {
        printf("edge type does not match\n");
      }
      const char* ep_name1 = grin_get_edge_property_name(g, et, ep);
      const char* et_name = grin_get_edge_type_name(g, et);
      printf("edge property name: %s, edge property type name: %s\n", ep_name1,
             et_name);

      grin_destroy_edge_type(g, et1);
      // grin_destroy_name(g, ep_name1);
      // grin_destroy_name(g, et_name);

#ifdef GRIN_WITH_EDGE_PROPERTY_NAME
      const char* ep_name = grin_get_edge_property_name(g, et, ep);
      GRIN_EDGE_PROPERTY ep2 = grin_get_edge_property_by_name(g, et, ep_name);
      if (!grin_equal_edge_property(g, ep, ep2)) {
        printf("edge property not match by name\n");
      }
#else
      const char* ep_name = "unknown";
#endif
      grin_destroy_edge_property(g, ep);
    }
#ifdef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_PROPERTY
    GRIN_EDGE_PROPERTY ep3 = grin_get_edge_property_by_id(g, et, epl_size);
    if (ep3 == GRIN_NULL_EDGE_PROPERTY) {
      printf("(Correct) edge property of id %zu does not exist\n", epl_size);
    } else {
      printf("(Wrong) edge property of id %zu exists\n", epl_size);
      grin_destroy_edge_property(g, ep3);
    }
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY_NAME
    GRIN_EDGE_PROPERTY ep4 = grin_get_edge_property_by_name(g, et, "unknown");
    if (ep4 == GRIN_NULL_EDGE_PROPERTY) {
      printf("(Correct) edge property of name \"unknown\" does not exist\n");
    } else {
      printf("(Wrong) edge property of name \"unknown\" exists\n");
      grin_destroy_edge_property(g, ep4);
    }

    GRIN_EDGE_PROPERTY_LIST epl1 =
        grin_get_edge_properties_by_name(g, "unknown");
    if (epl1 == GRIN_NULL_LIST) {
      printf("(Correct) edge properties of name \"unknown\" does not exist\n");
    } else {
      printf("(Wrong) edge properties of name \"unknown\" exists\n");
      grin_destroy_edge_property_list(g, epl1);
    }

    GRIN_EDGE_PROPERTY_LIST epl2 =
        grin_get_edge_properties_by_name(g, "person_knows_person_prop");
    if (epl2 == GRIN_NULL_LIST) {
      printf(
          "(Wrong) edge properties of name \"person_knows_person_prop\" does "
          "not exist\n");
    } else {
      printf(
          "(Correct) edge properties of name \"person_knows_person_prop\" "
          "exists\n");
      size_t epl2_size = grin_get_edge_property_list_size(g, epl2);
      for (size_t i = 0; i < epl2_size; ++i) {
        GRIN_EDGE_PROPERTY ep5 = grin_get_edge_property_from_list(g, epl2, i);
        GRIN_EDGE_TYPE et5 = grin_get_edge_type_from_property(g, ep5);
        const char* ep5_name = grin_get_edge_property_name(g, et5, ep5);
        const char* et5_name = grin_get_edge_type_name(g, et5);
        printf("edge type name: %s, edge property name: %s\n", et5_name,
               ep5_name);
        grin_destroy_edge_property(g, ep5);
        grin_destroy_edge_type(g, et5);
        // grin_destroy_name(g, et5_name);
        // grin_destroy_name(g, ep5_name);
      }
      grin_destroy_edge_property_list(g, epl2);
    }
#endif
    grin_destroy_edge_type(g, et);
  }

  grin_destroy_vertex(g, v);
  grin_destroy_vertex_type(g, vt);
  grin_destroy_adjacent_list(g, al);
  grin_destroy_edge_type_list(g, etl);
  grin_destroy_graph(g);
}

#ifdef GRIN_ENABLE_VERTEX_PRIMARY_KEYS
void test_property_primary_key(int argc, char** argv) {
  printf(
      "+++++++++++++++++++++ Test property/primary key "
      "+++++++++++++++++++++\n");
  GRIN_GRAPH g = get_graph(argc, argv);
  GRIN_VERTEX_TYPE_LIST vtl = grin_get_vertex_types_with_primary_keys(g);
  size_t vtl_size = grin_get_vertex_type_list_size(g, vtl);
  printf("vertex type list size: %zu\n", vtl_size);

  unsigned id_type[7] = {~0, 0, 0, 1, 0, 1, 0};

  for (size_t i = 0; i < vtl_size; ++i) {
    GRIN_VERTEX_TYPE vt = grin_get_vertex_type_from_list(g, vtl, i);
    const char* vt_name = grin_get_vertex_type_name(g, vt);
    printf("vertex type name: %s\n", vt_name);
    // grin_destroy_name(g, vt_name);

    GRIN_VERTEX_PROPERTY_LIST vpl = grin_get_primary_keys_by_vertex_type(g, vt);
    size_t vpl_size = grin_get_vertex_property_list_size(g, vpl);
    printf("primary key list size: %zu\n", vpl_size);

    for (size_t j = 0; j < vpl_size; ++j) {
      GRIN_VERTEX_PROPERTY vp = grin_get_vertex_property_from_list(g, vpl, j);
      const char* vp_name = grin_get_vertex_property_name(g, vt, vp);
      printf("primary key name: %s\n", vp_name);
      // grin_destroy_name(g, vp_name);
      grin_destroy_vertex_property(g, vp);
    }

    GRIN_VERTEX_PROPERTY vp = grin_get_vertex_property_from_list(g, vpl, 0);
    GRIN_DATATYPE dt = grin_get_vertex_property_datatype(g, vp);

    for (size_t j = 1; j <= 6; ++j) {
      GRIN_ROW r = grin_create_row(g);
      if (dt == Int64) {
        grin_insert_int64_to_row(g, r, j);
      } else {
        printf("(Wrong) the primary key type is not int64");
      }
      GRIN_VERTEX v = grin_get_vertex_by_primary_keys(g, vt, r);
      if (id_type[j] == i) {
        if (v == GRIN_NULL_VERTEX) {
          printf("(Wrong) vertex of primary keys %zu does not exist\n", j);
        } else {
          grin_destroy_vertex(g, v);
        }
      } else {
        if (v == GRIN_NULL_VERTEX) {
          printf("(Correct) vertex of primary keys %zu does not exist\n", j);
        } else {
          printf("(Wrong) vertex of primary keys %zu exists\n", j);
          grin_destroy_vertex(g, v);
        }
      }
      grin_destroy_row(g, r);
    }

    grin_destroy_vertex_property(g, vp);
    grin_destroy_vertex_property_list(g, vpl);
    grin_destroy_vertex_type(g, vt);
  }
}
#endif

void test_error_code(int argc, char** argv) {
  printf("+++++++++++++++++++++ Test error code +++++++++++++++++++++\n");
  GRIN_GRAPH g = get_graph(argc, argv);

  GRIN_VERTEX_TYPE vt1 = grin_get_vertex_type_by_name(g, "person");
  GRIN_VERTEX_TYPE vt2 = grin_get_vertex_type_by_name(g, "post");
  GRIN_VERTEX_PROPERTY vp =
      grin_get_vertex_property_by_name(g, vt2, "post_topic");
  GRIN_VERTEX v = get_one_vertex(g);

#ifdef GRIN_TRAIT_CONST_VALUE_PTR
  const void* value = grin_get_vertex_property_value(g, v, vp);
#else
  const char* value = grin_get_vertex_property_value_of_string(g, v, vp);
#endif
  if (grin_get_last_error_code() == INVALID_VALUE) {
    printf("(Correct) invalid value\n");
  } else {
    printf("(Wrong) error code: %d\n", grin_get_last_error_code());
  }
}

void test_property(int argc, char** argv) {
  test_property_type(argc, argv);
  test_property_topology(argc, argv);
  test_property_vertex_property_value(argc, argv);
  test_property_edge_property_value(argc, argv);
#ifdef GRIN_ENABLE_VERTEX_PRIMARY_KEYS
  test_property_primary_key(argc, argv);
#endif
  test_error_code(argc, argv);
}

void test_partition_reference(int argc, char** argv) {
  printf(
      "+++++++++++++++++++++ Test partition/reference +++++++++++++++++++++\n");
  GRIN_GRAPH g = get_graph(argc, argv);
  GRIN_PARTITION p0 = get_partition(argc, argv);

#ifdef GRIN_TRAIT_SELECT_MASTER_FOR_VERTEX_LIST
  GRIN_VERTEX_LIST vlist = grin_get_vertex_list(g);
  GRIN_VERTEX_LIST mvlist = grin_select_master_for_vertex_list(g, vlist);
  GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, mvlist);
  grin_destroy_vertex_list(g, vlist);
#else
  GRIN_VERTEX_LIST mvlist = grin_get_vertex_list(g);
  GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, mvlist);
#endif

  size_t cnt = 0;
  size_t mcnt = 0;
  while (!grin_is_vertex_list_end(g, vli)) {
    cnt++;
    GRIN_VERTEX v = grin_get_vertex_from_iter(g, vli);
    GRIN_VERTEX_REF vref = grin_get_vertex_ref_by_vertex(g, v);
#ifdef GRIN_TRAIT_FAST_VERTEX_REF
    long long int sref = grin_serialize_vertex_ref_as_int64(g, vref);
    GRIN_VERTEX_REF vref1 = grin_deserialize_int64_to_vertex_ref(g, sref);
#else
    const char* sref = grin_serialize_vertex_ref(g, vref);
    GRIN_VERTEX_REF vref1 = grin_deserialize_vertex_ref(g, sref);
    grin_destroy_string_value(g, sref);
#endif
    GRIN_VERTEX v1 = grin_get_vertex_from_vertex_ref(g, vref1);
    if (!grin_equal_vertex(g, v, v1)) {
      printf("vertex not match\n");
    }

    if (grin_is_master_vertex(g, v)) {
      mcnt++;
      GRIN_PARTITION p = grin_get_master_partition_from_vertex_ref(g, vref);
      if (!grin_equal_partition(g, p, p0)) {
        printf("(Wrong) partition not match\n");
      }
    } else if (grin_is_mirror_vertex(g, v)) {
      GRIN_PARTITION p = grin_get_master_partition_from_vertex_ref(g, vref);
      if (grin_equal_partition(g, p, p0)) {
        printf("(Wrong) partition match\n");
      }
    } else {
      printf("(Wrong) vertex other than master or mirror\n");
    }

    grin_destroy_vertex_ref(g, vref);
    grin_destroy_vertex(g, v);
    grin_get_next_vertex_list_iter(g, vli);
  }
  printf("num of vertex checked: %zu\n", cnt);

#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
  size_t mvlist_size = grin_get_vertex_list_size(g, mvlist);
  if (mvlist_size != mcnt) {
    printf("(Wrong) master vertex list size not match\n");
  } else {
    printf("Master vertex number: %zu\n", mcnt);
  }
#endif

  grin_destroy_vertex_list(g, mvlist);
  grin_destroy_graph(g);
}

void test_partition(int argc, char** argv) {
#ifdef GRIN_ENABLE_GRAPH_PARTITION
  test_partition_reference(argc, argv);
#endif
}

void test_topology_structure(int argc, char** argv) {
  GRIN_GRAPH g = get_graph(argc, argv);

  printf("vnum: %zu, enum: %zu\n", grin_get_vertex_num(g),
         grin_get_edge_num(g));

  grin_destroy_graph(g);
}

void test_topology_adjacent_list(int argc, char** argv, GRIN_DIRECTION dir) {
  printf(
      "+++++++++++++++++++++ Test topology/adjacent_list "
      "+++++++++++++++++++++\n");
  GRIN_GRAPH g = get_graph(argc, argv);

  GRIN_VERTEX_LIST vl = grin_get_vertex_list(g);
  GRIN_VERTEX_LIST_ITERATOR vli = grin_get_vertex_list_begin(g, vl);
  grin_destroy_vertex_list(g, vl);

  GRIN_EDGE_TYPE_LIST etl = grin_get_edge_type_list(g);
  size_t etl_size = grin_get_edge_type_list_size(g, etl);

  while (!grin_is_vertex_list_end(g, vli)) {
    GRIN_VERTEX v = grin_get_vertex_from_iter(g, vli);
#ifdef GRIN_ENABLE_GRAPH_PARTITION
    if (!grin_is_master_vertex(g, v)) {
      grin_destroy_vertex(g, v);
      grin_get_next_vertex_list_iter(g, vli);
      continue;
    }
#endif
    GRIN_ADJACENT_LIST al = grin_get_adjacent_list(g, dir, v);
    for (size_t i = 0; i <= etl_size; ++i) {
      GRIN_ADJACENT_LIST al1 = al;
      if (i < etl_size) {
        GRIN_EDGE_TYPE et = grin_get_edge_type_from_list(g, etl, i);
        al1 = grin_select_edge_type_for_adjacent_list(g, et, al);
        grin_destroy_edge_type(g, et);
      }

      GRIN_ADJACENT_LIST_ITERATOR ali = grin_get_adjacent_list_begin(g, al1);
      grin_destroy_adjacent_list(g, al1);

      size_t cnt = 0;
      while (!grin_is_adjacent_list_end(g, ali)) {
        cnt++;
        GRIN_EDGE e = grin_get_edge_from_adjacent_list_iter(g, ali);
        GRIN_VERTEX v1 = grin_get_src_vertex_from_edge(g, e);
        GRIN_VERTEX v2 = grin_get_dst_vertex_from_edge(g, e);
        GRIN_VERTEX u = grin_get_neighbor_from_adjacent_list_iter(g, ali);

        if (dir == OUT) {
          if (!grin_equal_vertex(g, v, v1)) {
            printf("vertex not match\n");
          }
          if (!grin_equal_vertex(g, v2, u)) {
            printf("vertex not match\n");
          }
        } else {
          if (!grin_equal_vertex(g, v, v2)) {
            printf("vertex not match\n");
          }
          if (!grin_equal_vertex(g, v1, u)) {
            printf("vertex not match\n");
          }
        }

        grin_destroy_vertex(g, v1);
        grin_destroy_vertex(g, v2);
        grin_destroy_vertex(g, u);
        grin_destroy_edge(g, e);
        grin_get_next_adjacent_list_iter(g, ali);
      }
#ifdef GRIN_ENABLE_VERTEX_ORIGINAL_ID_OF_INT64
      long long int vid = grin_get_vertex_original_id_of_int64(g, v);
      if (dir == OUT) {
        if (i < etl_size) {
          printf(
              "vertex %lld OUT adjacent list, edgetype: %zu checked num: %zu\n",
              vid, i, cnt);
        } else {
          printf(
              "vertex %lld OUT adjacent list, edgetype: all checked num: %zu\n",
              vid, cnt);
        }
      } else {
        if (i < etl_size) {
          printf(
              "vertex %lld IN adjacent list, edgetype: %zu checked num: %zu\n",
              vid, i, cnt);
        } else {
          printf(
              "vertex %lld IN adjacent list, edgetype: all checked num: %zu\n",
              vid, cnt);
        }
      }
#endif

      grin_destroy_adjacent_list_iter(g, ali);
    }
    grin_destroy_vertex(g, v);
    grin_get_next_vertex_list_iter(g, vli);
  }

  grin_destroy_graph(g);
}

void test_topology(int argc, char** argv) {
  test_topology_structure(argc, argv);
  test_topology_adjacent_list(argc, argv, OUT);
  test_topology_adjacent_list(argc, argv, IN);
}

int main(int argc, char** argv) {
  grape::InitMPIComm();
  grape::CommSpec comm_spec;
  comm_spec.Init(MPI_COMM_WORLD);
  int read_epoch = 4;
  std::string etcd_endpoint = "http://127.0.0.1:23799";
  std::string meta_prefix = "gart_meta_";
  char** argv_new = new char*[5];
  argv_new[0] = new char[etcd_endpoint.length() + 1];
  argv_new[1] = new char[std::to_string(comm_spec.fnum()).length() + 1];
  argv_new[2] = new char[std::to_string(comm_spec.fid()).length() + 1];
  argv_new[3] = new char[std::to_string(read_epoch).length() + 1];
  argv_new[4] = new char[meta_prefix.length() + 1];

  strcpy(argv_new[0], etcd_endpoint.c_str());
  strcpy(argv_new[1], std::to_string(comm_spec.fnum()).c_str());
  strcpy(argv_new[2], std::to_string(comm_spec.fid()).c_str());
  strcpy(argv_new[3], std::to_string(read_epoch).c_str());
  strcpy(argv_new[4], meta_prefix.c_str());

  test_property(6, argv_new);
  test_partition(6, argv_new);
  test_topology(6, argv_new);
  return 0;
}