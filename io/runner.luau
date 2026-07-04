local april = require("@april/april")
local natives = require("@april/natives")
local input = require("@lune/process").args[1]
local libstdio = require("@lune/stdio")

natives.registerMethods("tech/hog/a", {
    ["nativeTest-()D"] = function(...)
        return april.newDouble(math.random())
    end,
    ["nativeTest2-()J"] = function(...)
        return april.newLong(math.random() * 10)
    end,
})

april.registerClass("java/lang/Object", {
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

april.registerClass("java/lang/String", {
    name = "java/lang/String",
    superclass = "java/lang/Object",
    hasloaded = false,
    isinterface = false,
    issuper = true,
    interfaces = {},
    fieldcache = {},
    methodcache = {},
    fields = {
    },
    methods = {
        ["<init>-([C)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(...)
                local self, arr = ...
                -- print("char array for string", arr)
                rawset(self.value, "arr", arr)
                rawset(self.value, "tostring", function(self)
                    local list = {}
                    for k,v in self.arr.value.list do
                        list[k] = v.value
                    end
                    return utf8.char(table.unpack(list))
                end)
            end
        }
    }
})

-- local function printstub(self, ...)
--     local strt = {}
--     for i = 1, select('#', ...) do
--         strt[i] = tostring((select(i, ...)))
--     end
--     libstdio.write(table.concat(strt, '\t'))
-- end

april.registerClass("java/io/PrintStream", {
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
            func = function(self, value)
                libstdio.write(value.value:tostring())
            end
        },

        ["println-(I)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value)
            end,
        },
        ["println-(C)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(utf8.char(value.value))
            end,
        },
        ["println-(F)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value)
            end,
        },
        ["println-(J)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value)
            end,
        },
        ["println-(D)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value)
            end,
        },
        ["println-(Z)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value)
            end,
        },
        ["println-(Ljava/lang/String;)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value:tostring())
            end
        },
        ["println-(Ljava/lang/Object;)V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(self, value)
                print(value.value.class.name .. '@' .. ("%x"):format(value.value.hashcode))
            end
        },
    }
})
april.registerClass("java/lang/System", {
    name = "java/lang/System",
    hasloaded = false,
    isinterface = false,
    issuper = true,
    interfaces = {},
    fieldcache = {},
    methodcache = {},
    fields = {
        ["out"] = { name = "out", type = "Ljava/io/PrintStream;" },
        ["err"] = { name = "err", type = "Ljava/io/PrintStream;" },
    },
    methods = {
        ["<clinit>-()V"] = {
            isstatic = false,
            isnative = false,
            isabstract = false,
            func = function(...)
                local printstream = april.getClass("java/io/PrintStream")

                do
                local field, class = april.lookupField("java/lang/System", "out", "Ljava/io/PrintStream;")
                local object = class.staticinstance

                object[field] = april.newReference(april.newObject(printstream))
                end
                do
                local field, class = april.lookupField("java/lang/System", "err", "Ljava/io/PrintStream;")
                local object = class.staticinstance

                object[field] = april.newReference(april.newObject(printstream))
                end
            end
        }
    }
})

local libfs = require("@lune/fs")

for _, entry in libfs.readDir("./out") do
    require("./out/" .. entry:match("(.+)%..+$"))
end

local class = april.loadClassByName(input)
local method = class.methods["main-([Ljava/lang/String;)V"]
coroutine.wrap(function()
    method.func( april.newReference(april.newArray("[C", 2)) )
end)()
