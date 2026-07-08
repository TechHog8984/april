--!optimize 2
--!nolint LocalShadow
-- stolen from aswartz by techhog

local BASE_TYPE_NAME_MAP = {
    B = "byte",
    C = "char",
    D = "double",
    F = "float",
    I = "int",
    J = "long",
    S = "short",
    Z = "boolean"
}
local BASE_TYPE_SIZE_MAP = {
    B = 1,
    C = 1,
    D = 2,
    F = 1,
    I = 1,
    J = 2,
    S = 1,
    Z = 1
}

type Reader = {
    offset: number,
    data: string,
    length: number,
    is_at_end: boolean,

    advance: ( Reader, number ) -> number,
    char: ( Reader ) -> string
}
local Reader = {}
Reader.__index = Reader

local function newReader(data: string): Reader
    local length = #data

    local reader = setmetatable({
        offset = 1,
        data = data,
        length = length,
        is_at_end = false
    } :: Reader, Reader)

    return reader :: any
end

function Reader.advance(self: Reader, amount: number): number
    local offset = self.offset
    self.offset += amount

    if self.offset >= self.length then
        self.is_at_end = true
    end

    return offset
end

function Reader.char(self: Reader): string
    local offset = self:advance(1)
    return string.sub(self.data, offset, offset)
end

export type FieldDescriptorValueBase = {
    tag: "Base",
    base_type: "byte" |
        "char" |
        "double" |
        "float" |
        "int" |
        "long" |
        "short" |
        "boolean"
}
export type FieldDescriptorValueClass = {
    tag: "Class",
    name: string
}
export type FieldDescriptorValueArray = {
    tag: "Array",
    array_type: FieldDescriptor
}

export type FieldDescriptorValue =
    FieldDescriptorValueBase |
    FieldDescriptorValueClass |
    FieldDescriptorValueArray

export type FieldDescriptor = {
    tag: "FieldDescriptor",
    size: number,
    raw: string,
    value: FieldDescriptorValue
}

export type ReturnDescriptor = {
    tag: "Void"
} | {
    tag: "Field",
    value: FieldDescriptor
}

export type MethodDescriptor = {
    tag: "MethodDescriptor",
    size: number,
    stacksize: number,
    raw: string,
    parameter_count: number,
    parameter_list: { FieldDescriptor },
    returns: ReturnDescriptor
}

export type Descriptor = FieldDescriptor | MethodDescriptor

local function _readFieldDescriptor(reader: Reader): FieldDescriptor?
    local start = reader.offset
    local char = reader:char()
    if char:match("[BCDFIJSZ]") then
        return {
            tag = "FieldDescriptor",
            value = {
                tag = "Base",
                base_type = BASE_TYPE_NAME_MAP[char]
            },
            size = BASE_TYPE_SIZE_MAP[char],
            stacksize = BASE_TYPE_SIZE_MAP[char],
            raw = string.sub(reader.data, start, start)
        }
    elseif char == 'L' then
        local class_name = ""
        while not reader.is_at_end do
            local char = reader:char()
            if char == ';' then
                break
            end
            class_name ..= char
        end
        local offset = reader.offset
        if offset < reader.length then
            offset -= 1
        end
        return {
            tag = "FieldDescriptor",
            value = {
                tag = "Class",
                name = class_name
            },
            size = 1,
            stacksize = 1,
            raw = string.sub(reader.data, start, offset)
        }
    elseif char == '[' then
        local array_type = assert(_readFieldDescriptor(reader), "failed to read sub field descriptor in array")
        local offset = reader.offset
        if offset < reader.length then
            offset -= 1
        end
        return {
            tag = "FieldDescriptor",
            value = {
                tag = "Array",
                array_type = array_type
            },
            size = 1 + array_type.size,
            stacksize = 1,
            raw = string.sub(reader.data, start, offset)
        }
    end

    return nil
end
local function _readReturnDescriptor(reader: Reader): ReturnDescriptor
    local char = reader:char()

    if char == 'V' then
        return {
            tag = "Void",
        }
    else
        reader.offset -= 1
        return {
            tag = "Field",
            value = assert(_readFieldDescriptor(reader), "failed to read sub field descriptor in return descriptor")
        }
    end
end
local function _readMethodDescriptor(reader: Reader): MethodDescriptor
    local start = reader.offset
    local char = reader:char()

    assert(char == '(', "first char in method descriptor must be '('")

    local parameter_list = {}
    local parameter_count = 0
    local total_parameters_size = 0
    local total_stack_size = 0
    while not reader.is_at_end do
        local char = reader:char()
        if char == ')' then
            break
        end
        reader.offset -= 1

        local field = assert(_readFieldDescriptor(reader), "failed to read field descriptor in method args")
        parameter_list[#parameter_list + 1] = field
        parameter_count += 1
        total_parameters_size += field.size
        total_stack_size += field.stacksize
    end

    local returns = _readReturnDescriptor(reader)

    return {
        tag = "MethodDescriptor",
        size = total_parameters_size,
        stacksize = total_stack_size,
        raw = string.sub(reader.data, start, reader.offset),

        parameter_count = parameter_count,
        parameter_list = parameter_list,
        returns = returns
    }
end

local field_descriptor_cache = {}
local function parseFieldDescriptor(descriptor): FieldDescriptor
    local cached = field_descriptor_cache[descriptor]
    if cached then
        return cached
    end
    local reader = newReader(descriptor)

    local result = _readFieldDescriptor(reader)
    if not result then
        error("[java_class.descriptor.parseFieldDescriptor] failed to parse field descriptor")
    end

    assert(reader.is_at_end)

    assert(result.raw == descriptor, ("%s, %s"):format(descriptor, result.raw))
    field_descriptor_cache[descriptor] = result

    return result
end
local method_descriptor_cache = {}
local function parseMethodDescriptor(descriptor): MethodDescriptor
    local cached = method_descriptor_cache[descriptor]
    if cached then
        return cached
    end
    local reader = newReader(descriptor)

    local result = _readMethodDescriptor(reader)
    assert(result, "failed to parse method descriptor")
    if not result then
        error("[java_class.descriptor.parseMethodDescriptor] failed to parse method descriptor")
    end

    assert(reader.is_at_end)

    assert(result.raw == descriptor, ("%s, %s"):format(descriptor, result.raw))
    method_descriptor_cache[descriptor] = result

    return result
end
local function parseFieldOrMethodDescriptor(descriptor): FieldDescriptor | MethodDescriptor
    local cached = field_descriptor_cache[descriptor] or method_descriptor_cache[descriptor]
    if cached then
        return cached
    end

    local reader = newReader(descriptor)

    local field_result = _readFieldDescriptor(reader)
    local method_result
    if not field_result then
        reader.offset -= 1
        method_result = _readMethodDescriptor(reader)
    end

    local result = field_result or method_result
    assert(result, "failed to parse field or method descriptor")
    if not result then
        error("[java_class.descriptor.parsefieldOrMethodDescriptor] failed to parse field or method descriptor")
    end

    assert(reader.is_at_end)

    assert(result.raw == descriptor, ("%s, %s"):format(descriptor, result.raw))
    ;(if field_result then field_descriptor_cache else method_descriptor_cache)[descriptor] = result

    return result
end

return {
    parseFieldDescriptor = parseFieldDescriptor,
    parseMethodDescriptor = parseMethodDescriptor,
    parseFieldOrMethodDescriptor = parseFieldOrMethodDescriptor
}

