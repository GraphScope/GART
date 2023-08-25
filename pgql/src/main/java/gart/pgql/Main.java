/** Copyright 2020-2023 Alibaba Group Holding Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package gart.pgql;

import oracle.pgql.lang.PgqlException;

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

import oracle.pgql.lang.Pgql;
import oracle.pgql.lang.PgqlResult;
import oracle.pgql.lang.ddl.propertygraph.CreatePropertyGraph;

public class Main {

    public static void main(String[] args) throws PgqlException {

        try (Pgql pgql = new Pgql()) {

            // parse DDL
            String ddlString = "CREATE PROPERTY GRAPH socialNetwork "
                    + "  VERTEX TABLES ("
                    + "    Person KEY (p_id)"
                    + "      LABEL VPerson PROPERTIES (p_id AS id, p_name AS name, p_age AS age)"
                    + "      LABEL Person_age PROPERTIES (p_age)"
                    + "  )"
                    + "  EDGE TABLES ("
                    + "    Trans KEY (t_id)"
                    + "      SOURCE KEY (P_ID1) REFERENCES Person (p_id)"
                    + "      DESTINATION KEY (P_ID2) REFERENCES Person (p_id)"
                    + "      LABEL Transfer PROPERTIES (t_data AS data)"
                    + "  )";
            PgqlResult result3 = pgql.parse(ddlString);

            if (!result3.isQueryValid()) {
                System.out.println(ddlString);
                System.out.println(result3.getErrorMessages());

                return;
            }

            CreatePropertyGraph createPropertyGraph = (CreatePropertyGraph) result3.getPgqlStatement();
            System.out.println(createPropertyGraph);

            String output = "file.yaml";
            try {
                FileWriter writer = new FileWriter(output);
                YamlConverter yamlConventer = new YamlConverter(writer, createPropertyGraph);
                yamlConventer.convert();
            } catch (IOException ie) {
                System.out.println("Error: " + ie.getMessage());
            }

            String input = "vegito/test/schema/rgmapping-ldbc.yaml";
            try {
                FileReader reader = new FileReader(input);
                PgqlConverter pgqlConverter = new PgqlConverter(reader);
                CreatePropertyGraph ddl = pgqlConverter.convert();
                System.out.println(ddl);
            } catch (IOException ie) {
                System.out.println("Error: " + ie.getMessage());
            }

        }

    }
}
