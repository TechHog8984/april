--!strict

local descriptor_parser = require("@april/descriptor")

local april = {
    name_to_class_map = {},
    array_class_map = {}
}

export type JavaByte = {
    tag: "byte",
    value: number,
    category1: true
}
export type JavaChar = {
    tag: "char",
    value: number,
    category1: true
}
export type JavaInteger = {
    tag: "integer",
    value: number,
    category1: true
}
export type JavaFloat = {
    tag: "float",
    value: number,
    category1: true
}
export type JavaLong = {
    tag: "long",
    value: number, -- TODO: store high and low bytes?
    category1: false
}
export type JavaDouble = {
    tag: "double",
    value: number,
    category1: false
}
export type JavaBoolean = {
    tag: "boolean",
    value: boolean,
    category1: true
}
export type JavaStub = {
    tag: "stub",
    value: JavaLong | JavaDouble,
    category1: false
}
export type JavaReference = {
    tag: "reference",
    value: any,
    category1: true
}

export type JavaValue = JavaByte | JavaChar | JavaInteger | JavaFloat | JavaLong | JavaDouble | JavaBoolean | JavaStub | JavaReference

function april.wrapUnsigned(value: number, bits: number)
    local mod = 2 ^ bits
    return value % mod
end

function april.wrapSigned(value: number, bits: number)
    local mod = 2 ^ bits
    value = value % mod
    local half = mod / 2
    if value >= half then
        value = value - mod
    end
    return value
end

function april.newByte(value: number): JavaByte
    value = april.wrapSigned(math.floor(value), 8)

    return {
        tag = "byte",
        value = value,
        category1 = true
    }
end
function april.newChar(value: number): JavaChar
    value = april.wrapUnsigned(math.floor(value), 16)

    return {
        tag = "char",
        value = value,
        category1 = true
    }
end
function april.newInteger(value: number): JavaInteger
    value = april.wrapSigned(math.floor(value), 32)

    return {
        tag = "integer",
        value = value,
        category1 = true
    }
end
function april.newFloat(value: number): JavaFloat
    -- narrow to f32
    value = tonumber(("%f"):format(value)) :: number

    return {
        tag = "float",
        value = value,
        category1 = true
    }
end
function april.newLong(value: number): JavaLong
    value = math.floor(value)

    return {
        tag = "long",
        value = value,
        category1 = false
    }
end
function april.newDouble(value: number): JavaDouble
    return {
        tag = "double",
        value = value,
        category1 = false
    }
end
function april.newBoolean(value: boolean): JavaBoolean
    return {
        tag = "boolean",
        value = value,
        category1 = true
    }
end
function april.newStub(value: JavaLong | JavaDouble): JavaStub
    return {
        tag = "stub",
        value = value,
        category1 = false
    }
end
function april.newReference(value: any): JavaReference
    return {
        tag = "reference",
        value = value,
        category1 = true
    }
end

