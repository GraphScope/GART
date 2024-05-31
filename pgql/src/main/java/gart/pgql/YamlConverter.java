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

import java.util.List;
import java.io.FileWriter;

import oracle.pgql.lang.ddl.propertygraph.CreatePropertyGraph;
import oracle.pgql.lang.ddl.propertygraph.EdgeTable;
import oracle.pgql.lang.ddl.propertygraph.VertexTable;

// Converter PGQL DDL to YAML
public class YamlConverter {
    public YamlConverter(FileWriter writer, CreatePropertyGraph ddlStatement) {
        this.writer = writer;
        this.ddlStatement = ddlStatement;
    }

    public void convert() {
        GSchema gSchema = new GSchema();
        gSchema.graph = ddlStatement.getGraphName().toString();

        gSchema.loadingConfig = new LoadingConfig();
        gSchema.loadingConfig.dataSource = "rdbms";
        gSchema.loadingConfig.database = "ldbc";

        List<VertexTable> vertexTables = ddlStatement.getVertexTables();
        List<EdgeTable> edgeTables = ddlStatement.getEdgeTables();

        gSchema.vertexMappings = new VertexMappings(vertexTables);
        gSchema.edgeMappings = new EdgeMappings(edgeTables, vertexTables);

        gSchema.format();

        // output yaml
        CustomYaml.getWriteYaml().dump(gSchema, writer);
        // String str = yaml.dump(schema);
        // System.out.println(str);
    }

    final private CreatePropertyGraph ddlStatement;
    final private FileWriter writer;
}
