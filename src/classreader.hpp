#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <unordered_map>

enum class ConstantType: uint8_t {
    Class = 7,
    Fieldref = 9,
    Methodref = 10,
    InterfaceMethodref = 11,
    String = 8,
    Integer = 3,
    Float = 4,
    Long = 5,
    Double = 6,
    NameAndType = 12,
    Utf8 = 1,
    MethodHandle = 15,
    MethodType = 16,
    InvokeDynamic = 18
};
static std::unordered_map<ConstantType, const char*> constant_type_names = {
    {ConstantType::Class, "Class"},
    {ConstantType::Fieldref, "Fieldref"},
    {ConstantType::Methodref, "Methodref"},
    {ConstantType::InterfaceMethodref, "InterfaceMethodref"},
    {ConstantType::String, "String"},
    {ConstantType::Integer, "Integer"},
    {ConstantType::Float, "Float"},
    {ConstantType::Long, "Long"},
    {ConstantType::Double, "Double"},
    {ConstantType::NameAndType, "NameAndType"},
    {ConstantType::Utf8, "Utf8"},
    {ConstantType::MethodHandle, "MethodHandle"},
    {ConstantType::MethodType, "MethodType"},
    {ConstantType::InvokeDynamic, "InvokeDynamic"}
};

enum class FloatingType {
    PositiveInfinity,
    NegativeInfinity,
    NaN,
    Normal
};
struct DoubleValue {
    FloatingType type;
    double value; // NOTE: only set when type == Normal
};
struct FloatValue {
    FloatingType type;
    float value; // NOTE: only set when type == Normal
};

std::string doubleTostring(DoubleValue& value);
std::string floatTostring(FloatValue& value);

struct Constant {
    ConstantType tag;
    union {
        struct {
            uint16_t name_index; Constant* name;
        } Class;

        struct {
            uint16_t class_index; Constant* _class;
            uint16_t name_and_type_index; Constant* name_and_type;
        } GeneralRef; // Fieldref, Methodref, & InterfaceMethodref

        struct {
            uint16_t string_index; Constant* string;
        } String;

        struct {
            uint32_t bytes;
        } Integer;

        struct {
            uint32_t bytes;
            FloatValue value;
        } Float;

        struct {
            uint64_t bytes;
        } Long;

        struct {
            uint64_t bits;
            DoubleValue value;
        } Double;

        struct {
            uint16_t name_index; Constant* name;
            uint16_t descriptor_index; Constant* descriptor;
        } NameAndType;

        struct {
            int8_t atom;
            uint16_t bytes_size;
            uint8_t* bytes;
        } Utf8;
    };
};

struct Exception {
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type_index; Constant* catch_type;
};
struct LineNumber {
    uint16_t start_pc;
    uint16_t line_number;
};

struct Attribute {
    uint16_t name_index; Constant* name;
    uint32_t length;
    union {
        struct {
            uint16_t max_stack;
            uint16_t max_locals;
            uint32_t code_count;
            uint8_t* code_list;
            uint16_t exception_count;
            Exception* exception_list;
            uint16_t attribute_count;
            Attribute* attribute_list;
        } Code;

        struct {
            uint16_t sourcefile_index; Constant* sourcefile;
        } SourceFile;

        struct {
            uint16_t count;
            LineNumber* list;
        } LineNumberTable;
    };
};

struct Field {
    uint16_t access_flags;
    uint16_t name_index; Constant* name;
    uint16_t descriptor_index; Constant* descriptor;
    uint16_t attribute_count;
    Attribute* attribute_list;
};
struct Method {
    uint16_t access_flags;
    uint16_t name_index; Constant* name;
    uint16_t descriptor_index; Constant* descriptor;
    uint16_t attribute_count;
    Attribute* attribute_list;
};

#define SIMPLESTRINGATOMLIST                 \
    X(ConstantValue)                         \
    X(Code)                                  \
    X(StackMapTable)                         \
    X(Exceptions)                            \
    X(InnerClasses)                          \
    X(EnclosingMethod)                       \
    X(Synthetic)                             \
    X(Signature)                             \
    X(SourceFile)                            \
    X(SourceDebugExtension)                  \
    X(LineNumberTable)                       \
    X(LocalVariableTable)                    \
    X(LocalVariableTypeTable)                \
    X(Deprecated)                            \
    X(RuntimeVisibleAnnotations)             \
    X(RuntimeInvisibleAnnotations)           \
    X(RuntimeVisibleParameterAnnotations)    \
    X(RuntimeInvisibleParameterAnnotations)  \
    X(RuntimeVisibleTypeAnnotations)         \
    X(RuntimeInvisibleTypeAnnotations)       \
    X(AnnotationDefault)                     \
    X(BootstrapMethods)                      \
    X(MethodParameters)                      

enum StringAtom: uint8_t {
    #define X(name) Atom_##name,
    SIMPLESTRINGATOMLIST
    #undef X
    Atom_kInstanceInitialization
};

struct Class {
    uint16_t major_version;
    uint16_t minor_version;

    uint16_t constant_pool_count;
    Constant* constant_pool;

    uint16_t access_flags;

    Constant* this_class;
    Constant* super_class;

    uint16_t interface_count;
    uint16_t* interface_list;

    uint16_t field_count;
    Field* field_list;

    uint16_t method_count;
    Method* method_list;

    uint16_t attribute_count;
    Attribute* attribute_list;
};

int readClassFile(std::ifstream& classfile, Class& _class);
void destroyClass(Class& _class);

