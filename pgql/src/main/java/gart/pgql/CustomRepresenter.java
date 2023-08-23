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

import java.util.LinkedHashSet;
import java.util.Set;
import java.util.stream.Collectors;

import org.yaml.snakeyaml.introspector.*;
import org.yaml.snakeyaml.representer.Representer;

// https://stackoverflow.com/questions/62956776/how-to-order-nodes-in-yaml-with-snakeyaml
public class CustomRepresenter extends Representer {

    public CustomRepresenter() {
        super();
        PropertyUtils propUtil = new PropertyUtils() {
            @Override
            protected Set<Property> createPropertySet(Class<? extends Object> type, BeanAccess bAccess) {
                return getPropertiesMap(type, bAccess).values().stream().sequential()
                        .filter(prop -> prop.isReadable() && (isAllowReadOnlyProperties() || prop.isWritable()))
                        .collect(Collectors.toCollection(LinkedHashSet::new));
            }
        };
        setPropertyUtils(propUtil);
    }
}
