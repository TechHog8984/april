local natives = {
    class_to_dict_map = {}
}

function natives.registerDefault()
end
function natives.registerMethods(classname, dict)
    natives.class_to_dict_map[classname] = dict
end

function natives.findNative(classname, combinedname)
    local dict = natives.class_to_dict_map[classname]
    if not dict then
        error(("class '%s' has no native methods registered!"):format(classname))
    end

    local native = dict[combinedname]
    if not native then
        error(("class '%s' has no native method '%s' registered!"):format(classname, combinedname))
    end

    return native
end

return natives
