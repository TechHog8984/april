--!optimize 2
--!strict

local descriptor_parser = require("@april/descriptor")

local april = {
    name_to_class_map = {},
    array_class_map = {},
    callstack = {},
    hooks_newObject = {} :: any,
    exception_thrown = nil,
    constant_patch_objects = {}
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
export type JavaShort = {
    tag: "short",
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
    bytes: number,
    category1: true
}
export type JavaLong = {
    tag: "long",
    value: number,
    high: number,
    low: number,
    category1: false
}
export type JavaDouble = {
    tag: "double",
    value: number,
    high: number,
    low: number,
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
export type JavaReturnAddress = {
    tag: "returnAddress",
    value: number,
    category1: true
}

export type JavaValue = JavaByte | JavaChar | JavaShort | JavaInteger | JavaFloat | JavaLong | JavaDouble | JavaBoolean | JavaStub | JavaReference | JavaReturnAddress

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
function april.newShort(value: number): JavaShort
    value = april.wrapSigned(math.floor(value), 16)

    return {
        tag = "short",
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
    local str = string.pack('f', value)

    local bytes = string.unpack("I4", str)

    return {
        tag = "float",
        value = value,
        bytes = bytes,
        category1 = true
    }
end
function april.newFloatFromBytes(bytes: number): JavaFloat
    local str = string.pack("I4", bytes)

    local value = string.unpack('f', str)

    return {
        tag = "float",
        bytes = bytes,
        value = value,
        category1 = true
    }
end
function april.newLong(value: number): JavaLong
    value = math.floor(value)
    local str = string.pack('l', value)

    local low, high = string.unpack("I4I4", str)

    return {
        tag = "long",
        value = value,
        high = high,
        low = low,
        category1 = false
    }
end
function april.newLongFromBits(high: number, low: number): JavaLong
    high = april.wrapUnsigned(high, 32)
    low = april.wrapUnsigned(low, 32)
    local str = string.pack("I4I4", low, high)

    local value = string.unpack('l', str)

    return {
        tag = "long",
        value = value,
        high = high,
        low = low,
        category1 = false
    }
end
function april.newDouble(value: number): JavaDouble
    local str = string.pack('n', value)

    local low, high = string.unpack("I4I4", str)

    return {
        tag = "double",
        value = value,
        high = high,
        low = low,
        category1 = false
    }
end
function april.newDoubleFromBits(high: number, low: number): JavaDouble
    local str = string.pack("I4I4", low, high)

    local value = string.unpack('n', str)

    return {
        tag = "double",
        value = value,
        high = high,
        low = low,
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
-- TODO: april.newNull = () -> april.newReference().... call april.newNull instead and our code will be more readable
function april.newReference(value: any): JavaReference
    return {
        tag = "reference",
        value = value,
        category1 = true
    }
end
function april.newReturnAddress(value: number): JavaReturnAddress
    return {
        tag = "returnAddress",
        value = value,
        category1 = true
    }
end

local function longadd(ah: number, al: number, bh: number, bl: number): (number, number)
    local low = al + bl
    local carry = 0

    if low >= 4294967296 then
        low -= 4294967296
        carry = 1
    end

    return april.wrapSigned(ah + bh + carry, 32),
        april.wrapUnsigned(low, 32)
end
function april.longAdd(a: JavaLong, b: JavaLong): JavaLong
    return april.newLongFromBits(longadd(a.high, a.low, b.high, b.low))
end

local function longsub(ah: number, al: number, bh: number, bl: number): (number, number)
    local borrow = 0

    local low = al - bl

    if low < 0 then
        low += 4294967296
        borrow = 1
    end

    return april.wrapSigned(ah - bh - borrow, 32),
        april.wrapUnsigned(low, 32)
end
function april.longSub(a: JavaLong, b: JavaLong): JavaLong
    return april.newLongFromBits(longsub(a.high, a.low, b.high, b.low))
end

local function longmul32(a: number, b: number): (number, number)
    local a0 = bit32.band(a, 0xffff)
    local a1 = bit32.rshift(a, 16)

    local b0 = bit32.band(b, 0xffff)
    local b1 = bit32.rshift(b, 16)

    local p0 = a0*b0
    local p1 = a0*b1
    local p2 = a1*b0
    local p3 = a1*b1

    local middle = bit32.rshift(p0, 16) + bit32.band(p1, 0xffff) + bit32.band(p2, 0xffff)

    local low = bit32.bor(
        bit32.lshift(bit32.band(middle, 0xffff), 16),
        bit32.band(p0, 0xffff))

    local high = p3 +
        bit32.rshift(p1, 16) +
        bit32.rshift(p2, 16) +
        bit32.rshift(middle, 16)

    return high, low
end
function april.longMul(a: JavaLong, b: JavaLong): JavaLong
    local p0h, p0l = longmul32(a.low, b.low)

    local _, c1 = longmul32(a.low, b.high)
    local _, c2 = longmul32(a.high, b.low)

    return april.newLongFromBits(
        april.wrapSigned(p0h + c1 + c2, 32),
        p0l
    )
end

local function longcmp(ah: number, al: number, bh: number, bl: number)
    if ah < bh then return -1 end
    if ah > bh then return 1 end

    if al < bl then return -1 end
    if al > bl then return 1 end

    return 0
end

local function longshl1(h: number, l: number): (number, number)
    local newh = bit32.bor(bit32.lshift(h, 1), bit32.rshift(l, 31))
    local newl = bit32.band(bit32.lshift(l, 1), 0xffffffff)

    return april.wrapSigned(newh, 32), newl
end

local function longudiv(ah: number, al: number, bh: number, bl: number): (number, number)
    local qh = 0
    local ql = 0

    local rh = 0
    local rl = 0

    for i = 63, 0, -1 do
        rh, rl = longshl1(rh, rl)

        local bit
        if i >= 32 then
            bit = bit32.band(bit32.rshift(ah, (i-32)), 1)
        else
            bit = bit32.band(bit32.rshift(al, i), 1)
        end

        rl = bit32.bor(rl, bit)

        if longcmp(rh, rl, bh, bl) >= 0 then
            rh, rl = longsub(rh, rl, bh, bl)

            if i >= 32 then
                qh = bit32.bor(qh, bit32.lshift(1, (i-32)))
            else
                ql = bit32.bor(ql, bit32.lshift(1, i))
            end
        end
    end

    return qh, ql
end

function longnegate(high: number, low: number): (number, number)
    high = bit32.bnot(high)
    low = bit32.bnot(low)

    return longadd(high, low, 0, 1)
end
function april.longNegate(a: JavaLong): JavaLong
    return april.newLongFromBits(longnegate(a.high, a.low))
end
function april.longDiv(a: JavaLong, b: JavaLong): JavaLong
    local a_lt0 = longcmp(a.high, a.low, 0, 0) == -1
    local b_lt0 = longcmp(b.high, b.low, 0, 0) == -1

    local sign = bit32.bxor(if a_lt0 then 1 else 0, if b_lt0 then 1 else 0)

    local ah = a.high
    local al = a.low

    local bh = b.high
    local bl = b.low

    if a_lt0 then
        ah, al = longnegate(ah, al)
    end
    if b_lt0 then
        bh, bl = longnegate(bh, bl)
    end

    local qh, ql = longudiv(ah, al, bh, bl)

    if sign ~= 0 then
        qh, ql = longnegate(qh, ql)
    end

    return april.newLongFromBits(qh, ql)
end

function april.coerce(input: JavaValue, target: descriptor_parser.FieldDescriptor): JavaValue
    local targetvalue = target.value
    if targetvalue.tag == "Array" then
        if input.value then
            assert(input.tag == "reference" and input.value.class.isarray, "could not coerce value into array")
            local arrayclass = april.getArrayClass(target.raw)

            local inputvalueclass = april.getClassFromDescriptor(input.value.class.valuedescriptor)
            local arrayvalueclass = april.getClassFromDescriptor(targetvalue.array_type)
            assert(input.value.class == arrayclass or
                if arrayvalueclass.isinterface then
                    april.implementsInterface(inputvalueclass, arrayvalueclass)
                else
                    april.hasSuperClass(inputvalueclass,arrayvalueclass))
        end
        return input
    elseif targetvalue.tag == "Class" then
        -- TODO: check cast
        assert(input.tag == "reference", "could not coerce value into instance")
        return input
    elseif targetvalue.tag == "Base" then
        if targetvalue.base_type == "byte" then
            if input.tag == "byte" then
                return input
            elseif input.tag == "boolean" then
                return april.newByte(if input.value then 1 else 0)
            end
            assert(input.tag == "integer" or input.tag == "short" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into byte")
            return april.newByte(input.value)
        elseif targetvalue.base_type == "char" then
            if input.tag == "char" then
                return input
            elseif input.tag == "boolean" then
                return april.newChar(if input.value then 1 else 0)
            end
            assert(input.tag == "integer" or input.tag == "short" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into char")
            return april.newChar(input.value)
        elseif targetvalue.base_type == "double" then
            if input.tag == "double" then
                return input
            elseif input.tag == "boolean" then
                return april.newDouble(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "short" or input.tag == "integer" or input.tag == "float" or input.tag == "long", "could not coerce value into double")
            return april.newDouble(input.value)
        elseif targetvalue.base_type == "float" then
            if input.tag == "float" then
                return input
            elseif input.tag == "boolean" then
                return april.newFloat(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "short" or input.tag == "integer" or input.tag == "long" or input.tag == "double", "could not coerce value into float")
            return april.newFloat(input.value)
        elseif targetvalue.base_type == "int" then
            if input.tag == "integer" then
                return input
            elseif input.tag == "boolean" then
                return april.newInteger(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "short" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into integer")
            return april.newInteger(input.value)
        elseif targetvalue.base_type == "long" then
            if input.tag == "long" then
                return input
            elseif input.tag == "boolean" then
                return april.newLong(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "short" or input.tag == "integer" or input.tag == "float" or input.tag == "double", "could not coerce value into long")
            return april.newLong(input.value)
        elseif targetvalue.base_type == "short" then
            if input.tag == "short" then
                return input
            elseif input.tag == "boolean" then
                return april.newShort(if input.value then 1 else 0)
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "integer" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into short")
            return april.newShort(input.value)
        elseif targetvalue.base_type == "boolean" then
            if input.tag == "boolean" then
                return input
            end
            assert(input.tag == "byte" or input.tag == "char" or input.tag == "short" or input.tag == "integer" or input.tag == "float" or input.tag == "long" or input.tag == "double", "could not coerce value into boolean")
            return april.newBoolean(if input.value == 0 then false else true)
        end
    end

    error("failed to coerce")
end

function april.intValue(value: JavaValue): number?
    if value.tag == "integer" or value.tag == "byte" or value.tag == "char" or value.tag == "short" then
        return value.value
    elseif value.tag == "boolean" then
        return if value.value then 1 else 0
    end

    return nil
end
function april.intJavaValue(value: JavaValue): JavaValue?
    if value.tag == "integer" then
        return value
    elseif value.tag == "byte" or value.tag == "char" or value.tag == "short" then
        return april.newInteger(value.value)
    elseif value.tag == "boolean" then
        return april.newInteger(if value.value then 1 else 0)
    end

    return nil
end

function april.getDefault(descriptor: descriptor_parser.FieldDescriptor)
    local value: JavaValue
    if descriptor.value.tag == "Base" then
        if descriptor.value.base_type == "byte" then
            value = april.newByte(0)
        elseif descriptor.value.base_type == "short" then
            value = april.newShort(0)
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

function april.getDescriptorFromClass(class)
    if class.isprimitive then
        return descriptor_parser.parseFieldDescriptor(class.name)
    elseif class.isarray then
        return descriptor_parser.parseFieldDescriptor('[' .. class.valuedescriptor.raw)
    end

    return descriptor_parser.parseFieldDescriptor('L' .. class.name .. ';')
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
function april.newArrayFromClass(class, size)
    return april.newArray(april.getDescriptorFromClass(class).raw, size)
end

function april.newStringFromLuaCharArray(array: { number }): JavaReference
    local count = #array
    local newarray = table.create(#array)
    do
        local n = 0
        local i = 1
        while i <= count do
            local b = array[i]

            if bit32.band(b, 0x80) == 0 then
                n += 1
                newarray[n] = b
            elseif bit32.band(b, 0xE0) == 0xC0 then
                i += 1
                local b2 = assert(array[i], "invalid utf8")
                n += 1
                newarray[n] = bit32.bor(bit32.lshift(bit32.band(b, 0x1F), 6), bit32.band(b2, 0x3F))
            else
                i += 1
                local b2 = assert(array[i], "invalid utf8")
                i += 1
                local b3 = assert(array[i], "invalid utf8")
                n += 1
                newarray[n] = bit32.bor(bit32.lshift(bit32.band(b, 0x0F), 12), bit32.lshift(bit32.band(b2, 0x3F), 6), bit32.band(b3, 0x3F))
            end

            i += 1
        end
        count = n
        array = newarray
    end

    local object = april.newObjectFromClassName("java/lang/String")
    local char_array = april.newArray('C', count)
    for i = 1, count do
        char_array.list[i] = april.newChar(array[i])
    end
    local init = object.class.methods["<init>-([C)V"]
    local objectref = april.newReference(object)
    init.func(objectref, april.newReference(char_array))
    april.checkException()
    return objectref
end
function april.newString(value: string): JavaReference
    local array = table.create(#value)
    for i, c: any in value:gmatch("()(.)") do
        array[i] = c:byte()
    end
    return april.newStringFromLuaCharArray(array)
end

function april.clone(value: JavaValue): JavaValue
    if value.tag == "byte" then
        return april.newByte(value.value)
    elseif value.tag == "char" then
        return april.newChar(value.value)
    elseif value.tag == "short" then
        return april.newShort(value.value)
    elseif value.tag == "integer" then
        return april.newInteger(value.value)
    elseif value.tag == "float" then
        return april.newFloat(value.value)
    elseif value.tag == "long" then
        return april.newLongFromBits(value.high, value.low)
    elseif value.tag == "double" then
        return april.newDouble(value.value)
    elseif value.tag == "boolean" then
        return april.newBoolean(value.value)
    elseif value.tag == "reference" then
        return value
    end

    return error("unhandled tag " .. tostring(value.tag))
end

local slot_counter = 0
function april.nextSlot()
    local value = slot_counter
    slot_counter = value + 1
    return value
end

-- NOTE: override me!
function april.getClassLoader(...): any
    return
end

local function assignClassObject(class)
    local class_class = april.getClass("java/lang/Class")
    local classobject = april.newObject(class_class)

    local classloader = april.getClassLoader(class)
    if classloader then
        local ref = april.newReference(classloader)
        rawset(ref, "assignedclass", class)
        classobject[class_class.fields.classLoader] = ref
    end

    classobject.classclass = class
    class.classobject = classobject
end

function april.fetchClass(classname): any
    error("april.fetchClass should be overriden by your environment")
end
function april.findClass(classname)
    local cached = april.name_to_class_map[classname]
    if cached then
        return cached
    end

    local class
    if classname:match("^%[") then
        class = april.getArrayClass(classname)
    elseif classname:match("^[ZBCSIJFDV]$") then
        class = april.getPrimitiveClass(classname)
    else
        class = april.fetchClass(classname)

        if class and not class.classobject then
            class.classobject = true -- don't recurse indefinitely
            assignClassObject(class)
        end
    end
    if class then
        april.name_to_class_map[classname] = class
    end

    return class
end

-- TODO: return type is cast to any because the shitty solver thinks it returns two things somehow
function april.getClass(classname): any
    local class = april.findClass(classname)
    if not class then
        error(("failed to find class '%s'"):format(classname))
    end

    return class
end
function april.getClassFromDescriptor(descriptor: descriptor_parser.FieldDescriptor): any
    local classname
    if descriptor.value.tag == "Class" then
        classname = descriptor.value.name
    else
        classname = descriptor.raw
    end

    return april.getClass(classname)
end
function april.loadClass(class, initialclassoverride: unknown)
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
        april.checkException()
    end

    return class
end
function april.loadClassByName(classname)
    return april.loadClass(april.getClass(classname))
end

-- TODO: make a like AprilException class that we can throw each time we do an error like for example to error that an opcode isn't implemented
local function newClassInternal(anonymous: boolean, name: string, namewithdots: string, interfaces: { string }?, fields: unknown, methods: unknown, issuper: boolean?, isinterface: boolean?)
    assert(type(anonymous) == "boolean", "missing anonymous (arg 0)")

    if not anonymous then
        assert(not april.name_to_class_map[name], "attempt to create an already-existing class")
    end

    local class = {
        name = assert(name, "missing name (arg 1)"),
        namewithdots = assert(namewithdots, "missing namewithdots (arg 2)"),
        hasloaded = false,
        isarray = false,
        isprimitive = false,
        issuper = not not issuper,
        isinterface = not not isinterface,
        interfaces = interfaces or {},
        fields = fields or {},
        methods = methods or {},
        fieldcache = {},
        methodcache = {}
    }
    assignClassObject(class)

    if not anonymous then
        april.name_to_class_map[name] = class
    end

    return class
end
function april.newClass(name: string, namewithdots: string, interfaces: { string }?, fields: unknown, methods: unknown, issuper: boolean?, isinterface: boolean?)
    return newClassInternal(false, name, namewithdots, interfaces, fields, methods, issuper, isinterface)
end
function april.newClassAnonymous(name: string, namewithdots: string, interfaces: { string }?, fields: unknown, methods: unknown, issuper: boolean?, isinterface: boolean?)
    return newClassInternal(true, name, namewithdots, interfaces, fields, methods, issuper, isinterface)
end

local primitive_name_map = {
    Z = "boolean",
    B = "byte",
    C = "char",
    S = "short",
    I = "int",
    J = "long",
    F = "float",
    D = "double",
    V = "void"
}
function april.getPrimitiveClass(name): any
    local realname = primitive_name_map[name]
    assert(realname)

    local cached = april.name_to_class_map[realname]
    if cached then
        return cached
    end

    local class = april.newClass(realname, realname)
    class.isprimitive = true
    class.issuper = true

    return class
end

local function newMonitor(object)
    return {
        thread = nil,
        waiting_threads = {},
        waiting_threads2 = {},
        count = 0
    }
end

local hashcode_counter = tonumber(tostring({}):match("^table: (0x[%d%a]+)$")) or 1000
function april.newObject(class, static: boolean?): any
    april.loadClass(class)

    local object = {
        class = class,
        isstatic = static
    }
    object.monitor = newMonitor(object)

    local hashcode = hashcode_counter + 4
    object.hashcode = hashcode
    hashcode_counter = hashcode

    local iter = 0
    local classname = class.name
    while classname do
        iter += 1
        local targetclass
        if iter == 1 then
            -- NOTE: this exists to fix defineAnonymousClass native
            targetclass = class
        else
            targetclass = april.getClass(classname)
        end

        for k, v in targetclass.fields do
            if v.isstatic and not static then continue end
            local value: JavaValue
            local constantvalue = v.constantvalue
            if constantvalue then
                local t = constantvalue[1]
                if t == "long" then
                    value = april.newLongFromBits(constantvalue[2], constantvalue[3])
                elseif t == "float" then
                    value = april.newFloatFromBytes(constantvalue[2])
                elseif t == "double" then
                    value = april.newDoubleFromBits(constantvalue[2], constantvalue[3])
                elseif t == "integer" then
                    value = april.newInteger(constantvalue[2])
                elseif t == "string" then
                    value = april.string_intern_map[april.newStringFromLuaCharArray(constantvalue[2])]
                else
                    error("unhandled constant value type: " .. tostring(t))
                end
            else
                value = april.getDefault(descriptor_parser.parseFieldDescriptor(v.type))
            end

            object[v] = value
        end

        classname = targetclass.superclass
    end

    for _, hook in april.hooks_newObject do
        hook(object, static)
    end

    return object
end
function april.newObjectFromClassName(classname)
    return april.newObject(april.getClass(classname))
end

-- function april.registerClass(classname, class)
--     if april.name_to_class_map[classname] then
--         error(("class '%s' has already been registered!"):format(classname))
--     end

--     april.name_to_class_map[classname] = class
--     local classobject = april.newObject(april.getClass("java/lang/Class"))
--     rawset(classobject, "classclass", class)
--     class.classobject = classobject
-- end
function april.registerClassMap(map)
    april.name_to_class_map = map
end

-- TODO: lookupField and lookupMethod are inconsistent in that lookupfield takes a classname whereas lookupmethod has a separate function for taking a classname
-- this really urks me, please change

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
        error(("failed to find field '%s' ('%s') on class '%s' (field existed of a different type: '%s')"):format(fieldname, fieldtype, classname, field.type))
    end

    if usecache then
        class.fieldcache[cachename] = { field, fieldclass }
    end

    april.loadClass(fieldclass)

    return field, fieldclass
end

function april.lookupMethod(class, methodname: string, methodtype: string, usecache: boolean?, canbeabstract: boolean?)
    local classname = class.name
    local combinedmethodname = methodname .. '-' .. methodtype

    local cached = if usecache then class.methodcache[combinedmethodname] else nil

    if cached then
        return table.unpack(cached)
    end

    -- TODO: signature polymorphic methods (see 2.9)

    local methodclass = class
    local method = class.methods[combinedmethodname]

    if not method then
        for _, v in class.methods do
            if v.name == methodname and v.issignaturepolymorphic then
                method = v
                break
            end
        end
    end

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
            if not potentialmethod then
                continue
            end
            if canbeabstract or not potentialmethod.isabstract then
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
function april.lookupMethodByClassName(classname: string, methodname: string, methodtype: string, usecache: boolean?, canbeabstract: boolean?)
    return april.lookupMethod(april.getClass(classname), methodname, methodtype, usecache, canbeabstract)
end
function april.lookupMethodSpecial(currentclassname: string, classname: string, methodname: string, methodtype: string, usecache: boolean?)
    local _, resolvedclass = april.lookupMethodByClassName(classname, methodname, methodtype, usecache)

    local currentclass = april.getClass(currentclassname)
    local class = resolvedclass

    local superclassname = currentclass.superclass
    if superclassname then
        local superclass = april.getClass(superclassname)
        local use_super = (methodname ~= "<init>" and methodname ~= "<clinit>") and
            (if resolvedclass.isinterface then true else april.hasSuperClass(resolvedclass, currentclass)) and
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

function april.arrayClone(array)
    local class = array.class
    local size = array.size
    local cloned = april.newArray(class.valuedescriptor.raw, size)

    for i = 1, size do
        cloned.list[i] = april.clone(array.list[i])
    end

    return cloned
end

function april.getArrayClass(arraytype): (any, descriptor_parser.FieldDescriptorValueArray)
    local descriptor = descriptor_parser.parseFieldDescriptor(arraytype)
    assert(descriptor.value.tag == "Array", "getArrayClass expects an array type descriptor")

    local class = april.array_class_map[descriptor]
    if class then
        return class, descriptor.value
    end

    class = april.newClass(arraytype, arraytype, {
        "java/lang/Cloneable",
        "java/io/Serializable",
    })
    class.isarray = true
    class.issuper = true
    class.valuedescriptor = descriptor.value.array_type
    class.methods["clone-()Ljava/lang/Object;"] = {
        name = "clone",
        type = "()Ljava/lang/Object;",
        slot = april.nextSlot(),
        access_flags = 0x0001,
        isstatic = false,
        isnative = false,
        isabstract = false,
        issignaturepolymorphic = false,
        func = function(this)
            return april.newReference(april.arrayClone(this.value))
        end,
    }
    april.array_class_map[descriptor] = class

    return class, descriptor.value
end

-- TODO: shitty solver says 'expected this to be true, but got false' ???
function april.hasSuperClass(class, targetsuperclass): boolean
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
    for _, interfacename in class.interfaces do
        if interfacename == superinterface.name then
            return true
        end
    end

    return false
end

function april.implementsInterface(class, interface)
    while true do
        if april.hasSuperInterface(class, interface) then
            return true
        end
        if not class.superclass then
            return false
        end

        class = april.getClass(class.superclass)
    end

    return false
end

function april.instanceofByClassAndTargetClass(class: any, targetclass: any)
    if class.isinterface then
        -- TODO: verify all interface branch cases
        if targetclass.isinterface then
            return targetclass == class or april.hasSuperInterface(class, targetclass)
        else
            return targetclass == april.getClass("java/lang/Object")
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
                    return april.instanceofByClassAndTargetName(
                        april.getClass((Svaluedescriptor.value :: descriptor_parser.FieldDescriptorValueClass).name),
                        Tvaluedescriptor.value.name)
                end
            end
        else
            return targetclass == april.getClass("java/lang/Object")
        end
    else
        if targetclass.isinterface then
            return april.implementsInterface(class, targetclass)
        else
            return targetclass == class or april.hasSuperClass(class, targetclass)
        end
    end
end
function april.instanceofByClassAndTargetName(class, targetname)
    return april.instanceofByClassAndTargetClass(class, april.getClass(targetname))
end

-- local wrapped_primitive_map = {
--     ["java/lang/Byte"] = "byte",
--     ["java/lang/Char"] = "char",
--     ["java/lang/Short"] = "short",
--     ["java/lang/Integer"] = "integer",
--     ["java/lang/Float"] = "float",
--     ["java/lang/Long"] = "long",
--     ["java/lang/Double"] = "double",
--     ["java/lang/Boolean"] = "boolean",
-- }
-- function april.instanceofByValueAndWrappedClass(value, class)
--     local classname = class.name
--     local expected_tag = wrapped_primitive_map[classname]
--     if not classname then
--         error("unexpected class in instanceofByValueAndWrappedClass: " .. tostring(classname))
--     end

--     return value.tag == expected_tag
-- end

function april.javaStringArray(value)
    local field = value.class.fields["value"]
    local arr = value[field].value

    return arr
end
function april.javaStringToLuaString(value)
    local arr = april.javaStringArray(value)
    local str = table.create(arr.size)

    for i = 1, arr.size do
        str[i] = utf8.char(arr.list[i].value)
    end

    return table.concat(str)
end
function april.javaStringSize(value)
    return april.javaStringArray(value).size
end

function april.callToString(valueref)
    local methodtype = "()Ljava/lang/String;"
    local method = april.lookupMethodByClassName("java/lang/Object", "toString", methodtype, true)
    local methodoverride
    pcall(function()
        methodoverride = april.lookupMethod(valueref.value.class, "toString", methodtype, true)
    end)
    if methodoverride then
        method = methodoverride
    end

    local result = method.func(valueref)
    april.checkException()

    return result
end

function april.objectRefToLuaString(objectref)
    return april.javaStringToLuaString(april.callToString(objectref).value)
end

april.string_intern_map = setmetatable({}, {
    __index = function(self, key)
        local stringvalue = april.javaStringToLuaString(key.value)
        local obj = rawget(self, stringvalue)
        if not obj then
            obj = key
            self[stringvalue] = key
        end

        return obj
    end
})

function april.getException(): any
    local exception = april.exception_thrown
    if exception then
        april.exception_thrown = nil
        return exception
    end

    return nil
end

function april.checkException()
    local exception = april.getException()
    if exception then
        local method = april.lookupMethod(exception.value.class, "toString", "()Ljava/lang/String;")
        -- TODO: check for stack overflow?
        local str = method.func(exception)
        april.getClass("java/lang/Throwable").methods["printStackTrace-()V"].func(exception)
        error("EXCEPTION THROWN! " .. april.javaStringToLuaString(str.value))
    end
end

function april.newMethodTypeFromDescriptor(descriptor: descriptor_parser.MethodDescriptor)
    local ptypes = april.newArray("Ljava/lang/Class;", descriptor.parameter_count)
    for i, v in descriptor.parameter_list do
        ptypes.list[i] = april.newReference(april.getClassFromDescriptor(v).classobject)
    end
    assert(descriptor.returns.tag == "Field")
    local T = april.getClassFromDescriptor(descriptor.returns.value)

    local methodtypeclass = april.getClass("java/lang/invoke/MethodType")
    april.loadClass(methodtypeclass)

    local makeimpl = methodtypeclass.methods["makeImpl-(Ljava/lang/Class;[Ljava/lang/Class;Z)Ljava/lang/invoke/MethodType;"];

    local methodtyperef = makeimpl.func(april.newReference(T.classobject), april.newReference(ptypes), april.newBoolean(true))
    april.checkException()

    return methodtyperef
end

return april

