local classloader = require("../src/classloader")
local descriptor_parser = require("../src/descriptor")
local input = require("@lune/process").args[1]

classloader.registerClass("java/io/PrintStream", {
    name = "java/io/PrintStream",
    hasloaded = false,
    fieldcache = {},
    methodcache = {},
    fields = {
    },
    methods = {
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

local code = require("@lune/fs").readFile(input)
local chunk = loadstring(code)
getfenv(chunk).classloader = classloader
getfenv(chunk).descriptor_parser = descriptor_parser

local class = chunk()
classloader.loadClass(class.name)
local method = class.methods["main-([Ljava/lang/String;)V"]
method()
