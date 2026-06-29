local classloader = {
    name_to_class_map = {}
}

function classloader.findClass(classname)
    local class = classloader.name_to_class_map[classname]
    if not class then
        error(("failed to find class '%s'"):format(classname))
    end
    return class
end
function classloader.loadClass(class)
    if class.hasloaded then
        return class
    end
    class.hasloaded = true

    local superclassname = class.superclass
    while superclassname do
        local superclass = classloader.loadClassByName(superclassname)
        superclassname = superclass.superclass
    end

    local staticinstance = classloader.newObject(class, true)
    class.staticinstance = staticinstance

    local clinit = class.methods["<clinit>-()V"]
    if clinit then
        clinit.func()
    end

    return class
end
function classloader.loadClassByName(classname)
    return classloader.loadClass(classloader.findClass(classname))
end

function classloader.registerClass(classname, class)
    if classloader.name_to_class_map[classname] then
        error(("class '%s' has already been registered!"):format(classname))
    end

    classloader.name_to_class_map[classname] = class
end

function classloader.lookupField(classname, fieldname, fieldtype, usecache)
    local class = classloader.findClass(classname)

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

            local potentialclass = classloader.findClass(value)
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
            local potentialclass = classloader.findClass(superclassname)
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

    classloader.loadClass(fieldclass)

    return field, fieldclass
end

function classloader.lookupMethod(classname, methodname, methodtype, usecache)
    local class = classloader.findClass(classname)

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
            local potentialclass = classloader.findClass(superclassname)
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

            local potentialclass = classloader.findClass(value)
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

    classloader.loadClass(methodclass)

    return method, methodclass
end
function classloader.lookupMethodSpecial(currentclassname, classname, methodname, methodtype, usecache)
    local _, resolvedclass = classloader.lookupMethod(classname, methodname, methodtype, usecache)

    local currentclass = classloader.findClass(currentclassname)
    local class = resolvedclass

    local superclassname = currentclass.superclass
    if superclassname then
        local superclass = classloader.findClass(superclassname)
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
            local potentialclass = classloader.findClass(superclassname)
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
            local potentialclass = classloader.findClass("java/lang/Object")
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

            local potentialclass = classloader.findClass(value)
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

function classloader.newObject(class, static)
    classloader.loadClass(class)

    local object = {
        class = class,
        isstatic = static
    }

    local init = class.methods["<init>-()V"]
    if init then
        init.func(object)
    end

    return object
end
function classloader.newObjectFromClassName(classname)
    return classloader.newObject(classloader.findClass(classname))
end

return classloader
