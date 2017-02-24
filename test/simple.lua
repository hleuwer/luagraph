local graph = require "graph"
local n={}
local e={}
local g=graph.open("g", "directed")
--local r=g:node("R")
local N=5
for i=1,N do
   table.insert(n, g:node("n"..i))
end
for i=1,N do
   if i < N then
      local _e = g:edge(n[i], n[i+1], "e"..i)
--      table.insert(e, _e)
   else
      local _e = g:edge(n[i], n[1], "e"..i)
--      table.insert(e, _e)
   end
end
print("Collecting ...")
collectgarbage()
g:show()
--print("Deleting node 3 ...")
n[3]:delete()
g:show()
   --local n1, n2 = g:node("n1"), g:node("n2")
--local e1 = g:edge(n1, n2, "e1")
--os.execute("sleep 2")
print("Hit return to close ...")
x=io.stdin:read()
g:close()
collectgarbage()
--os.execute("sleep 1")
print("Hit return to exit ...")
x=io.stdin:read()
