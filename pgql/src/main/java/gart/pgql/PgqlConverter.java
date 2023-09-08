package gart.pgql;

import java.io.FileReader;
import java.util.List;

import org.yaml.snakeyaml.DumperOptions;
import org.yaml.snakeyaml.LoaderOptions;
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.constructor.Constructor;
import org.yaml.snakeyaml.representer.Representer;

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
            // ignore extra properties in a YAML file
            Representer representer = new Representer(new DumperOptions());
            representer.getPropertyUtils().setSkipMissingProperties(true);
            LoaderOptions loaderOptions = new LoaderOptions();
            Yaml yaml = new Yaml(new Constructor(GSchema.class, loaderOptions), representer);
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
