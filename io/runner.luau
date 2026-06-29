local classloader = require("../src/classloader")
local descriptor_parser = require("../src/descriptor")
local natives = require("../src/natives")
local input = require("@lune/process").args[1]
local libstdio = require("@lune/stdio")

natives.registerMethods("tech/hog/a", {
    ["nativeTest-()D"] = function(...)
        return math.random()
    end
})

classloader.registerClass("java/lang/Object", {
    name = "java/lang/Object",
    hasloaded = false,
    isinterface = false,
    issuper = true,
    interfaces = {},
    fieldcache = {},
    methodcache = {},
    fields = {
    },
    methods = {
        ["<init>-()V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(...) -- this is accurate btw
            end
        }
    }
})

local function printstub(self, ...)
    local strt = {}
    for i = 1, select('#', ...) do
        strt[i] = tostring((select(i, ...)))
    end
    libstdio.write(table.concat(strt, '\t'))
end

classloader.registerClass("java/io/PrintStream", {
    name = "java/io/PrintStream",
    hasloaded = false,
    isinterface = false,
    issuper = true,
    interfaces = {},
    fieldcache = {},
    methodcache = {},
    fields = {
    },
    methods = {
        ["print-(Ljava/lang/String;)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = printstub
        },

        ["println-(I)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, ...)
                print(...)
            end,
        },
        ["println-(F)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, ...)
                print(...)
            end,
        },
        ["println-(J)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, ...)
                print(...)
            end,
        },
        ["println-(D)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, ...)
                print(...)
            end,
        },
        ["println-(Ljava/lang/String;)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, ...)
                print(...)
            end
        },
    }
})
classloader.registerClass("java/lang/System", {
    name = "java/lang/System",
    hasloaded = false,
    fieldcache = {},
    methodcache = {},
    fields = {
        ["out"] = { name = "out", type = "Ljava/io/PrintStream;" }
    },
    methods = {
        ["<clinit>-()V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(...)
                local field, class = classloader.lookupField("java/lang/System", "out", "Ljava/io/PrintStream;")
                local object = class.staticinstance

                object[field] = { class = class }
            end
        }
    }
})

local libfs = require("@lune/fs")

local function runFile(filepath)
    local code = libfs.readFile(filepath)
    local chunk = loadstring(code)
    getfenv(chunk).classloader = classloader
    getfenv(chunk).descriptor_parser = descriptor_parser
    getfenv(chunk).natives = natives

    return chunk()
end

for _, entry in libfs.readDir("./out") do
    runFile("./out/" .. entry)
end

local class = classloader.loadClassByName(input)
local method = class.methods["main-([Ljava/lang/String;)V"]
method.func()
