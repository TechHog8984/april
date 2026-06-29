#include "codegenluau.hpp"
#include "classreader.hpp"

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

        output.append("\"] = function(...)\n");

        addIndent();

        Attribute* code_attribute = nullptr;
        for (uint16_t j = 0; j < method.attribute_count; j++) {
            Attribute* attribute = &method.attribute_list[j];
            if (attribute->name->Utf8.atom == Atom_Code)
                code_attribute = attribute;
        }
        if (!code_attribute) {
            std::cerr << "[ERROR]: failed to find code attribute in class " << class_name << "'s " << method.name->Utf8.bytes << " method" << i;
            return 1;
        }

        indent(output);
        output.append("local pc = 0\n");
        indent(output);
        output.append("local local_array = table.create(")
            .append(std::to_string(code_attribute->Code.max_locals))
            .append(")\n");

        indent(output);
        output.append("for i = 1, select('#', ...) do\n");
        addIndent();

        indent(output);
        output.append("local_array[i] = select(i, ...)\n");

        subIndent();
        indent(output);
        output.append("end\n");

        indent(output);
        output.append("local stack = table.create(")
            .append(std::to_string(code_attribute->Code.max_stack))
            .append(")\n");

        indent(output);
        output.append("local function push(value)\n");
        addIndent();

        indent(output);
        output.append("stack[#stack + 1] = value\n");

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
        output.append("return value\n");

        subIndent();
        indent(output);
        output.append("end\n");

        indent(output);
        output.append("local argarrayscratch = table.create(256)\n");

        indent(output);
        output.append("while true do\n");
        addIndent();

        indent(output);
        output.append("if false then\n");
        for (uint32_t pc = 0; pc < code_attribute->Code.code_count; pc++) {
            uint8_t code = code_attribute->Code.code_list[pc];

            uint32_t old_pc = pc;

            #define OPCONDITIONAL(op)                \
                    subIndent();                     \
                    break;                           \
                }                                    \
                case op: {                           \
                    indent(output);                  \
                    output.append("elseif pc == ")   \
                        .append(std::to_string(pc))  \
                        .append(" then\n");          \
                    addIndent();                      

            #define SETPC indent(output); output.append("pc = ").append(std::to_string(pc + 1)).push_back('\n');

            #define GETAUX(resultvar) uint8_t resultvar = code_attribute->Code.code_list[++pc];
            #define GETAUXCONSTINDEX(resultvar) \
                uint16_t resultvar; { \
                GETAUX(indexbyte1) \
                GETAUX(indexbyte2) \
                index = (indexbyte1 << 8) | indexbyte2; \
                }

            switch (code) {
                case uint8_t(-1): { break;
                OPCONDITIONAL(0x4) // iconst_1
                    indent(output);
                    output.append("push(1)\n");

                    SETPC
                OPCONDITIONAL(0x12) // ldc
                    GETAUX(index);

                    if (index < 1 || index > _class.constant_pool_count) {
                        std::cerr << "[ERROR]: instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                        return 1;
                    }
                    Constant& constant = _class.constant_pool[index - 1];
                    // TODO: reference, class, and method types
                    switch (constant.tag) {
                        case ConstantType::String:
                            indent(output);
                            output.append("push(utf8.char(");
                            for (uint16_t j = 0; j < constant.String.string->Utf8.bytes_size - 1; j++)
                                output.append(std::to_string((int) constant.String.string->Utf8.bytes[j]))
                                    .append(", ");

                            if (constant.String.string->Utf8.bytes_size)
                                output.append(std::to_string((int) constant.String.string->Utf8.bytes[constant.String.string->Utf8.bytes_size - 1]));
                            output.append("))\n");

                            SETPC
                            break;
                        case ConstantType::Integer:
                            indent(output);
                            output.append("push(")
                                .append(std::to_string(constant.Integer.bytes))
                                .append(")\n");

                            SETPC
                            break;
                        case ConstantType::Float:
                            indent(output);
                            output.append("push(")
                                .append(floatTostring(constant.Float.value))
                                .append(")\n");

                            SETPC
                            break;
                        default:
                            std::cerr << "[ERROR]: expected String, Integer, or Float for ldc instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                            break;
                    }
                OPCONDITIONAL(0x14) // ldc2_w
                    GETAUXCONSTINDEX(index)

                    if (index < 1 || index > _class.constant_pool_count) {
                        std::cerr << "[ERROR]: instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                        return 1;
                    }
                    Constant& constant = _class.constant_pool[index - 1];
                    switch (constant.tag) {
                        case ConstantType::Long:
                            indent(output);
                            output.append("push(")
                                .append(std::to_string(constant.Long.bytes))
                                .append(")\n");
                            break;
                        case ConstantType::Double:
                            indent(output);
                            output.append("push(")
                                .append(doubleTostring(constant.Double.value))
                                .append(")\n");
                            break;

                        default:
                            std::cerr << "[ERROR]: expected Double for ldc2_w instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                            return 1;
                            break;
                    }

                    SETPC
                OPCONDITIONAL(0x2a) // aload_0
                    indent(output);
                    output.append("push(local_array[1])\n");
                    SETPC
                OPCONDITIONAL(0xb1) // return
                    indent(output);
                    output.append("return\n");
                OPCONDITIONAL(0xb2) // getstatic
                    GETAUXCONSTINDEX(index)

                    if (index < 1 || index > _class.constant_pool_count) {
                        std::cerr << "[ERROR]: instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                        return 1;
                    }
                    Constant& constant = _class.constant_pool[index - 1];
                    if (constant.tag != ConstantType::Fieldref) {
                        std::cerr << "[ERROR]: expected Fieldref for getstatic instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                        return 1;
                    }

                    indent(output);
                    output.append("local field, class = classloader.lookupField(\"");
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
                    output.append("push(value)\n");

                    SETPC

                OPCONDITIONAL(0xb3) // putstatic
                    GETAUXCONSTINDEX(index)

                    if (index < 1 || index > _class.constant_pool_count) {
                        std::cerr << "[ERROR]: instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                        return 1;
                    }
                    Constant& constant = _class.constant_pool[index - 1];
                    if (constant.tag != ConstantType::Fieldref) {
                        std::cerr << "[ERROR]: expected Fieldref for putstatic instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                        return 1;
                    }

                    indent(output);
                    output.append("local field, class = classloader.lookupField(\"");
                    output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);

                    output.append("\", \"");
                    output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                    output.append("\", \"");
                    output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.descriptor->Utf8.bytes_size);
                    output.append("\", true)\n");

                    indent(output);
                    output.append("local object = class.staticinstance\n");
                    indent(output);
                    output.append("local value = pop()\n");
                    indent(output);
                    output.append("object[field] = value\n");

                    SETPC
                OPCONDITIONAL(0xb6) // invokevirtual
                    GETAUXCONSTINDEX(index)

                    if (index < 1 || index > _class.constant_pool_count) {
                        std::cerr << "[ERROR]: instruction #" << old_pc << "'s index was out of bounds" << std::endl;
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
                    output.append("local method, methodclass = classloader.lookupMethod(\"");
                    output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);

                    output.append("\", \"");
                    output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                    output.append("\", methodtype, true)\n");

                    indent(output);
                    output.append("local object = stack[#stack - parameter_count]\n");

                    // TODO: check if object is a valid instance?

                    indent(output);
                    output.append("local methodoverride, methodclassoverride\n");
                    indent(output);
                    output.append("pcall(function()\n");
                    addIndent();

                    indent(output);
                    output.append("methodoverride, methodclassoverride = classloader.lookupMethod(object.class.name, \"");
                    output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                    output.append("\", methodtype, true)\n");

                    subIndent();
                    indent(output);
                    output.append("end)\n");

                    indent(output);
                    output.append("if methodoverride then\n");
                    addIndent();

                    indent(output);
                    output.append("method, methodclass = methodoverride, methodclassoverride\n");
                    subIndent();

                    indent(output);
                    output.append("end\n");

                    indent(output);
                    output.append("for i = parameter_count, 1, -1 do\n");
                    addIndent();

                    indent(output);
                    output.append("argarrayscratch[i] = pop()\n");

                    subIndent();
                    indent(output);
                    output.append("end\n");

                    indent(output);
                    output.append("local results = table.pack(method(object, table.unpack(argarrayscratch, 1, parameter_count)))\n");

                    indent(output);
                    output.append("table.clear(argarrayscratch)\n");

                    indent(output);
                    output.append("for i = 1, results.n do\n");
                    addIndent();

                    indent(output);
                    output.append("push(results[i])\n");

                    subIndent();
                    indent(output);
                    output.append("end\n");

                    SETPC
                OPCONDITIONAL(0xb7) // invokespecial
                    GETAUXCONSTINDEX(index)

                    if (index < 1 || index > _class.constant_pool_count) {
                        std::cerr << "[ERROR]: instruction #" << old_pc << "'s index was out of bounds" << std::endl;
                        return 1;
                    }
                    Constant& constant = _class.constant_pool[index - 1];
                    // TODO: InterfaceMethodref
                    if (constant.tag != ConstantType::Methodref) {
                        std::cerr << "[ERROR]: expected Methodref for invokespecial instruction #" << old_pc << "'s tag but got " << constant_type_names.at(constant.tag) << std::endl;
                        return 1;
                    }

                    indent(output);
                    output.append("-- invokespecial on ");
                    output.insert(output.end(), constant.GeneralRef._class->Class.name->Utf8.bytes, constant.GeneralRef._class->Class.name->Utf8.bytes + constant.GeneralRef._class->Class.name->Utf8.bytes_size);
                    output.append("'s '");
                    output.insert(output.end(), constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes, constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes + constant.GeneralRef.name_and_type->NameAndType.name->Utf8.bytes_size);
                    output.append("' method\n");

                    SETPC

                subIndent();
                break; }
                default:
                    std::cerr << "[WARNING]: unhandled operand " << int(code) << std::endl;
                    break;
            }

            #undef OPCONDITIONAL
        }

        indent(output);
        output.append("end\n");

        subIndent();
        indent(output);
        output.append("end\n");

        subIndent();

        indent(output);
        output.append("end,\n");
    }

    subIndent();
    indent(output);
    output.append("},\n");
    subIndent();
    indent(output);
    // output.append("}\nEXPORTS[\"")
    //     .append(class_name)
    //     .append("\"] = class_")
    //     .append(std::to_string(class_name_index))
    //     .append("\nclassloader.registerClass(\"")
    output.append("}\nclassloader.registerClass(\"")
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

    output.append("--[[\n    this file was generated by april by techhog\n    see https://github.com/TechHog8984/april for more information\n    \n");
    if (sourcefile) {
        output.append("    file: ");
        output.insert(output.end(), sourcefile->SourceFile.sourcefile->Utf8.bytes, sourcefile->SourceFile.sourcefile->Utf8.bytes + sourcefile->SourceFile.sourcefile->Utf8.bytes_size);
        output.push_back('\n');
    }

    output.append("]]\n");

    // output.append("local EXPORTS = {}\n");

    if (outputClass(_class, output))
        return 1;

    // output.append("return EXPORTS");
    if (!output.empty())
        output.erase(output.size() - 1, 1);
    return 0;
}
