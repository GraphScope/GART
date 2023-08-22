/*
 * Copyright (C) 2013 - 2023 Oracle and/or its affiliates. All rights reserved.
 */
package gart.pgql;

import oracle.pgql.lang.PgqlException;
import oracle.pgql.lang.Pgql;
import oracle.pgql.lang.PgqlResult;
import oracle.pgql.lang.ddl.propertygraph.CreatePropertyGraph;

public class Main {

    public static void main(String[] args) throws PgqlException {

        try (Pgql pgql = new Pgql()) {

            // parse query and print graph query
            PgqlResult result1 = pgql
                    .parse("SELECT n FROM MATCH (n IS Person) -[e IS likes]-> (m IS Person) WHERE n.name = 'Dave'");
            System.out.println(result1.getPgqlStatement());

            // parse query with errors and print error messages
            PgqlResult result2 = pgql.parse("SELECT x, y FROM MATCH (n) -[e]-> (m)");
            System.out.println(result2.getErrorMessages());

            // parse DDL
            String ddlString = "CREATE PROPERTY GRAPH socialNetwork "
                    + "  VERTEX TABLES ("
                    + "    Person KEY (p_id)"
                    + "      LABEL Person PROPERTIES (p_name AS name, p_age AS age)"
                    + "      LABEL Person_age PROPERTIES (p_age)"
                    + "  )"
                    + "  EDGE TABLES ("
                    + "    Trans KEY (t_id)"
                    + "      SOURCE KEY (P_ID1) REFERENCES Person (p_id)"
                    + "      DESTINATION KEY (P_ID2) REFERENCES Person (p_id)"
                    + "  )";
            PgqlResult result3 = pgql.parse(ddlString);

            if (result3.isQueryValid()) {
                CreatePropertyGraph createPropertyGraph = (CreatePropertyGraph) result3.getPgqlStatement();
                System.out.println(createPropertyGraph.getVertexTables());
            } else {
                System.out.println(ddlString);
                System.out.println(result3.getErrorMessages());
            }
        }
    }
}
