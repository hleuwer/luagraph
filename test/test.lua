local graph = require "graph"
local logger = require "logging.console"
local pretty = require "pl.pretty"
local tmp_prefix
local function tmpname()
   if graph._SYSTEM == "Win32" then
      return "."..os.tmpname()
   else
      return os.tmpname()
   end
end
----------------------------------------------------------------------
-- Logging
----------------------------------------------------------------------
local loglevel = string.upper(arg[1] or "INFO")
local log = logger("TEST:".."%message")
log:setLevel(loglevel)
print()

local intro = function(fmt, ...)
   log:info("===== "..fmt.."\n", ...)
   io.stdout:flush()
end
local info = function(fmt, ...) 
   log:info(string.format(fmt.."\n", ...))
   io.stdout:flush()
end
_debug=debug
local debug = function(fmt, ...) 
   log:debug(string.format(fmt.."\n", ...))
   io.stdout:flush()
end


local function collect()
     collectgarbage("collect")
end

local function newgraph(name, kind)
   local g, err = graph.open(name, kind)
   assert(g, err);
   return g
end

local showgraph = (os.getenv("SHOWGRAPH") == "yes") or false
local printgraph = (os.getenv("PRINTGRAPH") == "yes") or false
local showinfo = (os.getenv("SHOWINFO") == "yes") or false

local function gprint(g) 
  if printgraph == true then
    print()
    g:write()
  end
end

local function _gshow(g)
  if showgraph == true then
    local fname = tmpname()..".dot"
    g:write(fname) os.execute("dotty "..fname)
    os.remove(fname)
  end
end

local function gshow(g)
   if showgraph == true then
      g:show()
   end
end

local function gcompare(g, ref)
  local fn = tmpname()
  g:write(fn)
  local fref = io.open(ref, "r")
  local sres = fref:read("*a")
  local fnow = io.open(fn, "r")
  local snow = fnow:read("*a")
  assert(sres == snow)
end

----------------------------------------------------------------------
-- Tests
----------------------------------------------------------------------
local function test_graph_base()
  intro("Test graph: open and read ...")
  local g = {}, err
  -- New graph: create with discipline
  g[1] = assert(newgraph("G1-base", "undirected"))
  debug("G1-base created: %s", tostring(g[1]))
  -- New graph: create with default discipline
  g[2] = assert(newgraph("G2-base"))
  debug("G2-base created: %s", tostring(g[2]))
  -- New graph: read from file
  g[3] = assert(graph.read("test/test_dat1.dot"))
  debug("test_dat1.dot read: %s", tostring(g[3]))
  -- New graph: undirected
  g[4] = assert(newgraph("G4-base", "undirected"))
  debug("G4-base created: %s", tostring(g[4]))


  -- Show properties
  debug("Properties:")
  for _,h in pairs(g) do
    debug("  %q: type=%s strict=%s directed=%s nnodes=%d nedges=%d id=%d",
	  h.name, type(h), tostring(h.isstrict), tostring(h.isdirected),
	  h.nnodes, h.nedges, h.id)
  end

  -- New graph: error in reading
  g[5], err = graph.read("___does__not__exist__")
  assert(g[5] == nil and type(err) == "string")

  -- Close all created graphs
  for _,v in pairs(g) do
    debug("closing %q", v.name)
    assert(v:close()) 
  end
  intro("passed")
end

local function test_graph_properties(g)
  intro("Test graph: properties ...")
  -- Show properties
  intro("passed")
end

local function test_graph_meta(g)
  intro("Test graph: metamethods ...")
  local g1 = assert(graph.open("G1-meta"))
  local g2 = assert(graph.open("G2-meta"))
  local h = g1
  assert(h == g1)
  assert(h ~= g2)
  g1:close()
  g2:close()
  collectgarbage("collect")
  intro("passed")
  return h
end

