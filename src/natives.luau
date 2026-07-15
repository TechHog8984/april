--!optimize 2

local april = require("@april/april")
local descriptor_parser = require("@april/descriptor")

local natives = {
    class_to_dict_map = {}
}

function natives.runProcess(...)
    error("please override natives.runProcess")
end
function natives.isFile(...)
    error("please override natives.isFile")
end
function natives.isDir(...)
    error("please override natives.isDir")
end
function natives.fileExists(...)
    error("please override natives.fileExists")
end
function natives.readFile(...)
    error("please override natives.readFile")
end
function natives.setJavaHome(java_home)
    natives.java_home = java_home
end
function natives.setJavaLibraryPath(java_library_path)
    natives.java_library_path = java_library_path
end

function natives.registerDefault()
    natives.registerMethods("java/lang/Object", {
        ["registerNatives-()V"] = function()
            local dict = natives.class_to_dict_map["java/lang/Object"]

            dict["hashCode-()I"] = function(this)
                return april.newInteger(this.value.hashcode)
            end
            dict["getClass-()Ljava/lang/Class;"] = function(this)
                return april.newReference(this.value.class.classobject)
            end
        end,
        ["clone-()Ljava/lang/Object;"] = function(this)
            local class = this.value.class
            if class.isarray then
                local size = this.value.size
                local cloned = april.newArray(class.valuedescriptor.raw, size)

                for i = 1, size do
                    cloned.list[i] = april.clone(this.value.list[i])
                end

                return april.newReference(cloned)
            else
                local cloned = april.newObject(this.value.class)
                -- TODO: check Cloneable interface

                local classname = this.value.class.name
                while classname do
                    local targetclass = april.getClass(classname)

                    for _, field in targetclass.fields do
                        if field.isstatic then continue end
                        cloned[field] = april.clone(this.value[field])
                    end

                    classname = targetclass.superclass
                end

                return april.newReference(cloned)
            end
        end,
        ["wait-(J)V"] = function(this, timeout)
            assert(timeout.value == 0)

            local monitor = this.value.monitor
            local current_thread = coroutine.running()
            local owning_thread = monitor.thread

            assert(current_thread == owning_thread, "you do not own the monitor")

            monitor.waiting_threads2[#monitor.waiting_threads2 + 1] = current_thread
            coroutine.yield()
        end,
        ["notifyAll-()V"] = function(this)
            local monitor = this.value.monitor

            while true do
                local waiting_thread = table.remove(monitor.waiting_threads2, #monitor.waiting_threads2)
                if not waiting_thread then
                    break
                end
                coroutine.resume(waiting_thread)
            end
        end
    })

    local primitive_name_map = {
        boolean = 'Z',
        byte = 'B',
        char = 'C',
        short = 'S',
        int = 'I',
        long = 'J',
        float = 'F',
        double = 'D',
        void = 'V',
    }
    natives.registerMethods("java/lang/Class", {
        ["registerNatives-()V"] = function()
            local dict = natives.class_to_dict_map["java/lang/Class"]

            -- TODO: proper desiredAssertionStatus0
            dict["desiredAssertionStatus0-(Ljava/lang/Class;)Z"] = function(classobj)
                return april.newBoolean(true)
            end
            dict["getPrimitiveClass-(Ljava/lang/String;)Ljava/lang/Class;"] = function(name)
                return april.newReference(april.getPrimitiveClass(primitive_name_map[april.javaStringToLuaString(name.value)]).classobject)
            end
            dict["getName0-()Ljava/lang/String;"] = function(classobj)
                local class = classobj.value.classclass
                return april.newString(class.namewithdots)
            end
            dict["forName0-(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;"] = function(classname, initialize, loader, caller)
                local name = april.javaStringToLuaString(classname.value):gsub("%.", "/")
                local class = april.getClass(name)

                if initialize then
                    april.loadClass(class)
                end

                return april.newReference(class.classobject)
            end
            dict["getEnclosingMethod0-()[Ljava/lang/Object;"] = function(this)
                local class = this.value.classclass

                local enclosing = class.enclosingmethod
                if not enclosing then
                    return april.newReference()
                end

                local enclosingclass = april.getClass(enclosing.classname)
                local methodname = enclosing.methodname;
                local methodtype = enclosing.methodtype;

                local array = april.newArray("Ljava/lang/Object;", 3)

                array.list[1] = april.newReference(enclosingclass.classobject)
                if methodname then
                    assert(methodtype)
                    array.list[2] = april.newString(methodname)
                    array.list[3] = april.newString(methodtype)
                end

                return april.newReference(april)
            end
            dict["getDeclaringClass0-()Ljava/lang/Class;"] = function(this)
                local class = this.value.classclass

                if class.innerclasses then
                    for _, info in class.innerclasses do
                        if info.innerclassname == class.name then
                            return april.newReference(april.getClass(info.outerclassname).classobject)
                        end
                    end
                end

                return april.newReference()
            end
            dict["getDeclaredFields0-(Z)[Ljava/lang/reflect/Field;"] = function(this, public_only)
                local class = this.value.classclass

                local list = {}
                local count = 0

                local fieldclass = april.getClass("java/lang/reflect/Field")

                local field_clazz = assert(fieldclass.fields["clazz"], "failed to find clazz field")
                local field_slot = assert(fieldclass.fields["slot"], "failed to find slot field")
                local field_name = assert(fieldclass.fields["name"], "failed to find name field")
                local field_type = assert(fieldclass.fields["type"], "failed to find type field")
                local field_modifiers = assert(fieldclass.fields["modifiers"], "failed to find modifiers field")

                for _, field in class.fields do
                    if public_only.value and not field.ispublic then
                        continue
                    end

                    local fieldobj = april.newObject(fieldclass)
                    fieldobj[field_clazz] = this
                    fieldobj[field_slot] = april.newInteger(field.slot)
                    fieldobj[field_name] = april.string_intern_map[april.newString(field.name)]
                    fieldobj[field_type] = april.newReference(april.getClassFromDescriptor(descriptor_parser.parseFieldDescriptor(field.type)).classobject)
                    fieldobj[field_modifiers] = april.newInteger(field.access_flags)

                    count += 1
                    list[count] = fieldobj
                end

                local array = april.newArray("Ljava/lang/reflect/Field;", count)
                for i = 1, count do
                    array.list[i] = april.newReference(list[i])
                end

                return april.newReference(array)
            end
            dict["getDeclaredConstructors0-(Z)[Ljava/lang/reflect/Constructor;"] = function(this, public_only)
                local class = this.value.classclass

                local list = {}
                local count = 0

                local constructorclass = april.getClass("java/lang/reflect/Constructor")

                local field_clazz = assert(constructorclass.fields["clazz"], "failed to find clazz field")
                local field_slot = assert(constructorclass.fields["slot"], "failed to find slot field")
                local field_parameterTypes = assert(constructorclass.fields["parameterTypes"], "failed to find parameterTypes field")
                local field_exceptionTypes = assert(constructorclass.fields["exceptionTypes"], "failed to find exceptionTypes field")
                local field_modifiers = assert(constructorclass.fields["modifiers"], "failed to find modifiers field")

                for _, method in class.methods do
                    if method.name ~= "<init>" then
                        continue
                    end
                    if public_only.value and not method.ispublic then
                        continue
                    end

                    local constructorobj = april.newObject(constructorclass)
                    constructorobj[field_clazz] = this
                    constructorobj[field_slot] = april.newInteger(method.slot)

                    local parameter_type_list = {}
                    local descriptor = descriptor_parser.parseMethodDescriptor(method.type)
                    for i, param in descriptor.parameter_list do
                        parameter_type_list[i] = april.getClassFromDescriptor(param).classobject
                    end

                    local parameter_type_array = april.newArray("Ljava/lang/Class;", #parameter_type_list)
                    for i, v in parameter_type_list do
                        parameter_type_array.list[i] = april.newReference(v)
                    end

                    constructorobj[field_parameterTypes] = april.newReference(parameter_type_array)
                    constructorobj[field_exceptionTypes] = april.newReference(april.newArray("Ljava/lang/Class;", 0))

                    constructorobj[field_modifiers] = april.newInteger(method.access_flags)

                    count += 1
                    list[count] = constructorobj
                end

                local array = april.newArray("Ljava/lang/reflect/Constructor;", count)
                for i = 1, count do
                    array.list[i] = april.newReference(list[i])
                end

                return april.newReference(array)
            end
            dict["isArray-()Z"] = function(this)
                local class = this.value.classclass

                return april.newBoolean(class.isarray)
            end
            dict["getComponentType-()Ljava/lang/Class;"] = function(this)
                local class = this.value.classclass

                return april.newReference(april.getClassFromDescriptor(class.valuedescriptor).classobject)
            end
            dict["isPrimitive-()Z"] = function(this)
                local class = this.value.classclass

                return april.newBoolean(class.isprimitive)
            end
            dict["isInterface-()Z"] = function(this)
                local class = this.value.classclass

                return april.newBoolean(class.isinterface)
            end
            dict["isAssignableFrom-(Ljava/lang/Class;)Z"] = function(this, other)
                local class = this.value.classclass
                local otherclass = other.value.classclass

                assert(not (class.isarray and otherclass.isarray), "TODO: array checks")
                return april.newBoolean(class == otherclass or
                    april.hasSuperClass(otherclass, class) or
                    april.implementsInterface(otherclass, class))
            end
            dict["isInstance-(Ljava/lang/Object;)Z"] = function(this, other)
                local class = this.value.classclass
                return april.newBoolean(april.instanceofByClassAndTargetClass(class, other.value.class))
            end
            dict["getModifiers-()I"] = function(classobj)
                -- TODO: something about InnerClass attribute? idk read sun/reflect/Reflection.java
                local class = classobj.value.classclass

                return april.newInteger(class.access_flags)
            end
            dict["getSuperclass-()Ljava/lang/Class;"] = function(classobj)
                local superclassname = classobj.superclass
                if not superclassname then
                    return april.newReference()
                end

                return april.newReference(april.getClass(superclassname).classobject)
            end
        end
    })
    natives.registerMethods("java/lang/System", {
        ["registerNatives-()V"] = function()
            local dict = natives.class_to_dict_map["java/lang/System"]

            dict["currentTimeMillis-()J"] = function()
                return april.newLong(os.clock() * 1000)
            end
            dict["nanoTime-()J"] = function()
                return april.newLong(os.clock() * 1000000000)
            end

            dict["identityHashCode-(Ljava/lang/Object;)I"] = function(o)
                return april.newInteger(o.value.hashcode)
            end

            dict["arraycopy-(Ljava/lang/Object;ILjava/lang/Object;II)V"] = function(src, srcpos, dest, destpos, length)
                -- TODO: exceptions

                for i = 1, length.value do
                    dest.value.list[destpos.value + i] = april.clone(src.value.list[srcpos.value + i])
                end
            end
            dict["initProperties-(Ljava/util/Properties;)Ljava/util/Properties;"] = function(properties)
                local class = properties.value.class

                local putmethod = april.lookupMethod(class.name, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;")

                local function putProp(key: string, value: string)
                    putmethod.func(properties, april.newString(key), april.newString(value))
                end

                local MAJOR_VERSION = 1
                local MINOR_VERSION = 8
                local RELEASE = "1.8.0_452"
                local SUPPORTED_VERSION = 52
                local SUPPORTED_MINOR_VERSION = 0

                -- TODO: os.arch, user.language, etc should be supplied to the vm or something else
                -- ^ includes all linux specific things

                putProp("java.specification.version", ("%d.%d"):format(MAJOR_VERSION, MINOR_VERSION))
                putProp("java.specification.name", "Java Platform API Specification")
                putProp("java.specification.vendor", "april")

                putProp("java.version", RELEASE)
                putProp("java.vendor", "techhog")
                putProp("java.vendor.url", "https://github.com/TechHog8984/april")
                putProp("java.vendor.url.bug", "https://github.com/TechHog8984/april/issues")

                putProp("java.class.version", ("%d.%d"):format(SUPPORTED_VERSION, SUPPORTED_MINOR_VERSION))

                putProp("java.home", (assert(natives.java_home, "you must call natives.setJavaHome")))

                putProp("java.awt.toolkit", "sun.awt.X11.XToolkit") -- TODO: better value?

                putProp("os.name", "Linux")
                putProp("os.version", "6.12.63_1")
                putProp("os.arch", "amd64")

                putProp("file.separator", "/")
                putProp("path.separator", ":")
                putProp("line.separator", "\n")

                putProp("java.library.path", (assert(natives.java_library_path, "you must call natives.setJavaLibraryPath")))

                putProp("user.language", "en")
                putProp("user.country", "US")

                putProp("file.encoding", "UTF-8")
                putProp("sun.jnu.encoding", "UTF-8")
                putProp("file.encoding.pkg", "sun.io")

                putProp("sun.io.unicode.encoding", "UnicodeLittle")
                putProp("sun.cpu.isalist", "")
                putProp("sun.cpu.endian", "little")

                putProp("java.awt.printerjob", "sun.print.PSPrinterJob")

                putProp("sun.arch.data.model", "64")

                putProp("sun.os.patch.level", "unknown")

                putProp("sun.awt.graphicsenv", "sun.awt.X11GraphicsEnvironment")

                putProp("java.io.tmpdir", "/tmp")

                putProp("user.name", "techhog")
                putProp("user.home", "/home/techhog")
                putProp("user.timezone", "")
                -- NOTE: this is actually the working directory
                putProp("user.dir", "/home/techhog")

                return properties
            end
            dict["setIn0-(Ljava/io/InputStream;)V"] = function(newin)
                local class = april.getClass("java/lang/System")
                local field = class.fields["in"]

                local obj = class.staticinstance

                obj[field] = newin
            end
            dict["setOut0-(Ljava/io/PrintStream;)V"] = function(newout)
                local class = april.getClass("java/lang/System")
                local field = class.fields["out"]

                local obj = class.staticinstance

                obj[field] = newout
            end
            dict["setErr0-(Ljava/io/PrintStream;)V"] = function(newerr)
                local class = april.getClass("java/lang/System")
                local field = class.fields["err"]

                local obj = class.staticinstance

                obj[field] = newerr
            end

            dict["mapLibraryName-(Ljava/lang/String;)Ljava/lang/String;"] = function(from)
                return april.newString("lib" .. april.javaStringToLuaString(from.value) .. ".so")
            end
        end,
    })
    natives.registerMethods("java/lang/Runtime", {
        ["availableProcessors-()I"] = function(_)
            -- TODO: (probably in runtimelayer) call nproc or whatever on mac/windows
            return april.newInteger(16)
        end
    });

    natives.registerMethods("java/io/UnixFileSystem", {
        ["initIDs-()V"] = function()

        end,
        ["canonicalize0-(Ljava/lang/String;)Ljava/lang/String;"] = function(_, relpath)
            local result = natives.runProcess("realpath", { april.javaStringToLuaString(relpath.value) })

            assert(result.ok)

            return april.newString((result.stdout:match("(.+)\n$")))
        end,
        ["getBooleanAttributes0-(Ljava/io/File;)I"] = function(_, file)
            local class = april.getClass("java/io/File");
            local path = april.javaStringToLuaString(file.value[class.fields.path].value)

            local exists = natives.fileExists(path)
            if not exists then
                return april.newInteger(0)
            end

            local is_file = natives.isFile(path)
            local is_dir = natives.isDir(path)

            local attributes = 0x01

            if is_file then
                attributes = bit32.bor(attributes, 0x02)
            elseif is_dir then
                attributes = bit32.bor(attributes, 0x04)
            end

            return april.newInteger(attributes)
        end
    })

    natives.registerMethods("java/lang/ClassLoader", {
        ["registerNatives-()V"] = function()
            local dict = natives.class_to_dict_map["java/lang/ClassLoader"]

            dict["findBuiltinLib-(Ljava/lang/String;)Ljava/lang/String;"] = function(name)
                -- print(april.javaStringToLuaString(name.value))

                -- return april.newString("hey bro");
                return name
            end
            dict["findLoadedClass0-(Ljava/lang/String;)Ljava/lang/Class;"] = function(_, name)
                local newname = april.javaStringToLuaString(name.value):gsub("%.", "/")

                local class = april.findClass(newname)
                return april.newReference(if class then class.classobject else nil)
            end
        end
    })
    natives.registerMethods("java/lang/ClassLoader$NativeLibrary", {
        ["load-(Ljava/lang/String;Z)V"] = function(this, name, isbuiltin)
            local class = this.value.class

            this.value[class.fields.loaded] = april.newBoolean(true)
        end
    })

    natives.registerMethods("java/lang/Float", {
        ["floatToRawIntBits-(F)I"] = function(float)
            return april.newInteger(float.bytes)
        end
    })
    natives.registerMethods("java/lang/Double", {
        ["doubleToRawLongBits-(D)J"] = function(double)
            -- TODO: double check NaN differences between this and doubleToLongBits

            return april.newLongFromBits(double.high, double.low)
        end,
        ["longBitsToDouble-(J)D"] = function(long)
            return april.newDoubleFromBits(long.high, long.low)
        end
    })

    local threadinfo = {}

    local mainthread
    local mainthreadgroup

    natives.registerMethods("java/lang/Thread", {
        ["registerNatives-()V"] = function()
            local dict = natives.class_to_dict_map["java/lang/Thread"]

            dict["currentThread-()Ljava/lang/Thread;"] = function()
                -- TODO: actually return the real thread
                return april.newReference(mainthread)
            end
            dict["setPriority0-(I)V"] = function(this)

            end
            dict["isAlive-()Z"] = function(this)
                return april.newBoolean(threadinfo[this.value].alive)
            end
            dict["start0-()V"] = function(this)
                -- print("STARTING A THREAD", debug.traceback())
                threadinfo[this.value].alive = true

                local run = april.lookupMethod(this.value.class.name, "run", "()V")

                coroutine.wrap(function()
                    run.func(this)
                end)()
                -- error"aight just spawned it"
            end
        end
    })

    local stack_access_control_context

    natives.registerMethods("sun/misc/VM", {
        ["initialize-()V"] = function()
            -- TODO: should this thread code go here? idk

            mainthread = april.newObjectFromClassName("java/lang/Thread")
            -- TODO: is this thread group handling good enough?
            mainthreadgroup = april.newObjectFromClassName("java/lang/ThreadGroup")

            mainthread[mainthread.class.fields.group] = april.newReference(mainthreadgroup)
            mainthread[mainthread.class.fields.priority] = april.newInteger(1)

            april.hooks_newObject[#april.hooks_newObject] = function(object)
                if not april.instanceofByClassAndTargetName(object.class, "java/lang/Thread") then
                    return
                end

                threadinfo[object] = {
                    alive = false
                }
            end

            -- TODO: no idea what to do for this stuff
            do
                stack_access_control_context = april.newObjectFromClassName("java/security/AccessControlContext")
                local method = stack_access_control_context.class.methods["<init>-([Ljava/security/ProtectionDomain;Z)V"]

                method.func(
                    april.newReference(stack_access_control_context),
                    april.newReference(april.newArray("Ljava/security/ProtectionDomain;", 0)),
                    april.newBoolean(true)
                )
            end
        end
    })
    natives.registerMethods("sun/misc/Unsafe", {
        ["registerNatives-()V"] = function()
            local dict = natives.class_to_dict_map["sun/misc/Unsafe"]

            dict["ensureClassInitialized-(Ljava/lang/Class;)V"] = function(_, classobject)
                april.loadClass(classobject.value.classclass)
            end

            local array_layout_map = {
                ["[B"] = {
                    base_offset = 0,
                    index_scale = 1
                },
                ["[Z"] = {
                    base_offset = 0,
                    index_scale = 1
                },
                ["[S"] = {
                    base_offset = 0,
                    index_scale = 2
                },
                ["[C"] = {
                    base_offset = 0,
                    index_scale = 2
                },
                ["[I"] = {
                    base_offset = 0,
                    index_scale = 4
                },
                ["[F"] = {
                    base_offset = 0,
                    index_scale = 4
                },
                ["[J"] = {
                    base_offset = 0,
                    index_scale = 8
                },
                ["[D"] = {
                    base_offset = 0,
                    index_scale = 8
                },
                ["[Ljava/lang/Object;"] = {
                    base_offset = 0,
                    index_scale = 4
                },
            }

            local function getArrayLayout(valuedescriptor: descriptor_parser.FieldDescriptor)
                local layout
                local valuetype = valuedescriptor.value
                if valuetype.tag == "Base" then
                    layout = array_layout_map['[' .. valuedescriptor.raw]
                else
                    assert(valuetype.tag ~= "Array")
                    layout = array_layout_map["[Ljava/lang/Object;"]
                end

                return assert(layout)
            end

            dict["arrayBaseOffset-(Ljava/lang/Class;)I"] = function(_, classobject)
                local class = classobject.value.classclass
                local layout = getArrayLayout(class.valuedescriptor)

                return april.newInteger(layout.base_offset)
            end
            dict["arrayIndexScale-(Ljava/lang/Class;)I"] = function(_, classobject)
                local class = classobject.value.classclass
                local layout = getArrayLayout(class.valuedescriptor)

                return april.newInteger(layout.index_scale)
            end
            dict["addressSize-()I"] = function()
                return april.newInteger(8)
            end
            -- TODO: checks for static field?
            dict["objectFieldOffset-(Ljava/lang/reflect/Field;)J"] = function(_, fieldobj)
                local slot = fieldobj.value[fieldobj.value.class.fields["slot"]]
                return april.newLong(slot.value)
            end
            dict["staticFieldOffset-(Ljava/lang/reflect/Field;)J"] = function(_, fieldobj)
                local slot = fieldobj.value[fieldobj.value.class.fields["slot"]]
                return april.newLong(slot.value)
            end
            dict["staticFieldBase-(Ljava/lang/reflect/Field;)Ljava/lang/Object;"] = function(_, fieldobj)
                -- TODO: idk if this is right
                local fieldclass = april.getClass("java/lang/reflect/Field")
                local class = fieldobj.value[fieldclass.fields.clazz].value.classclass

                return april.newReference(class.staticinstance)
            end

            local function getObjectValue(o, offset)
                local class = o.value.class
                if class.isarray then
                    -- we could probably hardcode 4 instead of getting index_scale, but idk
                    local layout = getArrayLayout(class.valuedescriptor)
                    local index = (offset / layout.index_scale) + 1
                    return o.value.list[index], true, index
                else
                    local target_field
                    for k in o.value do
                        if type(k) ~= "table" then
                            continue
                        end

                        if k.slot == offset then
                            target_field = k
                            break
                        end
                    end
                    assert(target_field)

                    return o.value[target_field], false, target_field
                end
            end
            -- FIXME: for all these slot loop to get field whatever stuff, i think i should check if the object is an array (given memoryCopy happens on arrays and i didn't realize and for arrays you do NOT do field slot lookup you just use the actual list values)
            dict["compareAndSwapObject-(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z"] = function(_, o, offset, expected, x)
                local value, isarray, indexorfield = getObjectValue(o, offset.value)

                -- TODO: make this atomic?

                if value.value == expected.value then
                    if isarray then
                        o.value.list[indexorfield] = x
                    else
                        o.value[indexorfield] = x
                    end
                    return april.newBoolean(true)
                end

                return april.newBoolean(false)
            end
            dict["compareAndSwapInt-(Ljava/lang/Object;JII)Z"] = function(_, o, offset, expected, x)
                assert(not o.value.class.isarray)
                local target_field
                for k in o.value do
                    if type(k) ~= "table" then
                        continue
                    end

                    if k.slot == offset.value then
                        target_field = k
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]
                assert(value.tag == "integer")

                if value.value == expected.value then
                    o.value[target_field] = x
                    return april.newBoolean(true)
                end

                return april.newBoolean(false)
            end
            dict["compareAndSwapLong-(Ljava/lang/Object;JJJ)Z"] = function(_, o, offset, expected, x)
                assert(not o.value.class.isarray)
                local target_field
                for k in o.value do
                    if type(k) ~= "table" then
                        continue
                    end

                    if k.slot == offset.value then
                        target_field = k
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]
                assert(value.tag == "long")

                if value.value == expected.value then
                    o.value[target_field] = expected
                    return april.newBoolean(true)
                end

                return april.newBoolean(false)
            end
            dict["getIntVolatile-(Ljava/lang/Object;J)I"] = function(_, o, offset)
                assert(not o.value.class.isarray)
                local target_field
                for k in o.value do
                    if type(k) ~= "table" then
                        continue
                    end

                    if k.slot == offset.value then
                        target_field = k
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]
                assert(value.tag == "integer")

                return value
            end
            dict["getLongVolatile-(Ljava/lang/Object;J)J"] = function(_, o, offset)
                assert(not o.value.class.isarray)
                local target_field
                for k in o.value do
                    if type(k) ~= "table" then
                        continue
                    end

                    if k.slot == offset.value then
                        target_field = k
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]
                assert(value.tag == "long")

                return value
            end
            dict["getObjectVolatile-(Ljava/lang/Object;J)Ljava/lang/Object;"] = function(_, o, offset)
                local value = getObjectValue(o, offset.value)

                -- TODO: make this atomic?

                assert(value.tag == "reference")
                return value
            end
            dict["putObjectVolatile-(Ljava/lang/Object;JLjava/lang/Object;)V"] = function(_, o, offset, value)
                local _, isarray, indexorfield = getObjectValue(o, offset.value)

                if isarray then
                    o.value.list[indexorfield] = value
                else
                    o.value[indexorfield] = value
                end

                -- TODO: make this atomic?
            end

            local memory_pool_counter = 1
            local memory_pool: { Memory } = {}

            type Memory = { tag: "raw", buf: buffer, size: number } | { tag: "reference", parent: Memory, offset: number }

            local function memoryReadGeneric(func, memory: Memory, start: number)
                if memory.tag == "raw" then
                    return func(memory.buf, start)
                elseif memory.tag == "reference" then
                    return memoryReadGeneric(func, memory.parent, memory.offset + start)
                end

                print(memory.tag)
                error("invalid memory tag")
            end
            local function memoryWriteGeneric(func, memory: Memory, start: number, value: number)
                if memory.tag == "raw" then
                    func(memory.buf, start, value)
                elseif memory.tag == "reference" then
                    memoryWriteGeneric(func, memory.parent, memory.offset + start, value)
                else
                    print(memory.tag)
                    error("invalid memory tag")
                end
            end

            local function memoryReadI8(memory: Memory, start: number)
                return memoryReadGeneric(buffer.readi8, memory, start)
            end

            local function memoryWriteU8(memory: Memory, start: number, value: number)
                memoryWriteGeneric(buffer.writeu8, memory, start, value)
            end
            local function memoryWriteI8(memory: Memory, start: number, value: number)
                memoryWriteGeneric(buffer.writei8, memory, start, value)
            end
            local function memoryWriteU32(memory: Memory, start: number, value: number)
                memoryWriteGeneric(buffer.writeu32, memory, start, value)
            end

            dict["allocateMemory-(J)J"] = function(_, size)
                local sizev = size.value
                local id = memory_pool_counter
                local memory: Memory = { tag = "raw", buf = buffer.create(sizev), size = sizev }
                memory_pool[id] = memory

                memory_pool_counter += sizev
                for i = id + 1, memory_pool_counter - 1 do
                    memory_pool[i] = { tag = "reference", parent = memory, offset = i - id }
                end

                return april.newLong(id)
            end
            dict["freeMemory-(J)V"] = function(_, pointer)
                local id = pointer.value
                local memory = memory_pool[id]
                assert(memory.tag == "raw")
                for i = id, id + memory.size - 1 do
                    memory_pool[i] = nil
                end
            end
            dict["setMemory-(Ljava/lang/Object;JJB)V"] = function(_, o, offset, bytes, value)
                if o.value then
                    error("TODO: setMemory on an object")
                end

                local memory = assert(memory_pool[offset.value])
                local count = bytes.value
                local val = value.value

                for i = 1, count do
                    memoryWriteU8(memory, i - 1, val)
                end
            end
            dict["copyMemory-(Ljava/lang/Object;JLjava/lang/Object;JJ)V"] = function(_, src_base, src_offset, dest_base, dest_offset, bytes)
                local src_isnil = not src_base.value
                local dest_isnil = not dest_base.value

                local count = bytes.value

                if src_isnil then
                    -- NOTE: commented because untested
                    -- if dest_isnil then
                    --     local src_memory = assert(memory_pool[src_offset.value])
                    --     local dest_memory = assert(memory_pool[dest_offset.value])

                    --     for i = 1, count do
                    --         memoryWriteU8(dest_memory, i - 1, memoryReadU8(src_memory, i - 1))
                    --     end

                    --     return
                    -- else
                    --     local src_memory = assert(memory_pool[src_offset.value])

                    --     local class = dest_base.value.class
                    --     local slot = dest_offset.value
                    --     local target_field
                    --     -- FIXME: update the slot code cuz i changed things

                    --     for _, field in class.fields do
                    --         if field.slot == slot then
                    --             target_field = field
                    --             break
                    --         end
                    --     end
                    --     assert(target_field)

                    --     local fieldtype = descriptor_parser.parseFieldDescriptor(target_field.type).value

                    --     if fieldtype.tag == "Base" then
                    --         if fieldtype.base_type == "float" then
                    --             assert(count == 4)
                    --             dest_base.value[target_field] = april.newFloatFromBytes(buffer.readu32(src_memory, 0))
                    --
                    --             return
                    --         end
                    --     end

                    --     for k,v in fieldtype do print(k,v) end
                    --     error("TODO: unimplemented field type in copyMemory")
                    -- end
                else
                    if dest_isnil then
                        local dest_memory = assert(memory_pool[dest_offset.value])

                        if src_base.value.class.isarray then
                            local valuetype: descriptor_parser.FieldDescriptorValue = src_base.value.class.valuedescriptor.value
                            if valuetype.tag == "Base" then
                                if valuetype.base_type == "float" then
                                    assert(count % 4 == 0)
                                    assert(src_offset.value == 0)
                                    for i = 1, count, 4 do
                                        memoryWriteU32(dest_memory, i - 1, src_base.value.list[(i - 1) / 4 + 1].bytes)
                                    end

                                    return
                                end
                            end

                            for k,v in (valuetype::any) do print(k,v) end
                            error("TODO: unimplemented array value type in copyMemory")
                        else
                            -- NOTE: commented because untested
                            -- local slot = src_offset.value
                            -- local target_field
                            -- for k in src_base.value do
                            --     if type(k) ~= "table" then
                            --         continue
                            --     end

                            --     if k.slot == slot.value then
                            --         target_field = k
                            --         break
                            --     end
                            -- end
                            -- assert(target_field)

                            -- local fieldtype = descriptor_parser.parseFieldDescriptor(target_field.type).value

                            -- if fieldtype.tag == "Base" then
                            --     if fieldtype.base_type == "float" then
                            --         assert(count == 4)
                            --         memoryWriteU32(dest_memory, 0, src_base.value[target_field].bytes)
                            --     end
                            -- end

                            -- for k,v in fieldtype do print(k,v) end
                            -- error("TODO: unimplemented field type in copyMemory")
                        end
                    end
                end

                print(type(src_base.value), type(dest_base.value))
                error("TODO: unimplemented copyMemory configuration")
            end
            dict["putLong-(JJ)V"] = function(_, address, value)
                local memory = assert(memory_pool[address.value])
                memoryWriteU32(memory, 0, value.low)
                memoryWriteU32(memory, 4, value.high)
            end
            dict["putByte-(JB)V"] = function(_, address, value)
                local memory = assert(memory_pool[address.value])
                memoryWriteI8(memory, 0, value.value)
            end
            dict["getByte-(J)B"] = function(_, address)
                local memory = assert(memory_pool[address.value])
                return april.newByte(memoryReadI8(memory, 0))
            end

            dict["pageSize-()I"] = function(_)
                -- TODO: actually get _SC_PAGESIZE or wtv windows/mac does for it
                return april.newInteger(4096)
            end
        end
    })

    local byte_newline = string.byte('\n')
    local stdout_buffer = table.create(256)
    local function writeStdoutBytes(data, start, count)
        if count < 1 then return end

        local index = 0
        local endd = start + count
        for i = start, endd - 1 do
            local char = data[i + 1].value
            -- TODO: carriage return
            if char == byte_newline then
                index = i + 1
                break
            end
            char = april.wrapUnsigned(char, 8)
            stdout_buffer[#stdout_buffer + 1] = string.pack('B', char)
        end

        if index == 0 then
            return
        end

        print(table.concat(stdout_buffer))
        table.clear(stdout_buffer)

        writeStdoutBytes(data, index, count - index)
    end

    type FileReference = {
        path: string,
        fd: number,
        offset: number
    }
    local fd_map: { FileReference } = {}
    local fd_counter = 10
    natives.registerMethods("java/io/FileInputStream", {
        ["initIDs-()V"] = function()

        end,
        ["open0-(Ljava/lang/String;)V"] = function(this, path)
            local fd = fd_counter
            fd_counter += 1
            fd_map[fd] = {
                path = april.javaStringToLuaString(path.value),
                fd = fd,
                offset = 0
            }

            local file_input_stream_class = april.getClass("java/io/FileInputStream")
            local file_descriptor_class = april.getClass("java/io/FileDescriptor")

            local fdobj = april.newObject(file_descriptor_class)
            local fdobjref = april.newReference(fdobj)

            local init = file_descriptor_class.methods["<init>-(I)V"]
            local attach = file_descriptor_class.methods["attach-(Ljava/io/Closeable;)V"]

            init.func(fdobjref, april.newInteger(fd))
            this.value[file_input_stream_class.fields.fd] = fdobjref
            attach.func(fdobjref, this)
        end,
        ["close0-()V"] = function(this)
            local file_input_stream_class = april.getClass("java/io/FileInputStream")
            local file_descriptor_class = april.getClass("java/io/FileDescriptor")
            local fdobj = this.value[file_input_stream_class.fields.fd].value
            local fd = fdobj[file_descriptor_class.fields.fd].value

            fd_map[fd] = nil
        end,
        ["readBytes-([BII)I"] = function(this, b, boffset, len)
            local file_input_stream_class = april.getClass("java/io/FileInputStream")
            local file_descriptor_class = april.getClass("java/io/FileDescriptor")
            local fdobj = this.value[file_input_stream_class.fields.fd].value
            local fd = fdobj[file_descriptor_class.fields.fd].value

            local boffsetv = boffset.value
            local lengthv = len.value

            local file = fd_map[fd]
            if not file then
                -- TODO: ioexception
                error("attempt to read file that has been closed / not been opened")
            end
            local path = file.path
            local startoffset = file.offset

            local filecontents = ""
            -- TODO: throw exception
            pcall(function()
                filecontents = natives.readFile(path)
            end)
            local contentsize = #filecontents

            local read = 0
            for i = 1, lengthv do
                local pos = startoffset + i
                if pos > contentsize then
                    break
                end

                local char = filecontents:sub(pos, pos)
                b.value.list[boffsetv + i] = april.newByte(string.byte(char))

                read += 1
            end

            file.offset = startoffset + read

            -- local bytestr = {}
            -- for k,v in b.value.list do
            --     bytestr[k] = v.value
            -- end
            -- print("buffer after reading " .. read .. " bytes from " .. path .. ": " .. table.concat(bytestr, ','))

            return april.newInteger(if read == 0 then -1 else read)
        end,
        ["available0-()I"] = function(this)
            -- NOTE: this function is WRONG since we block on every single read :D

            local file_input_stream_class = april.getClass("java/io/FileInputStream")
            local file_descriptor_class = april.getClass("java/io/FileDescriptor")
            local fdobj = this.value[file_input_stream_class.fields.fd].value
            local fd = fdobj[file_descriptor_class.fields.fd].value

            local file = fd_map[fd]
            if not file then
                -- TODO: ioexception
                error("attempt to read file that has been closed / not been opened")
            end
            local path = file.path
            local offset = file.offset

            local filecontents = ""
            -- TODO: throw exception
            pcall(function()
                filecontents = natives.readFile(path)
            end)
            local contentsize = #filecontents

            local remaining = contentsize - offset
            if remaining > 0 then
                return april.newInteger(remaining)
            end

            return april.newInteger(0)
        end
    })
    natives.registerMethods("java/io/FileOutputStream", {
        ["initIDs-()V"] = function()

        end,
        ["writeBytes-([BIIZ)V"] = function(this, b, off, len, append)
            local class = april.getClass("java/io/FileOutputStream")
            local fd = this.value[class.fields.fd].value

            local filedescriptor_class = april.getClass("java/io/FileDescriptor")
            local fd_out = filedescriptor_class.staticinstance[filedescriptor_class.fields["out"]].value

            if fd == fd_out then
                if not b.value.list then error"larp" end
                writeStdoutBytes(b.value.list, off.value, len.value)

                return
            end

            error"unhandled file descriptor"
        end
    })
    natives.registerMethods("java/io/FileDescriptor", {
        ["initIDs-()V"] = function()

        end
    })

    natives.registerMethods("sun/reflect/Reflection", {
        ["getCallerClass-()Ljava/lang/Class;"] = function()
            -- TODO: ignore java.lang.reflect.Method.invoke frames
            local callstack = april.callstack

            local callstack_count = #callstack
            for i = callstack_count, 1, -1 do
                local obj = callstack[i]
                if i == callstack_count then
                    -- TODO: check caller sensitive
                    -- print("skipping cuz it's the first one", obj.method.name)
                    continue
                end
                if obj.method.name == "invoke" then
                    -- print("TODO: check if this is the right class", obj.class.name)
                    continue
                end
                -- print(obj.method.name)
                -- TODO: check lambda

                return april.newReference(obj.class.classobject)
            end

            error"nope"
        end,
        ["getClassAccessFlags-(Ljava/lang/Class;)I"] = function(classobj)
            local class = classobj.value.classclass

            return april.newInteger(class.access_flags)
        end
    })

    natives.registerMethods("java/security/AccessController", {
        ["doPrivileged-(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;"] = function(action)
            -- TODO: actually escalate privileges or whatever

            local method = april.lookupMethod(action.value.class.name, "run", "()Ljava/lang/Object;")
            assert(not method.isabstract)
            return method.func(action)
        end,
        ["doPrivileged-(Ljava/security/PrivilegedAction;)Ljava/lang/Object;"] = function(action)
            -- TODO: actually escalate privileges or whatever

            local method = april.lookupMethod(action.value.class.name, "run", "()Ljava/lang/Object;")
            assert(not method.isabstract)
            return method.func(action)
        end,
        ["doPrivileged-(Ljava/security/PrivilegedAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;"] = function(action, context)
            -- TODO: actually escalate privileges or whatever

            local method = april.lookupMethod(action.value.class.name, "run", "()Ljava/lang/Object;")
            assert(not method.isabstract)
            return method.func(action)
        end,
        ["getStackAccessControlContext-()Ljava/security/AccessControlContext;"] = function()
            return april.newReference(stack_access_control_context)
        end
    })

    natives.registerMethods("java/lang/String", {
        ["intern-()Ljava/lang/String;"] = function(stringobj)
            return april.string_intern_map[stringobj]
        end
    })

    natives.registerMethods("sun/reflect/NativeConstructorAccessorImpl", {
        ["newInstance0-(Ljava/lang/reflect/Constructor;[Ljava/lang/Object;)Ljava/lang/Object;"] = function(constructor, args)
            local constructorclass = april.getClass("java/lang/reflect/Constructor")
            local field_class = constructorclass.fields.clazz
            local field_slot = constructorclass.fields.slot

            local class = constructor.value[field_class].value.classclass
            local slot = constructor.value[field_slot].value

            local target_method
            for _, method in class.methods do
                if method.slot == slot then
                    target_method = method
                    break
                end
            end
            assert(target_method)

            local object = april.newObject(class)
            local objectref = april.newReference(object)
            if args.value then
                target_method.func(objectref, table.unpack(args.value.list, 1, args.value.size))
            else
                target_method.func(objectref)
            end

            return objectref
        end
    })

    natives.registerMethods("java/util/concurrent/atomic/AtomicLong", {
        ["VMSupportsCS8-()Z"] = function()
            return april.newBoolean(false)
        end
    })

    natives.registerMethods("sun/misc/Signal", {
        ["findSignal-(Ljava/lang/String;)I"] = function(name)
            local signal = april.javaStringToLuaString(name.value)

            if signal == "HUP" then
                return april.newInteger(1)
            elseif signal == "INT" then
                return april.newInteger(2)
            elseif signal == "TERM" then
                return april.newInteger(15)
            end

            error("fah " .. signal)
        end,
        ["handle0-(IJ)J"] = function(sig, handler)
            return april.newLong(0)
        end
    })

    natives.registerMethods("sun/misc/URLClassPath", {
        ["getLookupCacheURLs-(Ljava/lang/ClassLoader;)[Ljava/net/URL;"] = function(loader)
            -- TODO: we could probably do this pretty easily since we do actually keep a cache, im just not doing it cuz im lazy (also we have to take into account the ClassLoader...)
            return april.newReference()
        end
    })
end
function natives.registerMethods(classname, dict)
    natives.class_to_dict_map[classname] = dict
end

function natives.findNative(classname, combinedname)
    local dict = natives.class_to_dict_map[classname]
    if not dict then
        print(combinedname)
        error(("class '%s' has no native methods registered!"):format(classname))
    end

    local native = dict[combinedname]
    if not native then
        error(("class '%s' has no native method '%s' registered!"):format(classname, combinedname))
    end

    return native
end

return natives
