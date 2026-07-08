--!optimize 2

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
-- local lutestdfs
-- local luteluau
pcall(function()
    lutefs = require("@lute/fs")
    -- lutestdfs = require("@std/fs")
    -- luteluau = require("@lute/luau")
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
    return lutefs.exists(path) and lutefs.type(path) == "file"
end

return {
    getProcessArgs = getProcessArgs,
    listDir = listDir,
    -- readFile = readFile,
    isFile = isFile,
}
