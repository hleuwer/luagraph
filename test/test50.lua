local graph = require "graph"
logging = require "logging.console"

local function newgraph(name, kind)
  local g, err = graph.open(name, kind)
  assert(g, err);
  return g
end

local showgraph = false
local printgraph = false

local function gprint(g) 
  if printgraph == true then
    print()
    g:write()
  end
end

local function gshow(g)
  if showgraph == true then
    local fname = os.tmpname()..".dot"
    print(fname)
    g:write(fname) os.execute("dotty "..fname)
    os.remove(fname)
  end
end

local function gcompare(g, ref)
  local fn = os.tmpname()
  g:write(fn)
  local fref = io.open(ref, "r")
  local sres = fref:read("*a")
  local fnow = io.open(fn, "r")
  local snow = fnow:read("*a")
  assert(sres == snow)
end
----------------------------------------------------------------------
-- Logging
----------------------------------------------------------------------
local loglevel = string.upper(arg[1] or "INFO")
local log = logging.console("%message")
log:setLevel(loglevel)

local intro = function(fmt, ...)
		log:debug("============================ ")
		log:info(fmt, unpack(arg))
		log:debug("\n")
	      end
local info = function(fmt, ...) 
	       log:info(string.format(fmt, unpack(arg)))
	       io.stdout:flush()
	     end
local debug = function(fmt, ...) 
		log:debug(string.format(fmt, unpack(arg))) 
	      end

----------------------------------------------------------------------
-- Tests
----------------------------------------------------------------------
local function test_graph_base()
  intro("Test graph: open and read ...")
  local g = {}, err
  -- New graph: create with discipline
  g[1] = assert(newgraph("G1-base", "undirected"))
  -- New graph: create with default discipline
  g[2] = assert(newgraph("G2-base"))
  -- New graph: read from file
  g[3] = assert(graph.read("test/test_dat1.dot"))
  -- New graph: undirected
  g[4] = assert(newgraph("G4-base", "undirected"))


  -- Show properties
  debug("Properties:\n")
  for _,h in pairs(g) do
    debug("  %q: type=%s strict=%s directed=%s nnodes=%d nedges=%d id=%d\n",
	  h.name, type(h), tostring(h.isstrict), tostring(h.isdirected),
	  h.nnodes, h.nedges, h.id)
  end

  -- New graph: error in reading
  g[5], err = graph.read("___does__not__exist__")
  assert(g[5] == nil and type(err) == "string")

  -- Close all created graphs
  for _,v in pairs(g) do 
    debug("closing %q\n", v.name)
    assert(v:close()) 
  end
  intro("passed\n")
end

local function test_graph_properties(g)
  intro("Test graph: properties ...")
  -- Show properties
  intro("passed\n")
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
  collectgarbage(0)
  intro("passed\n")
  return h
end

local function test_graph_write()
  intro("Test graph: write ...")
  local h = assert(graph.read("test/test_dat1.dot"))
  -- Write to stdout
  gprint(h)
  -- Write to file
  local fn = os.tmpname()
  h:write(fn)
  -- Compare contents with reference
  local fref = io.open("test/ref.dot","r")
  local sref = fref:read("*a")
  local f = io.open(fn)
  local s = f:read("*a")
  assert(s == sref)
  fref:close()
  f:close()
  h:close()
  intro("passed\n")
end

