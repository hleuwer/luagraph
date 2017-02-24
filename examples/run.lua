gr = require "graph"

if #arg < 1 then
  print("usage: lua run.lua FILE")
  os.exit(-1)
end

--
-- For simple formatted printing
--
local function printf(fmt, ...)
  print(string.format(fmt, ...))
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
local g = f()

printf("Generated 'dot' format:")
g:write()
gshow(g)
g:close()