function april.coerce(input: JavaValue, target: descriptor_parser.FieldDescriptor): JavaValue
    -- print(input, target)
    local targetvalue = target.value
    if targetvalue.tag == "Array" then
        assert(input.tag == "reference" and input.value.class.isarray, "could not coerce value into array")
        assert(input.value.class == april.getArrayClass(target.raw))
        return input
    elseif targetvalue.tag == "Class" then
        assert(input.tag == "reference", "could not coerce value into instance")
        return input
    elseif targetvalue.tag == "Base" then
        -- TODO: i kept copypasting so go back and ensure consistent tag order for readability
        if targetvalue.base_type == "byte" then
            if input.tag == "byte" then
                return input
            elseif input.tag == "boolean" then
                return april.newByte(if input.value then 1 else 0)
            end
            assert(input.tag == "integer" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into byte")
            return april.newByte(input.value)
        elseif targetvalue.base_type == "char" then
            if input.tag == "char" then
                return input
            elseif input.tag == "boolean" then
                return april.newChar(if input.value then 1 else 0)
            end
            assert(input.tag == "integer" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into char")
            return april.newChar(input.value)
        elseif targetvalue.base_type == "double" then
            if input.tag == "double" then
                return input
            elseif input.tag == "boolean" then
                return april.newDouble(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "integer" or input.tag == "float" or input.tag == "long", "could not coerce value into double")
            return april.newDouble(input.value)
        elseif targetvalue.base_type == "float" then
            if input.tag == "float" then
                return input
            elseif input.tag == "boolean" then
                return april.newFloat(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "integer" or input.tag == "long" or input.tag == "double", "could not coerce value into float")
            return april.newFloat(input.value)
        elseif targetvalue.base_type == "int" then
            if input.tag == "integer" then
                return input
            elseif input.tag == "boolean" then
                return april.newInteger(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into integer")
            return april.newInteger(input.value)
        elseif targetvalue.base_type == "long" then
            if input.tag == "long" then
                return input
            elseif input.tag == "boolean" then
                return april.newLong(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "integer" or input.tag == "float" or input.tag == "double", "could not coerce value into long")
            return april.newLong(input.value)
        elseif targetvalue.base_type == "short" then
            -- TODO: short
        elseif targetvalue.base_type == "boolean" then
            if input.tag == "boolean" then
                return input
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "integer" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into boolean")
            return april.newBoolean(if input.value == 0 then false else true)
        end
    end

    error("failed to coerce")
end

function april.getDefault(descriptor: descriptor_parser.FieldDescriptor)
    local value: JavaValue
    if descriptor.value.tag == "Base" then
        if descriptor.value.base_type == "byte" then
            value = april.newByte(0)
        elseif descriptor.value.base_type == "short" then
            -- TODO: short
        elseif descriptor.value.base_type == "int" then
            value = april.newInteger(0)
        elseif descriptor.value.base_type == "long" then
            value = april.newLong(0)
        elseif descriptor.value.base_type == "char" then
            value = april.newChar(0)
        elseif descriptor.value.base_type == "float" then
            value = april.newFloat(0)
        elseif descriptor.value.base_type == "double" then
            value = april.newDouble(0)
        elseif descriptor.value.base_type == "boolean" then
            value = april.newBoolean(false)
        end
    elseif descriptor.value.tag == "Class" or descriptor.value.tag == "Array" then
        value = april.newReference()
    end

    return assert(value)
end

function april.newArray(valuetype, size)
    local class, descriptor = april.getArrayClass('[' .. valuetype)
    local object = april.newObject(class)

    size = size or 0
    object.size = size
    object.list = table.create(size)
    for i = 1, size do
        object.list[i] = april.getDefault(descriptor.array_type)
    end

    return object
end

-- TODO: String intern
function april.newString(value: string): JavaReference
    local object = april.newObjectFromClassName("java/lang/String")
    local char_array = april.newArray('C', #value)
    for i, c: any in value:gmatch("()(.)") do
        char_array.list[i] = april.newChar(c:byte())
    end
    local init = object.class.methods["<init>-([C)V"]
    local objectref = april.newReference(object)
    init.func(objectref, april.newReference(char_array))
    return objectref
end

function april.clone(value: JavaValue): JavaValue
    if value.tag == "byte" then
        return april.newByte(value.value)
    elseif value.tag == "char" then
        return april.newChar(value.value)
    elseif value.tag == "integer" then
        return april.newInteger(value.value)
    elseif value.tag == "float" then
        return april.newFloat(value.value)
    -- NOTE: we don't clone category 2 values; dup explicitly does not support them
    -- elseif value.tag == "long" then
    --     return JavaValue.newLong(value.value)
    -- elseif value.tag == "double" then
    --     return JavaValue.newDouble(value.value)
    elseif value.tag == "reference" then
        return value
    end

    return error("unhandled tag " .. tostring(value.tag))
end


function april.findClass(classname)
    local class = april.name_to_class_map[classname]
    return class
end
function april.getClass(classname)
    local class = april.findClass(classname)
    if not class then
        if classname:sub(1, 1) == '[' then
            return (april.getArrayClass(classname))
        end

        error(("failed to find class '%s'"):format(classname))
    end
    return class
end
function april.loadClass(class)
    if class.hasloaded then
        return class
    end
    class.hasloaded = true

    local superclassname = class.superclass
    while superclassname do
        local superclass = april.loadClassByName(superclassname)
        superclassname = superclass.superclass
    end

    local staticinstance = april.newObject(class, true)
    class.staticinstance = staticinstance

    local clinit = class.methods["<clinit>-()V"]
    if clinit then
        clinit.func()
    end

    return class
end
function april.loadClassByName(classname)
    return april.loadClass(april.getClass(classname))
end

local function newMonitor(object)
    return {
        thread = nil,
        waiting_threads = {},
        count = 0
    }
end

function april.newObject(class, static: boolean?): any
    april.loadClass(class)

    local object = {
        class = class,
        isstatic = static
    }
    object.monitor = newMonitor(object)
    object.hashcode = tonumber(tostring(object):match("^table: (0x[%d%a]+)$"))

    local superclassname = class.superclass
    while superclassname do
        local superclass = april.getClass(superclassname)

        for k, v in superclass.fields do
            object[k] = april.getDefault(descriptor_parser.parseFieldDescriptor(v.type))
        end

        superclassname = superclass.superclass
    end

    -- local init = class.methods["<init>-()V"]
    -- if init then
    --     init.func(object)
    -- end

    return object
end
function april.newObjectFromClassName(classname)
    return april.newObject(april.getClass(classname))
end

function april.registerClass(classname, class)
    if april.name_to_class_map[classname] then
        error(("class '%s' has already been registered!"):format(classname))
    end

    april.name_to_class_map[classname] = class
end

function april.lookupField(classname: string, fieldname: string, fieldtype: string, usecache: boolean?)
    local class = april.getClass(classname)

    local cachename = fieldname .. '-' .. fieldtype

    local cached = if usecache then class.fieldcache[cachename] else nil

    if cached then
        return table.unpack(cached)
    end

    local fieldclass = class
    local field = class.fields[fieldname]

    -- superinterfaces lookup
    do
        local key = nil
        local value = nil
        while not field do
            key, value = next(class.interfaces, key)

            if not value then
                break
            end

            local potentialclass = april.getClass(value)
            local potentialfield = potentialclass.fields[fieldname]
            if potentialfield and potentialfield.type == fieldtype then
                field, fieldclass = potentialfield, potentialclass
            end
        end
    end

    -- superclass lookup
    do
        local superclassname = class.superclass
        while superclassname and not field do
            local potentialclass = april.getClass(superclassname)
            local potentialfield = potentialclass.fields[fieldname]
            if potentialfield and potentialfield.type == fieldtype then
                field, fieldclass = potentialfield, potentialclass
            else
                superclassname = potentialclass.superclass
            end
        end
    end

    if not field then
        error(("failed to find field '%s' ('%s') on class '%s'"):format(fieldname, fieldtype, classname))
    end

    if field.type ~= fieldtype then
        error(("failed to find field '%s' ('%s') on class '%s' (field existed by invalid type)"):format(fieldname, fieldtype, classname))
    end

    if usecache then
        class.fieldcache[cachename] = { field, fieldclass }
    end

    april.loadClass(fieldclass)

    return field, fieldclass
end

function april.lookupMethod(classname: string, methodname: string, methodtype: string, usecache: boolean?)
    local class = april.getClass(classname)

    local combinedmethodname = methodname .. '-' .. methodtype

    local cached = if usecache then class.methodcache[combinedmethodname] else nil

    if cached then
        return table.unpack(cached)
    end

    -- TODO: signature polymorphic methods (see 2.9)

    local methodclass = class
    local method = class.methods[combinedmethodname]

    -- superclass lookup
    do
        local superclassname = class.superclass
        while superclassname and not method do
            local potentialclass = april.getClass(superclassname)
            local potentialmethod = potentialclass.methods[combinedmethodname]
            if potentialmethod then
                method, methodclass = potentialmethod, potentialclass
            else
                superclassname = potentialclass.superclass
            end
        end
    end

    -- superinterfaces lookup
    -- TODO: find maximally-specific superinterface
    do
        local key = nil
        local value = nil
        while not method do
            key, value = next(class.interfaces, key)

            if not value then
                break
            end

            local potentialclass = april.getClass(value)
            local potentialmethod = potentialclass.methods[combinedmethodname]
            -- TODO: check abstract
            if potentialmethod then
                method, methodclass = potentialmethod, potentialclass
            end
        end
    end

    if not method then
        error(("failed to find method '%s' ('%s') on class '%s'"):format(methodname, methodtype, classname))
    end

    if usecache then
        class.methodcache[combinedmethodname] = { method, methodclass }
    end

    april.loadClass(methodclass)

    return method, methodclass
end
function april.lookupMethodSpecial(currentclassname, classname, methodname, methodtype, usecache)
    local _, resolvedclass = april.lookupMethod(classname, methodname, methodtype, usecache)

    local currentclass = april.getClass(currentclassname)
    local class = resolvedclass

    local superclassname = currentclass.superclass
    if superclassname then
        local superclass = april.getClass(superclassname)
        local use_super = (methodname ~= "<init>" and methodname ~= "<clinit>") and
            (not resolvedclass.isinterface) and
            (superclass.issuper)
        if use_super then
            class = superclass
        end
    end

    local combinedmethodname = methodname .. '-' .. methodtype

    local methodclass = class
    local method = class.methods[combinedmethodname]
    -- superclass lookup
    do
        while superclassname and not method do
            local potentialclass = april.getClass(superclassname)
            local potentialmethod = potentialclass.methods[combinedmethodname]
            if potentialmethod then
                method, methodclass = potentialmethod, potentialclass
            else
                superclassname = potentialclass.superclass
            end
        end
    end

    if not method then
        if methodclass.isinterface then
            local potentialclass = april.getClass("java/lang/Object")
            local potentialmethod = potentialclass.methods[combinedmethodname]
            method, methodclass = potentialmethod, potentialclass
        end
    end

    -- superinterfaces lookup
    -- TODO: find maximally-specific superinterface
    do
        local key = nil
        local value = nil
        while not method do
            key, value = next(class.interfaces, key)

            if not value then
                break
            end

            local potentialclass = april.getClass(value)
            local potentialmethod = potentialclass.methods[combinedmethodname]
            -- TODO: check abstract
            if potentialmethod then
                method, methodclass = potentialmethod, potentialclass
            end
        end
    end

    if not method then
        error(("failed to find method '%s' ('%s') on class '%s'"):format(methodname, methodtype, classname))
    end

    return method, methodclass
end

function april.getArrayClass(arraytype): (any, descriptor_parser.FieldDescriptorValueArray)
    local descriptor = descriptor_parser.parseFieldDescriptor(arraytype)
    assert(descriptor.value.tag == "Array", "getArrayClass expects an array type descriptor")

    local class = april.array_class_map[descriptor]
    if class then
        return class, descriptor.value
    end

    class = {
        name = arraytype,
        hasloaded = false,
        isarray = true,
        issuper = true,
        isinterface = false,
        valuedescriptor = descriptor.value.array_type,
        superclass = "java/lang/Object",
        interfaces = {
            -- FIXME: put these back when they exist
            -- "java/lang/Cloneable",
            -- "java/io/Serializable"
        },
        fieldcache = {},
        methodcache = {},
        fields = {},
        methods = {}
    }
    april.array_class_map[descriptor] = class
    april.registerClass(arraytype, class)

    return class, descriptor.value
end

function april.hasSuperClass(class, targetsuperclass)
    local superclassname = class.superclass
    while superclassname do
        local superclass = april.getClass(superclassname)

        if superclass == targetsuperclass then
            return true
        end

        superclassname = superclass.superclass
    end

    return false
end
function april.hasSuperClassByName(class, targetsuperclassname)
    return april.hasSuperClass(class, april.getClass(targetsuperclassname))
end
function april.hasSuperInterface(class, superinterface)
    for _, interface in class.interfaces do
        if interface == superinterface then
            return true
        end
    end

    return false
end

function april.instanceofByClassAndTargetClass(class: any, targetclass: any)
    if class.isinterface then
        -- TODO: verify all interface branch cases
        if targetclass.isinterface then
            return targetclass == class or april.hasSuperInterface(class, targetclass)
        else
            return targetclass == april.getClass("java/lang/Object");
        end
    elseif class.isarray then
        if targetclass.isinterface then
            return targetclass == april.getClass("java/lang/Cloneable") or targetclass == april.getClass("java/io/Serializable")
        -- TODO: verify all types of array branch cases
        elseif targetclass.isarray then
            local Svaluedescriptor:descriptor_parser.FieldDescriptor = class.valuedescriptor
            local Tvaluedescriptor: descriptor_parser.FieldDescriptor = targetclass.valuedescriptor

            while true do
                if Tvaluedescriptor.value.tag ~= Svaluedescriptor.value.tag then
                    return false
                end
                if Tvaluedescriptor.value.tag == "Base" then
                    return Tvaluedescriptor.value.base_type == (Svaluedescriptor.value :: descriptor_parser.FieldDescriptorValueBase).base_type
                elseif Tvaluedescriptor.value.tag == "Array" then
                    Svaluedescriptor = (Svaluedescriptor.value :: descriptor_parser.FieldDescriptorValueArray).array_type
                    Tvaluedescriptor = Tvaluedescriptor.value.array_type
                    continue
                elseif Tvaluedescriptor.value.tag == "Class" then
                    return Tvaluedescriptor.value.name == (Svaluedescriptor.value :: descriptor_parser.FieldDescriptorValueClass).name
                end
            end
        else
            return targetclass == april.getClass("java/lang/Object");
        end
    else
        if targetclass.isinterface then
            return april.hasSuperInterface(class, targetclass)
        else
            return targetclass == class or april.hasSuperClass(class, targetclass)
        end
    end
end
function april.instanceofByClassAndTargetName(class, targetname)
    return april.instanceofByClassAndTargetClass(class, april.getClass(targetname))
end

return april

