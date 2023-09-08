package gart.pgql;

import java.util.ArrayList;
import java.util.List;

import oracle.pgql.lang.ddl.propertygraph.EdgeTable;
import oracle.pgql.lang.ddl.propertygraph.Key;
import oracle.pgql.lang.ddl.propertygraph.Label;
import oracle.pgql.lang.ddl.propertygraph.Property;
import oracle.pgql.lang.ddl.propertygraph.VertexTable;
import oracle.pgql.lang.ir.ExpAsVar;
import oracle.pgql.lang.ir.SchemaQualifiedName;
import oracle.pgql.lang.ir.QueryExpression.VarRef;

class DataField {
    DataField() {
    }

    DataField(String name) {
        this.name = name;
    }

    public String name;
}

class Mapping {
    Mapping() {
    }

    Mapping(String property, String columnName) {
        this.property = property;
        this.dataField = new DataField(columnName);
    }

    public String property;
    public DataField dataField;
}

class EdgeVertexMapping {
    EdgeVertexMapping() {
    }

    EdgeVertexMapping(String columnName) {
        this.dataField = new DataField(columnName);
    }

    public DataField dataField;
}

class LoadingConfig {
    LoadingConfig() {
        this.method = "append";
        this.enableRowStore = false;
    }

    public String dataSource;
    public String database;
    public String method;
    public Boolean enableRowStore;
}

class VertexType {
    public VertexType() {
    }

    VertexType(VertexTable vertexTable) {
        // Now we only support one label and one key column for each table
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

    VertexTable convert() {
        SchemaQualifiedName tableName = new SchemaQualifiedName("", this.dataSourceName);
        List<String> keyColumns = new ArrayList<>();
        keyColumns.add(this.idFieldName);
        Key key = new Key(keyColumns);

        List<Property> properties = new ArrayList<>();
        for (Mapping mapping : this.mappings) {
            String property = mapping.property;
            String colName = mapping.dataField.name;
            VarRef varRef = new VarRef(new ExpAsVar(null, colName, false));
            Property prop = new Property(varRef, property);
            properties.add(prop);
        }

        List<Label> labels = new ArrayList<>();
        labels.add(new Label(this.type_name, properties));

        VertexTable vertexTable = new VertexTable(tableName, key, labels);
        return vertexTable;
    }

    public String type_name;
    public String dataSourceName;
    public String idFieldName;
    public Mapping[] mappings;
}

class VertexMappings {

    VertexMappings() {
    }

    VertexMappings(List<VertexTable> vertexTables) {
        this.vertex_types = new VertexType[vertexTables.size()];
        for (int i = 0; i < vertexTables.size(); ++i) {
            VertexTable vertexTable = vertexTables.get(i);
            this.vertex_types[i] = new VertexType(vertexTable);
        }
    }

    public VertexType[] vertex_types;
}

class TypePair {
    TypePair() {
    }

    TypePair(String edge, String source_vertex, String destination_vertex) {
        this.edge = edge;
        this.source_vertex = source_vertex;
        this.destination_vertex = destination_vertex;
    }

    public String edge;
    public String source_vertex;
    public String destination_vertex;

}

class EdgeType {
    EdgeType() {
    }

    EdgeType(EdgeTable edgeTable, List<VertexTable> vertexTables) {
        // Now we only support one label and one key column for each table
        Label label = edgeTable.getLabels().get(0);
        String edge = label.getName();
        String sourceTable = edgeTable.getSourceVertexTable().getTableName().getName();
        String destinationTable = edgeTable.getDestinationVertexTable().getTableName().getName();
        String sourceVertex = null, destinationVertex = null;

        // table name in PGQL, while label name in YAML
        for (VertexTable vertexTable : vertexTables) {
            String vtable = vertexTable.getTableName().getName();
            String vlabel = vertexTable.getLabels().get(0).getName();
            if (vtable.equals(sourceTable)) {
                sourceVertex = vlabel;
            }
            if (vtable.equals(destinationTable)) {
                destinationVertex = vlabel;
            }
        }
        if (sourceVertex == null || destinationVertex == null) {
            System.out.println("Error: cannot find vertex table for edge"
                    + edge + " with sourceTable: " + sourceTable
                    + ", destinationTable: " + destinationTable);
            throw new RuntimeException();
        }

        this.type_pair = new TypePair(edge, sourceVertex, destinationVertex);

        this.dataSourceName = edgeTable.getTableName().getName();

        this.sourceVertexMappings = new EdgeVertexMapping[1];
        this.sourceVertexMappings[0] = new EdgeVertexMapping(edgeTable.getEdgeSourceKey().getColumnNames().get(0));
        this.destinationVertexMappings = new EdgeVertexMapping[1];
        this.destinationVertexMappings[0] = new EdgeVertexMapping(
                edgeTable.getEdgeDestinationKey().getColumnNames().get(0));

        int numProperties = 0;
        if (label.getProperties() != null) {
            numProperties = label.getProperties().size();
        }
        this.dataFieldMappings = new Mapping[numProperties];
        for (int i = 0; i < numProperties; ++i) {
            String property = label.getProperties().get(i).getPropertyName();
            String colName = label.getProperties().get(i).getColumnName();
            this.dataFieldMappings[i] = new Mapping(property, colName);
        }
    }

