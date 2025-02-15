package com.rusefi.output;

import com.rusefi.*;

import static com.rusefi.ToolUtil.EOL;

public class BaseCHeaderConsumer implements ConfigurationConsumer {
    private static final String BOOLEAN_TYPE = "bool";
    private final StringBuilder content = new StringBuilder();

    private static String getHeaderText(FieldIteratorWithOffset iterator) {
        ConfigField configField = iterator.cf;
        if (configField.isBit()) {
            String comment = "\t/**" + EOL + packComment(configField.getCommentContent(), "\t") + "\toffset " + iterator.currentOffset + " bit " + iterator.bitState.get() + " */" + EOL;
            return comment + "\t" + BOOLEAN_TYPE + " " + configField.getName() + " : 1 {};" + EOL;
        }

        String cEntry = getComment(configField.getCommentContent(), iterator.currentOffset, configField.getUnits());

        String typeName = configField.getType();

        String autoscaleSpec = configField.autoscaleSpec();
        if (autoscaleSpec != null) {
            typeName = "scaled_channel<" + typeName + ", " + autoscaleSpec + ">";
        }

        if (!configField.isArray()) {
            // not an array
            cEntry += "\t" + typeName + " " + configField.getName();
            if (ConfigDefinition.needZeroInit && TypesHelper.isPrimitive(configField.getType())) {
                // we need this cast in case of enums
                cEntry += " = (" + configField.getType() + ")0";
            }
            cEntry += ";" + EOL;
        } else {
            cEntry += "\t" + typeName + " " + configField.getName() + "[" + configField.arraySizeVariableName + "];" + EOL;
        }
        return cEntry;
    }

    private static String getComment(String comment, int currentOffset, String units) {
        String start = "\t/**";
        String packedComment = packComment(comment, "\t");
        String unitsComment = units.isEmpty() ? "" : "\t" + units + EOL;
        return start + EOL +
                packedComment +
                unitsComment +
                "\t * offset " + currentOffset + EOL + "\t */" + EOL;
    }

    public static String packComment(String comment, String linePrefix) {
        if (comment == null)
            return "";
        if (comment.trim().isEmpty())
            return "";
        String result = "";
        for (String line : comment.split("\\\\n")) {
            result += linePrefix + " * " + line + EOL;
        }
        return result;
    }

    @Override
    public void handleEndStruct(ReaderState readerState, ConfigStructure structure) {
        if (structure.comment != null) {
            content.append("/**" + EOL + packComment(structure.comment, "") + EOL + "*/" + EOL);
        }

        content.append("// start of " + structure.name + EOL);
        content.append("struct " + structure.name + " {" + EOL);

        FieldIteratorWithOffset iterator = new FieldIteratorWithOffset(structure.cFields);
        for (int i = 0; i < structure.cFields.size(); i++) {
            iterator.start(i);
            content.append(getHeaderText(iterator));

            iterator.currentOffset += iterator.cf.getSize(iterator.next);
            iterator.end();
        }

        content.append("};" + EOL);
        content.append("static_assert(sizeof(" + structure.name + ") == " + iterator.currentOffset + ");\n");
        content.append(EOL);
    }

    public String getContent() {
        return content.toString();
    }
}
