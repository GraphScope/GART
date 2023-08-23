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
import oracle.pgql.lang.ddl.propertygraph.Label;
import oracle.pgql.lang.ddl.propertygraph.VertexTable;

class DataField {
    DataField(String name) {
        this.name = name;
    }

    public String name;
}

class Mapping {
    Mapping(String property, String columnName) {
        this.property = property;
        this.dataField = new DataField(columnName);
    }

    public String property;
    public DataField dataField;
}

class LoadingConfig {
    public String dataSource;
    public String database;
    public String method = "append";
    public Boolean enableRowStore = false;
}

class VertexMappings {

    class VertexType {
        VertexType(VertexTable vertexTable) {
            // Now we only support one label and one key column for each vertex
            // table
            this.dataSourceName = vertexTable.getTableName().getName();
            this.idFieldName = vertexTable.getKey().getColumnNames().get(0);

            Label label = vertexTable.getLabels().get(0);
            this.type_name = label.getName();
            this.mappings = new Mapping[label.getProperties().size()];
            for (int i = 0; i < label.getProperties().size(); ++i) {
                String property = label.getProperties().get(i).getPropertyName();
                String colName = label.getProperties().get(i).getColumnName();
                this.mappings[i] = new Mapping(property, colName);
            }
        }

        public String type_name;
        public String dataSourceName;
        public String idFieldName;
        public Mapping[] mappings;
    }

    VertexMappings(List<VertexTable> vertexTables) {
        this.vertexTypes = new VertexType[vertexTables.size()];
        for (int i = 0; i < vertexTables.size(); ++i) {
            VertexTable vertexTable = vertexTables.get(i);
            this.vertexTypes[i] = new VertexType(vertexTable);
        }
    }

    public VertexType[] vertexTypes;
}

class EdgeMappings {
    class DataField {
        public String name;
    }

    class Mapping {
        public String property;
        public DataField dataField = new DataField();
    }

    class TypePair {
        public String edge;
        public String source_vertex;
        public String destination_vertex;
    }

    class EdgeType {
        public TypePair type_pair = new TypePair();
        public String dataSourceName;
        public DataField[] sourceVertexMappings;
        public DataField[] destinationVertexMappings;
        public Mapping[] dataFieldMappings;
    }

    public EdgeType[] edgeTypes;
}

class GSchema {
    public String graph;
    public LoadingConfig loadingConfig = new LoadingConfig();
    public VertexMappings vertexMappings;
    public EdgeMappings edgeMappings = new EdgeMappings();
}

public class YamlConverter {
    public YamlConverter(FileWriter writer, CreatePropertyGraph ddlStatement) {
        this.writer = writer;
        this.ddlStatement = ddlStatement;

        // initialize yaml
        DumperOptions options = new DumperOptions();
        options.setIndent(2);
        options.setPrettyFlow(true);
        options.setDefaultFlowStyle(DumperOptions.FlowStyle.BLOCK);
        Representer representer = new CustomRepresenter();
        this.yaml = new Yaml(representer, options);
    }

    public void convert() {
        GSchema schema = new GSchema();
        schema.graph = ddlStatement.getGraphName().toString();
        schema.loadingConfig.dataSource = "mysql";
        schema.loadingConfig.database = "ldbc";

        List<VertexTable> vertexTables = ddlStatement.getVertexTables();

        schema.vertexMappings = new VertexMappings(vertexTables);

        // output yaml
        yaml.dump(schema, writer);
        String str = yaml.dump(schema);
        System.out.println(str);

        // Object obj =
        // yaml.load(this.getClass().getClassLoader().getResourceAsStream(filename));
        // System.out.println(obj);
    }

    private FileWriter writer;
    private CreatePropertyGraph ddlStatement;
    private Yaml yaml;
}