local function test_graph_defattr()
  intro("Test graph: default attributes ...")
  local h = assert(graph.read("test/test_dat1.dot"))
  -- Getting initial default attributes
  debug("Initial default attributes:\n")
  debug("  graphs:\n")
  table.foreach(h:getgraphattr(), function(k,v) debug("    %s=%q\n", k, v) end)
  debug("  nodes:\n")
  table.foreach(h:getnodeattr(), function(k,v) debug("    %s=%q\n", k, v) end)
  debug("  edges:\n")
  table.foreach(h:getedgeattr(), function(k,v) debug("    %s=%q\n", k, v) end)
  -- Set attributes in parent
  debug("Default attributes after modifications:\n")
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
  debug("  h.edge.att = %q\n", h:defaults().edge.att)
  debug("  h2.edge.att = %q\n", h2:defaults().edge.att)
  debug("  h3.edge.att = %q\n", hh3:defaults().edge.att or "nil")
  debug("  h4.edge.att = %q\n", hh4:defaults().edge.att or "nil")

  -- Check: child shares defaults with parent
  assert(h:defaults().edge.att ~= h2:defaults().edge.att)
  assert(hh3:defaults().edge.att == hh4:defaults().edge.att)

  -- Check: defaults in different subtrees are not shared
  assert(h:defaults().edge.att ~= hh3:defaults().edge.att)

  h:close()
  h2:close()
  hh3:close()
  hh4:close()
  intro("passed\n")
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
  debug("root=%q mother=%q son=%q daughter=%q\n",
	root.name, mother.name, son.name, daughter.name)
  debug("root=%d mother=%d son=%d daughter=%d\n",
	root.id, mother.id, son.id, daughter.id)
  debug("%q: parent=%q root=%q\n", 
	mother.name, mother.parent.name, mother.root.name)
  debug("%q: parent=%q root=%q\n", 
	son.name, son.parent.name, son.root.name)
  debug("%q: parent=%q root=%q\n", 
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
  debug("Closing graph root ...\n")
  root:close()
  collectgarbage(0)
  intro("passed\n")
end

local function test_graph_iterate()
  intro("Test graph: graph iteration ...")
  local root = assert(graph.open("Root", "directed"))
  local mother = assert(root:subgraph("Mother"))
  local father = assert(root:subgraph("Father"))
  local son = assert(mother:subgraph("Son"))
  local daughter = assert(father:subgraph("Daughter"))

  -- Iteration 1
  debug("Iteration 1\n")
  local g = nil
  repeat
    g = root:getnext(g)
    if g then
      debug("  next of %s: %s\n", root.name, g.name)
    end
  until g == nil
  
  -- Iteration 2
  debug("Iteration 2\n")
  for g in root:walk() do
    debug(" %s\n", g.name)
    for h in g:walk() do
      debug("    %s\n", h.name)
    end
  end
  
  local g = {}
  for i = 1, 20 do
    g[i] = root:subgraph("sub-"..i)
  end
  
  -- Iteration 3
  debug("Iteration 3\n")
  local t = {}
  for g in root:walk() do 
    assert(g.parent == root) 
  end

  -- Closes all graphs
  root:close()
  collectgarbage(0)

  -- Iteration 4
  debug("Iteration 4\n")
  g = assert(graph.read("test/test_dat2.dot"))
  for v in g:walkgraphs() do
    debug(" %s\n", v.name)
    for w in v:walk() do
      debug("   %s\n", w.name)
    end
  end
  g:close()
  intro("passed\n")
end

local function test_graph_strict()
  intro("Test graph: strict ...")
  local g = graph.open("G", "strictdirected")
  local n1 = g:node("N1")
  local n2 = g:node("N2")
  local e1 = g:edge(n1, n2, "n1=>n2")
  local e2 = g:edge(n1, n2, "n1...n2")
  local e3 = g:edge(n2, n1, "n2=>n1")
  local e4 = g:edge(n1, n1, "n1=>n1")
  -- first edge must be present
  assert(g:findedge(n1, n2, "n1=>n2"))
  -- second edge must not be present
  assert(not g:findedge(n1, n2, "n1...n2"))
  g:close()
  intro("passed\n")
end

local function test_delete()
  intro("Test graph: delete object ...")
  local g = graph.open("G-delete")
  local sg = g:subgraph("SG-delete")
  local ssg = sg:subgraph("SSG-delete")
  local rv, err  = g:delete(sg)
  g:close()
  collectgarbage(0)
  intro("passed\n")
end

local function test_close()
  intro("Test graph: close ...")
  -- Closing graphs
  local g = graph.open("G-close")
  local sg1 = assert(g:subgraph("SG1-close"))
  local sg2 = assert(g:subgraph("SG2-close"))
  local ssg1 = assert(sg1:subgraph("SSG1-close"))
  assert(g:close())
  collectgarbage(0)
  intro("passed\n")
end

local function test_close_error()
  intro("Test graph: close with error ...")
  local rv, err = pcall(graph.close, 0)
  if not rv then debug("Error while closing: %q\n", err) end
  intro("passed\n")
end

local function test_node_base()
  intro("Test node: base node tests ...")
  local g = assert(graph.open("G-base"))

  -- Implict creation
  local n1 = assert(g:node("N1"))
  local n2 = assert(g:node("N2"))
  assert(g == n1.graph)
  assert(g == n2.graph)
  debug("n1 is %s\n", tostring(n1))
  debug("graph of n1 is %q\n", n1.graph.name)
  -- Lookup
  debug("n1 is %s\n", tostring(g:node("N1")))

  -- A second node
  local n2 = assert(g:node("N2"))
  debug("n2 is %s\n", tostring(n2))

  g:close()
  collectgarbage(0)
  intro("passed\n")
end

local function test_node_properties()
  intro("Test node: node properties ...")
  local g = assert(graph.open("G"))
  local sg = assert(g:subgraph("SG"))
  local n1 = assert(g:node("Na1"))
  local n2 = assert(g:node("Na2"))
  local n3 = assert(sg:node("Na3"))
  debug("nx.id = %d %d %d\n", n1.id, n2.id, n3.id)
  debug("nx.name = %q %q %q\n", n1.name, n2.name, n3.name)
  debug("nx.graph = %q %q %q\n", n1.graph.name, n2.graph.name, n3.graph.name)
  g:close()
  collectgarbage(0)
  intro("passed\n")
end

local function test_node_meta()
  intro("Test node: node metamethods ...")
  local g = assert(graph.open("G"))
  local n1 = assert(g:node("Nx1"))
  local n2 = assert(g:node("Nx1"))
  local n3 = assert(g:node("Nx3"))
  assert(n1 == n2)
  local e = assert(n1..n2)
  e.label = "n1 ==> n2"
  local e2 = assert(n3+n1)
  e.label = "n3 ==> n1"
  gprint(g)
  local rv = assert(n1:delete())
  -- Check whether userdata became invalid after node deletion
  local rv, err = pcall(function(n) return n.name end, n1)
  assert(rv == false)
  debug("check userdata invalidation: rv=%s err=%q\n", tostring(rv), err)
  g:close()
  debug("Collecting garbage ...\n")
  collectgarbage(0)
  intro("passed\n")
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
  debug("n.name=%q sb.name=%q\n", n.name, sn.name)
  debug("n.seq=%d sn.seq=%d\n", n.seq, sn.seq)
  debug("n.id=%d sn.id=%d\n", n.id, sn.id)
  gprint(g)
  n:delete()
  gprint(g)
  sg1:close()
  gprint(g)
  g:close()
  debug("Collecting garbage ...\n")
  collectgarbage(0)
  intro("passed\n")
end

local function test_node_degree()
  intro("Test node: node degree ...")
  local g = assert(graph.open("G-degree"))
  local n = assert(g:node("N1-degree"))
  local rv = assert(n:degree())
  debug("n:degree() = %d\n", rv)
  n:delete()
  g:close()
  collectgarbage(0)
  intro("passed\n")
end

local function test_node_degree2()
  intro("Test node: node degree ...")
  local g = graph.read("test/test_dat1.dot")
  local n = g:node("NE_WEST")
  debug("inputs: %d outputs: %d sum: %d\n", 
	n:degree("*i"), n:degree("*o"), n:degree("*a"))
  assert(n:degree("*i") == 3)
  assert(n:degree("*o") == 4)
  assert(n:degree() == 7)
  g:close()
  intro("passed\n")
end

local function test_node_iterate()
  intro("Test node: node iteration ...")
  local g = assert(graph.open("G-nodeiter"))
  local n = {}
  for i = 1, 20 do
    n[i] = assert(g:node("N"..i.."-nodeiter"))
  end

  -- Iteration 1
  debug("Iteration 1\n")
  local n = nil
  repeat
    n = g:nextnode(n)
    if n then
      debug("  next node of %s: %s %d\n", g.name, n.name, n.id)
    end
  until n == nil

  -- Iteration 2
  debug("Iteration 2\n")
  for n in g:walknodes() do
    debug("  %s\n", n.name)
  end
  g:close()

  -- Iteration 3
  debug("Iteration 3\n")
  g = assert(graph.read("test/test_dat1.dot"))
  for n in g:walknodes() do
    debug("  %s\n", n.name)
  end

  -- Iteration 4
  debug("Iteration 4\n")
  sg =assert(g:subgraph("SG"))
  sn1 = sg:node("SN1")
  sn2 = sg:node("SN2")
  for h in g:walk() do
    debug("  sg.name=%s\n", h.name)
    for n in h:walknodes() do
      debug("    %s\n", n.name)
    end
  end
  for n in g:walknodes() do
    debug("  %s\n", n.name)
  end
  g:close()
  collectgarbage(0)
  intro("passed\n")
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
  assert(e1 == e2)

  -- check head and tail
  assert(e1.tail == n1)
  assert(e1.head == n2)
  gprint(g)
  e1:delete()
  gprint(g)
  g:close()
  collectgarbage(0)
  intro("passed\n")
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
  collectgarbage(0)
  intro("passed\n")
end

local function test_edge_iterate()
  intro("Test edge: iteration ...")
  local g = assert(graph.read("test/test_dat1.dot"))
  -- Iteration 1
  if true then
  debug("Iteration 1\n")
  for n in g:walknodes() do
    debug("  node: %q\n", n.name)
    for e in n:walkedges() do
      debug("    edge %d %q %q %s\n", e.id, e.name, e.label, tostring(e))
      if e.tail then debug("    - tail: %q\n", e.tail.name) end
      if e.head then debug("    - head: %q\n", e.head.name) end
    end
  end
  end
  -- Iteration 2
  if true then
  debug("Iteration 2\n")
  for n in g:walknodes() do
    debug("  node: %q\n", n.name)
    for e in n:walkinputs() do
      debug("    input %q\n", "%"..e.name)
    end
    for e in n:walkoutputs() do
      debug("    output %q\n", "%"..e.name)
    end
  end
  end
  g:close()
  collectgarbage(0)
  intro("passed\n")
end

local function test_rename()
  intro("Test misc: renaming objects ...")
  local g = assert(graph.open("G"))
  local n = assert(g:node("N1"))
  local sg = assert(g:subgraph("SG"))
  gprint(g)
  local rv = assert(n:rename("N2") == "N1")
  local rv = assert(g:rename("G1") == "G")
  assert(n.name == "N2")
  assert(g.name == "G1")
  local rv = assert(g:rename("G2") == "G1")
  local rv = assert(sg:rename("SG2") == "SG")
  local rv = assert(n:rename())
  assert(n.name == "node@"..n.id)
  gprint(g)
  g:close()
  intro("passed\n");
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
  debug("1. shape: %q\n", n.shape)
  -- Failed get
--  rv, err = pcall(function () print(n.hello) end)
--  assert(rv == false)
--  debug("Test: failed get error: %q\n", err)
  -- Set
  n.shape = "circle"
  n.width = 5
  debug("2. shape: %s width: %d\n", n.shape, tonumber(n.width))
  gprint(g)
  -- Failed set
--  rv, err = pcall(function(key, val) n[key] = val end, "hello", "helloval")
--  assert(rv == false)
--  debug("Test: failed set error: %q\n", err)
  -- Implicit set
  local xg = graph.open("XG")
  local xn = xg:node("XN")
  xn.anysym = "anysym"
  local yn = xg:node("YN")
  debug("xn.anysym=%s\n", xn.anysym)
  assert(xn.anysym == "anysym")
  yn.anysym="none"
  debug("yn.anysym=%s\n", yn.anysym)
  assert(yn.anysym == "none")
  -- Just a try
  n.shape="box"
  n2.shape="circle"
  n2.color="red"
  n.width=1
  e.color = "green"
  e.label = "this is an edge"
  gshow(g)
  intro("passed\n");
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
		debug("node name: %q %q\n", self.name, param) 
		return param  
	      end
  e1.someval = 17.2
  e2.method = function(self, param) 
		debug("edge label: %q %q\n", self.label, param) 
		return param
	      end
  assert(n1:method("called n1") == "called n1")
  assert(e2:method("called e2") == "called e2")
  e2[t] = "hey"
  assert(e2[t] == "hey")
  assert(e2[n1.someval] == "hey")
  intro("passed\n");
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
  debug("Test: edge with nodes in different graphs: %q %q\n", tostring(res1), tostring(res2));
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
  debug("n1.id=%d n2.id=%d n3.id=%d\n", n1.id, n2.id, n3.id)
  debug("e1.graph=%q e2.graph=%q\n", e1.graph.name, e2.graph.name)
  gprint(g1)
  gprint(g2)
--  print(n1:type(), n3:type()) 
--  print(n1.shape, n3.shape)
--  print("g1:", g1:contains(n1), g1:contains(n2), g1:contains(n3), g1:node("N2", false), g1:node("N1"))
--  print("g2:", g2:contains(n1), g2:contains(n2), g2:contains(n3), g2:node("N2", false), g2:node("N1"))
  intro("passed\n");
end

local function test_find()
  intro("Test misc: finding  ...")
  local g = assert(graph.read("test/test_dat1.dot"))
  local tail = g:findnode("NE_EAST")
  local head = g:findnode("CE_WEST")
  debug("tail.name = %q\n", tail.name)
  debug("head.name = %q\n", head.name)
  local e = g:findedge(tail, head)
  assert(e.label == "out1 => in2")
  debug("e.label = %q\n", e.label)
  intro("passed\n");
end

local function test_layout()
  intro("Test layout: layout  ...")
  local g, t = graph.open("Gx")
  local e1 = g:edge{"n1", "n2", label="n1=>n2"}
  local fn = os.tmpname()
  assert(g:layout())
  debug("PLAIN:\n")
  assert(g:render("plain", fn))
  debug("XDOT:\n")
  assert(g:render("xdot", fn))
  os.remove(fn)
  g:close()
  intro("passed\n");
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
  intro("passed\n")
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
  debug("g.name=%q\n", g.name)
  local s = tostring
  for n in g:walknodes() do
    debug("  n.name=%q n.shape=%q n.color=%q\n", 
	   s(n.name), s(n.shape), s(n.color))
    for e in n:walkedges() do
      debug("    e.label=%q e.color=%q\n", s(e.label), s(e.color))
    end
  end
  assert(g.nnodes == 6)
  assert(g.nedges == 6)
  debug("%d nodes\n", g.nnodes)
  debug("%d edges\n", g.nedges)
  gprint(g)
  gshow(g)
  g:close()
  intro("passed\n")
end

local function test_xx()
  local g = graph.open("G")
  for i = 1,500 do
    local n = g:node("A"..i)
    g:node("B"..i):edge(n, "E"..i)
  end
  g:write("x3.dot")
end

local tests = {
  -- Graph tests
  test_graph_base,
  test_graph_properties,
  test_graph_meta,
  test_graph_write,
  test_graph_subgraph,
  test_graph_defattr,
  test_graph_iterate,
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
  test_rename,
  test_attr,
  test_contains,
  test_find,
  test_anyattrib,
  test_cluster,
  test_graphtab,
  -- Layout and rendering
  test_layout,
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
debug("Collecting garbage ...\n")
collectgarbage(0)
--print(">>> ", gcinfo())

info("FINISHED\n")
