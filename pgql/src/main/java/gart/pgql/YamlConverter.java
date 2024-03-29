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

import org.yaml.snakeyaml.DumperOptions;
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.representer.Representer;

import oracle.pgql.lang.ddl.propertygraph.CreatePropertyGraph;
import oracle.pgql.lang.ddl.propertygraph.EdgeTable;
import oracle.pgql.lang.ddl.propertygraph.VertexTable;

// Converter PGQL DDL to YAML
public class YamlConverter {
    public YamlConverter(FileWriter writer, CreatePropertyGraph ddlStatement) {
        this.writer = writer;
        this.ddlStatement = ddlStatement;

        // initialize yaml
        DumperOptions options = new DumperOptions();
        options.setIndent(2);
        options.setPrettyFlow(true);
        options.setDefaultFlowStyle(DumperOptions.FlowStyle.BLOCK);
        Representer representer = new CustomRepresenter(options);
        this.yaml = new Yaml(representer, options);
    }

    public void convert() {
        GSchema schema = new GSchema();
        schema.graph = ddlStatement.getGraphName().toString();

        schema.loadingConfig = new LoadingConfig();
        schema.loadingConfig.dataSource = "mysql";
        schema.loadingConfig.database = "ldbc";

        List<VertexTable> vertexTables = ddlStatement.getVertexTables();
        List<EdgeTable> edgeTables = ddlStatement.getEdgeTables();

        schema.vertexMappings = new VertexMappings(vertexTables);
        schema.edgeMappings = new EdgeMappings(edgeTables, vertexTables);

        // output yaml
        yaml.dump(schema, writer);
        // String str = yaml.dump(schema);
        // System.out.println(str);
    }

    final private CreatePropertyGraph ddlStatement;
    final private FileWriter writer;
    final private Yaml yaml;
}
