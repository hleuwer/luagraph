gr = require "graph"

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
node, edge, subgraph, cluster, digraph, strictdigraph = 
  gr.node, gr.edge, gr.subgraph, gr.cluster, gr.digraph, gr.strictdigraph

local f, err = assert(loadfile(arg[1]))
local g = f()

printf("Generated 'dot' format:")
g:write()
gshow(g)
g:close()