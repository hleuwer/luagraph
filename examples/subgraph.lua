gr = require "graph"

--
-- simple printing
--
local function printf(fmt, ...)
  print(string.format(fmt, ...))
end

-- 
-- Create a graph
--
local g = gr.open("G")
g:declare{
  node = {shape = "circle", color = "blue"}, 
  edge = {color = "green"}
}
--
-- Create a subgraph with different prototype
--
local sg = g:subgraph("SG")
assert(sg:declare{
	 node = {shape = "box", color="red"}, 
	 edge = {color="red"}
       })
--
-- Some nodes and edges
--
g:edge{"n1", "n2", label = "n1=>n2"}
sg:edge{"n3", "n4", label = "n3=>n4"}
g:edge("n1", "n4", "n1=>n4")

--
-- Print prototypes
--
g:write()
g:show()
g:close()