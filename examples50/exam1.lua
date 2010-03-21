gr = require "graph"

--
-- Simple printing
--
local function printf(fmt, ...)
  print(string.format(fmt, unpack(arg)))
end

-- 
-- Convenience
--
local node, edge, subgraph, cluster, digraph = 
  gr.node, gr.edge, gr.subgraph, gr.cluster, gr.digraph
local s = tostring

--
-- A node constructor
--
xn = node{"xn", color="red"}

--
-- The graph
--
local g = digraph{"G",
  -- Prototype
  node = {shape = "box", color = "blue", style = "filled", fillcolor = "lightgrey"},
  edge = {color = "red"},
  graph = {style = "filled", label = ""},
  
  -- General graph properties
  rankdir = "LR",
  compound = "1",

  -- Graph objects
  cluster{"SG1",
    fillcolor = "grey",
    node{"SomeNode", fillcolor = "cyan"},
    edge{
      node{"sn1", style = "filled", fillcolor = "green"},
      node{"sn2"},
      xn,
      label = "hello world",
      dir = "both"
    }
  },
  cluster{"SG2",
    fillcolor = "green",
    node{"SomeOtherNode"},
  },
  edge{
    node{"n1", shape = "box", color = "black"},
    node{"n2", shape = "circle"},
    node{"n3", shape = "ellipse"},
    xn,
    label = "n1=>n2=>n3",
    color = "green"
  },
  edge{"sn2", "n1", label="sn1=>n1"},
  edge{"SomeNode", "SomeOtherNode", label="this=>other", color="magenta"},
  edge{"SomeNode", "SomeNode", label="this=>this", color="grey"},
  edge{"SomeOtherNode", "sn1", ltail="cluster_SG2", lhead="cluster_SG1", label="compound"},
}

printf("GRAPH: %q", g.name)

-- Print subgraphs/clusters, nodes and edges
for sg in g:walk() do
  printf("  SUBGRAPH: %q", sg.name)
  for n in sg:walknodes() do
    printf("    NODE: %q, shape=%q, color=%q", 
	   s(n.name), s(n.shape), s(n.color))
    for e in n:walkedges() do
      printf("      EDGE: %q, color=%q", s(e.label), s(e.color))
    end
  end
end

printf("NUMBER OF NODES: %d nodes", g.nnodes)
printf("NUMBER OF EDGES: %d edges", g.nedges)
g:show()
g:close()
