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

local filemap = {}

local function traverse(dir, root)
    root = root or dir
    local relative = dir:sub(#root + 2, -1)
    for _, entry in runtimelayer.listDir(dir) do
        local path = dir .. '/' .. entry
        if runtimelayer.isFile(path) then
            filemap[relative .. '/' .. entry:match("(.+)%..+$")] = path
        else
            traverse(path, root)
        end
    end
end
traverse("./skep")
traverse("./out")

-- print(filemap)

function april.fetchClass(classname)
    local path = filemap[classname]
    assert(path, "failed to find class from classpath: " .. classname)

    local cleanpath = path:match("(.+)%..+$")

    -- if runtimelayer.requireisfuckingbroken then
    --     return runtimelayer.requireworkaround(path, cleanpath)
    -- end

    return require(cleanpath)
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
