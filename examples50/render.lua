local gr = require "graph"

if arg.n < 2 then
  print("usage: lua render.lua FILE FORMAT")
  print("       FORMAT: dot, neato, twopi, fdp, circo")
  os.exit(-1)
end

local fmt = arg[2] or "dot"
--
-- For simple formatted printing
--
local function printf(fmt, ...)
  print(string.format(fmt, unpack(arg)))
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
local g = f(fmt)

printf("Generated '%s' format:", fmt)
if string.find(fmt, "nop") then
  g:layout("neato")
end
g:write()
g:show()
g:layout(fmt)
g:render("png", "out.png")
g:close()