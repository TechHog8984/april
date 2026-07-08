--!optimize 2

local april = require("@april/april")
local descriptor_parser = require("@april/descriptor")

local natives = {
    class_to_dict_map = {}
}

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
                return april.getPrimitiveClass(primitive_name_map[april.javaStringToLuaString(name.value)])
            end
            dict["getName0-()Ljava/lang/String;"] = function(classobj)
                local class = classobj.value.classclass
                return april.newString((class.name:gsub("/", ".")))
            end
            dict["forName0-(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;"] = function(classname, initialize, loader, caller)
                local name = april.javaStringToLuaString(classname.value):gsub("%.", "/")
                local class = april.getClass(name)

                if initialize then
                    april.loadClass(class)
                end

                return april.newReference(class.classobject)
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
                    april.hasSuperClass(class, otherclass) or
                    april.implementsInterface(class, otherclass))
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

                putProp("java.awt.toolkit", "sun.awt.X11.XToolkit") -- TODO: better value?

                putProp("os.name", "Linux")
                putProp("os.version", "6.12.63_1")
                putProp("os.arch", "amd64")

                putProp("file.separator", "/")
                putProp("path.separator", ":")
                putProp("line.separator", "\n")

                putProp("java.library.path", "/media/Documents/cpp/april/io")

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

    natives.registerMethods("java/io/UnixFileSystem", {
        ["initIDs-()V"] = function()

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
                    run.func(this.value)
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

            local array_layout_map = {
                ["[B"] = {
                    base_offset = 16,
                    index_scale = 1
                },
                ["[Z"] = {
                    base_offset = 16,
                    index_scale = 1
                },
                ["[S"] = {
                    base_offset = 16,
                    index_scale = 2
                },
                ["[C"] = {
                    base_offset = 16,
                    index_scale = 2
                },
                ["[I"] = {
                    base_offset = 16,
                    index_scale = 4
                },
                ["[F"] = {
                    base_offset = 16,
                    index_scale = 4
                },
                ["[J"] = {
                    base_offset = 16,
                    index_scale = 8
                },
                ["[D"] = {
                    base_offset = 16,
                    index_scale = 8
                },
                ["[Ljava/lang/Object;"] = {
                    base_offset = 16,
                    index_scale = 4
                },
            }

            dict["arrayBaseOffset-(Ljava/lang/Class;)I"] = function(_, classobject)
                local class = classobject.value.classclass
                local layout = assert(array_layout_map[class.name])

                return april.newInteger(layout.base_offset)
            end
            dict["arrayIndexScale-(Ljava/lang/Class;)I"] = function(_, classobject)
                local class = classobject.value.classclass
                local layout = assert(array_layout_map[class.name])

                return april.newInteger(layout.index_scale)
            end
            dict["addressSize-()I"] = function()
                return april.newInteger(8)
            end
            dict["objectFieldOffset-(Ljava/lang/reflect/Field;)J"] = function(_, fieldobj)
                local slot = fieldobj.value[fieldobj.value.class.fields["slot"]]
                return april.newLong(slot.value)
            end
            dict["compareAndSwapObject-(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z"] = function(_, o, offset, expected, x)
                local class = o.value.class
                local target_field
                for _, field in class.fields do
                    if field.slot == offset.value then
                        target_field = field
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]

                if value.value == expected.value then
                    o.value[target_field] = expected
                    return april.newBoolean(true)
                end

                return april.newBoolean(false)
            end
            dict["compareAndSwapInt-(Ljava/lang/Object;JII)Z"] = function(_, o, offset, expected, x)
                local class = o.value.class
                local target_field
                for _, field in class.fields do
                    if field.slot == offset.value then
                        target_field = field
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]
                assert(value.tag == "integer")

                if value.value == expected.value then
                    o.value[target_field] = expected
                    return april.newBoolean(true)
                end

                return april.newBoolean(false)
            end
            dict["compareAndSwapLong-(Ljava/lang/Object;JJJ)Z"] = function(_, o, offset, expected, x)
                local class = o.value.class
                local target_field
                for _, field in class.fields do
                    if field.slot == offset.value then
                        target_field = field
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
                local class = o.value.class
                local target_field
                for _, field in class.fields do
                    if field.slot == offset.value then
                        target_field = field
                        break
                    end
                end
                assert(target_field)

                -- TODO: make this atomic?

                local value = o.value[target_field]
                assert(value.tag == "integer")

                return value
            end

            local memory_pool_counter = 0
            local memory_pool = {}
            dict["allocateMemory-(J)J"] = function(_, size)
                memory_pool_counter += 1
                memory_pool[memory_pool_counter] = buffer.create(size.value)

                return april.newLong(memory_pool_counter)
            end
            dict["freeMemory-(J)V"] = function(_, pointer)
                memory_pool[pointer] = nil

                return april.newLong(memory_pool_counter)
            end
            dict["putLong-(JJ)V"] = function(_, address, value)
                local memory = assert(memory_pool[address.value])
                buffer.writei32(memory, 0, value.low)
                buffer.writei32(memory, 4, value.high)
            end
            dict["getByte-(J)B"] = function(_, address)
                local memory = assert(memory_pool[address.value])
                return april.newByte(buffer.readi8(memory, 0))
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
            -- print(start, count, i)
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

    natives.registerMethods("java/io/FileInputStream", {
        ["initIDs-()V"] = function()

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
            -- TODO: actually escelate privileges or whatever

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
