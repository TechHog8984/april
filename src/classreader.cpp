#include "classreader.hpp"
#include "bitreading.hpp"
#include "options.hpp"

#include <cmath>
#include <format>
#include <iostream>

std::string doubleTostring(DoubleValue& value) {
    switch (value.type) {
        case FloatingType::PositiveInfinity:
            return "(1 / 0)";
        case FloatingType::NegativeInfinity:
            return "(-1 / 0)";
        case FloatingType::NaN:
            return "(0 / 0)";
        case FloatingType::Normal:
            return std::to_string(value.value);
    }

    return "err";
}
std::string floatTostring(FloatValue& value) {
    switch (value.type) {
        case FloatingType::PositiveInfinity:
            return "(1 / 0)";
        case FloatingType::NegativeInfinity:
            return "(-1 / 0)";
        case FloatingType::NaN:
            return "(0 / 0)";
        case FloatingType::Normal:
            return std::to_string(value.value);
    }

    return "err";
}

std::string utf8Tostring(Utf8& utf8) {
    return std::string(reinterpret_cast<const char*>(utf8.bytes), utf8.bytes_size);
}

int readAttributeList(std::istream& classfile, uint16_t& attribute_count, Attribute*& attribute_list, uint16_t constant_pool_count, Constant* constant_pool, Attribute* parent_attribute) {
    // TODO: formatting trailing spaces need to adjust for when calling this inside a Code attribute (attribute lists can be nested, so we need to have a dynamic number of spaces)

    attribute_count = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "    attribute_count: " << attribute_count << std::endl;
    attribute_list = new Attribute[attribute_count];
    std::memset(attribute_list, 0, sizeof(Attribute) * attribute_count);

    for (uint16_t j = 0; j < attribute_count; j++) {
        Attribute& attribute = attribute_list[j];

        if (april_logging_enabled)
            std::cout << "      attribute #" << j << ':' << std::endl;

        attribute.name_index = readu2(classfile);
        if (attribute.name_index < 1 || attribute.name_index > constant_pool_count) {
            std::cerr << "[ERROR]: attribute #" << j << "'s name index was out of bounds" << std::endl;
            return 1;
        }
        attribute.name = &constant_pool[attribute.name_index - 1];
        if (attribute.name->tag != ConstantType::Utf8) {
            std::cerr << "[ERROR]: expected Utf8 for attribute #" << j << "'s name tag but got " << constant_type_names.at(attribute.name->tag) << std::endl;
            return 1;
        }

        if (april_logging_enabled)
            std::cout << "        name: " << utf8Tostring(attribute.name->Utf8) << std::endl;

        attribute.length = readu4(classfile);
        if (april_logging_enabled)
            std::cout << "        length: " << attribute.length << std::endl;

        std::streampos startpos = classfile.tellg();
        switch (attribute.name->Utf8.atom) {
            case Atom_ConstantValue:
                attribute.ConstantValue.value_index = readu2(classfile);
                if (attribute.ConstantValue.value_index < 1 || attribute.ConstantValue.value_index > constant_pool_count) {
                    std::cerr << "[ERROR]: attribute #" << j << "'s value index was out of bounds" << std::endl;
                    return 1;
                }
                attribute.ConstantValue.value = &constant_pool[attribute.ConstantValue.value_index - 1];
                switch (attribute.ConstantValue.value->tag) {
                    case ConstantType::String:
                        if (april_logging_enabled)
                            std::cout << "          string value (lazy conversion): " << utf8Tostring(attribute.ConstantValue.value->String.string->Utf8) << std::endl;
                        break;
                    case ConstantType::Integer:
                        if (april_logging_enabled)
                            std::cout << "          integer value: " << attribute.ConstantValue.value->Integer.bytes << std::endl;
                        break;
                    case ConstantType::Float:
                        if (april_logging_enabled)
                            std::cout << "          float value: " << floatTostring(attribute.ConstantValue.value->Float.value) << std::endl;
                        break;
                    case ConstantType::Long:
                        if (april_logging_enabled)
                            std::cout << "          long value: " << attribute.ConstantValue.value->Long.bytes << std::endl;
                        break;
                    case ConstantType::Double:
                        if (april_logging_enabled)
                            std::cout << "          double value: " << doubleTostring(attribute.ConstantValue.value->Double.value) << std::endl;
                        break;
                    default:
                        std::cerr << "[ERROR]: TODO - do the following bracketed types" << std::endl;
                        std::cerr << "[ERROR]: expected Long, [Float], [Double], Integer, [Short], [Char], [Byte], [Boolean], or [String] for attribute #" << j << "'s value tag but got " << constant_type_names.at(attribute.ConstantValue.value->tag) << std::endl;
                        return 1;
                        break;
                }

                break;
            case Atom_Code:
                attribute.Code.max_stack = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          max_stack: " << attribute.Code.max_stack << std::endl;
                attribute.Code.max_locals = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          max_locals: " << attribute.Code.max_locals << std::endl;

                attribute.Code.code_count = readu4(classfile);
                if (april_logging_enabled)
                    std::cout << "          code_count: " << attribute.Code.code_count << std::endl;
                if (attribute.Code.code_count < 1 || attribute.Code.code_count >= 65536) {
                    std::cerr << "[ERROR]: attribute #" << j << " had an invalid code count" << std::endl;
                    return 1;
                }
                attribute.Code.code_list = new uint8_t[attribute.Code.code_count];
                std::memset(attribute.Code.code_list, 0, sizeof(uint8_t) * attribute.Code.code_count);

                for (uint32_t j = 0; j < attribute.Code.code_count; j++)
                    attribute.Code.code_list[j] = readu1(classfile);

                attribute.Code.exception_count = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          exception_count: " << attribute.Code.exception_count << std::endl;
                attribute.Code.exception_list = new Exception[attribute.Code.exception_count];
                std::memset(attribute.Code.exception_list, 0, sizeof(Exception) * attribute.Code.exception_count);

                for (uint16_t j = 0; j < attribute.Code.exception_count; j++) {
                    Exception& exception = attribute.Code.exception_list[j];

                    if (april_logging_enabled)
                        std::cout << "            exception #" << j << std::endl;

                    exception.start_pc = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "            start_pc: " << exception.start_pc << std::endl;
                    exception.end_pc = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              end_pc: " << exception.end_pc << std::endl;

                    if (exception.start_pc < 0 || exception.start_pc >= attribute.Code.code_count) {
                        std::cerr << "[ERROR]: exception #" << j << " had an out-of-range start pc" << std::endl;
                        return 1;
                    }
                    if (exception.end_pc < 0 || exception.end_pc > attribute.Code.code_count) {
                        std::cerr << "[ERROR]: exception #" << j << " had an out-of-range end pc" << std::endl;
                        return 1;
                    }

                    if (exception.start_pc >= exception.end_pc) {
                        std::cerr << "[ERROR]: exception #" << j << "'s start pc was not less than its end pc" << std::endl;
                        return 1;
                    }

                    exception.handler_pc = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              handler_pc: " << exception.handler_pc << std::endl;

                    if (exception.handler_pc < 0 || exception.handler_pc >= attribute.Code.code_count) {
                        std::cerr << "[ERROR]: exception #" << j << " had an out-of-range handler pc" << std::endl;
                        return 1;
                    }
                    // TODO: parse code array and verify exception.handler_pc points to an opcode (see 4.7.3)

                    exception.catch_type_index = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              catch_type_index: " << exception.catch_type_index << std::endl;

                    if (exception.catch_type_index) {
                        if (exception.catch_type_index < 1 || exception.catch_type_index > constant_pool_count) {
                            std::cerr << "[ERROR]: exception #" << j << "'s catch type index was out of bounds" << std::endl;
                            return 1;
                        }
                        exception.catch_type = &constant_pool[exception.catch_type_index - 1];
                        if (exception.catch_type->tag != ConstantType::Class) {
                            std::cerr << "[ERROR]: expected Class for exception #" << j << "'s catch_type tag but got " << constant_type_names.at(exception.catch_type->tag) << std::endl;
                            return 1;
                        }

                        if (april_logging_enabled)
                            std::cout << "              catch_type: " << utf8Tostring(exception.catch_type->Class.name->Utf8) << std::endl;
                    }
                }

                if (april_logging_enabled)
                    std::cout << "          reading code attributes..." << std::endl;
                if (readAttributeList(classfile, attribute.Code.attribute_count, attribute.Code.attribute_list, constant_pool_count, constant_pool, &attribute))
                    return 1;
                if (april_logging_enabled)
                    std::cout << "          done reading code attributes" << std::endl;
                break;
            case Atom_StackMapTable:
                classfile.ignore(attribute.length);
                break;
            case Atom_Exceptions:
                attribute.Exceptions.exception_count = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          exception_count: " << attribute.Exceptions.exception_count << std::endl;
                attribute.Exceptions.exception_list = new Constant*[attribute.Exceptions.exception_count];
                std::memset(attribute.Exceptions.exception_list, 0, sizeof(Constant*) * attribute.Exceptions.exception_count);

                for (uint16_t j = 0; j < attribute.Exceptions.exception_count; j++) {
                    Constant*& exception = attribute.Exceptions.exception_list[j];

                    if (april_logging_enabled)
                        std::cout << "            exception #" << j << std::endl;

                    uint16_t index = readu2(classfile);

                    if (index < 1 || index > constant_pool_count) {
                        std::cerr << "[ERROR]: exception #" << j << "'s catch type index was out of bounds" << std::endl;
                        return 1;
                    }
                    exception = &constant_pool[index - 1];
                    if (exception->tag != ConstantType::Class) {
                        std::cerr << "[ERROR]: expected Class for exception #" << j << "'s catch_type tag but got " << constant_type_names.at(exception->tag) << std::endl;
                        return 1;
                    }

                    if (april_logging_enabled)
                        std::cout << "              class: " << utf8Tostring(exception->Class.name->Utf8) << std::endl;
                }
                break;
            case Atom_InnerClasses:
                attribute.InnerClasses.count = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          count: " << attribute.InnerClasses.count << std::endl;

                attribute.InnerClasses.list = new InnerClass[attribute.InnerClasses.count];
                std::memset(attribute.InnerClasses.list, 0, sizeof(InnerClass) * attribute.InnerClasses.count);

                if (april_logging_enabled)
                    std::cout << "          list: " << std::endl;
                for (uint32_t j = 0; j < attribute.InnerClasses.count; j++) {
                    InnerClass& inner_class = attribute.InnerClasses.list[j];

                    inner_class.inner_class_index = readu2(classfile);
                    if (inner_class.inner_class_index < 1 || inner_class.inner_class_index > constant_pool_count) {
                        std::cerr << "[ERROR]: attribute #" << j << "'s inner class index was out of bounds" << std::endl;
                        return 1;
                    }
                    inner_class.inner_class = &constant_pool[inner_class.inner_class_index - 1];
                    if (inner_class.inner_class->tag != ConstantType::Class) {
                        std::cerr << "[ERROR]: expected Class for attribute #" << j << "'s inner class tag but got " << constant_type_names.at(inner_class.inner_class->tag) << std::endl;
                        return 1;
                    }

                    if (april_logging_enabled)
                        std::cout << "            inner_class: " << utf8Tostring(inner_class.inner_class->Class.name->Utf8) << std::endl;

                    inner_class.outer_class_index = readu2(classfile);
                    if (inner_class.outer_class_index) {
                        if (inner_class.outer_class_index < 1 || inner_class.outer_class_index > constant_pool_count) {
                            std::cerr << "[ERROR]: attribute #" << j << "'s outer class index was out of bounds" << std::endl;
                            return 1;
                        }
                        inner_class.outer_class = &constant_pool[inner_class.outer_class_index - 1];
                        if (inner_class.outer_class->tag != ConstantType::Class) {
                            std::cerr << "[ERROR]: expected Class for attribute #" << j << "'s outer class tag but got " << constant_type_names.at(inner_class.outer_class->tag) << std::endl;
                            return 1;
                        }

                        if (april_logging_enabled)
                            std::cout << "            outer_class: " << utf8Tostring(inner_class.outer_class->Class.name->Utf8) << std::endl;
                    } else if (april_logging_enabled)
                        std::cout << "            outer_class: [none]" << std::endl;

                    inner_class.inner_name_index = readu2(classfile);
                    if (inner_class.inner_name_index) {
                        if (inner_class.inner_name_index < 1 || inner_class.inner_name_index > constant_pool_count) {
                            std::cerr << "[ERROR]: attribute #" << j << "'s inner name index was out of bounds" << std::endl;
                            return 1;
                        }
                        inner_class.inner_name = &constant_pool[inner_class.inner_name_index - 1];
                        if (inner_class.inner_name->tag != ConstantType::Utf8) {
                            std::cerr << "[ERROR]: expected Utf8 for attribute #" << j << "'s inner name tag but got " << constant_type_names.at(inner_class.inner_name->tag) << std::endl;
                            return 1;
                        }

                        if (april_logging_enabled)
                            std::cout << "            inner_class_name: " << utf8Tostring(inner_class.inner_name->Utf8) << std::endl;
                    } else if (april_logging_enabled)
                        std::cout << "            inner_class_name: [none]" << std::endl;

                    inner_class.inner_access_flags = readu2(classfile);
                    if (april_logging_enabled) {
                        std::cout << "            access flags: " << inner_class.inner_access_flags << " (";

                        if (inner_class.inner_access_flags & CLASS_ACC_PUBLIC)
                            std::cout << "PUBLIC, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_FINAL)
                            std::cout << "FINAL, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_SUPER)
                            std::cout << "SUPER, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_INTERFACE)
                            std::cout << "INTERFACE, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_ABSTRACT)
                            std::cout << "ABSTRACT, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_SYNTHETIC)
                            std::cout << "SYNTHETIC, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_ANNOTATION)
                            std::cout << "ANNOTATION, ";
                        if (inner_class.inner_access_flags & CLASS_ACC_ENUM)
                            std::cout << "ENUM, ";

                        std::cout << ')' << std::endl;
                    }
                }

                break;
            case Atom_EnclosingMethod:
                attribute.EnclosingMethod.class_index = readu2(classfile);
                if (attribute.EnclosingMethod.class_index < 1 || attribute.EnclosingMethod.class_index > constant_pool_count) {
                    std::cerr << "[ERROR]: attribute #" << j << "'s class index was out of bounds" << std::endl;
                    return 1;
                }
                attribute.EnclosingMethod._class = &constant_pool[attribute.EnclosingMethod.class_index - 1];
                if (attribute.EnclosingMethod._class->tag != ConstantType::Class) {
                    std::cerr << "[ERROR]: expected Class for attribute #" << j << "'s class tag but got " << constant_type_names.at(attribute.EnclosingMethod._class->tag) << std::endl;
                    return 1;
                }

                if (april_logging_enabled)
                    std::cout << "          class: " << utf8Tostring(attribute.EnclosingMethod._class->Class.name->Utf8) << std::endl;

                attribute.EnclosingMethod.method_index = readu2(classfile);
                if (attribute.EnclosingMethod.method_index) {
                    if (attribute.EnclosingMethod.method_index < 1 || attribute.EnclosingMethod.method_index > constant_pool_count) {
                        std::cerr << "[ERROR]: attribute #" << j << "'s method index was out of bounds" << std::endl;
                        return 1;
                    }
                    attribute.EnclosingMethod.method = &constant_pool[attribute.EnclosingMethod.method_index - 1];
                    if (attribute.EnclosingMethod.method->tag != ConstantType::NameAndType) {
                        std::cerr << "[ERROR]: expected NameAndType for attribute #" << j << "'s method tag but got " << constant_type_names.at(attribute.EnclosingMethod.method->tag) << std::endl;
                        return 1;
                    }
                }

                break;
            case Atom_Synthetic:
                break;
            case Atom_Signature:
                attribute.Signature.signature_index = readu2(classfile);
                if (attribute.Signature.signature_index < 1 || attribute.Signature.signature_index > constant_pool_count) {
                    std::cerr << "[ERROR]: attribute #" << j << "'s signature index was out of bounds" << std::endl;
                    return 1;
                }
                attribute.Signature.signature = &constant_pool[attribute.Signature.signature_index - 1];
                if (attribute.Signature.signature->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected Utf8 for attribute #" << j << "'s signature tag but got " << constant_type_names.at(attribute.Signature.signature->tag) << std::endl;
                    return 1;
                }

                if (april_logging_enabled)
                    std::cout << "          signature: " << utf8Tostring(attribute.Signature.signature->Utf8) << std::endl;

                break;
            case Atom_SourceFile:
                attribute.SourceFile.sourcefile_index = readu2(classfile);
                if (attribute.SourceFile.sourcefile_index < 1 || attribute.SourceFile.sourcefile_index > constant_pool_count) {
                    std::cerr << "[ERROR]: attribute #" << j << "'s sourcefile index was out of bounds" << std::endl;
                    return 1;
                }
                attribute.SourceFile.sourcefile = &constant_pool[attribute.SourceFile.sourcefile_index - 1];
                if (attribute.SourceFile.sourcefile->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected Utf8 for attribute #" << j << "'s sourcefile tag but got " << constant_type_names.at(attribute.SourceFile.sourcefile->tag) << std::endl;
                    return 1;
                }

                if (april_logging_enabled)
                    std::cout << "          sourcefile: " << utf8Tostring(attribute.SourceFile.sourcefile->Utf8) << std::endl;

                break;
            case Atom_LineNumberTable:
                if (!parent_attribute) {
                    std::cerr << "[ERROR]: no parent attribute in LineNumberTable" << std::endl;
                    return 1;
                }

                attribute.LineNumberTable.count = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          count: " << attribute.LineNumberTable.count << std::endl;

                attribute.LineNumberTable.list = new LineNumber[attribute.LineNumberTable.count];
                std::memset(attribute.LineNumberTable.list, 0, sizeof(LineNumber) * attribute.LineNumberTable.count);

                if (april_logging_enabled)
                    std::cout << "          list: " << std::endl;
                for (uint32_t j = 0; j < attribute.LineNumberTable.count; j++) {
                    LineNumber& line_number = attribute.LineNumberTable.list[j];
                    line_number.start_pc = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              start_pc: " << line_number.start_pc << std::endl;

                    if (line_number.start_pc < 0 || line_number.start_pc >= parent_attribute->Code.code_count) {
                        std::cerr << "[ERROR]: line number #" << j << " had an out-of-range start pc" << std::endl;
                        return 1;
                    }

                    line_number.line_number = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              line_number: " << line_number.line_number << std::endl;
                }
                break;
            case Atom_LocalVariableTable:
                if (!parent_attribute) {
                    std::cerr << "[ERROR]: no parent attribute in LocalVariableTable" << std::endl;
                    return 1;
                }

                attribute.LocalVariableTable.count = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          count: " << attribute.LocalVariableTable.count << std::endl;

                attribute.LocalVariableTable.list = new LocalVariable[attribute.LocalVariableTable.count];
                std::memset(attribute.LocalVariableTable.list, 0, sizeof(LocalVariable) * attribute.LocalVariableTable.count);

                if (april_logging_enabled)
                    std::cout << "          list: " << std::endl;
                for (uint32_t j = 0; j < attribute.LocalVariableTable.count; j++) {
                    LocalVariable& local_variable = attribute.LocalVariableTable.list[j];
                    local_variable.start_pc = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              start_pc: " << local_variable.start_pc << std::endl;

                    if (local_variable.start_pc < 0 || local_variable.start_pc >= parent_attribute->Code.code_count) {
                        std::cerr << "[ERROR]: line number #" << j << " had an out-of-range start pc" << std::endl;
                        return 1;
                    }

                    uint16_t length = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              length: " << length << std::endl;

                    local_variable.end_pc = local_variable.start_pc + length;
                    if (local_variable.end_pc < 0 || local_variable.end_pc > parent_attribute->Code.code_count) {
                        std::cerr << "[ERROR]: local variable #" << j << " had an out-of-range end pc: " << local_variable.end_pc << ", " << parent_attribute->Code.code_count << std::endl;
                        return 1;
                    }

                    local_variable.name_index = readu2(classfile);
                    if (local_variable.name_index < 1 || local_variable.name_index > constant_pool_count) {
                        std::cerr << "[ERROR]: local variable #" << j << "'s name index was out of bounds" << std::endl;
                        return 1;
                    }
                    local_variable.name = &constant_pool[local_variable.name_index - 1];
                    if (local_variable.name->tag != ConstantType::Utf8) {
                        std::cerr << "[ERROR]: expected Utf8 for local variable #" << j << "'s name tag but got " << constant_type_names.at(local_variable.name->tag) << std::endl;
                        return 1;
                    }

                    if (april_logging_enabled)
                        std::cout << "              name: " << utf8Tostring(local_variable.name->Utf8) << std::endl;

                    local_variable.descriptor_index = readu2(classfile);
                    if (local_variable.descriptor_index < 1 || local_variable.descriptor_index > constant_pool_count) {
                        std::cerr << "[ERROR]: local variable #" << j << "'s descriptor index was out of bounds" << std::endl;
                        return 1;
                    }
                    local_variable.descriptor = &constant_pool[local_variable.descriptor_index - 1];
                    if (local_variable.descriptor->tag != ConstantType::Utf8) {
                        std::cerr << "[ERROR]: expected Utf8 for local variable #" << j << "'s descriptor tag but got " << constant_type_names.at(local_variable.descriptor->tag) << std::endl;
                        return 1;
                    }

                    if (april_logging_enabled)
                        std::cout << "              descriptor: " << utf8Tostring(local_variable.descriptor->Utf8) << std::endl;

                    local_variable.index = readu2(classfile);
                    if (local_variable.index < 0 || local_variable.index > parent_attribute->Code.max_locals) {
                        std::cerr << "[ERROR]: local variable #" << j << "'s index was out of bounds" << std::endl;
                        return 1;
                    }

                    if (april_logging_enabled)
                        std::cout << "              index: " << local_variable.index << std::endl;
                }
                break;
            case Atom_LocalVariableTypeTable:
                classfile.ignore(attribute.length);
                break;
            case Atom_Deprecated:
                break;
            case Atom_RuntimeVisibleAnnotations:
            case Atom_RuntimeInvisibleAnnotations:
            case Atom_RuntimeInvisibleParameterAnnotations:
                classfile.ignore(attribute.length);
                break;
            case Atom_AnnotationDefault:
                classfile.ignore(attribute.length);
                break;
            case Atom_BootstrapMethods:
                attribute.BootstrapMethods.bootstrap_method_count = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "          bootstrap_method_count: " << attribute.BootstrapMethods.bootstrap_method_count << std::endl;
                attribute.BootstrapMethods.bootstrap_method_list = new BootstrapMethod[attribute.BootstrapMethods.bootstrap_method_count];
                std::memset(attribute.BootstrapMethods.bootstrap_method_list, 0, sizeof(BootstrapMethod) * attribute.BootstrapMethods.bootstrap_method_count);

                for (uint16_t j = 0; j < attribute.BootstrapMethods.bootstrap_method_count; j++) {
                    BootstrapMethod& bootstrap_method = attribute.BootstrapMethods.bootstrap_method_list[j];

                    if (april_logging_enabled)
                        std::cout << "            bootstrap method #" << j << std::endl;

                    bootstrap_method.method_ref_index = readu2(classfile);

                    if (bootstrap_method.method_ref_index < 1 || bootstrap_method.method_ref_index > constant_pool_count) {
                        std::cerr << "[ERROR]: exception #" << j << "'s bootstrap method ref index was out of bounds" << std::endl;
                        return 1;
                    }
                    bootstrap_method.method_ref = &constant_pool[bootstrap_method.method_ref_index - 1];
                    if (bootstrap_method.method_ref->tag != ConstantType::MethodHandle) {
                        std::cerr << "[ERROR]: expected MethodHandle for bootstrap method #" << j << "'s method_ref tag but got " << constant_type_names.at(bootstrap_method.method_ref->tag) << std::endl;
                        return 1;
                    }

                    if (april_logging_enabled)
                        std::cout << "              method_ref reference_index: " << bootstrap_method.method_ref->MethodHandle.reference_index << std::endl;

                    bootstrap_method.arg_count = readu2(classfile);
                    if (april_logging_enabled)
                        std::cout << "              arg_count: " << bootstrap_method.arg_count << std::endl;
                    bootstrap_method.arg_list = new Constant*[bootstrap_method.arg_count];
                    std::memset(bootstrap_method.arg_list, 0, sizeof(Constant*) * bootstrap_method.arg_count);

                    for (uint16_t k = 0; k < bootstrap_method.arg_count; k++) {
                        Constant*& constant = bootstrap_method.arg_list[k];

                        uint16_t index = readu2(classfile);
                        if (index < 1 || index > constant_pool_count) {
                            std::cerr << "[ERROR]: attribute #" << j << "'s arg #" << k << "'s constant index was out of bounds" << std::endl;
                            return 1;
                        }
                        constant = &constant_pool[index - 1];

                        // TODO: print info
                        switch (constant->tag) {
                            case ConstantType::String:
                            case ConstantType::Class:
                            case ConstantType::Integer:
                            case ConstantType::Long:
                            case ConstantType::Float:
                            case ConstantType::Double:
                            case ConstantType::MethodHandle:
                            case ConstantType::MethodType:
                                break;
                            default:
                                std::cerr << "[ERROR]: expected String, Class, Integer, Long, Float, Double, MethodHandle, or MethodType for attribute #" << j << "'s #" << k << "'s' tag but got " << constant_type_names.at(constant->tag) << std::endl;
                                return 1;
                        }
                    }
                }
                break;
            default:
                std::cerr << "[ERROR]: attribute #" << j << " had an invalid name (" << utf8Tostring(attribute.name->Utf8) << ')' << std::endl;
                return 1;
                break;
        }

        std::streampos endpos = classfile.tellg();
        if (endpos - startpos != attribute.length) {
            std::cerr << "[ERROR]: attribute #" << j << "'s length (" << attribute.length << ") did not match the number of bytes read (" << (endpos - startpos) << ')' << std::endl;
            return 1;
        }
    }

    return 0;
}
void deleteAttribute(Attribute& attribute) {
    if (attribute.length == 0)
        return;

    switch (attribute.name->Utf8.atom) {
        case Atom_Code:
            if (attribute.Code.code_list)
                delete[] attribute.Code.code_list;
            if (attribute.Code.exception_list)
                delete[] attribute.Code.exception_list;
            if (attribute.Code.attribute_list) {
                for (uint16_t i = 0; i < attribute.Code.attribute_count; i++)
                    deleteAttribute(attribute.Code.attribute_list[i]);
                delete[] attribute.Code.attribute_list;
            }
            break;
        case Atom_Exceptions:
            if (attribute.Exceptions.exception_list)
                delete[] attribute.Exceptions.exception_list;
            break;
        case Atom_InnerClasses:
            if (attribute.InnerClasses.list)
                delete[] attribute.InnerClasses.list;
            break;
        case Atom_LineNumberTable:
            if (attribute.LineNumberTable.list)
                delete[] attribute.LineNumberTable.list;
            break;
        case Atom_LocalVariableTable:
            if (attribute.LocalVariableTable.list)
                delete[] attribute.LocalVariableTable.list;
            break;
        case Atom_BootstrapMethods:
            if (attribute.BootstrapMethods.bootstrap_method_list) {
                for (uint16_t i = 0; i < attribute.BootstrapMethods.bootstrap_method_count; i++)
                    if (attribute.BootstrapMethods.bootstrap_method_list[i].arg_list)
                        delete[] attribute.BootstrapMethods.bootstrap_method_list[i].arg_list;
                delete[] attribute.BootstrapMethods.bootstrap_method_list;
            }
            break;
        default:
            break;
    }
}

