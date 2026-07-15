--!optimize 2

local require = (require::any)

local is_lune = false
local luneprocess
local lunefs
local lunestdio
pcall(function()
    luneprocess = require("@lune/process")
    lunefs = require("@lune/fs")
    lunestdio = require("@lune/stdio")
    is_lune = true
end)

local is_lute = false
local lutefs
local luteprocess
local luteio
local lutestdfs
-- local luteluau
pcall(function()
    lutefs = require("@lute/fs")
    luteprocess = require("@lute/process")
    luteio = require("@lute/io")
    lutestdfs = require("@std/fs")
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
local isFile = if lunefs then lunefs.isFile else function(path)
    return lutefs.exists(path) and lutefs.type(path) == "file"
end
local isDir = if lunefs then lunefs.isDir else function(path)
    return lutefs.exists(path) and lutefs.type(path) == "dir"
end
local fileExists = if lunefs then function(path)
    return isFile(path) or isDir(path)
end else lutefs.exists
local readFile = if lunefs then lunefs.readFile else lutestdfs.readFileToString

local env = (luneprocess or luteprocess).env

-- TODO: when lute has stderr, use it
local stderrWrite = if lunestdio then lunestdio.ewrite else luteio.write
local processExit = (luneprocess or luteprocess).exit

local function panic(message)
    stderrWrite(message)
    processExit(1)
end

local runProcess = if luneprocess then luneprocess.exec else function(exe, args)
    local argc = #args
    local realargs = table.create(argc + 1)
    realargs[1] = exe
    for i = 1, argc do
        realargs[i + 1] = args[i]
    end

    return luteprocess.run(realargs)
end

return {
    getProcessArgs = getProcessArgs,
    listDir = listDir,
    isFile = isFile,
    isDir = isDir,
    fileExists = fileExists,
    readFile = readFile,
    env = env,
    panic = panic,
    runProcess = runProcess
}
