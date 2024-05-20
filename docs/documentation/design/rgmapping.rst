.. _rgmapping:

RGMapping
===========

What is RGMapping?
-------------------

GART facilitates seamless conversion between data models, enabling the transformation of data updates from relational databases into corresponding graph data constructs as per customized model mapping rules, termed RGMapping. The versatility of GART lies in its provision for database administrators (DBAs) to articulate a variety of data model conversion principles.

The illustration below serves as an example of the RGMapping process within GART in a fraud detection example.
It delineates how users can establish mapping directives that link relational data constructs to their graph-based counterparts. GART processes these mapping instructions, methodically applying them to the relational data, subsequently ensuring the relational and graph data sets are in sync. Users are empowered to interrogate the graph data using graph-specific query languages, such as Cypher or Gremlin.

.. figure:: /images/rgmapping.png
    :width: 80%
    :alt: An overview of RGMapping in GART.

    An overview of RGMapping in GART.

.. tip::
    On the left side, there's a representation of a relational OLTP (Online Transaction Processing) database with a traditional relational model. This indicates that data, likely involving participant IDs and the method of transaction, is being added to a table named ``TRANSACTION``.

    The middle section shows the definition of transformation rules (RGMapping) through an example in Python code. Functions named ``def_vertex`` and ``add_edge`` are defined, which are used to convert relational model data—related to persons (``Person``) and transactions (``Trans``)—into vertices and edges in a graph model. ``P_ID1`` and ``P_ID2`` represent individual IDs, and HOW represents the transaction mode, and these are used to create corresponding elements in the graph.

    On the right side is a graph-based OLAP (Online Analytical Processing) model, where data is presented in graph form. There is a routine named ``FRAUD_DETECTION``, which performs queries on the graph such as using the ``findCycle`` function to identify potential cycles within transactions (``Trans``). Identifying such cycles could be useful for detecting fraudulent activities.

    Overall, the image illustrates a process where relational data is transformed into graph data through mapping rules (RGMapping), allowing database administrators (DBAs) to define rules for such conversions and enabling advanced analytic operations like fraud detection using a graph database.

Interface of RGMapping
-----------------------

In the fraud detection example, the user can define rules for RGMapping as described below. This model transformation rule includes the user's definition of the graph structure and how the information in the relational data is mapped to the vertices, edges, and attributes in the graph.

SQL/PGQ
^^^^^^^

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

The RGMapping rule can be written in `SQL/PGQ`_. It creates a graph named ``ldbc``, which contains a type of vertices ``PERSON`` and a type of edges ``TRANSFER`` (it ignores the label name which is the same as the table name). It shows the correspondence between the vertices, edges and their properties in the graph and the columns in the table.

YAML
^^^^

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

The RGMapping rule can also be written in `YAML`_. It defines the graph schema for the graph named ``ldbc``. It specifies the mapping between the vertices, edges and their properties in the graph and the columns in the table.
The examples we give are equivalent conversion conditions to those described above for SQL/PGQ.

.. _SQL/PGQ: https://pgql-lang.org/
.. _YAML: https://yaml.org/
