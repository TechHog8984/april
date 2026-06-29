local classloader = require("../src/classloader")
local descriptor_parser = require("../src/descriptor")
local input = require("@lune/process").args[1]

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
        ["<init>-()V"] = function(...) -- this is accurate btw
        end
    }
})
local libstdio = require("@lune/stdio")
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
        ["print-(Ljava/lang/String;)V"] = printstub,

        ["println-(I)V"] = function(self, ...)
            print(...)
        end,
        ["println-(F)V"] = function(self, ...)
            print(...)
        end,
        ["println-(J)V"] = function(self, ...)
            print(...)
        end,
        ["println-(D)V"] = function(self, ...)
            print(...)
        end,
        ["println-(Ljava/lang/String;)V"] = function(self, ...)
            print(...)
        end
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
        ["<clinit>-()V"] = function(...)
            local field, class = classloader.lookupField("java/lang/System", "out", "Ljava/io/PrintStream;")
            local object = class.staticinstance

            object[field] = { class = class }
        end
    }
})

local libfs = require("@lune/fs")

local function runFile(filepath)
    local code = libfs.readFile(filepath)
    local chunk = loadstring(code)
    getfenv(chunk).classloader = classloader
    getfenv(chunk).descriptor_parser = descriptor_parser

    return chunk()
end

for _, entry in libfs.readDir("./out") do
    runFile("./out/" .. entry)
end

local class = classloader.loadClassByName(input)
local method = class.methods["main-([Ljava/lang/String;)V"]
method()
