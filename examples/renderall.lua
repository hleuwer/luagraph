local gr = require "graph"

local doall = true

if #arg < 2 then
  print("usage: lua renderall.lua FILE FORMAT")
  print("       FORMAT: dot, neato, twopi, fdp, circo")
  print("       FILE: Lua file defining and returning graph")
  os.exit(-1)
end

local layoutfmt = arg[2] or "dot"
--
-- For simple formatted printing
--
local function printf(layoutfmt, ...)
  print(string.format(layoutfmt, ...))
end

local function gshow(g)
  local fn = os.tmpname() .. ".dot"
  g:write(fn)
  os.execute("dotty "..fn)
  os.remove(fn)
end

-- Provide simple reference for frequently used functions (optional)
node, edge, subgraph, cluster, digraph, strictdigraph, ugraph = 
  gr.node, gr.edge, gr.subgraph, gr.cluster, gr.digraph, gr.strictdigraph, gr.graph

local f, err = assert(loadfile(arg[1]))
local g = f(layoutfmt)

printf("Generated '%s' format.", layoutfmt)
if string.find(layoutfmt, "nop") then
  g:layout("neato")
end
g:write()
g:show()
os.execute("rm -rf outdir")
os.execute("mkdir -p outdir")
local notworking = {
   Darwin = {
      cairo = 1,lasi = 1, map = 1, quartz = 1
   }
}
local sys = io.popen("uname"):read()
local fmtlist, err = gr.plugins("render")
print("available formats: ".. table.concat(fmtlist, " "))
print("system: "..sys)
for _, fmt in ipairs(fmtlist) do
   local fname = "outdir/out."..fmt
   if (not notworking[sys][fmt]) then
      printf("Rendering on system %q with format %q into file %q", sys, fmt, fname)
      local stat, rv, err = pcall(g.render, g, fmt, fname, "dot")
      print(stat, rv, err)
      if rv == nil then break end
   end
end
g:close()
