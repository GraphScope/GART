RGMapping Configuration
=========================

This section will specifically cover the definition rules in RGMapping. If you are not familiar with RGMapping, please refer to `this section <../design/rgmapping.html>`_.

SQL/PGQ
^^^^^^^

In the context of transitioning from relational models to graph models, the process often involves mapping tables and their relationships into nodes (vertices) and edges within a property graph. The syntax provided seems to be an illustrative example of how one might define this transformation using a hypothetical SQL-like or extended SQL syntax, tailored specifically for creating property graphs. Let's break down the conceptual structure behind such a conversion rule definition, though please note that the exact syntax you've shown is not standard SQL but appears to be inspired by extensions like PostgreSQL's PGQ or other graph database management systems.

The RGMapping rule can be written in `SQL/PGQ`_.

.. code-block:: postgresql
    :linenos:

    CREATE PROPERTY GRAPH ldbc
    VERTEX TABLES (
        "PERSON"
        KEY ( "p_id" )
        LABEL "person" PROPERTIES ( p_id AS "p_id", name AS "p_name" )
    )
    EDGE TABLES (
        "TRANSFER"
        SOURCE KEY ( "P_ID1" ) REFERENCES "PERSON"
        DESTINATION KEY ( "P_ID2" ) REFERENCES "PERSON"
        LABEL "transfer" PROPERTIES ( t_data AS "t_date" )
    )

When defining the rules for converting a relational schema to a property graph model, several key components are typically addressed:

1. Graph Declaration

    - This initiates the creation of a new graph database object.
      Example: ``CREATE PROPERTY GRAPH ldbc``

2. Vertex Tables Definition

    - **Table Naming**: Specifies which relational tables will be represented as vertex types in the graph.
      Example: ``"PERSON"``

    - **Key Definition**: Determines the primary key column(s) of the table which will serve as the unique identifier (ID) for vertices.
      Example: ``KEY ( "p_id" )``

    - **Label Assignment**: Assigns a label to the vertex type, reflecting its role or category in the graph.
      Example: ``LABEL "person"``

    - **Properties Mapping**: Maps columns from the relational table to properties on the vertex, maintaining data integrity.
      Example: ``PROPERTIES ( p_id AS "p_id", name AS "p_name" )``

3. Edge Tables Definition

    - **Table Naming**: Identifies the relational table representing relationships between vertices.
      Example: ``"TRANSFER"``

    - **Source and Destination Key Definition**: Specifies the columns in the edge table that refer to the start (source) and end (destination) vertices of the edge, usually referencing the primary keys of the vertex tables.
      Example: ``SOURCE KEY ( "P_ID1" ) REFERENCES "PERSON" and DESTINATION KEY ( "P_ID2" ) REFERENCES "PERSON"``

    - **Label Assignment**: Assign a label to the edge type, describing the nature of the relationship.
      Example: ``LABEL "transfer"``

    - **Properties Mapping**: Maps additional columns to properties on the edge, capturing details about the relationship.
      Example: ``PROPERTIES ( t_data AS "t_date" )``

YAML
^^^^

The RGMapping rule can also be written in `YAML`_.
The examples we give are equivalent conversion conditions to those described above for SQL/PGQ.

.. code-block:: yaml
    :linenos:

    !!gart.pgql.GSchema
    graph: ldbc
    database: ldbc
    enableRowStore: false
    vertexMappings:
        vertex_types:
            - type_name: person
              dataSourceName: PERSON
              idFieldName: p_id
              mappings:
                - property: p_id
                  dataField:
                      name: P_ID
                - property: p_name
                  dataField:
                      name: NAME
    edgeMappings:
        edge_types:
            - type_pair:
                edge: transfer
                source_vertex: person
                destination_vertex: person
              dataSourceName: TRANSFER
              sourceVertexMappings:
                - dataField:
                      name: P_ID1
              destinationVertexMappings:
                - dataField:
                      name: P_ID2
              dataFieldMappings:
                - property: t_date
                  dataField:
                      name: T_DATA

1. Graph Declaration

    - This initiates the creation of a new graph database object.
      Example: ``graph: ldbc``

2. Vertex Tables Definition (``vertexMappings.vertex_types``)

    - **Table Naming**: Specifies which relational tables will be represented as vertex types in the graph.
      Example: ``dataSourceName``

    - **Key Definition**: Determines the primary key column(s) of the table which will serve as the unique identifier (ID) for vertices.
      Example: ``idFieldName``

    - **Label Assignment**: Assigns a label to the vertex type, reflecting its role or category in the graph.
      Example: ``type_name``

    - **Properties Mapping**: Maps columns from the relational table to properties on the vertex, maintaining data integrity.
      Example: ``mappings``

3. Edge Tables Definition (``edgeMappings.edge_types``)

    - **Table Naming**: Identifies the relational table representing relationships between vertices.
      Example: ``"TRANSFER"``

    - **Source and Destination Key Definition**: Specifies the columns in the edge table that refer to the start (source) and end (destination) vertices of the edge, usually referencing the primary keys of the vertex tables.
      Example: ``type_pair``, ``sourceVertexMappings``, ``destinationVertexMappings``
    - **Label Assignment**: Assign a label to the edge type, describing the nature of the relationship.
      Example: ``edge``
    - **Properties Mapping**: Maps additional columns to properties on the edge, capturing details about the relationship.
      Example: ``dataFieldMappings``

.. _SQL/PGQ: https://pgql-lang.org/
.. _YAML: https://yaml.org/
