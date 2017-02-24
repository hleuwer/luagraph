gr = require "graph"

--
-- For simple formatted printing
--
local function printf(fmt, ...)
   print(string.format(fmt, ...))
end

--
-- Get a local reference for frequently used functions (optional)
--
local node, edge, subgraph, cluster, digraph, strictdigraph = 
  gr.node, gr.edge, gr.subgraph, gr.cluster, gr.digraph, gr.strictdigraph

--
-- Example graph
--
local g = digraph{"G",
  cluster{"c0",
    edge{"a0", "a1", "a2", "a3"}
  },
  cluster{"c1",
    edge{"b0", "b1", "b2", "b3"}
  },
  edge{"x", "a0"},
  edge{"x", "b0"},
  edge{"a1", "a3"},
  edge{"a3", "a0"}
}

--
-- Same graph in dot notation
--
local dotsource = [[
digraph G {
	subgraph cluster_c0 {a0 -> a1 -> a2 -> a3;}
	subgraph cluster_c1 {b0 -> b1 -> b2 -> b3;}
	x -> a0;
	x -> b0;
	a1 -> a3;
	a3 -> a0;
}
]]

-- Print graph as dotfile
printf("1. Generated 'dot' format:")
g:write()
printf("2. Source in 'dot' format:\n %s\n", dotsource)

-- Show the graph
g:show()

-- Close the graph
g:close()
