--!optimize 2

local april = require("@april/april")
local natives = require("@april/natives")
local runtimelayer = require("./runtimelayer")
local argv = runtimelayer.getProcessArgs(...)

local argc = #argv
if argc < 2 then
    runtimelayer.panic("Usage: " .. argv[1] .. " [options] MAINCLASS\n\noptions:\n  --class-path=COMMA_SEPARATED_DIRECTORY_LIST\n")
end

local main_class
local classpath_list = { }

do
    local main_class_i
    local i = 2
    while i <= argc do
        local arg = argv[i]
        local classpath = arg:match("^%-%-class%-path=(.+)")
        if classpath then
            for a in classpath:gmatch("([^,]+),?") do
                classpath_list[#classpath_list + 1] = a
            end

            i += 1
            continue
        end

        if main_class_i then
            runtimelayer.panic("Invalid argument " .. arg .. ". Run with no arguments for usage\n")
        end

        main_class_i = i

        i += 1
    end

    if not main_class_i then
        runtimelayer.panic("You must provide main class. Run with no arguments for usage\n")
    end
    main_class = argv[main_class_i]
end

natives.runProcess = runtimelayer.runProcess
natives.isFile = runtimelayer.isFile
natives.isDir = runtimelayer.isDir
natives.fileExists = runtimelayer.fileExists
natives.readFile = runtimelayer.readFile
natives.setJavaHome((assert(runtimelayer.env.JAVA_HOME, "no JAVA_HOME environment variable")))
natives.setJavaLibraryPath("/media/Documents/cpp/april/io")

natives.registerDefault()
natives.registerMethods("tech/hog/a", {
    ["nativeTest-()D"] = function(...)
        return april.newDouble(math.random())
    end,
    ["nativeTest2-()J"] = function(...)
        return april.newLong(math.random() * 10)
    end,
})


local classmap = {}
april.registerClassMap(classmap)

function april.fetchClass(classname)
    for _, dir in classpath_list do
        local path = dir .. '/' .. classname-- .. ".luau"
        if runtimelayer.isFile(path .. ".luau") then
            return require(path)
        end
    end
    error("failed to find class from classpath: " .. classname)
end

-- TODO: thread initialization first
do
    local class = april.loadClassByName("java/lang/System")
    local method = class.methods["initializeSystemClass-()V"]
    method.func()
end

local class = april.loadClassByName(main_class)
local method = class.methods["main-([Ljava/lang/String;)V"]
coroutine.wrap(function()
    method.func( april.newReference(april.newArray("[C", 2)) )
end)()
