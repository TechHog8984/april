local classloader = {
    name_to_class_map = {}
}

function classloader.loadClass(classname)
    local class = classloader.name_to_class_map[classname]
    if not class then
        error(("failed to find class '%s'"):format(classname))
    end

    if class.hasloaded then
        return class
    end
    class.hasloaded = true

    local staticinstance = {
        class = class
    }
    local init = class.methods["<init>-()V"]
    if init then
        init(staticinstance)
    end
    class.staticinstance = staticinstance

    local clinit = class.methods["<clinit>-()V"]
    if clinit then
        clinit()
    end

    return class
end

function classloader.registerClass(classname, class)
    if classloader.name_to_class_map[classname] then
        error(("class '%s' has already been registered!"):format(classname))
    end

    classloader.name_to_class_map[classname] = class
end

function classloader.lookupField(classname, fieldname, fieldtype, usecache)
    local class = classloader.loadClass(classname)

    local cachename = fieldname .. '-' .. fieldtype

    local cached = if usecache then class.fieldcache[cachename] else nil

    if cached then
        return table.unpack(cached)
    end

    -- TODO: local field = class.fields[fieldname] or classloader.lookupField(class.superclass, fieldnamek, fieldtype, usecache)
    -- ^ but what would usecache be???
    -- ^ also superinterfaces

    local field = class.fields[fieldname]
    if not field then
        error(("failed to find field '%s' ('%s') on class '%s'"):format(fieldname, fieldtype, classname))
    end

    if field.type ~= fieldtype then
        error(("failed to find field '%s' ('%s') on class '%s' (field existed by invalid type)"):format(fieldname, fieldtype, classname))
    end

    if usecache then
        -- TODO: update class when we handle superclass
        class.fieldcache[cachename] = { field, class }
    end

    -- TODO: update class when we handle superclass
    return field, class
end

function classloader.lookupMethod(classname, methodname, methodtype, usecache)
    local class = classloader.loadClass(classname)

    local combinedmethodname = methodname .. '-' .. methodtype

    local cached = if usecache then class.methodcache[combinedmethodname] else nil

    if cached then
        return table.unpack(cached)
    end

    -- TODO: signature polymorphic methods (see 2.9)

    -- TODO: local method = class.methods[methodname] or classloader.lookupMethod(class.superclass, methodnamek, methodtype, usecache)
    -- ^ but what would usecache be???
    -- ^ also superinterfaces

    local method = class.methods[combinedmethodname]
    if not method then
        error(("failed to find method '%s' ('%s') on class '%s'"):format(methodname, methodtype, classname))
    end

    if usecache then
        -- TODO: update class when we handle superclass
        class.methodcache[combinedmethodname] = { method, class }
    end

    -- TODO: update class when we handle superclass
    return method, class
end

return classloader
