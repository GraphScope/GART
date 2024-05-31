package gart.pgql;

import java.io.FileReader;
import java.util.List;

import org.yaml.snakeyaml.Yaml;

import oracle.pgql.lang.ddl.propertygraph.CreatePropertyGraph;
import oracle.pgql.lang.ddl.propertygraph.EdgeTable;
import oracle.pgql.lang.ddl.propertygraph.VertexTable;
import oracle.pgql.lang.ir.SchemaQualifiedName;

// Converter PGQL YAML to DDL
public class PgqlConverter {

    public PgqlConverter(FileReader reader) {
        this.reader = reader;
    }

    public CreatePropertyGraph convert() {
        // test input
        GSchema gSchema = null;
        try {
            Yaml yaml = CustomYaml.getReadYaml();
            gSchema = yaml.load(reader);
            gSchema.format();
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }

        SchemaQualifiedName graphName = new SchemaQualifiedName("", gSchema.graph);
        List<VertexTable> vertexTables = gSchema.getVertexTables();
        List<EdgeTable> edgeTables = gSchema.getEdgeTables(vertexTables);

        CreatePropertyGraph ddl = new CreatePropertyGraph(graphName, vertexTables, edgeTables);
        return ddl;
    }

    private FileReader reader;
}
