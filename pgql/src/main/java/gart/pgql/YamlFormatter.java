/**
 * Copyright 2020-2023 Alibaba Group Holding Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package gart.pgql;

import java.io.FileReader;
import java.io.FileWriter;
import java.util.List;

import org.yaml.snakeyaml.DumperOptions;
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.representer.Representer;

import oracle.pgql.lang.ddl.propertygraph.EdgeTable;
import oracle.pgql.lang.ddl.propertygraph.VertexTable;

public class YamlFormatter {

    public YamlFormatter(FileReader reader) {
        this.reader = reader;
        this.yaml = CustomYaml.getReadYaml();
    }

    public GSchema check() {
        // test input
        GSchema gSchema = null;
        try {
            gSchema = yaml.load(reader);
        } catch (Exception e) {
            System.out.println(e.getMessage());
            return null;
        }

        String graphName = gSchema.graph;
        List<VertexTable> vertexTables = gSchema.getVertexTables();
        List<EdgeTable> edgeTables = gSchema.getEdgeTables(vertexTables);

        if (graphName == null) {
            System.out.println("Graph name is missing");
        }
        if (vertexTables == null) {
            System.out.println("Vertex tables are missing");
        }
        if (edgeTables == null) {
            System.out.println("Edge tables are missing");
        }

        if (graphName != null && vertexTables != null && edgeTables != null) {
            System.out.println("YAML format is correct");
        }

        return gSchema;

    }

    public void format(FileWriter writer) {
        GSchema gSchema = check();
        gSchema.format();

        Yaml writeYaml = CustomYaml.getWriteYaml();
        writeYaml.dump(gSchema, writer);
    }

    private FileReader reader;
    private Yaml yaml;
}