local function test_graph_write()
  intro("Test graph: write ...")
  local h = assert(graph.read("test/test_dat1.dot"))
  -- Write to stdout
  gprint(h)
  -- Write to file
  local fn = tmpname()
  log:debug("write to %s", fn);
  h:write(fn)
  -- Compare contents with reference
  local fref = assert(io.open("test/ref.dot","r"))
  local sref = assert(fref:read("*a"))
  log:debug("ref: %s", sref)
  local f = assert(io.open(fn))
  local s = assert(f:read("*a"))
  log:debug("out: %s ", s)
  log:info("!!! ATTENTION: Todo: compare dot files")
  fref:close()
  f:close()
  h:close()
  intro("passed")
end

local function test_graph_defattr()
  intro("Test graph: default attributes ...")
  local h = assert(graph.read("test/test_dat1.dot"))
  -- Getting initial default attributes
  debug("  Initial default attributes:")
  debug("  graphs: %s", tostring(h:getgraphattr()))
  for k,v in pairs(h:getgraphattr()) do
     debug("    %s=%q", k, v)
  end
  debug("  nodes: %s", tostring(h:getnodeattr()))
  for k,v in pairs(h:getnodeattr()) do
     debug("    %s=%q", k, v)
  end
  debug("  edges: %s", tostring(h:getedgeattr()))
  for k,v in pairs(h:getedgeattr()) do
     debug("    %s=%q", k, v)
  end
  
  -- Set attributes in parent
  debug("Default attributes after modifications:")
  n = assert(h:setattr{
	       edge = {shape="box", color="blue", whatever="noidea", anumber=71, ['3']="spu"},
	       node = {color="red", whatever="donotknow", anumber=17, ['3'] = "ups"}
	     })
  assert(n == 9)
  local h2 = assert(h:subgraph("h2"))
  local hh3 = assert(graph.open("hh3"))
  local hh4 = assert(hh3:subgraph("hh4"))

  -- Check: parent shares attribute with client if client doesn't define something
  --        different
  assert(h2:defaults().edge.whatever == h:defaults().edge.whatever)

  -- Set attributes in childs
  assert(h:setattr{edge={att="att"}})
  assert(hh4:setattr{edge={att="tta"}})
  assert(h2:setattr{edge={att="xtt2"}})
  debug("  h.edge.att = %q", h:defaults().edge.att or "nil")
  debug("  h2.edge.att = %q", h2:defaults().edge.att or "nil")
  debug("  h3.edge.att = %q", hh3:defaults().edge.att or "nil")
  debug("  h4.edge.att = %q", hh4:defaults().edge.att or "nil")

  -- Check: child shares defaults with parent
  assert(h:defaults().edge.att ~= h2:defaults().edge.att)
  assert(hh3:defaults().edge.att == hh4:defaults().edge.att)

  -- Check: defaults in different subtrees are not shared
  assert(h:defaults().edge.att ~= hh3:defaults().edge.att)
  debug("close 'h'")
  h:close()
  debug("close 'h2'")
  status, rv, err = pcall(h2.close, nil)
  assert(status == false)
  debug("close 'hh4'")
  hh4:close()
  debug("close 'hh3'")
  hh3:close()
  intro("passed")
end

