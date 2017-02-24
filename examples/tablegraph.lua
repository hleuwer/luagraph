local m = require "graph"

local vbox, hbox = m.vbox, m.hbox

-- Get rid of ':' in table representation - only for invisible strings
local function tname(t)
   local s = string.gsub(tostring(t), ": ", "")
   return s
end

-- Create a port name
local function tport(s)
   return "<"..tname(s)..">"
end

-- Return a closure that creates a node from a table
local function tnode(g, t, cache)
   cache = cache or {}
   local fields = {}
   for k,v in pairs(t) do
      if type(v) == "table" then
	 if t ~= v  and v ~= _G and v~= _G.graph then
	    if cache[v] == nil then
	       cache[v] = k
	       table.insert(fields, vbox{tostring(k), tport(k)..tostring(v)})
	       tnode(g, v, cache)
	    end
	    g:edge(tname(t)..":"..tname(k), tname(v))
	 end
      else
	 table.insert(fields, vbox{tostring(k), tport(k)..tostring(v)})
      end
   end
   g:record{tname(t), hbox{tostring(t), unpack(fields)}}
end

-- Retrun a graph representing a nested table structure
local function tgraph(t)
   local g = m.open("GG")
   g.rankdir="LR"
   tnode(g, t)
   return g
end

------------------------------------------
-- Example tables
------------------------------------------
local xt = {
   a=1,
   b="some text",
   c=3
}

local et = {}
et[1] = {
   [1] = "index-1",
   aField = "This is a field",
   anotherField = "This is another field",
   aTable = {
      subfield = "subfield",
      [1] = "index aTable[1]",
      subtab = { 
	 f = 3,
	 [1] = {
	    name="herbert",
	    age=47
	 }
      },
      anumber = 200
   },
   xf = "duda",
   t1 = xt,
   {
      x=2,y=3
   },
   numField = 999,
}

local t = {}
for i = 1,3 do
   t[i] = {}
   for j = 1,4 do
      t[i][j] = "field["..i.."]["..j.."]"
   end
end
et[2] = t

local ende = false
while not ende do
   print()
   print("Select table to show:")
   print("(1) example table")
   print("(2) double indexed array")
   print("(3) snmp module")
   print("(4) socket module")
   print("(q) quit")
   io.write("select: ") io.flush()
   local s = io.read()
   if s == "q" then 
      ende = true 
   else 
      if s == "3" then
	 et[3] = require "snmp"
      end
      if  s == "4" then
	 et[4] = require "socket"
	 print(et[4])
      end
      local ix = tonumber(s)
      if et[ix] ~= nil then
	 local g = tgraph(et[tonumber(s)])
	 print("Close the display window completely to continue.")
	 g:show()
	 g:render("png", "out.png", "dot")
	 g:close()
      else
	 print("Selected item is NIL - cannot produce graph")
      end
   end
end