int readClassFile(std::istream& classfile, Class &_class) {
    uint32_t magic = readu4(classfile);
    if (magic != 0xCAFEBABE) {
        std::cerr << "[ERROR]: invalid magic item; expected 0xCAFEBABE but got " << std::format("{:#X}", magic) << std::endl;
        return 1;
    }
    if (april_logging_enabled)
        std::cout << "magic: " << std::format("{:#X}", magic) << std::endl;

    _class.minor_version = readu2(classfile);
    _class.major_version = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "version: " << _class.major_version << '.' << _class.minor_version << std::endl;

    _class.constant_pool_count = readu2(classfile) - 1;
    if (april_logging_enabled)
        std::cout << "constant pool count: " << _class.constant_pool_count << std::endl;

    _class.constant_pool = new Constant[_class.constant_pool_count];
    std::memset(_class.constant_pool, 0, sizeof(Constant) * _class.constant_pool_count);

    if (april_logging_enabled)
        std::cout << "constant pool:" << std::endl;
    // first pass
    for (uint16_t i = 0; i < _class.constant_pool_count; i++) {
        ConstantType tag = static_cast<ConstantType>(readu1(classfile));
        auto tag_name = constant_type_names.find(tag);
        if (tag_name == constant_type_names.end()) {
            std::cerr << "[ERROR]: could not get name of constant #" << i << "'s tag (" << (int)tag << ')' << std::endl;
            return 1;
        }
        if (april_logging_enabled)
            std::cout << "  " << (i + 1) << " - tag: " << int(tag) << " (" << tag_name->second << ')' << std::endl;

        Constant& constant = _class.constant_pool[i];
        constant.tag = tag;

        switch (tag) {
            case ConstantType::Class:
                constant.Class.name_index = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "    name_index = " << constant.Class.name_index << std::endl;
                break;
            case ConstantType::Fieldref:
            case ConstantType::Methodref:
            case ConstantType::InterfaceMethodref:
                constant.GeneralRef.class_index = readu2(classfile);
                constant.GeneralRef.name_and_type_index = readu2(classfile);
                if (april_logging_enabled) {
                    std::cout << "    class_index = " << constant.GeneralRef.class_index << std::endl;
                    std::cout << "    name_and_type_index = " << constant.GeneralRef.name_and_type_index << std::endl;
                }
                break;
            case ConstantType::String:
                constant.String.string_index = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "    string_index = " << constant.String.string_index << std::endl;
                break;
            case ConstantType::Integer:
                constant.Integer.bytes = readu4(classfile);
                if (april_logging_enabled)
                    std::cout << "    bytes = " << constant.Integer.bytes << std::endl;
                break;
            case ConstantType::Float:
                constant.Float.bytes = readu4(classfile);
                if (april_logging_enabled) {
                    std::cout << "    bytes = " << constant.Float.bytes << std::endl;
                    std::cout << "    value = ";
                }
                if (constant.Float.bytes == 0x7f800000) {
                    constant.Float.value.type = FloatingType::PositiveInfinity;
                    if (april_logging_enabled)
                        std::cout << "positive infinity" << std::endl;
                } else if (constant.Float.bytes == 0xff800000) {
                    constant.Float.value.type = FloatingType::NegativeInfinity;
                    if (april_logging_enabled)
                        std::cout << "negative infinity" << std::endl;
                } else if ((constant.Float.bytes > 0x7f800001 && constant.Float.bytes < 0x7fffffff) || (constant.Float.bytes > 0xff800001 && constant.Float.bytes < 0xffffffff)) {
                    constant.Float.value.type = FloatingType::NaN;
                    if (april_logging_enabled)
                        std::cout << "NaN" << std::endl;
                } else {
                    int s = ((constant.Float.bytes >> 31) == 0) ? 1 : -1;
                    int e = ((constant.Float.bytes >> 23) & 0xff);
                    int m = (e == 0) ?
                          (constant.Float.bytes & 0x7fffff) << 1 :
                          (constant.Float.bytes & 0x7fffff) | 0x800000;
                    constant.Float.value.type = FloatingType::Normal;
                    constant.Float.value.value = s * m * std::pow(2, e - 150);
                    if (april_logging_enabled)
                        std::cout << constant.Float.value.value << std::endl;
                }
                break;
            case ConstantType::Long:
                constant.Long.high = readu4(classfile);
                constant.Long.low = readu4(classfile);
                constant.Long.bytes = ((long) constant.Long.high << 32) + constant.Long.low;
                i++;
                if (april_logging_enabled) {
                    std::cout << "    bytes = " << constant.Long.bytes << std::endl;
                    std::cout << "  " << i << " - Long padding" << std::endl;
                }
                break;
            case ConstantType::Double:
                constant.Double.high = readu4(classfile);
                constant.Double.low = readu4(classfile);
                constant.Double.bits = ((long) constant.Double.high << 32) + constant.Double.low;
                if (april_logging_enabled) {
                    std::cout << "    bits = " << constant.Double.bits << std::endl;
                    std::cout << "    value = ";
                }
                if (constant.Double.bits == 0x7ff0000000000000L) {
                    constant.Double.value.type = FloatingType::PositiveInfinity;
                    if (april_logging_enabled)
                        std::cout << "positive infinity" << std::endl;
                } else if (constant.Double.bits == 0xfff0000000000000L) {
                    constant.Double.value.type = FloatingType::NegativeInfinity;
                    if (april_logging_enabled)
                        std::cout << "negative infinity" << std::endl;
                } else if ((constant.Double.bits > 0x7ff0000000000001L && constant.Double.bits < 0x7fffffffffffffffL) || (constant.Double.bits > 0xfff0000000000001L && constant.Double.bits < 0xffffffffffffffffL)) {
                    constant.Double.value.type = FloatingType::NaN;
                    if (april_logging_enabled)
                        std::cout << "NaN" << std::endl;
                } else {
                    int s = ((constant.Double.bits >> 63) == 0) ? 1 : -1;
                    int e = (int)((constant.Double.bits >> 52) & 0x7ffL);
                    long m = (e == 0) ?
                               (constant.Double.bits & 0xfffffffffffffL) << 1 :
                               (constant.Double.bits & 0xfffffffffffffL) | 0x10000000000000L;
                    constant.Double.value.type = FloatingType::Normal;
                    constant.Double.value.value = s * m * std::pow(2, e - 1075);
                    if (april_logging_enabled)
                        std::cout << constant.Double.value.value << std::endl;
                }

                i++;
                if (april_logging_enabled)
                    std::cout << "  " << i << " - Double padding" << std::endl;
                break;
            case ConstantType::NameAndType:
                constant.NameAndType.name_index = readu2(classfile);
                constant.NameAndType.descriptor_index = readu2(classfile);
                if (april_logging_enabled) {
                    std::cout << "    name_index = " << constant.NameAndType.name_index << std::endl;
                    std::cout << "    descriptor_index = " << constant.NameAndType.descriptor_index << std::endl;
                }
                break;
            case ConstantType::Utf8:
                constant.Utf8.bytes_size = readu2(classfile);
                constant.Utf8.bytes = new uint8_t[constant.Utf8.bytes_size];
                for (uint16_t i = 0; i < constant.Utf8.bytes_size; i++)
                    constant.Utf8.bytes[i] = readu1(classfile);
                if (april_logging_enabled) {
                    std::cout << "    bytes_size = " << constant.Utf8.bytes_size << std::endl;
                    std::cout << "    bytes = [ ";
                    for (uint16_t i = 0; i < constant.Utf8.bytes_size; i++)
                        std::cout << std::format("{:x}", constant.Utf8.bytes[i]) << ", ";
                    std::cout << ']' << std::endl;
                    std::cout << "    bytes (lazy conversion) = " << utf8Tostring(constant.Utf8) << std::endl;
                }
                constant.Utf8.atom = -1;
                #define BRANCH(str, value) else if (std::strncmp(reinterpret_cast<const char*>(constant.Utf8.bytes), str, constant.Utf8.bytes_size) == 0) \
                    constant.Utf8.atom = Atom_##value;
                #define X(value) BRANCH(#value, value)
                if (false)
                    ;
                SIMPLESTRINGATOMLIST
                #undef X
                BRANCH("<init>", kInstanceInitialization)
                BRANCH("<clinit>", kClassInitialization)
                #undef BRANCH
                break;
            case ConstantType::MethodHandle:
                constant.MethodHandle.reference_kind = readu1(classfile);
                constant.MethodHandle.reference_index = readu2(classfile);
                if (april_logging_enabled) {
                    std::cout << "    reference_kind = " << (int) constant.MethodHandle.reference_kind << std::endl;
                    std::cout << "    reference_index = " << constant.MethodHandle.reference_index << std::endl;
                }
                break;
            case ConstantType::MethodType:
                constant.MethodType.descriptor_index = readu2(classfile);
                if (april_logging_enabled)
                    std::cout << "    descriptor_index = " << constant.MethodType.descriptor_index << std::endl;
                break;
            case ConstantType::InvokeDynamic:
                constant.InvokeDynamic.bootstrap_method_index = readu2(classfile);
                constant.InvokeDynamic.name_and_type_index = readu2(classfile);
                if (april_logging_enabled) {
                    std::cout << "    bootstrap_method_attribute_index = " << constant.InvokeDynamic.bootstrap_method_index << std::endl;
                    std::cout << "    name_and_type_index = " << constant.InvokeDynamic.name_and_type_index << std::endl;
                }
                break;
            default:
                std::cerr << "[ERROR]: constant #" << i << " had an invalid tag (" << (int)tag << ')' << std::endl;
                return 1;
                break;
        }
    }

    // second pass to set most constant references
    for (uint16_t i = 0; i < _class.constant_pool_count; i++) {
        Constant& constant = _class.constant_pool[i];

        switch (constant.tag) {
            case ConstantType::Long:
            case ConstantType::Double:
                i++;
                break;
            case ConstantType::Class:
                if (constant.Class.name_index < 1 || constant.Class.name_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s name index was out of bounds" << std::endl;
                    return 1;
                }
                constant.Class.name = &_class.constant_pool[constant.Class.name_index - 1];
                if (constant.Class.name->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected Utf8 for constant #" << i << "'s name tag but got " << constant_type_names.at(constant.Class.name->tag) << std::endl;
                    return 1;
                }
                break;
            case ConstantType::String:
                if (constant.String.string_index < 1 || constant.String.string_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s string index was out of bounds" << std::endl;
                    return 1;
                }
                constant.String.string = &_class.constant_pool[constant.String.string_index - 1];
                if (constant.String.string->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected String for constant #" << i << "'s string tag but got " << constant_type_names.at(constant.String.string->tag) << std::endl;
                    return 1;
                }
                break;
            case ConstantType::Fieldref:
            case ConstantType::Methodref:
            case ConstantType::InterfaceMethodref:
                if (constant.GeneralRef.class_index < 1 || constant.GeneralRef.class_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s class index was out of bounds" << std::endl;
                    return 1;
                }
                if (constant.GeneralRef.name_and_type_index < 1 || constant.GeneralRef.name_and_type_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s name and type index was out of bounds" << std::endl;
                    return 1;
                }
                constant.GeneralRef._class = &_class.constant_pool[constant.GeneralRef.class_index - 1];
                if (constant.GeneralRef._class->tag != ConstantType::Class) {
                    std::cerr << "[ERROR]: expected Class for constant #" << i << "'s class tag but got " << constant_type_names.at(constant.GeneralRef._class->tag) << std::endl;
                    return 1;
                }
                constant.GeneralRef.name_and_type = &_class.constant_pool[constant.GeneralRef.name_and_type_index - 1];
                if (constant.GeneralRef.name_and_type->tag != ConstantType::NameAndType) {
                    std::cerr << "[ERROR]: expected NameAndType for constant #" << i << "'s name and type tag but got " << constant_type_names.at(constant.GeneralRef.name_and_type->tag) << std::endl;
                    return 1;
                }
                break;
            case ConstantType::NameAndType:
                if (constant.NameAndType.name_index < 1 || constant.NameAndType.name_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s name index was out of bounds" << std::endl;
                    return 1;
                }
                if (constant.NameAndType.descriptor_index < 1 || constant.NameAndType.descriptor_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s descriptor index was out of bounds" << std::endl;
                    return 1;
                }
                constant.NameAndType.name = &_class.constant_pool[constant.NameAndType.name_index - 1];
                if (constant.NameAndType.name->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected Utf8 for constant #" << i << "'s name tag but got " << constant_type_names.at(constant.NameAndType.name->tag) << std::endl;
                    return 1;
                }
                constant.NameAndType.descriptor = &_class.constant_pool[constant.NameAndType.descriptor_index - 1];
                if (constant.NameAndType.descriptor->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected Utf8 for constant #" << i << "'s descriptor tag but got " << constant_type_names.at(constant.NameAndType.descriptor->tag) << std::endl;
                    return 1;
                }
                break;
            case ConstantType::MethodType:
                if (constant.MethodType.descriptor_index < 1 || constant.MethodType.descriptor_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s descriptor index was out of bounds" << std::endl;
                    return 1;
                }
                constant.MethodType.descriptor = &_class.constant_pool[constant.MethodType.descriptor_index - 1];
                if (constant.MethodType.descriptor->tag != ConstantType::Utf8) {
                    std::cerr << "[ERROR]: expected Utf8 for constant #" << i << "'s descriptor tag but got " << constant_type_names.at(constant.MethodType.descriptor->tag) << std::endl;
                    return 1;
                }
                break;
            case ConstantType::InvokeDynamic:
                if (constant.InvokeDynamic.name_and_type_index < 1 || constant.InvokeDynamic.name_and_type_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s name_and_type index was out of bounds" << std::endl;
                    return 1;
                }
                constant.InvokeDynamic.name_and_type = &_class.constant_pool[constant.InvokeDynamic.name_and_type_index - 1];
                if (constant.InvokeDynamic.name_and_type->tag != ConstantType::NameAndType) {
                    std::cerr << "[ERROR]: expected NameAndType for constant #" << i << "'s descriptor tag but got " << constant_type_names.at(constant.InvokeDynamic.name_and_type->tag) << std::endl;
                    return 1;
                }
                break;
            default:
                break;
        }
    }

    // third pass to ensure NameAndType are evaluated
    for (uint16_t i = 0; i < _class.constant_pool_count; i++) {
        Constant& constant = _class.constant_pool[i];

        switch (constant.tag) {
            case ConstantType::MethodHandle:
                if (constant.MethodHandle.reference_index < 1 || constant.MethodHandle.reference_index > _class.constant_pool_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s reference index was out of bounds" << std::endl;
                    return 1;
                }
                constant.MethodHandle.reference = &_class.constant_pool[constant.MethodHandle.reference_index - 1];
                switch (constant.MethodHandle.reference_kind) {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        if (constant.MethodHandle.reference->tag != ConstantType::Fieldref) {
                            std::cerr << "[ERROR]: expected Fieldref for constant #" << i << "'s reference tag but got " << constant_type_names.at(constant.MethodHandle.reference->tag) << std::endl;
                            return 1;
                        }
                        break;
                    case 5:
                    case 8:
                        if (constant.MethodHandle.reference->tag != ConstantType::Methodref) {
                            std::cerr << "[ERROR]: expected Methodref for constant #" << i << "'s reference tag but got " << constant_type_names.at(constant.MethodHandle.reference->tag) << std::endl;
                            return 1;
                        }
                        break;
                    case 6:
                    case 7:
                        if (_class.major_version < 52) {
                            if (constant.MethodHandle.reference->tag != ConstantType::Methodref) {
                                std::cerr << "[ERROR]: expected Methodref for constant #" << i << "'s reference tag but got " << constant_type_names.at(constant.MethodHandle.reference->tag) << std::endl;
                                return 1;
                            }
                        } else {
                            if (constant.MethodHandle.reference->tag != ConstantType::Methodref && constant.MethodHandle.reference->tag != ConstantType::InterfaceMethodref) {
                                std::cerr << "[ERROR]: expected Methodref or InterfaceMethodref for constant #" << i << "'s reference tag but got " << constant_type_names.at(constant.MethodHandle.reference->tag) << std::endl;
                                return 1;
                            }
                        }
                        break;
                    case 9:
                        if (constant.MethodHandle.reference->tag != ConstantType::InterfaceMethodref) {
                            std::cerr << "[ERROR]: expected InterfaceMethodref for constant #" << i << "'s reference tag but got " << constant_type_names.at(constant.MethodHandle.reference->tag) << std::endl;
                            return 1;
                            break;
                        }
                        break;
                    default:
                        std::cerr << "[ERROR]: expected value in range [1, 9] for constant #" << i << "'s reference kind but got " << (int)constant.MethodHandle.reference_kind << std::endl;
                        return 1;
                }
                switch (constant.MethodHandle.reference_kind) {
                    case 5:
                    case 6:
                    case 7:
                    case 9:
                        if (constant.MethodHandle.reference->GeneralRef.name_and_type->NameAndType.name->Utf8.atom == Atom_kClassInitialization ||
                            constant.MethodHandle.reference->GeneralRef.name_and_type->NameAndType.name->Utf8.atom == Atom_kInstanceInitialization)
                        {
                            std::cerr << "[ERROR]: constant #" << i << "'s reference pointed to a class or instance initialization method when it shouldn't have" << std::endl;
                            return 1;
                            break;
                        }
                        break;
                    case 8:
                        if (constant.MethodHandle.reference->GeneralRef.name_and_type->NameAndType.name->Utf8.atom != Atom_kInstanceInitialization) {
                            std::cerr << "[ERROR]: expected instance initialization method for constant #" << i << "'s reference method but got " << utf8Tostring(constant.MethodHandle.reference->GeneralRef.name_and_type->NameAndType.name->Utf8) << std::endl;
                            return 1;
                            break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    _class.access_flags = readu2(classfile);
    if (april_logging_enabled) {
        std::cout << "access flags: " << _class.access_flags << " (";

        if (_class.access_flags & CLASS_ACC_PUBLIC)
            std::cout << "PUBLIC, ";
        if (_class.access_flags & CLASS_ACC_FINAL)
            std::cout << "FINAL, ";
        if (_class.access_flags & CLASS_ACC_SUPER)
            std::cout << "SUPER, ";
        if (_class.access_flags & CLASS_ACC_INTERFACE)
            std::cout << "INTERFACE, ";
        if (_class.access_flags & CLASS_ACC_ABSTRACT)
            std::cout << "ABSTRACT, ";
        if (_class.access_flags & CLASS_ACC_SYNTHETIC)
            std::cout << "SYNTHETIC, ";
        if (_class.access_flags & CLASS_ACC_ANNOTATION)
            std::cout << "ANNOTATION, ";
        if (_class.access_flags & CLASS_ACC_ENUM)
            std::cout << "ENUM, ";

        std::cout << ')' << std::endl;
    }

    if (_class.access_flags & CLASS_ACC_INTERFACE) {
        if (!(_class.access_flags & CLASS_ACC_ABSTRACT)) {
            std::cerr << "[ERROR]: class was an interface but did not have ABSTRACT flag" << std::endl;
            return 1;
        } else if (_class.access_flags & CLASS_ACC_FINAL || _class.access_flags & CLASS_ACC_SUPER || _class.access_flags & CLASS_ACC_ENUM) {
            std::cerr << "[ERROR]: class was an interface and had FINAL, SUPER, or ENUM flag" << std::endl;
            return 1;
        }
    } else {
        if (_class.access_flags & CLASS_ACC_ANNOTATION) {
            std::cerr << "[ERROR]: class was not an interface and had ANNOTATION flag" << std::endl;
            return 1;
        } else if (_class.access_flags & CLASS_ACC_FINAL && _class.access_flags & CLASS_ACC_ABSTRACT) {
            std::cerr << "[ERROR]: class had both FINAL and ABSTRACT flags" << std::endl;
            return 1;
        }
    }

    uint16_t this_class_index = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "this class index: " << this_class_index << std::endl;
    uint16_t super_class_index = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "super class index: " << super_class_index << std::endl;

    if (this_class_index < 1 || this_class_index > _class.constant_pool_count) {
        std::cerr << "[ERROR]: this class index was out of bounds" << std::endl;
        return 1;
    }
    _class.this_class = &_class.constant_pool[this_class_index - 1];
    if (_class.this_class->tag != ConstantType::Class) {
        std::cerr << "[ERROR]: expected Class for this class constant tag but got " << constant_type_names.at(_class.this_class->tag) << std::endl;
        return 1;
    }

    if (april_logging_enabled)
        std::cout << "this class name: " <<  utf8Tostring(_class.this_class->Class.name->Utf8) << std::endl;

    if (super_class_index) {
        if (super_class_index < 1 || super_class_index > _class.constant_pool_count) {
            std::cerr << "[ERROR]: super class index was out of bounds" << std::endl;
            return 1;
        }
        _class.super_class = &_class.constant_pool[super_class_index - 1];
        if (_class.super_class->tag != ConstantType::Class) {
            std::cerr << "[ERROR]: expected Class for super class constant tag but got " << constant_type_names.at(_class.super_class->tag) << std::endl;
            return 1;
        }

        if (april_logging_enabled)
            std::cout << "super class name: " << utf8Tostring(_class.super_class->Class.name->Utf8) << std::endl;
    } else if (april_logging_enabled)
        std::cout << "super class name: [no super class]" << std::endl;

    _class.interface_count = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "interface count: " << _class.interface_count << std::endl;

    _class.interface_list = new Constant*[_class.interface_count];
    std::memset(_class.interface_list, 0, sizeof(Constant*) * _class.interface_count);

    for (uint16_t i = 0; i < _class.interface_count; i++) {
        uint16_t index = readu2(classfile);

        Constant* constant = &_class.constant_pool[index - 1];
        if (constant->tag != ConstantType::Class) {
            std::cerr << "[ERROR]: expected Class for interface constant tag but got " << constant_type_names.at(constant->tag) << std::endl;
            return 1;
        }

        _class.interface_list[i] = constant;
    }

    _class.field_count = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "field count: " << _class.field_count << std::endl;

    _class.field_list = new Field[_class.field_count];
    std::memset(_class.field_list, 0, sizeof(Field) * _class.field_count);
    for (uint16_t i = 0; i < _class.field_count; i++) {
        Field& field = _class.field_list[i];

        if (april_logging_enabled)
            std::cout << "  field #" << i << ':' << std::endl;

        field.access_flags = readu2(classfile);
        if (april_logging_enabled) {
            std::cout << "    access_flags: " << field.access_flags << " (";

            if (field.access_flags & FIELD_ACC_PUBLIC)
                std::cout << "PUBLIC, ";
            if (field.access_flags & FIELD_ACC_PRIVATE)
                std::cout << "PRIVATE, ";
            if (field.access_flags & FIELD_ACC_PROTECTED)
                std::cout << "PROTECTED, ";
            if (field.access_flags & FIELD_ACC_STATIC)
                std::cout << "STATIC, ";
            if (field.access_flags & FIELD_ACC_FINAL)
                std::cout << "FINAL, ";
            if (field.access_flags & FIELD_ACC_VOLATILE)
                std::cout << "VOLATILE, ";
            if (field.access_flags & FIELD_ACC_TRANSIENT)
                std::cout << "TRANSIENT, ";
            if (field.access_flags & FIELD_ACC_SYNTHETIC)
                std::cout << "SYNTHETIC, ";
            if (field.access_flags & FIELD_ACC_ENUM)
                std::cout << "ENUM, ";

            std::cout << ')' << std::endl;
        }

        if (_class.access_flags & CLASS_ACC_INTERFACE) {
            if (!(field.access_flags & FIELD_ACC_PUBLIC && field.access_flags & FIELD_ACC_STATIC && field.access_flags & FIELD_ACC_FINAL)) {
                std::cerr << "[ERROR]: field #" << i << " has invalid access flags: the class is interface but the field doesn't have PUBLIC, STATIC, and FINAL flag" << std::endl;
                return 1;
            }

            if (field.access_flags & FIELD_ACC_PRIVATE || field.access_flags & FIELD_ACC_PROTECTED || field.access_flags & FIELD_ACC_VOLATILE || field.access_flags & FIELD_ACC_TRANSIENT || field.access_flags & FIELD_ACC_ENUM) {
                std::cerr << "[ERROR]: field #" << i << " has invalid access flags: the class is interface but the field had either PRIVATE, PROTECTED, VOLATILE, TRANSIENT, or ENUM flag" << std::endl;
                return 1;
            }
        }

        if ((field.access_flags & FIELD_ACC_PUBLIC && field.access_flags & FIELD_ACC_PRIVATE) || (field.access_flags & FIELD_ACC_PUBLIC && field.access_flags & FIELD_ACC_PROTECTED) || (field.access_flags & FIELD_ACC_PRIVATE && field.access_flags & FIELD_ACC_PROTECTED)) {
            std::cerr << "[ERROR]: field #" << i << " has invalid access flags: the field did not have only one among PUBLIC, PRIVATE, and PROTECTED flags" << std::endl;
            return 1;
        }
        if (field.access_flags & FIELD_ACC_FINAL && field.access_flags & FIELD_ACC_VOLATILE) {
            std::cerr << "[ERROR]: field #" << i << " has invalid access flags: the field had both FINAL and VOLATILE flags" << std::endl;
            return 1;
        }

        field.name_index = readu2(classfile);
        if (field.name_index < 1 || field.name_index > _class.constant_pool_count) {
            std::cerr << "[ERROR]: field #" << i << "'s name index was out of bounds" << std::endl;
            return 1;
        }
        field.name = &_class.constant_pool[field.name_index - 1];
        if (field.name->tag != ConstantType::Utf8) {
            std::cerr << "[ERROR]: expected Utf8 for field #" << i << "'s name tag but got " << constant_type_names.at(field.name->tag) << std::endl;
            return 1;
        }

        if (april_logging_enabled)
            std::cout << "    name: " << utf8Tostring(field.name->Utf8) << std::endl;

        field.descriptor_index = readu2(classfile);
        if (april_logging_enabled)
            std::cout << "    descriptor_index: " << field.descriptor_index << std::endl;
        if (field.descriptor_index < 1 || field.descriptor_index > _class.constant_pool_count) {
            std::cerr << "[ERROR]: field #" << i << "'s descriptor index was out of bounds" << std::endl;
            return 1;
        }
        field.descriptor = &_class.constant_pool[field.descriptor_index - 1];
        if (field.descriptor->tag != ConstantType::Utf8) {
            std::cerr << "[ERROR]: expected Utf8 for field #" << i << "'s descriptor tag but got " << constant_type_names.at(field.descriptor->tag) << std::endl;
            return 1;
        }

        if (april_logging_enabled)
            std::cout << "    descriptor: " << utf8Tostring(field.descriptor->Utf8) << std::endl;

        if (readAttributeList(classfile, field.attribute_count, field.attribute_list, _class.constant_pool_count, _class.constant_pool, nullptr))
            return 1;
    }

    _class.method_count = readu2(classfile);
    if (april_logging_enabled)
        std::cout << "method count: " << _class.method_count << std::endl;

    _class.method_list = new Method[_class.method_count];
    std::memset(_class.method_list, 0, sizeof(Method) * _class.method_count);
    for (uint16_t i = 0; i < _class.method_count; i++) {
        Method& method = _class.method_list[i];

        if (april_logging_enabled)
            std::cout << "  method #" << i << ':' << std::endl;

        method.access_flags = readu2(classfile);

        method.name_index = readu2(classfile);
        if (april_logging_enabled)
            std::cout << "    name_index: " << method.name_index << std::endl;
        if (method.name_index < 1 || method.name_index > _class.constant_pool_count) {
            std::cerr << "[ERROR]: method #" << i << "'s name index was out of bounds" << std::endl;
            return 1;
        }
        method.name = &_class.constant_pool[method.name_index - 1];
        if (method.name->tag != ConstantType::Utf8) {
            std::cerr << "[ERROR]: expected Utf8 for method #" << i << "'s name tag but got " << constant_type_names.at(method.name->tag) << std::endl;
            return 1;
        }

        if (april_logging_enabled) {
            std::cout << "    name: " << utf8Tostring(method.name->Utf8) << std::endl;

            std::cout << "    access_flags: " << method.access_flags << " (";

            if (method.access_flags & METHOD_ACC_PUBLIC)
                std::cout << "PUBLIC, ";
            if (method.access_flags & METHOD_ACC_PRIVATE)
                std::cout << "PRIVATE, ";
            if (method.access_flags & METHOD_ACC_PROTECTED)
                std::cout << "PROTECTED, ";
            if (method.access_flags & METHOD_ACC_STATIC)
                std::cout << "STATIC, ";
            if (method.access_flags & METHOD_ACC_FINAL)
                std::cout << "FINAL, ";
            if (method.access_flags & METHOD_ACC_SYNCHRONIZED)
                std::cout << "SYNCHRONIZED, ";
            if (method.access_flags & METHOD_ACC_BRIDGE)
                std::cout << "BRIDGE, ";
            if (method.access_flags & METHOD_ACC_VARARGS)
                std::cout << "VARARGS, ";
            if (method.access_flags & METHOD_ACC_NATIVE)
                std::cout << "NATIVE, ";
            if (method.access_flags & METHOD_ACC_ABSTRACT)
                std::cout << "ABSTRACT, ";
            if (method.access_flags & METHOD_ACC_STRICT)
                std::cout << "STRICT, ";
            if (method.access_flags & METHOD_ACC_SYNTHETIC)
                std::cout << "SYNTHETIC, ";

            std::cout << ')' << std::endl;
        }

        if (!(method.name->Utf8.atom == Atom_kClassInitialization || (method.name->Utf8.atom == Atom_kInstanceInitialization && (_class.access_flags & CLASS_ACC_INTERFACE)))) {
            if (_class.access_flags & CLASS_ACC_INTERFACE) {
                if (method.access_flags & METHOD_ACC_PROTECTED || method.access_flags & METHOD_ACC_FINAL || method.access_flags & METHOD_ACC_SYNCHRONIZED || method.access_flags & METHOD_ACC_NATIVE) {
                    std::cerr << "[ERROR]: method #" << i << " has invalid access flags: the class is interface but the method had either PROTECTED, FINAL, SYNCHRONIZED, or NATIVE flag" << std::endl;
                    return 1;
                }
                if (_class.major_version < 52) {
                    if (!(method.access_flags & METHOD_ACC_PUBLIC && method.access_flags & METHOD_ACC_ABSTRACT)) {
                        std::cerr << "[ERROR]: method #" << i << " has invalid access flags: the class is interface but the method doesn't have PUBLIC, and ABSTRACT flag" << std::endl;
                        return 1;
                    }
                } else {
                    if (!(method.access_flags & FIELD_ACC_PUBLIC || method.access_flags & FIELD_ACC_PRIVATE)) {
                        std::cerr << "[ERROR]: method #" << i << " has invalid access flags: the class is interface but the method doesn't have either PUBLIC or PRIVATE flag" << std::endl;
                        return 1;
                    }
                    if (method.access_flags & FIELD_ACC_PUBLIC && method.access_flags & FIELD_ACC_PRIVATE) {
                        std::cerr << "[ERROR]: method #" << i << " has invalid access flags: the class is interface and the method had both PUBLIC and PRIVATE flag" << std::endl;
                        return 1;
                    }
                }
            }

            if ((method.access_flags & METHOD_ACC_PUBLIC && method.access_flags & METHOD_ACC_PRIVATE) || (method.access_flags & METHOD_ACC_PUBLIC && method.access_flags & METHOD_ACC_PROTECTED) || (method.access_flags & METHOD_ACC_PRIVATE && method.access_flags & METHOD_ACC_PROTECTED)) {
                std::cerr << "[ERROR]: method #" << i << "has invalid access flags: the method did not have only one among PUBLIC, PRIVATE, and PROTECTED flags" << std::endl;
                return 1;
            }

            if (method.access_flags & METHOD_ACC_ABSTRACT) {
                if (method.access_flags & METHOD_ACC_PRIVATE || method.access_flags & METHOD_ACC_STRICT || method.access_flags & METHOD_ACC_FINAL || method.access_flags & METHOD_ACC_SYNCHRONIZED || method.access_flags & METHOD_ACC_NATIVE || method.access_flags & METHOD_ACC_STRICT) {
                    std::cerr << "[ERROR]: method #" << i << " has invalid access flags: the method had ABSTRACT flag but also had either PRIVATE, STRICT, FINAL, SYNCHRONIZED, NATIVE, or STRICT flag" << std::endl;
                    return 1;
                }
            }

            if (method.name->Utf8.atom == Atom_kInstanceInitialization) {
                if (method.access_flags & METHOD_ACC_STATIC || method.access_flags & METHOD_ACC_FINAL || method.access_flags & METHOD_ACC_SYNCHRONIZED || method.access_flags & METHOD_ACC_BRIDGE || method.access_flags & METHOD_ACC_NATIVE || method.access_flags & METHOD_ACC_ABSTRACT) {
                    std::cerr << "[ERROR]: method #" << i << " has invalid access flags: the method was an instance initialization method but also had either STATIC, FINAL, SYNCHRONIZED, BRIDGE, NATIVE or ABSTRACT flag" << std::endl;
                    return 1;
                }
            }
        }

        method.descriptor_index = readu2(classfile);
        if (method.descriptor_index < 1 || method.descriptor_index > _class.constant_pool_count) {
            std::cerr << "[ERROR]: method #" << i << "'s descriptor index was out of bounds" << std::endl;
            return 1;
        }
        method.descriptor = &_class.constant_pool[method.descriptor_index - 1];
        if (method.descriptor->tag != ConstantType::Utf8) {
            std::cerr << "[ERROR]: expected Utf8 for method #" << i << "'s descriptor tag but got " << constant_type_names.at(method.descriptor->tag) << std::endl;
            return 1;
        }

        if (april_logging_enabled)
            std::cout << "    descriptor: " << utf8Tostring(method.descriptor->Utf8) << std::endl;

        if (readAttributeList(classfile, method.attribute_count, method.attribute_list, _class.constant_pool_count, _class.constant_pool, nullptr))
            return 1;
    }

    if (readAttributeList(classfile, _class.attribute_count, _class.attribute_list, _class.constant_pool_count, _class.constant_pool, nullptr))
        return 1;

    // TODO: error if there are multiple
    Attribute* bootstrap_methods_attribute = nullptr;
    for (uint16_t i = 0; i < _class.attribute_count; i++) {
        Attribute* attribute = &_class.attribute_list[i];
        if (attribute->name->Utf8.atom == Atom_BootstrapMethods) {
            bootstrap_methods_attribute = attribute;
            break;
        }
    }

    // post-attribute list parsing constant pass
    for (uint16_t i = 0; i < _class.constant_pool_count; i++) {
        Constant& constant = _class.constant_pool[i];

        switch (constant.tag) {
            case ConstantType::InvokeDynamic:
                if (!bootstrap_methods_attribute) {
                    std::cerr << "[ERROR]: there was an InvokeDynamic constant but no BootstrapMethods Attribute" << std::endl;
                    return 1;
                }
                if (constant.InvokeDynamic.bootstrap_method_index >= bootstrap_methods_attribute->BootstrapMethods.bootstrap_method_count) {
                    std::cerr << "[ERROR]: constant #" << i << "'s bootstrap_method index was out of bounds" << std::endl;
                    return 1;
                }
                constant.InvokeDynamic.bootstrap_method = &bootstrap_methods_attribute->BootstrapMethods.bootstrap_method_list[constant.InvokeDynamic.bootstrap_method_index - 1];
                break;
            default:
                break;
        }
    }

    classfile.get();

    if (!classfile.eof())
        std::cout << "[WARNING]: finished parsing class file but end of file was not reached" << std::endl;

    return 0;
}

void destroyClass(Class& _class) {
    if (_class.constant_pool) {
        for (uint16_t i = 0; i < _class.constant_pool_count; i++) {
            Constant& constant = _class.constant_pool[i];
            if (constant.tag == ConstantType::Utf8 && constant.Utf8.bytes)
                delete[] constant.Utf8.bytes;
        }
        delete[] _class.constant_pool;
    }
    if (_class.interface_list)
        delete[] _class.interface_list;
    if (_class.field_list) {
        for (uint16_t i = 0; i < _class.field_count; i++) {
            Field& field = _class.field_list[i];
            if (field.attribute_list) {
                for (uint16_t j = 0; j < field.attribute_count; j++)
                    deleteAttribute(field.attribute_list[j]);
                delete[] field.attribute_list;
            }
        }
        delete[] _class.field_list;
    }
    if (_class.method_list) {
        for (uint16_t i = 0; i < _class.method_count; i++) {
            Method& method = _class.method_list[i];
            if (method.attribute_list) {
                for (uint16_t j = 0; j < method.attribute_count; j++)
                    deleteAttribute(method.attribute_list[j]);
                delete[] method.attribute_list;
            }
        }
        delete[] _class.method_list;
    }
}