local function test_graph_subgraph()
  intro("Test graph: subgraphs ...")
  local root = assert(graph.open("Root", "directed"))
  local mother = assert(root:subgraph("Mother"))
  local father = assert(root:subgraph("Father"))
  assert(father ~= root:subgraph("Mother"))
  local son = assert(mother:subgraph("Son"))
  local daughter = assert(father:subgraph("Daughter"))
  -- Print the tree
  debug("root=%q mother=%q son=%q daughter=%q",
	root.name, mother.name, son.name, daughter.name)
  debug("root=%d mother=%d son=%d daughter=%d",
	root.id, mother.id, son.id, daughter.id)
  debug("%q: parent=%q root=%q", 
	mother.name, mother.parent.name, mother.root.name)
  debug("%q: parent=%q root=%q", 
	son.name, son.parent.name, son.root.name)
  debug("%q: parent=%q root=%q", 
	daughter.name, daughter.parent.name, daughter.root.name)
  -- Check parent/root relations 
  assert(root == mother.parent)
  assert(root == son.root)
  assert(mother == son.parent)
  assert(father == daughter.parent)
  assert(root:subgraph("Mother") == mother)
  assert(root:subgraph("Father") == father)
  assert(root.isroot == true)
  assert(mother.isroot == false)
  assert(father.isroot == false)
  assert(son.graph == son)
  assert(root.graph == root)
  rv, res1, res2 = pcall(root.subgraph, root)
  assert(rv == false)
  -- This must close all other graphs created here !
  debug("Closing graph root ...")
  root:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_graph_iterate()
  intro("Test graph: graph iteration ...")
  local root = assert(graph.open("Root", "directed"))
  local father = assert(root:subgraph("Father"))
  local mother = assert(root:subgraph("Mother"))
  local son = assert(mother:subgraph("Son"))
  local daughter = assert(father:subgraph("Daughter"))
  local t = {}
  for i = 1, 10 do
     t[i] = root:subgraph("sub-"..i)
  end
  -- Iteration 1
  debug("Iteration 1 - while ")
  local g = root:getnext(nil)
  while g do
     debug("  next of %s: %s", root.name, g.name)
     g = root:getnext(g)
  end
  debug("Iteration 1 - repeat ")
  local g = nil
  repeat
    g = root:getnext(g)
    if g ~= nil then
       debug("  next of %s: %s", root.name, g.name)
    end
  until g == nil

  -- Iteration 2
  debug("Iteration 2")
  for g in root:walk() do
    debug(" %s", g.name)
    for h in g:walk() do
      debug("    %s", h.name)
    end
  end
  
  local g = {}
  for i = 1, 20 do
    g[i] = root:subgraph("sub-"..i)
  end
  
  -- Iteration 3
  debug("Iteration 3")
  for g in root:walk() do 
     assert(g.parent == root) 
  end

  -- Closes all graphs
  root:close()
  collectgarbage("collect")

  -- Iteration 4
  debug("Iteration 4")
  g = assert(graph.read("test/test_dat2.dot"))
  for v in g:walkgraphs() do
    debug(" %s %s", v.name, v.status)
    for w in v:walk() do
      debug("   %s %s", w.name, v.status)
    end
  end
  g:close()
  intro("passed")
end

local function test_graph_test()
  intro("Test graph: test ...")
  local g = assert(graph.open("G", "strictdirected"))
  local n1 = assert(g:node("N1"))
  local n2 = assert(g:node("N2"))
  local e1 = assert(g:edge(n1, n2, "n1=>n2"))
  local e2 = assert(g:edge(n2, n1, "n2=>n1"))
  if showinfo then
     e1:info()
     e2:info()
  end
end

local function test_graph_strict()
  intro("Test graph: strict ...")
  local g = assert(graph.open("G", "strictdirected"))
  local n1 = assert(g:node("N1"))
  local n2 = assert(g:node("N2"))
  
  local e1 = assert(g:edge(n1, n2, "n1=>n2"))
  debug("  e1: %s label=%q name=%q", tostring(e1), e1.label, e1.name)
  -- repeated creation of edge fails for strict directed graphs
  local e2 = assert(g:edge(n1, n2, "n1...n2"))
  debug("  e2: %s label=%q name=%q", tostring(e2), e2.label, e2.name)
  local e3 = assert(g:edge(n2, n1, "n2=>n1"))
  debug("  e3: %s label=%q name=%q", tostring(e3), e3.label, e3.name)
  local e4 = assert(g:edge(n1, n1, "n1=>n1"))
  debug("  e4: %s label=%q name=%q", tostring(e4), e4.label, e4.name)
if showinfo then
  n1:info()
  n2:info()
  e1:info()
  e2:info()
  e3:info()
  e4:info()
end
  -- first edge must be present
  debug("  Finding edge n1=>n2: %q", e1.name)
  assert(g:findedge(n1, n2, e1.name))
  -- second edge must not be present
  debug("  Finding edge n1...n2: %q", e2.name)
  assert(g:findedge(n1, n2, e2.name))
  debug("  Finding edge n2=>n1 w/o name")
  assert(g:edge(n2, n1))
  debug("  Closing graph")
  g:close()
