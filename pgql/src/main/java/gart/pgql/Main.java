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
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

import oracle.pgql.lang.Pgql;
import oracle.pgql.lang.PgqlResult;
import oracle.pgql.lang.ddl.propertygraph.CreatePropertyGraph;

public class Main {

    public static void main(String[] args) throws PgqlException {

        if (args.length < 3) {
            System.out.println("Usage: pgql <yaml2sql/sql2yaml> <input_file> <output_file>");
            return;
        }

        if (args[0].equals("yaml2sql")) {
            yaml2sql(args[1], args[2]);
        } else if (args[0].equals("sql2yaml")) {
            sql2yaml(args[1], args[2]);
        } else {
            System.out.println("Usage: pgql <sql2yaml|yaml2sql> <input_file> <output_file>");
        }
    }

    private static void sql2yaml(String input_sql, String output_yaml) {
        try (Pgql pgql = new Pgql()) {
            String ddlString = new String(Files.readAllBytes(Paths.get(input_sql)), StandardCharsets.UTF_8);

            PgqlResult pgqlResult = pgql.parse(ddlString);

            if (!pgqlResult.isQueryValid()) {
                System.out.println(ddlString);
                System.out.println(pgqlResult.getErrorMessages());
                return;
            }

            CreatePropertyGraph createPropertyGraph = (CreatePropertyGraph) pgqlResult.getPgqlStatement();
            // System.out.println(createPropertyGraph);

            FileWriter writer = new FileWriter(output_yaml);
            YamlConverter yamlConventer = new YamlConverter(writer, createPropertyGraph);
            yamlConventer.convert();
        } catch (IOException ie) {
            System.out.println("Error: " + ie.getMessage());
        } catch (PgqlException e) {
            System.out.println("PSQL Parse Error: " + e.getMessage());
        }
    }

    private static void yaml2sql(String input_yaml, String output_yaml) {
        try {
            FileReader reader = new FileReader(input_yaml);
            PgqlConverter pgqlConverter = new PgqlConverter(reader);
            CreatePropertyGraph ddl = pgqlConverter.convert();
            FileWriter writer = new FileWriter(output_yaml);
            writer.write(ddl.toString());
            writer.close();
        } catch (IOException ie) {
            System.out.println("IO Error: " + ie.getMessage());
        }
    }
}
