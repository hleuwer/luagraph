local edges = {}
local random = math.random
math.randomseed(os.time())
local N = 20
for i = 1, N do
  table.insert(edges, edge{"n"..tostring(random(1,N/2)), "n"..tostring(random(1,N/2))})
end

if arg[1] == "dot" then
  return digraph{"G", unpack(edges)}
else
  return ugraph{"G", unpack(edges)}
end