--  collectgarbage("collect")
  intro("passed")
end

local function test_delete()
  intro("Test graph: delete object ...")
  local g = graph.open("G-delete")
  local sg = g:subgraph("SG-delete")
  local ssg = sg:subgraph("SSG-delete")
  debug("  Delete: sg=%s", tostring(sg))
  local rv, err  = g:delete(sg)
  debug("  Close: g=%s", tostring(g))
  g:close()
--  collectgarbage("collect")
  intro("passed")
end

local function test_close()
  intro("Test graph: close ...")
  -- Closing graphs
  local g = graph.open("G-close")
  local sg1 = assert(g:subgraph("SG1-close"))
  local sg2 = assert(g:subgraph("SG2-close"))
  local ssg1 = assert(sg1:subgraph("SSG1-close"))
  assert(g:close())
  debug("Collecting garbage...")
  collectgarbage("collect")
  intro("passed")
end

local function test_close_error()
  intro("Test graph: close with error ...")
  local rv, err = pcall(graph.close, 0)
  if not rv then debug("Error while closing: %q", err) end
  intro("passed")
end

local function test_node_base()
  intro("Test node: base node tests ...")
  local g = assert(graph.open("G-base"))

  -- Implict creation
  local n1 = assert(g:node("N1"))
  local n2 = assert(g:node("N2"))
  assert(g == n1.graph)
  assert(g == n2.graph)
  debug("n1 is %s", tostring(n1))
  debug("graph of n1 is %q", n1.graph.name)
  -- Lookup
  debug("n1 is %s", tostring(g:node("N1")))

  -- A second node
  local n2 = assert(g:node("N2"))
  debug("n2 is %s", tostring(n2))

  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_node_properties()
  intro("Test node: node properties ...")
  local g = assert(graph.open("G"))
  local sg = assert(g:subgraph("SG"))
  local n1 = assert(g:node("Na1"))
  local n2 = assert(g:node("Na2"))
  local n3 = assert(sg:node("Na3"))
  debug("nx.id = %d %d %d", n1.id, n2.id, n3.id)
  debug("nx.name = %q %q %q", n1.name, n2.name, n3.name)
  debug("nx.graph = %q %q %q", n1.graph.name, n2.graph.name, n3.graph.name)
  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_node_meta()
  intro("Test node: node metamethods ...")
  local g = assert(graph.open("G"))
  local n1 = assert(g:node("Nx1"))
  local n2 = assert(g:node("Nx1"))
  local n3 = assert(g:node("Nx3"))
  assert(n1 == n2)
  debug("Concatenate nodes %q %q with '..'", n1.name, n2.name)
  local e = assert(n1..n2)
  e.label = "n1 ==> n2"
  debug("e.label = %q", e.label)
  debug("Concatenate nodes %q %q with '+'", n3.name, n1.name)
  local e2 = assert(n3+n1)
  e2.label = "n3 ==> n1"
  debug("e2.label = %q", e2.label)
  gprint(g)
  debug("Delete node %q", n1.name) 
  local rv = assert(n1:delete())
  -- Check whether userdata became invalid after node deletion
  local status, rv, err = pcall(function(n) return n.name end, n1)
  assert(status == false)
  debug("check userdata invalidation: status=%s rv=%s err=%q", tostring(status), tostring(rv), err or "nil")
  g:close()
  debug("Collecting garbage ...")
  collectgarbage("collect")
  intro("passed")
end

local function test_node_subnode()
  intro("Test node: subnodes ...")
  local g = assert(graph.open("G"))
  local sg1 = assert(g:subgraph("SG1"))
  local sg2 = assert(g:subgraph("SG2"))
  local n = assert(sg1:node("N1"))
  gprint(g)
  local sn = assert(sg2:subnode(n, true))
  assert(n == sn)
  debug("n.name=%q sb.name=%q", n.name, sn.name)
  debug("n.seq=%d sn.seq=%d", n.seq, sn.seq)
  debug("n.id=%d sn.id=%d", n.id, sn.id)
  gprint(g)
  n:delete()
  gprint(g)
  sg1:close()
  gprint(g)
  g:close()
  debug("Collecting garbage ...")
  collectgarbage("collect")
  intro("passed")
