gr = require "graph"

--
-- Formatted printing
--
local function printf(fmt, ...)
   print(string.format(fmt, ...))
end

--
-- Convenience
--
local node, edge, subgraph, cluster, digraph, strictdigraph, graph = 
  gr.node, gr.edge, gr.subgraph, gr.cluster, gr.digraph, gr.strictdigraph, gr.graph

--
-- The definition of a graph
--
local g = graph{"G",
  compound = "1",
--  rankdir = "BT",
  size="6.5,6.5",
  comment = "LuaGraph: exam2.lua",
  cluster{
    "mueller",
    edge{
      node{"peter"},
      node{"maria"},
      label = "in love"
    },
    node{"toni"},
    node{"jupp"},
    edge{"toni", "jupp", label="brothers"},
    edge{"peter", "toni", label="father"},
    edge{"peter", "jupp", label="father"},
    edge{"maria", "toni", label="mother"},
    edge{"maria", "jupp", label="mother"}
  },
  cluster{
    "meier",
    edge{
      node{"johann"},
      node{"eva"},
      label = "in love"
    },
  },
  node{"berlin", fillcolor="blue"},
  edge{
    "peter", "berlin",
    ltail = "cluster_mueller", 
    label = "lives in"
  },
  edge{
    "berlin", "johann",
    lhead = "cluster_meier",
    label = "lives in"
  },
  edge{
    "peter", "eva", label="daughter"
  },
  edge{
    "maria", "eva", label="daughter"
  }
}

--
-- Show the graph 
--
if true then
  g:show()
end

--
-- Render the graph into postscript format
--
print("Render ...")
g:layout("dot")
g:render("ps", "out.ps")
g:render("gif", "out.gif")
g:render("svg", "out.svg")
g:render("png", "out.png")
g:freelayout()
--
-- Close the graph
--
print("Close ...")
g:close()

