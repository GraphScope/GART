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

import org.yaml.snakeyaml.DumperOptions;
import org.yaml.snakeyaml.LoaderOptions;
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.constructor.Constructor;
import org.yaml.snakeyaml.inspector.TagInspector;
import org.yaml.snakeyaml.representer.Representer;

public class CustomYaml {
    public static Yaml getReadYaml() {
        // ignore extra properties in a YAML file
        Representer representer = new CustomRepresenter(new DumperOptions());
        representer.getPropertyUtils().setSkipMissingProperties(true);
        TagInspector tagInspector = tag -> tag.getClassName().equals(GSchema.class.getName());
        LoaderOptions loaderOptions = new LoaderOptions();
        loaderOptions.setTagInspector(tagInspector);
        Yaml yaml = new Yaml(new Constructor(GSchema.class, loaderOptions), representer);

        return yaml;
    }

    public static Yaml getWriteYaml() {
        // initialize yaml
        DumperOptions options = new DumperOptions();
        options.setIndent(2);
        options.setPrettyFlow(true);
        options.setDefaultFlowStyle(DumperOptions.FlowStyle.BLOCK);
        Representer representer = new CustomRepresenter(options);
        Yaml yaml = new Yaml(representer, options);

        return yaml;
    }
}