end

local function test_node_degree()
  intro("Test node: node degree ...")
  local g = assert(graph.open("G-degree"))
  local n = assert(g:node("N1-degree"))
  local rv = assert(n:degree())
  debug("n:degree() = %d", rv)
  n:delete()
  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_node_degree2()
  intro("Test node: node degree ...")
  local g = graph.read("test/test_dat1.dot")
  local n = g:node("NE_WEST")
  debug("inputs: %d outputs: %d sum: %d", 
	n:degree("*i"), n:degree("*o"), n:degree("*a"))
  assert(n:degree("*i") == 3)
  assert(n:degree("*o") == 4)
  assert(n:degree() == 7)
  g:close()
  intro("passed")
end

local function test_node_iterate()
  intro("Test node: node iteration ...")
  local g = assert(graph.open("G-nodeiter"))
  local n = {}
  for i = 1, 20 do
    n[i] = assert(g:node("N"..i.."-nodeiter"))
  end

  -- Iteration 1
  debug("Iteration 1")
  local n = nil
  repeat
    n = g:nextnode(n)
    if n then
      debug("  next node of %s: %s %d", g.name, n.name, n.id)
    end
  until n == nil

  -- Iteration 2
  debug("Iteration 2")
  for n in g:walknodes() do
    debug("  %s", n.name)
  end
  g:close()

  -- Iteration 3
  debug("Iteration 3")
  g = assert(graph.read("test/test_dat1.dot"))
  for n in g:walknodes() do
    debug("  %s", n.name)
  end

  -- Iteration 4
  debug("Iteration 4")
  sg =assert(g:subgraph("SG"))
  sn1 = sg:node("SN1")
  sn2 = sg:node("SN2")
  for h in g:walk() do
    debug("  sg.name=%s", h.name)
    for n in h:walknodes() do
      debug("    %s", n.name)
    end
  end
  for n in g:walknodes() do
    debug("  %s", n.name)
  end
  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_edge_base()
  intro("Test edge: edge base ...")
  local g = assert(graph.open("G","strictdirected"))
  local sg = assert(g:subgraph("SG"))
  local n1 = assert(g:node("N1"))
  local n2 = assert(g:node("N2"))
  -- retrieve 'N2' into new variable for later use
  local n3 = assert(g:node("N2"))
  -- 1. edge
  local e1 = assert(g:edge(n1, n2, "E1:N1=>N2"))
  debug("e1.label=%q e1.name=%q", e1.label or "nil", e1.name or "nil")
  assert(e1.label == "E1:N1=>N2")

  -- 2. edge but with n3 instead of n2
  local e3 = assert(g:edge(n3, n1, "E2:N2=>N1"))
  assert(e1 ~= e3)
  e1.headport = "head"
  e3.tailport = "tail"
  -- Insert a subgraph
  local ns1 = assert(sg:node("SN1"))
  local ns2 = assert(sg:node("SN2"))
  local ns3 = assert(sg:node("SN3"))
  local es1 = assert(sg:edge(ns1, ns2, "ES2"))
  local es2 = assert(g:edge(n1, ns2, "XXX"))
  local es3 = assert(ns3:edge(n1, "duda"))
  local es4 = assert(ns3:edge("IMP1", "ES2"))
  -- retrieve edge check
  local e2 = assert(g:edge(n1, n2))
  if showinfo then
     e1:info()
     e2:info()
  end
  assert(e1 == e2)

  -- check head and tail
  assert(e1.tail == n1)
  assert(e1.head == n2)
  gprint(g)
  e1:delete()
  gprint(g)
  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_edge_subedge()
  intro("Test edge: subedges ...")
  local g = assert(graph.open("G"))
  local sg1 = assert(g:subgraph("SG1"))
  local sg2 = assert(g:subgraph("SG2"))
  local n1 = assert(sg1:node("N1"))
  local n2 = assert(sg1:node("N2"))
  local e1 = assert(n1:edge(n2, "E1"))
  gprint(g)
  local e2 = assert(sg2:subedge(e1, true))
  assert(n1 == sg2:node("N1"))
  assert(n2 == sg2:node("N2"))
  assert(e1 == e2)
  gprint(g)
  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function test_edge_iterate()
  intro("Test edge: iteration ...")
  local g = assert(graph.read("test/test_dat1.dot"))
  -- Iteration 1
  debug("Interation 0")
  local n = g:nextnode(nil)
  while n do
     debug("  node: %q", n.name)
     n = g:nextnode(n)
  end
  if false then
     debug("Iteration 1")
     for n in g:walknodes() do
        debug("  node: %q", n.name)
        for e in n:walkedges() do
           debug("    edge id=%d name=%q label=%q ptr=%s", e.id, e.name, e.label or "-", tostring(e))
           if e.tail then debug("    - tail: %q", e.tail.name) end
           if e.head then debug("    - head: %q", e.head.name) end
        end
     end
  end
  -- Iteration 2
  if false then
     debug("Iteration 2")
     for n in g:walknodes() do
        debug("  node: %q", n.name)
        for e in n:walkinputs() do
           debug("    input name=%q label=%q", e.name,  e.label)
        end
        for e in n:walkoutputs() do
           debug("    output name=%q label=%q", e.name, e.label)
        end
     end
  end
  g:close()
  collectgarbage("collect")
  intro("passed")
