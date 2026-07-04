#include "codegenluau.hpp"
#include "classreader.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

size_t class_name_counter = 0;
std::vector<std::string> class_name_list; 

static unsigned int indent_counter = 0;
inline void addIndent() { indent_counter++; };
inline void subIndent() { indent_counter--; };

inline void indent(std::string& output) {
    for (unsigned int i = 0; i < indent_counter; i++)
        output.append("    ");
}

int outputClass(Class& _class, std::string& output) {
    size_t class_name_index = class_name_counter;

    output.append("local class_")
        .append(std::to_string(class_name_index));

    std::string class_name(_class.this_class->Class.name->Utf8.bytes, _class.this_class->Class.name->Utf8.bytes + _class.this_class->Class.name->Utf8.bytes_size);
    class_name_counter++;
    class_name_list.push_back(class_name);

    output.append(" = { -- ")
        .append(class_name)
        .push_back('\n');

    addIndent();

    indent(output);
    output.append("name = \"")
        .append(class_name)
        .append("\",\n");
    indent(output);
    output.append("hasloaded = false,\n");

    indent(output);
    output.append("isarray = false,\n");

    indent(output);
    output.append("issuper = ")
        .append(_class.access_flags & CLASS_ACC_SUPER ? "true,\n" : "false,\n");
    indent(output);
    output.append("isinterface = ")
        .append(_class.access_flags & CLASS_ACC_INTERFACE ? "true,\n" : "false,\n");

    if (_class.super_class) {
        indent(output);
        output.append("superclass = \"");
        output.insert(output.end(), _class.super_class->Class.name->Utf8.bytes, _class.super_class->Class.name->Utf8.bytes + _class.super_class->Class.name->Utf8.bytes_size);
        output.append("\",\n");
    }
    indent(output);
    output.append("interfaces = {\n");
    addIndent();
    for (uint16_t i = 0; i < _class.interface_count; i++) {
        indent(output);
        output.push_back('"');
        output.insert(output.end(), _class.interface_list[i]->Class.name->Utf8.bytes, _class.interface_list[i]->Class.name->Utf8.bytes + _class.interface_list[i]->Class.name->Utf8.bytes_size);
        output.append("\",\n");
    }
    subIndent();
    indent(output);
    output.append("},\n");
    indent(output);
    output.append("fieldcache = {},\n");
    indent(output);
    output.append("methodcache = {},\n");

    indent(output);
    output.append("fields = {\n");
    addIndent();

    for (uint16_t i = 0; i < _class.field_count; i++) {
        Field& field = _class.field_list[i];

        std::string field_name(field.name->Utf8.bytes, field.name->Utf8.bytes + field.name->Utf8.bytes_size);

        indent(output);
        output.append("[\"")
            .append(field_name);

        output.append("\"] = { name = \"")
            .append(field_name)
            .append("\", type = \"");
        output.insert(output.end(), field.descriptor->Utf8.bytes, field.descriptor->Utf8.bytes + field.descriptor->Utf8.bytes_size);

        output.append("\" },\n");
    }

    subIndent();
    indent(output);
    output.append("},\n");
    indent(output);
    output.append("methods = {\n");

    addIndent();

    for (uint16_t i = 0; i < _class.method_count; i++) {
        Method& method = _class.method_list[i];

        indent(output);
        output.append("[\"");
        output.insert(output.end(), method.name->Utf8.bytes, method.name->Utf8.bytes + method.name->Utf8.bytes_size);
        output.push_back('-');
        output.insert(output.end(), method.descriptor->Utf8.bytes, method.descriptor->Utf8.bytes + method.descriptor->Utf8.bytes_size);

        output.append("\"] = {\n");
        addIndent();

        indent(output);
        output.append("isstatic = ")
            .append(method.access_flags & METHOD_ACC_STATIC ? "true,\n" : "false,\n");
        indent(output);
        output.append("isnative = ")
            .append(method.access_flags & METHOD_ACC_NATIVE ? "true,\n" : "false,\n");
        indent(output);
        output.append("isabstract = ")
            .append(method.access_flags & METHOD_ACC_ABSTRACT ? "true,\n" : "false,\n");

        bool is_native = false;
        Attribute* code_attribute = nullptr;
        for (uint16_t j = 0; j < method.attribute_count; j++) {
            Attribute* attribute = &method.attribute_list[j];
            if (attribute->name->Utf8.atom == Atom_Code)
                code_attribute = attribute;
        }
        if (!code_attribute) {
            if (method.access_flags & METHOD_ACC_NATIVE)
                is_native = true;
            else {
                std::cerr << "[ERROR]: failed to find code attribute in class " << class_name << "'s " << method.name->Utf8.bytes << " method" << i << std::endl;;
                return 1;
            }
        }

        indent(output);
        output.append("func = function(...)\n");

        addIndent();

        if (is_native) {
            indent(output);
            output.append("natives.registerDefault()\n");

            indent(output);
            output.append("local methodtype = \"");
            output.insert(output.end(), method.descriptor->Utf8.bytes, method.descriptor->Utf8.bytes + method.descriptor->Utf8.bytes_size);
            output.append("\"\n");

            indent(output);
            output.append("local descriptor = descriptor_parser.parseMethodDescriptor(methodtype)\n");
            indent(output);
            output.append("local parameter_count = descriptor.parameter_count\n");

            indent(output);
            output.append("local native = natives.findNative(\"")
                .append(class_name)
                .append("\", \"");
            output.insert(output.end(), method.name->Utf8.bytes, method.name->Utf8.bytes + method.name->Utf8.bytes_size);
            output.push_back('-');
            output.insert(output.end(), method.descriptor->Utf8.bytes, method.descriptor->Utf8.bytes + method.descriptor->Utf8.bytes_size);
            output.append("\")\n");

            indent(output);
            output.append("local argarray = table.pack(...)\n");

            indent(output);
            output.append("local results = table.pack(native(table.unpack(argarray, 1, parameter_count)))\n");

            indent(output);
            output.append("return table.unpack(results, 1, results.n)\n");
        } else {
            indent(output);
            output.append("local pc = 0\n");
            indent(output);
            output.append("local local_array = table.create(")
                .append(std::to_string(code_attribute->Code.max_locals))
                .append(")\n");

            indent(output);
            output.append("local stack = table.create(")
                .append(std::to_string(code_attribute->Code.max_stack))
                .append(")\n");

            indent(output);
            output.append("local function store(list, value, index)\n");
            addIndent();

            indent(output);
            output.append("index = index or (#list + 1)\n");

            indent(output);
            output.append("assert(type(value) ~= \"nil\", \"attempt to store nil value\")\n");

            indent(output);
            output.append("list[index] = value\n");
            indent(output);

            output.append("if value.tag == \"long\" or value.tag == \"double\" then\n");
            addIndent();
            indent(output);
            output.append("store(list, april.newStub(value), index + 1)\n");
            subIndent();
            indent(output);
            output.append("end\n");

            subIndent();
            indent(output);
            output.append("end\n");

            indent(output);
            output.append("for i = 1, select('#', ...) do\n");
            addIndent();

            indent(output);
            output.append("store(local_array, (select(i, ...)))\n");

            subIndent();
            indent(output);
            output.append("end\n");

            indent(output);
            output.append("local function pop()\n");
            addIndent();

                indent(output);
                output.append("local index = #stack\n");
                indent(output);
                output.append("local value = stack[index]\n");
                indent(output);
                output.append("stack[index] = nil\n");
                indent(output);

                output.append("if value.tag == \"stub\" then\n");
                addIndent();
                    indent(output);
                    output.append("value = pop()\n");
                subIndent();
                indent(output);
                output.append("end\n");

                indent(output);
                output.append("assert(type(value) ~= \"nil\", \"invalid value (nil) at the top of the stack\")\n");

                indent(output);
                output.append("return value\n");

            subIndent();
            indent(output);
            output.append("end\n");

            if (code_attribute->Code.code_count > 1) {
                indent(output);
                output.append("local argarrayscratch = table.create(256)\n");
            }

            indent(output);
            output.append("while true do\n");
            addIndent();

            Attribute* line_number_attribute = nullptr;
            for (uint16_t j = 0; j < code_attribute->Code.attribute_count; j++) {
                Attribute* attribute = &code_attribute->Code.attribute_list[j];
                if (attribute->name->Utf8.atom == Atom_LineNumberTable)
                    line_number_attribute = attribute;
            }

            uint16_t* line_number_map = nullptr;

            if (line_number_attribute) {
                line_number_map = new uint16_t[code_attribute->Code.code_count];
                std::memset(line_number_map, 0, sizeof(uint16_t) * code_attribute->Code.code_count);

                for (int16_t i = 0; i < line_number_attribute->LineNumberTable.count - 1; i++) {
                    auto a = line_number_attribute->LineNumberTable.list[i];
                    auto b = line_number_attribute->LineNumberTable.list[i + 1];

                    for (uint16_t pc = a.start_pc; pc < b.start_pc; pc++)
                        line_number_map[pc] = a.line_number;
                }
                if (line_number_attribute->LineNumberTable.count) {
                    auto a = line_number_attribute->LineNumberTable.list[line_number_attribute->LineNumberTable.count - 1];
                    for (uint16_t pc = a.start_pc; pc < line_number_attribute->Code.code_count; pc++)
                        line_number_map[pc] = a.line_number;
                }
            }

            for (uint32_t pc = 0; pc < code_attribute->Code.code_count; pc++) {
                uint8_t code = code_attribute->Code.code_list[pc];

                uint32_t old_pc = pc;
                std::string line_number_string;
                if (line_number_map)
                    line_number_string = std::string(", line number: ")
                        .append(std::to_string(line_number_map[pc]));

                #define CLOSEOPCONDITIONAL()     \
                        subIndent();             \
                        indent(output);          \
                        output.append("end\n");  \
                        break;                   \
                    }                            
                #define OPCONDITIONAL(op, name)                           \
                    CLOSEOPCONDITIONAL()                                  \
                    case op: {                                            \
                        indent(output);                                   \
                        output.append("if pc == ")                        \
                            .append(std::to_string(pc))                   \
                            .append(" then -- " #name)  \
                            .append(line_number_string)                   \
                            .push_back('\n');                             \
                        addIndent();                                      

                #define SETPC indent(output); output.append("pc = ").append(std::to_string(pc + 1)).push_back('\n'); indent(output); output.append("continue\n");

                #define GETAUX(resultvar) uint8_t resultvar = code_attribute->Code.code_list[++pc];
                #define GETAUXTWOBYTE(resultvar, type) \
                    type resultvar; { \
                    GETAUX(indexbyte1) \
                    GETAUX(indexbyte2) \
                    resultvar = (indexbyte1 << 8) | indexbyte2; \
                    }

                // FIXME: test: monitorenter, monitorexit, invokeinterface
                // TODO: null checks (reference tag with value == nil is a null object, hopefully)
                // TODO: go back through all instructions that emulate a monitorenter and also figure out when the exit should occur cuz idk

                unsigned int old_indent = indent_counter;

                switch (code) {
                    case uint8_t(-1): { break;
                    OPCONDITIONAL(0x1, aconst_null)
                        indent(output);
                        output.append("store(stack, april.newReference())\n");

                        SETPC
                    OPCONDITIONAL(0x2, iconst_m1)
                        indent(output);
                        output.append("store(stack, april.newInteger(-1))\n");

                        SETPC
                    OPCONDITIONAL(0x3, iconst_0)
                        indent(output);
                        output.append("store(stack, april.newInteger(0))\n");

                        SETPC
                    OPCONDITIONAL(0x4, iconst_1)
                        indent(output);
                        output.append("store(stack, april.newInteger(1))\n");

                        SETPC
                    OPCONDITIONAL(0x5, iconst_2)
                        indent(output);
                        output.append("store(stack, april.newInteger(2))\n");

                        SETPC
                    OPCONDITIONAL(0x6, iconst_3)
                        indent(output);
                        output.append("store(stack, april.newInteger(3))\n");

                        SETPC
                    OPCONDITIONAL(0x7, iconst_4)
                        indent(output);
                        output.append("store(stack, april.newInteger(4))\n");

                        SETPC
                    OPCONDITIONAL(0x8, iconst_5)
                        indent(output);
                        output.append("store(stack, april.newInteger(5))\n");

                        SETPC
                    OPCONDITIONAL(0x9, lconst_0)
                        indent(output);
                        output.append("store(stack, april.newLong(0))\n");

                        SETPC
                    OPCONDITIONAL(0xa, lconst_1)
                        indent(output);
                        output.append("store(stack, april.newLong(1))\n");

                        SETPC
                    OPCONDITIONAL(0x10, bipush)
                        GETAUX(value)

                        indent(output);
                        output.append("store(stack, april.newInteger(april.wrapSigned(")
                            .append(std::to_string(value))
                            .append(", 8)))\n");

                        SETPC
                    OPCONDITIONAL(0x11, sipush)
                        GETAUXTWOBYTE(value, int16_t)

                        indent(output);
                        output.append("store(stack, april.newInteger(april.wrapSigned(")
                            .append(std::to_string(value))
                            .append(", 16)))\n");

                        SETPC
                    OPCONDITIONAL(0x12, ldc)
                        GETAUX(index);

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: ldc instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        // TODO: reference, class, and method types
                        switch (constant.tag) {
                            case ConstantType::String:
                                indent(output);
                                output.append("local value = april.newString(utf8.char(");
                                for (uint16_t j = 0; j < constant.String.string->Utf8.bytes_size - 1; j++)
                                    output.append(std::to_string((int) constant.String.string->Utf8.bytes[j]))
                                        .append(", ");

                                if (constant.String.string->Utf8.bytes_size)
                                    output.append(std::to_string((int) constant.String.string->Utf8.bytes[constant.String.string->Utf8.bytes_size - 1]));
                                output.append("))\n");
                                break;
                            case ConstantType::Integer:
                                indent(output);
                                output.append("local value = april.newInteger(")
                                    .append(std::to_string(constant.Integer.bytes))
                                    .append(")\n");
                                break;
                            case ConstantType::Float:
                                indent(output);
                                output.append("local value = april.newFloat(")
                                    .append(floatTostring(constant.Float.value))
                                    .append(")\n");
                                break;
                            default:
                                std::cerr << "[ERROR]: expected String, Integer, or Float for ldc instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                                return 1;
                                break;
                        }

                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x14, ldc2_w)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: ldc2_w instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        switch (constant.tag) {
                            case ConstantType::Long:
                                indent(output);
                                output.append("local value = april.newLong(")
                                    .append(std::to_string(constant.Long.bytes))
                                    .append(")\n");
                                break;
                            case ConstantType::Double:
                                indent(output);
                                output.append("local value = april.newDouble(")
                                    .append(doubleTostring(constant.Double.value))
                                    .append(")\n");
                                break;

                            default:
                                std::cerr << "[ERROR]: expected Long or Double for ldc2_w instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                                return 1;
                                break;
                        }

                        indent(output);
                        output.append("store(stack, value);\n");

                        SETPC
                    OPCONDITIONAL(0x15, iload)
                        GETAUX(index)

                        if (index < 0 || index >= code_attribute->Code.max_locals) {
                            std::cerr << "[ERROR]: iload instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;
                        std::string index_str = std::to_string(index);

                        indent(output);
                        output.append("local value = local_array[")
                            .append(index_str)
                            .append("]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index ")
                            .append(index_str)
                            .append(" in instruction iload\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) in local array index ")
                            .append(index_str)
                            .append(" in instruction iload; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x16, lload)
                        GETAUX(index)

                        if (index < 0 || index >= code_attribute->Code.max_locals - 1) {
                            std::cerr << "[ERROR]: lload instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;
                        std::string index_str = std::to_string(index);

                        indent(output);
                        output.append("local value = local_array[")
                            .append(index_str)
                            .append("]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index ")
                            .append(index_str)
                            .append(" in instruction lload\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) in local array index ")
                            .append(index_str)
                            .append(" in instruction lload; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x19, aload)
                        GETAUX(index)

                        if (index < 0 || index >= code_attribute->Code.max_locals) {
                            std::cerr << "[ERROR]: aload instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;
                        std::string index_str = std::to_string(index);

                        indent(output);
                        output.append("local value = local_array[")
                            .append(index_str)
                            .append("]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index ")
                            .append(index_str)
                            .append(" in instruction aload\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) in local array index ")
                            .append(index_str)
                            .append(" in instruction aload; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x1a, iload_0)
                        indent(output);
                        output.append("local value = local_array[1]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 1 in instruction iload_0\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) in local array index 1 in instruction iload_0; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x1b, iload_1)
                        indent(output);
                        output.append("local value = local_array[2]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 2 in instruction iload_1\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) in local array index 2 in instruction iload_1; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x1c, iload_2)
                        indent(output);
                        output.append("local value = local_array[3]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 3 in instruction iload_2\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) in local array index 3 in instruction iload_2; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x1d, iload_3)
                        indent(output);
                        output.append("local value = local_array[4]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 4 in instruction iload_3\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) in local array index 4 in instruction iload_3; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x1e, lload_0)
                        indent(output);
                        output.append("local value = local_array[1]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 1 in instruction lload_0\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) in local array index 1 in instruction lload_0; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x1f, lload_1)
                        indent(output);
                        output.append("local value = local_array[2]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 2 in instruction lload_1\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) in local array index 2 in instruction lload_1; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x20, lload_2)
                        indent(output);
                        output.append("local value = local_array[3]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 3 in instruction lload_2\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) in local array index 3 in instruction lload_2; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x21, lload_3)
                        indent(output);
                        output.append("local value = local_array[4]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 4 in instruction lload_3\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) in local array index 4 in instruction lload_3; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x2a, aload_0)
                        indent(output);
                        output.append("local value = local_array[1]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 1 in instruction aload_0\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) in local array index 1 in instruction aload_0; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x2b, aload_1)
                        indent(output);
                        output.append("local value = local_array[2]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 2 in instruction aload_1\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) in local array index 2 in instruction aload_1; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x2c, aload_2)
                        indent(output);
                        output.append("local value = local_array[3]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 3 in instruction aload_2\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) in local array index 3 in instruction aload_2; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x2d, aload_3)
                        indent(output);
                        output.append("local value = local_array[4]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index 4 in instruction aload_3\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) in local array index 4 in instruction aload_3; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0x2e, iaload)
                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("local array = pop()\n");

                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer type) for index in iaload instruction; \" .. index.tag)\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in iaload instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in iaload instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class == april.getArrayClass(\"[I\"), \"invalid value (not integer array) for array in iaload instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("store(stack, april.clone(array.value.list[index.value + 1]))\n");

                        SETPC
                    OPCONDITIONAL(0x32, aaload)
                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("local array = pop()\n");

                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer type) for index in aaload instruction; \" .. index.tag)\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in aaload instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in aaload instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class.valuedescriptor.raw:match(\"^[%[L]\"), \"invalid value (not reference array) for array in aaload instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("store(stack, april.clone(array.value.list[index.value + 1]))\n");

                        SETPC
                    OPCONDITIONAL(0x33, baload)
                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer) for index in baload instruction; \" .. index.tag)\n");

                        indent(output);
                        output.append("local array = pop()\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in baload instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in baload instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class == april.getArrayClass(\"[B\") or array.value.class == april.getArrayClass(\"[Z\"), \"invalid value (not byte or boolean array) for array in baload instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("store(stack, april.coerce(array.value.list[index.value + 1], descriptor_parser.parseFieldDescriptor('I')))\n");

                        SETPC
                    OPCONDITIONAL(0x34, caload)
                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer) for index in caload instruction; \" .. index.tag)\n");

                        indent(output);
                        output.append("local array = pop()\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in caload instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in caload instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class == april.getArrayClass(\"[C\"), \"invalid value (not char array) for array in caload instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(array.value.list[index.value + 1].value))\n");

                        SETPC
                    OPCONDITIONAL(0x36, istore)
                        GETAUX(index)

                        if (index < 0 || index >= code_attribute->Code.max_locals) {
                            std::cerr << "[ERROR]: istore instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in instruction istore; \" .. value.tag)\n");
                        output.append("store(local_array, value, ")
                            .append(std::to_string(index))
                            .append(")\n");

                        SETPC
                    OPCONDITIONAL(0x37, lstore)
                        GETAUX(index)

                        if (index < 0 || index >= code_attribute->Code.max_locals - 1) {
                            std::cerr << "[ERROR]: lstore instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) at the top of the stack in instruction lstore; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, ")
                            .append(std::to_string(index))
                            .append(")\n");

                        SETPC
                    OPCONDITIONAL(0x3a, astore)
                        GETAUX(index)

                        if (index < 0 || index >= code_attribute->Code.max_locals) {
                            std::cerr << "[ERROR]: astore instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        // TODO: returnAddress type
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) at the top of the stack in instruction astore; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, ")
                            .append(std::to_string(index))
                            .append(")\n");

                        SETPC
                    OPCONDITIONAL(0x3b, istore_0)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in instruction istore_0; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 1)\n");

                        SETPC
                    OPCONDITIONAL(0x3c, istore_1)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in instruction istore_1; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 2)\n");

                        SETPC
                    OPCONDITIONAL(0x3d, istore_2)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in instruction istore_2; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 3)\n");

                        SETPC
                    OPCONDITIONAL(0x3e, istore_3)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in instruction istore_3; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 4)\n");

                        SETPC
                    OPCONDITIONAL(0x3f, lstore_0)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) at the top of the stack in instruction lstore_0; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 1)\n");

                        SETPC
                    OPCONDITIONAL(0x40, lstore_1)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) at the top of the stack in instruction lstore_1; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 2)\n");

                        SETPC
                    OPCONDITIONAL(0x41, lstore_2)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) at the top of the stack in instruction lstore_2; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 3)\n");

                        SETPC
                    OPCONDITIONAL(0x42, lstore_3)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"invalid value (not long type) at the top of the stack in instruction lstore_3; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 4)\n");

                        SETPC
                    OPCONDITIONAL(0x4b, astore_0)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        // TODO: returnAddress type
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) at the top of the stack in instruction astore_0; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 1)\n");

                        SETPC
                    OPCONDITIONAL(0x4c, astore_1)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        // TODO: returnAddress type
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) at the top of the stack in instruction astore_1; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 2)\n");

                        SETPC
                    OPCONDITIONAL(0x4d, astore_2)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        // TODO: returnAddress type
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) at the top of the stack in instruction astore_2; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 3)\n");

                        SETPC
                    OPCONDITIONAL(0x4e, astore_3)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        // TODO: returnAddress type
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) at the top of the stack in instruction astore_3; \" .. value.tag)\n");
                        indent(output);
                        output.append("store(local_array, value, 4)\n");

                        SETPC
                    OPCONDITIONAL(0x4f, iastore)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in iastore instruction; \" .. value.tag)\n");

                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer) for index in iastore instruction; \" .. index.tag)\n");

                        indent(output);
                        output.append("local array = pop()\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in iastore instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in iastore instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class == april.getArrayClass(\"[I\"), \"invalid value (not integer array) for array in iastore instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("array.value.list[index.value + 1] = april.clone(value)\n");

                        SETPC
                    OPCONDITIONAL(0x53, aastore)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"invalid value (not reference type) at the top of the stack in aastore instruction; \" .. value.tag)\n");

                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer) for index in aastore instruction; \" .. index.tag)\n");

                        indent(output);
                        output.append("local array = pop()\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in aastore instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in aastore instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class.valuedescriptor.value.tag == \"Class\" or array.value.class.valuedescriptor.value.tag == \"Array\", \"invalid value (not class or multidimensional array) for array in aastore instruction\")\n");

                        // indent(output);
                        // output.append("print(\"ONEONEONE\", value.value.class)\n");
                        // indent(output);
                        // output.append("print(\"TWOTWOTWO\", array.value.class)\n");

                        indent(output);
                        output.append("assert(april.instanceofByClassAndTargetClass(value.value.class, april.getClass(array.value.class.name:sub(2, -1))), \"value type did not match array type\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("array.value.list[index.value + 1] = april.clone(value)\n");

                        SETPC
                    OPCONDITIONAL(0x54, bastore)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in bastore instruction; \" .. value.tag)\n");

                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer) for index in bastore instruction; \" .. index.tag)\n");

                        indent(output);
                        output.append("local array = pop()\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in bastore instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in bastore instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class == april.getArrayClass(\"[Z\") or array.value.class == april.getArrayClass(\"[B\"), \"invalid value (not boolean or char array) for array in bastore instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("array.value.list[index.value + 1] = april.coerce(value, array.value.class.valuedescriptor)\n");

                        SETPC
                    OPCONDITIONAL(0x55, castore)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) at the top of the stack in castore instruction; \" .. value.tag)\n");

                        indent(output);
                        output.append("local index = pop()\n");
                        indent(output);
                        output.append("assert(index.tag == \"integer\", \"invalid value (not integer) for index in castore instruction; \" .. index.tag)\n");

                        indent(output);
                        output.append("local array = pop()\n");
                        indent(output);
                        output.append("assert(array.tag == \"reference\", \"invalid value (not reference type) for array in castore instruction; \" .. array.tag)\n");
                        indent(output);
                        output.append("assert(array.value.class.isarray, \"invalid value (not array) for array in castore instruction\")\n");
                        indent(output);
                        output.append("assert(array.value.class == april.getArrayClass(\"[C\"), \"invalid value (not char array) for array in castore instruction\")\n");

                        // TODO: outofbounds exception
                        indent(output);
                        output.append("assert(index.value >= 0 and index.value < array.value.size, \"out of bounds\")\n");

                        indent(output);
                        output.append("array.value.list[index.value + 1] = april.coerce(value, descriptor_parser.parseFieldDescriptor('I'))\n");

                        SETPC
                    OPCONDITIONAL(0x57, pop)
                        indent(output);
                        output.append("local value = pop()\n");

                        indent(output);
                        output.append("assert(value.category1, \"value wasn't category 1\")\n");

                        SETPC
                    OPCONDITIONAL(0x58, pop2)
                        indent(output);
                        output.append("local value = pop()\n");

                        indent(output);
                        output.append("if value.category1 then\n");
                        addIndent();

                        indent(output);
                        output.append("pop()\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x59, dup)
                        indent(output);
                        output.append("local value = stack[#stack]\n");
                        indent(output);
                        // TODO: error message
                        output.append("assert(value, \"no value\")\n");
                        indent(output);
                        // TODO: error message
                        output.append("assert(value.category1, \"value wasn't category 1\")\n");
                        indent(output);
                        output.append("store(stack, april.clone(value))\n");

                        SETPC
                    OPCONDITIONAL(0x60, iadd)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in iadd was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in iadd was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(value1.value + value2.value))\n");

                        SETPC
                    OPCONDITIONAL(0x61, ladd)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"long\", \"value1 in ladd was not a long\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"long\", \"value2 in ladd was not a long\")\n");

                        indent(output);
                        output.append("store(stack, april.newLong(value1.value + value2.value))\n");

                        SETPC
                    OPCONDITIONAL(0x64, isub)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in isub was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in isub was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(value1.value - value2.value))\n");

                        SETPC
                    OPCONDITIONAL(0x65, lsub)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"long\", \"value1 in lsub was not a long\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"long\", \"value2 in lsub was not a long\")\n");

                        indent(output);
                        output.append("store(stack, april.newLong(value1.value - value2.value))\n");

                        SETPC
                    OPCONDITIONAL(0x68, imul)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in imul was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in imul was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(value1.value * value2.value))\n");

                        SETPC
                    OPCONDITIONAL(0x6b, dmul)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"double\", \"value1 in dmul was not a double\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"double\", \"value2 in dmul was not a double\")\n");

                        indent(output);
                        output.append("store(stack, april.newDouble(value1.value * value2.value))\n");

                        SETPC
                    OPCONDITIONAL(0x74, ineg)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in ineg was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(-value.value))\n");

                        SETPC
                    OPCONDITIONAL(0x78, ishl)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in ishl was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in ishl was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(bit32.lshift(value1.value, bit32.band(value2.value, 0x1F))))\n");

                        SETPC
                    OPCONDITIONAL(0x7a, ishr)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in ishr was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in ishr was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(bit32.arshift(value1.value, bit32.band(value2.value, 0x1F))))\n");

                        SETPC
                    OPCONDITIONAL(0x7c, iushr)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in iushr was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in iushr was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(bit32.rshift(value1.value, bit32.band(value2.value, 0x1F))))\n");

                        SETPC
                    OPCONDITIONAL(0x7e, iand)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in iand was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in iand was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(bit32.band(value1.value, value2.value)))\n");

                        SETPC
                    OPCONDITIONAL(0x80, ior)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in ior was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in ior was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(bit32.bor(value1.value, value2.value)))\n");

                        SETPC
                    OPCONDITIONAL(0x82, ixor)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in ixor was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in ixor was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(bit32.bxor(value1.value, value2.value)))\n");

                        SETPC
                    OPCONDITIONAL(0x84, iinc)
                        GETAUX(index)
                        GETAUX(amount)

                        if (index < 0 || index >= code_attribute->Code.max_locals) {
                            std::cerr << "[ERROR]: iinc instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }

                        index++;
                        std::string index_str = std::to_string(index);

                        indent(output);
                        output.append("local value = local_array[")
                            .append(index_str)
                            .append("]\n");
                        indent(output);
                        output.append("assert(value, \"invalid value (nil) in local array index ")
                            .append(index_str)
                            .append(" in instruction iinc\")\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"invalid value (not integer type) in local array index ")
                            .append(index_str)
                            .append(" in instruction iinc; \" .. value.tag)\n");
                        indent(output);
                        output.append("value.value += ")
                            .append(std::to_string(amount))
                            .append("\n");

                        SETPC
                    OPCONDITIONAL(0x85, i2l)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in i2l was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newLong(value.value))\n");

                        SETPC
                    OPCONDITIONAL(0x8f, d2l)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"double\", \"value in d2l was not a double\")\n");

                        indent(output);
                        output.append("store(stack, april.newLong(value.value))\n");

                        SETPC
                    OPCONDITIONAL(0x91, i2b)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in i2b was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(april.wrapSigned(value.value, 8)))\n");

                        SETPC
                    OPCONDITIONAL(0x92, i2c)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in i2c was not a integer\")\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(april.wrapUnsigned(value.value, 16)))\n");

                        SETPC
                    OPCONDITIONAL(0x94, lcmp)
                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"long\", \"value1 in lcmp was not a long\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"long\", \"value2 in lcmp was not a long\")\n");

                        indent(output);
                        output.append("local num = if value1.value > value2.value then 1 elseif value1.value < value2.value then -1 else 0\n");

                        indent(output);
                        output.append("store(stack, april.newInteger(num))\n");

                        SETPC
                    OPCONDITIONAL(0x99, ifeq)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifeq instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in ifeq was not a integer\")\n");

                        indent(output);
                        output.append("if value.value == 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x9a, ifne)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifne instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in ifne was not a integer\")\n");

                        indent(output);
                        output.append("if value.value ~= 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x9b, iflt)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: iflt instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in iflt was not a integer\")\n");

                        indent(output);
                        output.append("if value.value < 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x9c, ifge)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifge instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in ifge was not a integer\")\n");

                        indent(output);
                        output.append("if value.value >= 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x9d, ifgt)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifgt instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in ifgt was not a integer\")\n");

                        indent(output);
                        output.append("if value.value > 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x9e, ifle)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifle instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value in ifle was not a integer\")\n");

                        indent(output);
                        output.append("if value.value <= 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0x9f, if_icmpeq)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_icmpeq instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in if_icmpeq was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in if_icmpeq was not a integer\")\n");

                        indent(output);
                        output.append("if value1.value == value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa0, if_icmpne)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_icmpne instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in if_icmpne was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in if_icmpne was not a integer\")\n");

                        indent(output);
                        output.append("if value1.value ~= value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa1, if_icmplt)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_icmplt instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in if_icmplt was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in if_icmplt was not a integer\")\n");

                        indent(output);
                        output.append("if value1.value <= value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa2, if_icmpge)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_icmpge instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in if_icmpge was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in if_icmpge was not a integer\")\n");

                        indent(output);
                        output.append("if value1.value >= value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa3, if_icmpgt)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_icmpgt instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in if_icmpgt was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in if_icmpgt was not a integer\")\n");

                        indent(output);
                        output.append("if value1.value > value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa4, if_icmple)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_icmple instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"integer\", \"value1 in if_icmple was not a integer\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"integer\", \"value2 in if_icmple was not a integer\")\n");

                        indent(output);
                        output.append("if value1.value <= value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa5, if_acmpeq)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_acmpeq instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"reference\", \"value1 in if_acmpeq was not a reference\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"reference\", \"value2 in if_acmpeq was not a reference\")\n");

                        indent(output);
                        output.append("if value1.value == value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa6, if_acmpne)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: if_acmpne instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value2 = pop()\n");
                        indent(output);
                        output.append("local value1 = pop()\n");
                        indent(output);
                        output.append("assert(value1.tag == \"reference\", \"value1 in if_acmpne was not a reference\")\n");
                        indent(output);
                        output.append("assert(value2.tag == \"reference\", \"value2 in if_acmpne was not a reference\")\n");

                        indent(output);
                        output.append("if value1.value ~= value2.value then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xa7, goto)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: goto instruction #" << old_pc << " had an out-of-range target " << target << " (offset: " << offset << ')' << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");
                    OPCONDITIONAL(0xac, ireturn)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"integer\", \"value was not an integer in ireturn\")\n");
                        char byte = method.descriptor->Utf8.bytes[method.descriptor->Utf8.bytes_size - 1];
                        switch (byte) {
                            case 'I':
                                break;
                            case 'C':
                            case 'Z':
                                indent(output);
                                output.append("value = april.coerce(value, descriptor_parser.parseFieldDescriptor('")
                                    .push_back(byte);
                                output.append("'))\n");
                                break;
                            default:
                                std::cerr << "[ERROR] unsupported return type " << utf8Tostring(method.descriptor->Utf8) << std::endl;
                                break;
                        }
                        indent(output);
                        output.append("return value\n");
                    OPCONDITIONAL(0xad, lreturn)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"long\", \"value was not an long in lreturn\")\n");
                        if (method.descriptor->Utf8.bytes[method.descriptor->Utf8.bytes_size - 1] != 'J') {
                            std::cerr << "[ERROR] unsupported return type " << utf8Tostring(method.descriptor->Utf8) << std::endl;
                            break;
                        }
                        indent(output);
                        output.append("return value\n");
                    OPCONDITIONAL(0xb0, areturn)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value was not an reference in areturn\")\n");
                        indent(output);
                        output.append("return value\n");
                    OPCONDITIONAL(0xb1, return)
                        indent(output);
                        output.append("return\n");
                    OPCONDITIONAL(0xb2, getstatic)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: getstatic instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Fieldref) {
                            std::cerr << "[ERROR]: expected Fieldref for getstatic instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local field, class = april.lookupField(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\", true)\n");

                        indent(output);
                        output.append("local object = class.staticinstance\n");
                        indent(output);
                        output.append("local value = object[field]\n");
                        indent(output);
                        output.append("store(stack, value)\n");

                        SETPC
                    OPCONDITIONAL(0xb3, putstatic)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: putstatic instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Fieldref) {
                            std::cerr << "[ERROR]: expected Fieldref for putstatic instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local field, class = april.lookupField(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\", true)\n");

                        // TODO: typecheck

                        indent(output);
                        output.append("local object = class.staticinstance\n");
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("object[field] = value\n");

                        SETPC
                    OPCONDITIONAL(0xb4, getfield)
                        GETAUXTWOBYTE(index, uint16_t);

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: getfield instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Fieldref) {
                            std::cerr << "[ERROR]: expected Fieldref for getfield instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local field = april.lookupField(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);

                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\", true)\n");

                        // TODO: check protected
                        // TODO: check array type

                        indent(output);
                        output.append("local object = pop()\n");
                        indent(output);
                        output.append("store(stack, object[field])\n");

                        SETPC
                    OPCONDITIONAL(0xb5, putfield)
                        GETAUXTWOBYTE(index, uint16_t);

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: putfield instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Fieldref) {
                            std::cerr << "[ERROR]: expected Fieldref for putfield instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local field = april.lookupField(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\", true)\n");

                        // TODO: typecheck

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("local object = pop()\n");
                        indent(output);
                        output.append("object[field] = value\n");

                        SETPC
                    OPCONDITIONAL(0xb6, invokevirtual)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: invokevirtual instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        // TODO: InterfaceMethodref
                        if (constant.tag != ConstantType::Methodref) {
                            std::cerr << "[ERROR]: expected Methodref for invokevirtual instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        // TODO: handle protected flag
                        // TODO: signature polymorphic methods (see 2.9)

                        indent(output);
                        output.append("local methodtype = \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\"\n");

                        indent(output);
                        output.append("local descriptor = descriptor_parser.parseMethodDescriptor(methodtype)\n");
                        indent(output);
                        output.append("local parameter_count = descriptor.parameter_count\n");
                        indent(output);
                        output.append("local stacksize = descriptor.size\n");

                        indent(output);
                        output.append("local method = april.lookupMethod(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", methodtype, true)\n");

                        indent(output);
                        output.append("local object = stack[#stack - stacksize]\n");

                        // TODO: check if object is a valid instance?

                        indent(output);
                        output.append("local methodoverride\n");
                        indent(output);
                        output.append("pcall(function()\n");
                        addIndent();

                        indent(output);
                        output.append("methodoverride = april.lookupMethod(object.class.name, \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", methodtype, true)\n");

                        subIndent();
                        indent(output);
                        output.append("end)\n");

                        indent(output);
                        output.append("if methodoverride then\n");
                        addIndent();

                        indent(output);
                        output.append("method = methodoverride\n");
                        subIndent();

                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("for i = parameter_count, 1, -1 do\n");
                        addIndent();

                        indent(output);
                        output.append("argarrayscratch[i] = april.coerce(pop(), descriptor.parameter_list[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("local results = table.pack(method.func(object, table.unpack(argarrayscratch, 1, parameter_count)))\n");

                        indent(output);
                        output.append("table.clear(argarrayscratch)\n");

                        indent(output);
                        output.append("for i = 1, results.n do\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, results[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xb7, invokespecial)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: invokespecial instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        // TODO: InterfaceMethodref
                        if (constant.tag != ConstantType::Methodref) {
                            std::cerr << "[ERROR]: expected Methodref for invokespecial instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local methodtype = \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\"\n");

                        indent(output);
                        output.append("local descriptor = descriptor_parser.parseMethodDescriptor(methodtype)\n");
                        indent(output);
                        output.append("local parameter_count = descriptor.parameter_count\n");
                        indent(output);
                        output.append("local stacksize = descriptor.size\n");

                        indent(output);
                        output.append("local method = april.lookupMethodSpecial(\"")
                            .append(class_name)
                            .append("\", \"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", methodtype, true)\n");

                        indent(output);
                        output.append("local object = stack[#stack - stacksize]\n");

                        // TODO: check if object is a valid instance?

                        indent(output);
                        output.append("for i = parameter_count, 1, -1 do\n");
                        addIndent();

                        indent(output);
                        output.append("argarrayscratch[i] = april.coerce(pop(), descriptor.parameter_list[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("local results = table.pack(method.func(object, table.unpack(argarrayscratch, 1, parameter_count)))\n");

                        indent(output);
                        output.append("table.clear(argarrayscratch)\n");

                        indent(output);
                        output.append("for i = 1, results.n do\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, results[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xb8, invokestatic)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: invokestatic instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        // TODO: InterfaceMethodref
                        if (constant.tag != ConstantType::Methodref) {
                            std::cerr << "[ERROR]: expected Methodref for invokestatic instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        if (constant.GeneralRef.name_and_type->NameAndType.name->Utf8.atom == Atom_kInstanceInitialization || constant.GeneralRef.name_and_type->NameAndType.name->Utf8.atom == Atom_kClassInitialization) {
                            std::cerr << "[ERROR]: invokestatic instruction #" << old_pc << "'s method was an instance or class initialization method" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local methodtype = \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\"\n");

                        indent(output);
                        output.append("local descriptor = descriptor_parser.parseMethodDescriptor(methodtype)\n");
                        indent(output);
                        output.append("local parameter_count = descriptor.parameter_count\n");

                        indent(output);
                        output.append("local method = april.lookupMethod(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", methodtype, true)\n");

                        indent(output);
                        output.append("assert(method.isstatic, \"method was not static in invokestatic instruction #")
                            .append(std::to_string(old_pc))
                            .append("\")\n");
                        indent(output);
                        output.append("assert(not method.isabstract, \"method was abstract in invokestatic instruction #")
                            .append(std::to_string(old_pc))
                            .append("\")\n");

                        // TODO: check if object is a valid instance?

                        indent(output);
                        output.append("for i = parameter_count, 1, -1 do\n");
                        addIndent();

                        indent(output);
                        output.append("argarrayscratch[i] = april.coerce(pop(), descriptor.parameter_list[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("local results = table.pack(method.func(table.unpack(argarrayscratch, 1, parameter_count)))\n");

                        indent(output);
                        output.append("table.clear(argarrayscratch)\n");

                        indent(output);
                        output.append("for i = 1, results.n do\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, results[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xb9, invokeinterface)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: invokeinterface instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::InterfaceMethodref) {
                            std::cerr << "[ERROR]: expected InterfaceMethodref for invokeinterface instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        if (constant.GeneralRef.name_and_type->NameAndType.name->Utf8.atom == Atom_kInstanceInitialization || constant.GeneralRef.name_and_type->NameAndType.name->Utf8.atom == Atom_kClassInitialization) {
                            std::cerr << "[ERROR]: invokeinterface instruction #" << old_pc << "'s method was an instance or class initialization method" << std::endl;
                            return 1;
                        }

                        pc++; // skip count (we will use the descriptor)
                        pc++; // skip funny bit

                        // TODO: handle protected flag
                        // TODO: signature polymorphic methods (see 2.9)

                        indent(output);
                        output.append("local methodtype = \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                        output.append("\"\n");

                        indent(output);
                        output.append("local descriptor = descriptor_parser.parseMethodDescriptor(methodtype)\n");
                        indent(output);
                        output.append("local parameter_count = descriptor.parameter_count\n");
                        indent(output);
                        output.append("local stacksize = descriptor.size\n");

                        // TODO: use 5.4.3.4 to verify this lookup...
                        indent(output);
                        output.append("local method = april.lookupMethod(\"");
                        output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                        output.append("\", \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", methodtype, true)\n");

                        indent(output);
                        output.append("local object = stack[#stack - stacksize]\n");

                        // TODO: check if object is a valid instance?

                        indent(output);
                        output.append("local methodoverride\n");
                        indent(output);
                        output.append("pcall(function()\n");
                        addIndent();

                        indent(output);
                        output.append("methodoverride = april.lookupMethod(object.class.name, \"");
                        output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                        output.append("\", methodtype, true)\n");

                        subIndent();
                        indent(output);
                        output.append("end)\n");

                        indent(output);
                        output.append("if methodoverride then\n");
                        addIndent();

                        indent(output);
                        output.append("method = methodoverride\n");
                        subIndent();

                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("for i = parameter_count, 1, -1 do\n");
                        addIndent();

                        indent(output);
                        output.append("argarrayscratch[i] = april.coerce(pop(), descriptor.parameter_list[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("local results = table.pack(method.func(object, table.unpack(argarrayscratch, 1, parameter_count)))\n");

                        indent(output);
                        output.append("table.clear(argarrayscratch)\n");

                        indent(output);
                        output.append("for i = 1, results.n do\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, results[i])\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xbb, new)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: new instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Class) {
                            std::cerr << "[ERROR]: expected Class for new instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("store(stack, april.newReference(april.newObjectFromClassName(\"");
                        output.insert(output.end(), constant.Class.name->Utf8.bytes, constant.Class.name->Utf8.bytes + constant.Class.name->Utf8.bytes_size);
                        output.append("\")))\n");

                        SETPC
                    OPCONDITIONAL(0xbc, newarray)
                        GETAUX(atype)

                        indent(output);
                        output.append("local count = pop()\n");
                        indent(output);
                        output.append("assert(count.tag == \"integer\", \"value in newarray was not an integer\")\n");

                        // TODO: exceptions
                        indent(output);
                        output.append("assert(count.value > 0)\n");

                        indent(output);
                        output.append("local atype = \"");

                        switch (atype) {
                            case 4:
                                output.push_back('Z');
                                break;
                            case 5:
                                output.push_back('C');
                                break;
                            case 6:
                                output.push_back('F');
                                break;
                            case 7:
                                output.push_back('D');
                                break;
                            case 8:
                                output.push_back('B');
                                break;
                            case 9:
                                output.push_back('S');
                                break;
                            case 10:
                                output.push_back('I');
                                break;
                            case 11:
                                output.push_back('J');
                                break;
                            default:
                                std::cerr << "[ERROR]: newarray instruction #" << old_pc << " had an invalid atype (" << (int)atype << ')' << std::endl;
                                return 1;
                                break;
                        }

                        output.append("\"\n");

                        indent(output);
                        output.append("store(stack, april.newReference(april.newArray(atype, count.value)))\n");

                        SETPC
                    OPCONDITIONAL(0xbd, anewarray)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: anewarray instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Class) {
                            std::cerr << "[ERROR]: expected Class for anewarray instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local count = pop()\n");
                        indent(output);
                        output.append("assert(count.tag == \"integer\", \"value in anewarray was not an integer\")\n");

                        // TODO: exceptions
                        indent(output);
                        output.append("assert(count.value > 0)\n");

                        indent(output);
                        output.append("local atype = \"");
                        output.insert(output.end(), constant.Class.name->Utf8.bytes, constant.Class.name->Utf8.bytes + constant.Class.name->Utf8.bytes_size);
                        output.append("\"\n");

                        indent(output);
                        output.append("store(stack, april.newReference(april.newArray(atype, count.value)))\n");

                        SETPC
                    OPCONDITIONAL(0xbe, arraylength)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in athrow was not a reference\")\n");
                        indent(output);
                        output.append("assert(value.value.class.isarray, \"value in athrow was not an array\")\n");
                        indent(output);
                        output.append("store(stack, april.newInteger(value.value.size))\n");

                        SETPC
                    OPCONDITIONAL(0xbf, athrow)
                        indent(output);
                        output.append("local value = pop()\n");

                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in athrow was not a reference\")\n");

                        indent(output);
                        output.append("assert(april.hasSuperClassByName(value.value.class, \"java/lang/Throwable\"), \"attempt to throw an object that doesn't inherit java/lang/Throwable\")\n");

                        indent(output);
                        output.append("error(\"EXCEPTION THROWN (TODO PROPER EXCEPTION HANDLING): \" .. value.value.class.name)\n");
                    OPCONDITIONAL(0xc0, checkcast)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: checkcast instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Class) {
                            std::cerr << "[ERROR]: expected Class for checkcast instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in checkcast was not a reference\")\n");

                        indent(output);
                        output.append("if type(value.value) == \"nil\" then\n");
                        addIndent();

                        SETPC

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("if april.instanceofByClassAndTargetName(value.value.class, \"");
                        output.insert(output.end(), constant.Class.name->Utf8.bytes, constant.Class.name->Utf8.bytes + constant.Class.name->Utf8.bytes_size);
                        output.append("\") then\n");
                        addIndent();

                        SETPC

                        subIndent();
                        indent(output);
                        output.append("else\n");
                        addIndent();

                        // TODO: exceptions

                        indent(output);
                        output.append("error(\"failed to check cast\")\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");
                    OPCONDITIONAL(0xc1, instanceof)
                        GETAUXTWOBYTE(index, uint16_t)

                        if (index < 1 || index > _class.constant_pool_count) {
                            std::cerr << "[ERROR]: instanceof instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                            return 1;
                        }
                        Constant& constant = _class.constant_pool[index - 1];
                        if (constant.tag != ConstantType::Class) {
                            std::cerr << "[ERROR]: expected Class for instanceof instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in instanceof was not a reference\")\n");

                        indent(output);
                        output.append("if type(value.value) == \"nil\" then\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, april.newInteger(0))\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        indent(output);
                        output.append("if april.instanceofByClassAndTargetName(value.value.class, \"");
                        output.insert(output.end(), constant.Class.name->Utf8.bytes, constant.Class.name->Utf8.bytes + constant.Class.name->Utf8.bytes_size);
                        output.append("\") then\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, april.newInteger(1))\n");

                        subIndent();
                        indent(output);
                        output.append("else\n");
                        addIndent();

                        indent(output);
                        output.append("store(stack, april.newInteger(0))\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xc2, monitorenter)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in monitorenter was not a reference\")\n");

                        indent(output);
                        output.append("local monitor = value.value.monitor\n");

                        indent(output);
                        output.append("local current_thread = coroutine.running()\n");

                        indent(output);
                        output.append("local owning_thread = monitor.thread\n");

                        indent(output);
                        output.append("if monitor.count == 0 then\n");
                        addIndent();

                        indent(output);
                        output.append("assert(not owning_thread, \"monitor count was 0, but it had an owning thread\")\n");

                        indent(output);
                        output.append("monitor.thread = current_thread\n");
                        indent(output);
                        output.append("monitor.count = 1\n");

                        subIndent();
                        indent(output);
                        output.append("elseif owning_thread == current_thread then\n");
                        addIndent();

                        indent(output);
                        output.append("monitor.count += 1\n");

                        subIndent();
                        indent(output);
                        output.append("else\n");
                        addIndent();

                        indent(output);
                        output.append("monitor.waiting_threads[#monitor.waiting_threads + 1] = current_thread\n");

                        indent(output);
                        output.append("coroutine.yield()\n");

                        indent(output);
                        output.append("owning_thread = monitor.thread\n");
                        indent(output);
                        output.append("assert(not owning_thread, \"monitor count was 0, but it had an owning thread\")\n");

                        indent(output);
                        output.append("monitor.thread = current_thread\n");
                        indent(output);
                        output.append("monitor.count = 1\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xc3, monitorexit)
                        indent(output);
                        output.append("local value = pop()\n");
                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in monitorexit was not a reference\")\n");

                        indent(output);
                        output.append("local monitor = value.value.monitor\n");

                        indent(output);
                        output.append("local current_thread = coroutine.running()\n");

                        indent(output);
                        output.append("local owning_thread = monitor.thread\n");

                        indent(output);
                        output.append("assert(owning_thread == current_thread, \"monitorexit called despite not owning the object\")\n");

                        indent(output);
                        output.append("monitor.count -= 1\n");

                        indent(output);
                        output.append("monitor.thread = nil\n");

                        indent(output);
                        output.append("while true do\n");
                        addIndent();

                            indent(output);
                            output.append("local waiting_thread = table.remove(monitor.waiting_threads, #monitor.waiting_threads)\n");

                            indent(output);
                            output.append("if not waiting_thread then\n");
                            addIndent();

                                indent(output);
                                output.append("break\n");

                            subIndent();
                            indent(output);
                            output.append("end\n");

                            indent(output);
                            output.append("coroutine.resume(waiting_thread)\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xc6, ifnull)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifnull instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");

                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in ifnull was not a reference\")\n");

                        indent(output);
                        output.append("if type(value.value) == \"nil\" then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC
                    OPCONDITIONAL(0xc7, ifnonnull)
                        GETAUXTWOBYTE(offset, int16_t)

                        uint32_t target = old_pc + offset;
                        if (target < 0 || target >= code_attribute->Code.code_count) {
                            std::cerr << "[ERROR]: ifnonnull instruction #" << old_pc << " had an out-of-range target" << std::endl;
                            return 1;
                        }

                        indent(output);
                        output.append("local value = pop()\n");

                        indent(output);
                        output.append("assert(value.tag == \"reference\", \"value in ifnonnull was not a reference\")\n");

                        indent(output);
                        output.append("if type(value.value) ~= \"nil\" then\n");
                        addIndent();

                        indent(output);
                        output.append("pc = ")
                            .append(std::to_string(target))
                            .push_back('\n');

                        indent(output);
                        output.append("continue\n");

                        subIndent();
                        indent(output);
                        output.append("end\n");

                        SETPC

                    CLOSEOPCONDITIONAL()
                    default:
                        std::cerr << "[WARNING]: unhandled opcode " << int(code) << std::endl;
                        return 1;
                        break;
                }

                if (indent_counter != old_indent) {
                    std::cerr << "[ERROR]: unbalanced indent level!" << std::endl;
                    return 1;
                }

                #undef CLOSEOPCONDITIONAL
                #undef OPCONDITIONAL
                #undef SETPC
                #undef GETAUX
                #undef GETAUXTWOBYTE
            }

            if (line_number_map)
                delete[] line_number_map;

            subIndent();
            indent(output);
            output.append("end\n");
        }

        subIndent();
        indent(output);
        output.append("end,\n");

        subIndent();
        indent(output);
        output.append("},\n");
    }

    subIndent();
    indent(output);
    output.append("},\n");
    subIndent();
    indent(output);
    output.append("}\napril.registerClass(\"")
        .append(class_name)
        .append("\", class_")
        .append(std::to_string(class_name_index))
        .append(")\nreturn class_")
        .append(std::to_string(class_name_index))
        .push_back('\n');

    return 0;
}
int generateLuau(Class& _class, std::string& output) {
    Attribute* sourcefile = nullptr;
    for (uint16_t i = 0; i < _class.attribute_count; i++) {
        Attribute* attribute = &_class.attribute_list[i];
        if (attribute->name->Utf8.atom == Atom_SourceFile) {
            sourcefile = attribute;
            break;
        }
    }

    output.append("--[[\n    this file was generated via april by techhog\n    see https://github.com/TechHog8984/april for more information\n    \n");
    if (sourcefile) {
        output.append("    file: ");
        output.insert(output.end(), sourcefile->SourceFile.sourcefile->Utf8.bytes, sourcefile->SourceFile.sourcefile->Utf8.bytes + sourcefile->SourceFile.sourcefile->Utf8.bytes_size);
        output.push_back('\n');
    }

    output.append("]]\n");

    output.append("assert(coroutine.isyieldable(), \"april needs to be ran in a yieldable thread\")\n");

    output.append("local april = require(\"@april/april\")\n")
        .append("local descriptor_parser = require(\"@april/descriptor\")\n")
        .append("local natives = require(\"@april/natives\")\n");

    if (outputClass(_class, output))
        return 1;

    if (!output.empty())
        output.erase(output.size() - 1, 1);
    return 0;
}
