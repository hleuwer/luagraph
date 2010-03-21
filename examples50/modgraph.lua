-- display the kernel module dependencies

-- author: John Ellson <ellson@research.att.com>
-- modified: Herbert Leuwer <herbert.leuwer@gmx.de>

local gr = require "graph"

local modules = io.lines('/proc/modules') 

local g = gr.open("G")

--
-- Default values
--
g:declare{
  graph = {
    rankdir = "LR",
    size = "6.5 6.5",
    nodesep=0.05
  },
  node = {
    shape = "box",
    width = 0,
    height = 0,
    margin = 0.03,
    fontsize = 8,
    fontname = "helvetica"
  },
  edge = {
    arrowsize = 0.4
  }
}

--
-- Run through the modules database
-- and build nodes and edges
--
for rec in modules do
  for mod, usedbylist in string.gfind(rec, "([_%w]+) %w+ %w+ ([-,_%w]+)") do
    n = g:node(mod)
    for usedby in string.gfind(usedbylist, "([-_%w]+)") do
      if (usedby ~= '-') and (usedby ~= '') then
	n:edge(g:node(usedby))
      end
    end
  end
end

g:layout()
g:show()
g:render('png', "out.png")
g:close()