end

local function _test_attr()
   intro("Test misc: attributes ...")
   local g = graph.open("G")
   assert(g:declare{
             node = {shape="box", color="blue", width=3},
             edge = {color="red", label=""}
   })
   local n1 = g:node("N1")
   local n2 = g:node("N2")
   n2.otto=100
   n1.egon="hallo"
   n1.anna={x=10,"blablabla"}
   debug("n1.shape=%q %d %q %s", n1.shape, n1.otto, n1.egon, type(n1.egon))
   debug("n2.shape=%q %d %d %q %s", n2.shape, n2.otto, n2.otto+10, n2.egon or "nil", type(n2.otto))
   n2.shape = "circle"
   debug("n2.shape=%q", n2.shape)
end

local function test_attr()
  intro("Test misc: attribute access ...")
  local g = assert(graph.open("G"))
  assert(g:declare{
	   node = {shape="box", color="blue", width=3},
	   edge = {color="red", label=""}})
  local n = assert(g:node("N1"))
  local n2 = assert(g:node("N2"))
  local n3 = assert(g:node("N3"))
  local e = assert(n:edge(n2, "E1"))
  local e2 = assert(n:edge(n3, "E2"))
  gprint(g)
  -- Types
  assert(g:type() == "graph")
  assert(n2:type() == "node")
  assert(e:type() == "edge")
  -- Get
  assert(n.shape == "box")
  debug("1. shape: %q", n.shape)
  -- Failed get
--  rv, err = pcall(function () print(n.hello) end)
--  assert(rv == false)
--  debug("Test: failed get error: %q", err)
  -- Set
  n.shape = "circle"
  n.width = 5
  debug("2. shape: %s width: %d", n.shape, tonumber(n.width))
  gprint(g)
  -- Failed set
--  rv, err = pcall(function(key, val) n[key] = val end, "hello", "helloval")
--  assert(rv == false)
--  debug("Test: failed set error: %q", err)
  -- Implicit set
  local xg = graph.open("XG")
  local xn = xg:node("XN")
  xn.anysym = "anysym"
  debug("xn.anysym=%s", xn.anysym)
  assert(xn.anysym == "anysym")
  local yn = xg:node("YN")
  yn.anysym="none"
  debug("yn.anysym=%s", yn.anysym)
  assert(yn.anysym == "none")
  -- Just a try
  n.shape="box"
  n2.shape="circle"
  n2.color="red"
  n.width=1
  e.color = "green"
  e.label = "this is an edge"
  gshow(g)
  intro("passed");
end

