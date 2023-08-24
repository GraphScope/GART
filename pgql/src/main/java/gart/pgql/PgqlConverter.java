package gart.pgql;

import java.io.FileReader;
import java.util.List;

import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.constructor.Constructor;

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
        GSchema data = null;
        try {
            Yaml yaml = new Yaml(new Constructor(GSchema.class));
            data = yaml.load(reader);
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }

        SchemaQualifiedName graphName = new SchemaQualifiedName("", data.graph);
        List<VertexTable> vertexTables = data.getVertexTables();
        List<EdgeTable> edgeTables = data.getEdgeTables(vertexTables);

        CreatePropertyGraph ddl = new CreatePropertyGraph(graphName, vertexTables, edgeTables);
        return ddl;
    }

    private FileReader reader;
}
