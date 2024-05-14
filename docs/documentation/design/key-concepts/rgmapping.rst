.. _rgmapping:

RGMapping
===========

What is RGMapping?
-------------------

GART provides a transparent data model conversion. During the use of GART, data changes from relational databases are converted into graph data updates based on user-defined model mapping rules (RGMapping).  GART allows DBAs to define data model conversion rules in multiple ways.
The following figure gives an example of RGMapping.

.. figure:: /images/rgmapping.png
    :width: 80%
    :alt: An overview of RGMapping in GART.

    An overview of RGMapping in GART.

Interface of RGMapping
-----------------------

In the fraud detection example, the user can define rules for RGMapping as described below. This model transformation rule includes the user's definition of the graph structure and how the information in the relational data is mapped to the vertices, edges, and attributes in the graph.

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

The RGMapping rule is written in `SQL/PGQ`_. It creates a graph named ``ldbc``, which contains a type of vertices ``PERSON`` and a type of edges ``TRANSFER`` (it ignores the label name which is the same as the table name). It shows the correspondence between the vertices, edges and their properties in the graph and the columns in the table.
After starting the GART service, updates to the relational data will be imposed on the graph data through log synchronization. Users can query the data for updates to their tests in a familiar graph language (e.g., Cypher, Gremlin).

.. _SQL/PGQ: https://pgql-lang.org/