local function test_anyattrib()
  intro("Test misc: anyattrib  ...")
  g = assert(graph.open("G"))
  n1 = assert(g:node("N1"))
  n2 = assert(g:node("N2"))
  e1 = assert(g:edge(n1, n2, "n1=>n2"))
  e2 = assert(g:edge(n2, n1, "n2=>n1"))
  local t = {1,2,3}
  n1.someval = t
  assert(n1.someval == t)
  n2.someval = true
  assert(n2.someval == true)
  n1.method = function(self, param) 
		debug("node name: %q %q", self.name, param) 
		return param  
	      end
  e1.someval = 17.2
  e2.method = function(self, param) 
		debug("edge label: %q %q", self.label, param) 
		return param
	      end
  assert(n1:method("called n1") == "called n1")
  assert(e2:method("called e2") == "called e2")
  e2[t] = "hey"
  assert(e2[t] == "hey")
  assert(e2[n1.someval] == "hey")
  intro("passed");
end

local function test_contains()
  intro("Test misc: containment  ...")
  local g1 = assert(graph.open("G1"))
  g1:declare{node={shape="box"}}
  local sg1 = assert(g1:subgraph("SG1"))
  local g2 = assert(graph.open("G2"))
  g2:declare{node={shape="box"}}
  local g3 = assert(graph.read("test/test_dat1.dot"))
  local n1 = assert(g1:node("N1")) n1.shape="circle"
  local n2 = assert(g2:node("N2"))
  local n3 = assert(g2:node("N1")) n3.shape="ellipse"
  local e1 = assert(g2:edge(n2,n3,"n2->n3"))
  local e2 = assert(g1:edge(n1,n1,"n1->n1"))
  local err, res1, res2 = pcall(function() local e3 = assert(g1:edge(n2,n1,"n2->n1")) end)
  assert(err == false)
  debug("Test: edge with nodes in different graphs: %q %q", tostring(res1), tostring(res2));
  assert(g1:contains(n1) == true)
  assert(g1:contains(n2) == false)
  assert(g1:contains(n3) == false)
  assert(g2:contains(n1) == false)
  assert(g2:contains(n2) == true)
  assert(g2:contains(n3) == true)
  assert(g1:node("N2", nil, true) == nil)
  assert(g1:node("N1", nil, true) == n1)
  assert(g1:idnode(n1.id) == n1)
  assert(g2:idnode(n2.id) ~= n3)
  debug("n1.id=%d n2.id=%d n3.id=%d", n1.id, n2.id, n3.id)
  debug("e1.graph=%q e2.graph=%q", e1.graph.name, e2.graph.name)
  gprint(g1)
  gprint(g2)
--  print(n1:type(), n3:type()) 
--  print(n1.shape, n3.shape)
--  print("g1:", g1:contains(n1), g1:contains(n2), g1:contains(n3), g1:node("N2", false), g1:node("N1"))
--  print("g2:", g2:contains(n1), g2:contains(n2), g2:contains(n3), g2:node("N2", false), g2:node("N1"))
  intro("passed");
end

local function test_find()
  intro("Test misc: finding  ...")
  local g = assert(graph.read("test/test_dat1.dot"))
  local tail = g:findnode("NE_EAST")
  local head = g:findnode("CE_WEST")
  debug("tail.name = %q", tail.name)
  debug("head.name = %q", head.name)
  local e = g:findedge(tail, head)
  assert(e.label == "out1 => in2")
  debug("e.label = %q", e.label)
  intro("passed");
end

local function test_layout()
  intro("Test layout: layout  ...")
  local g, t = graph.open("Gx")
  local e1 = g:edge{"n1", "n2", label="n1=>n2"}
  local fn = tmpname()
  debug("Layout:")
  assert(g:layout("dot"))
  debug("PLAIN:")
  assert(g:render("plain", fn))
  debug("Layout:")
  assert(g:layout("circo"))
  debug("XDOT:")
  assert(g:render("xdot", fn))
  debug("Cleanup ...")
  os.remove(fn)
  g:close()
  intro("passed");
end