    EdgeTable convert(List<VertexTable> vertexTables) {
        SchemaQualifiedName tableName = new SchemaQualifiedName("", this.dataSourceName);

        List<String> srcKeyColumns = new ArrayList<>();
        srcKeyColumns.add(this.sourceVertexMappings[0].dataField.name);
        List<String> dstKeyColumns = new ArrayList<>();
        dstKeyColumns.add(this.destinationVertexMappings[0].dataField.name);
        Key srcKey = new Key(srcKeyColumns);
        Key dstKey = new Key(dstKeyColumns);

        VertexTable srcVertexTable = null;
        VertexTable dstVertexTable = null;

        for (VertexTable vertexTable : vertexTables) {
            String vlabel = vertexTable.getLabels().get(0).getName();
            if (vlabel.equals(this.type_pair.source_vertex)) {
                srcVertexTable = vertexTable;
            }
            if (vlabel.equals(this.type_pair.destination_vertex)) {
                dstVertexTable = vertexTable;
            }
        }
        if (srcVertexTable == null || dstVertexTable == null) {
            System.out.println("Error: cannot find vertex table for edge" +
                    this.type_pair.edge + " with sourceTable: "
                    + this.type_pair.source_vertex
                    + ", destinationTable: " + this.type_pair.destination_vertex);
            throw new RuntimeException();
        }

        List<Property> properties = new ArrayList<>();
        for (Mapping mapping : this.dataFieldMappings) {
            String property = mapping.property;
            String colName = mapping.dataField.name;
            VarRef varRef = new VarRef(new ExpAsVar(null, colName, false));
            Property prop = new Property(varRef, property);
            properties.add(prop);
        }

        List<Label> labels = new ArrayList<>();
        labels.add(new Label(this.type_pair.edge, properties));

        EdgeTable edgeTable = new EdgeTable(tableName, srcVertexTable, srcKey, dstVertexTable, dstKey,
                labels);
        return edgeTable;
    }

    public TypePair type_pair;
    public String dataSourceName;
    public EdgeVertexMapping[] sourceVertexMappings;
    public EdgeVertexMapping[] destinationVertexMappings;
    public Mapping[] dataFieldMappings;
}

class EdgeMappings {

    EdgeMappings() {
    }

    EdgeMappings(List<EdgeTable> edgeTables, List<VertexTable> vertexTables) {
        this.edge_types = new EdgeType[edgeTables.size()];
        for (int i = 0; i < edgeTables.size(); ++i) {
            EdgeTable edgeTable = edgeTables.get(i);
            this.edge_types[i] = new EdgeType(edgeTable, vertexTables);
        }
    }

    public EdgeType[] edge_types;
}

public class GSchema {
    GSchema() {
    }

    public List<VertexTable> getVertexTables() {
        List<VertexTable> vertexTables = new ArrayList<>();
        for (VertexType vertexType : this.vertexMappings.vertex_types) {
            VertexTable vertexTable = vertexType.convert();
            vertexTables.add(vertexTable);
        }
        return vertexTables;
    }

    public List<EdgeTable> getEdgeTables(List<VertexTable> vertexTables) {
        List<EdgeTable> edgeTables = new ArrayList<>();
        for (EdgeType edgeType : this.edgeMappings.edge_types) {
            EdgeTable edgeTable = edgeType.convert(vertexTables);
            edgeTables.add(edgeTable);
        }
        return edgeTables;
    }

    public String graph;
    public LoadingConfig loadingConfig;
    public VertexMappings vertexMappings;
    public EdgeMappings edgeMappings;
}
