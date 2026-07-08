local require = (require::any)

local is_lune = false
local luneprocess
local lunefs
pcall(function()
    luneprocess = require("@lune/process")
    lunefs = require("@lune/fs")
    is_lune = true
end)

local is_lute = false
local lutefs
local lutestdfs
local luteluau
pcall(function()
    lutefs = require("@lute/fs")
    lutestdfs = require("@std/fs")
    luteluau = require("@lute/luau")
    is_lute = true
end)

if not (is_lune or is_lute) then
    error("unsupported runtime")
end

local getProcessArgs = if luneprocess then function()
    local realargs = luneprocess.args
    local args = table.clone(realargs)
    table.insert(args, 1, "file.lua")
    return args
end else function(...)
    return table.pack(...)
end

local listDir = if lunefs then lunefs.readDir else function(path)
    local reallist = lutefs.listdir(path)
    local list = table.create(#reallist)

    for k,v in reallist do
        list[k] = v.name
    end

    return list
end
-- local readFile = if lunefs then lunefs.readFile else lutestdfs.readFileToString
local isFile = if lunefs then lunefs.isFile else function(path)
    return lutefs.type(path) == "file"
end

local requirecache = {}
local requireworkaround = if is_lute then function(path, name)
    local cached = requirecache[name]
    if cached then
        return cached
    end
    local content = lutestdfs.readFileToString(path)
    local bytecode = luteluau.compile(content)
    local chunk = luteluau.load(bytecode, name)

    local result = chunk()
    requirecache[name] = result

    return result
end else function() error("require works on your runtime... just use it") end

return {
    getProcessArgs = getProcessArgs,
    listDir = listDir,
    -- readFile = readFile,
    isFile = isFile,
    requireisfuckingbroken = is_lute,
    requireworkaround = requireworkaround
}