local function test_cluster()
  intro("Test misc: cluster  ...")
  local g,t = graph.open("G", "directed")
  g:declare{node={shape = "box"}, edge={color="red"}}
  local c1 = g:cluster("SG1")
  local c2 = g:cluster("SG2")
  local n1 = c1:node{"n1", shape = "circle"}
  local n2 = c2:node{"n2", shape = "ellipse"}
  local n3 = c2:node{"n3"}
  local e1 = g:edge(n2, n1, "n2=>n1")
  local e2 = g:edge(n1, n2, "n1=>n2")
  local e3 = g:edge{n1, n2, n3, label = "n1=>n2=>n3", color="blue"}
  assert(g:type(c1) == "graph")
  assert(c1.parent == g)
  assert(c2.parent == g)
  gprint(g)
  gshow(g)
  g:close()
  intro("passed")
end

local function test_graphtab()
  intro("Test misc: graph from table  ...")
  local node, edge, subgraph, cluster, digraph = 
    graph.node, graph.edge, graph.subgraph, graph.cluster, graph.digraph
  xn = node{"xn", color="red"}
  local g = digraph{"G",
    node = {shape = "box", color = "blue"},
    edge = {color = "red"},
    cluster{"SG",
      edge{
	node{"sn1"},
	node{"sn2"},
	xn
      }
    },
    edge{
      node{"n1", shape = "box", color = "black"},
      node{"n2", shape = "circle"},
      node{"n3", shape = "ellipse"},
      xn,
      label = "n1=>n2=>n3",
      color = "green"
    },
    edge{"sn2", "n1", label="sn1=>n1"}
  }
  debug("Showing graph ...")
  gshow(g)
  debug("g.name=%q", g.name)
  local s = tostring
  for n in g:walknodes() do
    debug("  n.name=%q n.shape=%q n.color=%q", 
	   s(n.name), s(n.shape), s(n.color))
    for e in n:walkedges() do
      debug("    e.label=%q e.color=%q", s(e.label), s(e.color))
    end
  end
  assert(g.nnodes == 6)
  assert(g.nedges == 6)
  debug(" %d nodes", g.nnodes)
  debug(" %d edges", g.nedges)
  gprint(g)
  g:close()
  intro("passed")
end

local function test_xx()
  local g = graph.open("G")
  for i = 1,500 do
    local n = g:node("A"..i)
    g:node("B"..i):edge(n, "E"..i)
  end
  g:write("x3.dot")
end

local function test_huge()
   intro("Test with huge graph ...")
   local g = graph.open("G")
   local j = 1
   local N = 30
   local nodes={}
   debug("Creating nodes ...")
   for i = 1, N do
      nodes[i] = g:node("N"..i)
   end
   debug("Creating edges ...")
   for i = 1, N do
      for j = i, N do
         g:edge(nodes[i],nodes[j])
      end
   end
   gshow(g)
--   debug("Writing ...")
--   g:write("x3.dot")
   intro("passed")
end


local tests = {
  -- Graph tests
   --[[
   ]]
   test_graph_base,
   test_graph_properties,
   test_graph_meta,
   test_graph_write,
   test_graph_subgraph,
   test_graph_defattr,
   test_graph_iterate,
--   test_graph_test,
   test_graph_strict,
   test_delete,
   test_close,
   test_close_error,
   -- Node tests
   test_node_base,
   test_node_properties,
   test_node_meta,
   test_node_degree,
   test_node_degree2,
   test_node_iterate,
   -- Edge tests
   test_edge_base,
   test_edge_iterate,
   -- Misc tests
   test_attr,
   test_contains,
   test_find,
   test_anyattrib,
   test_cluster,
   test_graphtab,
   -- Layout and rendering
   test_layout,
   test_huge
      --[[
   ]]
}

local function test()
  for _, f in pairs(tests) do
    f()
    -- print(">>> ", gcinfo())
  end
  -- print(">>> ", gcinfo())
end

test()

-- Collect garbage
debug("Collecting garbage ...")
collectgarbage("collect")
debug("%s", collectgarbage("count"))

info("FINISHED")
