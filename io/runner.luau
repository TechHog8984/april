--!optimize 2

local april = require("@april/april")
local natives = require("@april/natives")
local runtimelayer = require("./runtimelayer")
local input = runtimelayer.getProcessArgs(...)[2]

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

local classpath = {
    "./skep",
    "./out"
}

function april.fetchClass(classname)
    for _, dir in classpath do
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

local class = april.loadClassByName(input)
local method = class.methods["main-([Ljava/lang/String;)V"]
coroutine.wrap(function()
    method.func( april.newReference(april.newArray("[C", 2)) )
end)()
