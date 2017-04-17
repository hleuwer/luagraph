--=========================================================================*\
-- LuaGRAPH toolkit
-- Graph support for Lua.
-- Herbert Leuwer
-- 30-7-2006, 30-12-2016, 01/2017
--
-- Lua module "graph.lua"
--=========================================================================*\
local graph = require "graph.core"
local string = require "string"
local table = require "table"

local type, getmetatable, pairs, ipairs, assert = 
  _G.type, _G.getmetatable, _G.pairs, _G.ipairs, _G.assert

module("graph", package.seeall)
--
-- Overloaded graph.open(), graph.read()
--
local _open = open
local _read = read

--==============================================================================
-- Constants
--==============================================================================
--
-- Default attributes
--
local defattr = {
  graph={
  },
  node={
  },
  edge={
  }
}

OUTPUTFORMATS = {
   {format = "canon", descr = "DOT pretty"},
   {format = "dot",   descr = "DOT"},
   {format = "xdot",  descr = "extended DOT"},
   {format = "cmap",  descr = "Client-side imagemap (deprecated)"},
   {format = "dia",   descr = "Dia format"},
   {format = "eps",   descr = "Encapsulated PostScript"},
   {format = "fig",   descr = "FIG"},
   {format = "gd",    descr = "Graphics Draw format"},
   {format = "gd2",   descr = "Graphics Draw 2 format"},
   {format = "gif",   descr = "Graphics Interchage Format"},
   {format = "gtk",   descr = "Antialiased image using a GTK 2.0 canvas"},
   {format = "hpgl",  descr = "Hewlett Packard Graphics Language HP-GL/2"},
   {format = "ico",   descr = "Icon Image File Format"},
   {format = "imap",  descr = "Server-side and client-side imagemaps"},
   {format = "imap_np", descr = "Server-side and client-side imagemaps"},
   {format = "cmapx", descr = "Server-side and client-side imagemaps"},
   {format = "cmapx_np", descr = "Server-side and client-side imagemaps"},
   {format = "ismap", descr = "Server-side imagemap (deprecated)"},
   {format = "jpg",   descr = "JPEG (deprecated - 8 May 2006 - will no longer be supported)"},
   {format = "jpeg",  descr = "JPEG (deprecated - 8 May 2006 - will no longer be supported)"},
   {format = "mif",   descr = "FrameMaker MIF format"},
   {format = "mp",    descr = "MetaPost"},
   {format = "pcl",   descr = "Hewlett Packard Printer Control"},
   {format = "pdf",   descr = "Portable Document Format (PDF)"},
   {format = "pic",   descr = "Autodesk PIC"},
   {format = "plain", descr = "Simple text format"},
   {format = "plain-ext", descr = "Extended text format"},
   {format = "png",   descr = "Portable Network Graphics format"},
   {format = "ps",    descr = "PostScript"},
   {format = "ps2",   descr = "PostScript for PDF"},
   {format = "svg",   descr = "Scalable Vector Graphics"},
   {format = "svgz",  descr = "Scalable Vector Graphics"},
   {format = "vml",   descr = "Vector Markup Language (VML)"},
   {format = "vmlz",  descr = "Vector Markup Language (VML) - compressed"},
   {format = "vrml",  descr = "VRML"},
   {format = "vtx",   descr ="Visual Thought format"},
   {format = "wbmp",  descr = "Wireless BitMap format"},
   {format = "xlib",  descr = "Xlib canvas"},
}
--==============================================================================
-- Utilities
--==============================================================================

--------------------------------------------------------------------------------
-- Iterator over non-numeric indices of a table
--------------------------------------------------------------------------------
local function npairs(t)
  return function(t, prev)
	   k,v = next(t, prev)
	   while type(k) == "number" do
	     k,v = next(t, prev)
	     prev = k
	   end
	   return k,v
	 end, t, nil
end

--------------------------------------------------------------------------------
-- Copy attributes from parameter table
--------------------------------------------------------------------------------
local function attribs(params)
  local t = {}
  for k,v in npairs(params) do
    t[k] = v
  end
  return t
end

--------------------------------------------------------------------------------
-- Get default attributes for the given object (type)
--------------------------------------------------------------------------------
local function getattrib(self)
  local t = {}
  local defined = self.graph:defaults()[self:type()]
  for k,v in pairs(defined) do 
    t[k] = self:rawget(k)
  end
  return t
end

--------------------------------------------------------------------------------
-- Add a method to an object (node, graph, edge)
--------------------------------------------------------------------------------
local function addmethod(self, name, func)
  local mt = getmetatable(self)
  if not mt or mt[name] then return end
  mt[name] = func
end

-- A forward declaration
local _overload

--==============================================================================
-- Advanced implementations
--==============================================================================
--------------------------------------------------------------------------------
-- Subgraph creation
-- sg = g:subgraph{"name", ATTR, ..., ATTR, nocreate}
-- sg = g:subgraph("name", {ATTR, ..., ATTR}, nocreate)
--------------------------------------------------------------------------------
local function _subgraph(self, ...)
  local arg = {...}
  local name
  local attr = {}
  if type(arg[1]) == "table" then
    name = arg[1][1]
    nocreate = arg[1][2]
    for k,v in npairs(arg[1]) do
      if type(v) == "table" then
	attr[k] = v
      else
	attr.graph[k] = v
      end
    end
  elseif type(arg[1]) == "string" then
    name = arg[1]
    attr = arg[2] or {}
    nocreate = arg[3]
  else
    error("missing subgraph name")
  end
  local g, err = self:__subgraph(name)
  _overload(g)
  g:declare(defattr)
  g:declare(attr)
  for k,v in pairs(attr) do
    if type(v) == "table" then
      g:declare(v)
    else
      g:declare{k=v}
    end
  end
  return g, err
end

--------------------------------------------------------------------------------
-- Cluster creation
-- c = g:cluster{"name", ATTR, ..., ATTR, nocreate}
-- c = g:cluster("name", {ATTR, ..., ATTR}, nocreate}
--------------------------------------------------------------------------------
local function _cluster(self, ...)
  local arg = {...}
  if type(arg[1]) == "table" then
    arg[1][1] = "cluster_"..arg[1][1]
  elseif type(arg[1]) == "string" then
    arg[1] = "cluster_"..arg[1]
  else
    error("missing cluster name")
  end
  return _subgraph(self, unpack(arg))
end

--------------------------------------------------------------------------------
-- Edge creation
-- e = g:edge{node, .., "node", ATTR, ..., ATTR}
--     with ATTR: key = value
--          node: object or node name
-- e = g:edge(tail, head, "label", nocreate)
-- e = g:edge("tail", "head", "label", nocreate)
--------------------------------------------------------------------------------
local function _edge(self, ...)
   local arg = {...}
   local nodes, edges = {}, {}
   local attr
   local node = {}
   local last
   if type(arg[1]) == "table" then
      attr = attribs(arg[1])
      -- create the edges
      for i, v in ipairs(arg[1]) do
         -- we must care about ports here:
         node = {}
         if type(v) == "string" then
            string.gsub(v, "^(%w+):*(%w*):*(%w*)", function(u, v, w)
                           node.name = u
                           node.port = v
                           node.compass = w
            end)
            node.node = self:__node(node.name)
         elseif type(v) == "userdata" then
            node.node = v
         else
            error("wrong node type")
         end
         table.insert(nodes, node.node)
         if i > 1 then
            -- Create edges and set attributes to each edge
            local e, tail_or_err, head = self:__edge(last.node, node.node)
            if not e then
               return nil, err
            end
            if last.port then e.tailport = last.port end
            if node.port then e.headport = node.port end
            addmethod(e, "getattrib", getattrib)
            for k,v in pairs(attr) do
               e[k] = v
            end
            table.insert(edges, e)
         end
         last = node
      end
      return edges, nodes
   elseif type(arg[1]) == "string" or type(arg[1]) == "userdata" then
      local node = {[1]={},[2]={}}
      for i = 1,2 do
         local v = arg[i]
         -- we must care about ports here:
         if type(v) == "string" then
            string.gsub(v, "^(%w+):*(%w*):*(%w*)", function(u, v, w)
                           node[i].name = u
                           node[i].port = v
                           node[i].compass = w
            end)
            arg[i] = self:__node(node[i].name)
         end
      end
      local e, tail_or_err, head = self:__edge(unpack(arg))
      if not e then return e, tail_or_err end
      if node[1].port then e.tailport=node[1].port end
      if node[2].port then e.headport=node[2].port end
      addmethod(e, "getattrib", getattrib)
      return e, tail_or_err, head
   else
      error("invalid edge declaration")
   end
end

--------------------------------------------------------------------------------
-- Node creation.
-- n = g:node{"name", ATTR, ..., ATTR, nocreate}
-- n = g:node("name", {ATTR, ..., ATTR}, nocreate)
--     with ATTR: key = value
--------------------------------------------------------------------------------
local function _node(self, ...)
  local arg = {...}
  local name, attr
  if type(arg[1]) == "table" then
    name = arg[1][1]
    nocreate = arg[1][2]
    attr = attribs(arg[1])
  elseif type(arg[1]) == "string" then
    name = arg[1]
    attr = arg[2] or {}
    nocreate = arg[3]
  else
    error("missing node name")
  end
  local n = self:__node(name, nocreate)
  for k,v in pairs(attr) do
    n[k] = v
  end
  addmethod(n, "getattrib", getattrib)
  return n
end

--------------------------------------------------------------------------------
-- Returns a function that creates horizontal oriented fields
-- gr.hbox{"field", gr.vbox{...}, "field", gr.vbox{...}}
--------------------------------------------------------------------------------
function hbox(t)
  return function(dir)
	   local ss = ""
	   for k, v in ipairs(t) do
	     if type(v) == "function" then
	       ss = ss .. v("h") .. "|"
	     elseif type(v) == "string" then
	       ss = ss .. v .. "|"
	     end
	   end
	   ss = string.sub(ss, 1, -2)
	   if dir ~= "h" then ss = "{" .. ss .. "}" end
	   return ss
	 end
end

--------------------------------------------------------------------------------
-- Returns a function that creates vertical oriented fields
-- gr.vbox{"field", gr.hbox{...}, "field", gr.hbox{...}}
--------------------------------------------------------------------------------
function vbox(t)
  return function(dir)
	   local ss = ""
	   for k, v in ipairs(t) do
	     if type(v) == "function" then
	       ss = ss .. v("v") .. "|"
	     elseif type(v) == "string" then
	       ss = ss .. v .. "|"
	     end
	   end
	   ss = string.sub(ss, 1, -2)
	   if dir ~= "v" then ss = "{" .. ss .. "}" end
	   return ss
	 end
end

--------------------------------------------------------------------------------
-- Record
-- n = g:record{"n1", vbox{...}, ATTR, ...,ATTR, nocreate}
-- n = g:record("n1", vbox{"name", hbox{...}, ...}, {ATTR, ..., ATTR}, nocreate) 
--------------------------------------------------------------------------------
local function _record(self, ...)
  local name, attr, labelfunc
  local arg = {...}
  if type(arg[1]) == "table" then
    name = arg[1][1]
    lfunc = arg[1][2]
    nocreate = arg[1][3]
    attr = attribs(arg[1])
  elseif type(arg[1]) == "string" then
    name = arg[1]
    lfunc = arg[2]
    attr = arg[3] or {}
    nocreate = arg[4]
  else
    error("missing record name")
  end
  assert(type(lfunc) == "function", "missing record struct")
  local label = lfunc("h")
  local n = self:__node(name, nocreate)
  n.shape = "record"
  n.label = label
  for k,v in pairs(attr) do
    n[k] = v
  end
  addmethod(n, "getattrib", getattrib)
  return n
end

--==============================================================================
-- Additional graph methods
--==============================================================================
--------------------------------------------------------------------------------
-- Find a node
--------------------------------------------------------------------------------
local function findnode(self, name)
  if not name then 
    return nil, "not found" 
  end
  return self:__node(name, true)
end

--------------------------------------------------------------------------------
-- Find an edge
--------------------------------------------------------------------------------
local function findedge(self, tail, head, label)
  return self:_findedge(tail, head, label)
end

--------------------------------------------------------------------------------
-- Find a subgraph
--------------------------------------------------------------------------------
local function findsubgraph(self, tail, head)
  return self:_subgraph(tail, head, false)
end

--------------------------------------------------------------------------------
-- Show graph with dotty
--------------------------------------------------------------------------------
local function showdotty(self, doit)
  doit = doit or true
  if doit == true then
    local fn
    print("SYSTEM:", self._SYSTEM)
    if graph._SYSTEM == "Win32" then
      fn = "tmpfile.dot"
    else
      fn = os.tmpname()..".dot"
    end
    local rv = self:write(fn)
    print("result: "..rv)
    if graph._SYSTEM == "Win32" then
  	  rv = os.execute("gvedit "..fn)
  	else
      rv = os.execute("dotty "..fn)
    end
    os.remove(fn)
    return rv
  end
end

--------------------------------------------------------------------------------
-- Show graph with dotty
--------------------------------------------------------------------------------
local function show(self, printdot)
   local printdot = printdot or false
   self:showdotty(doit)
   if printdot == true then
      self.render(nil, "dot", "out.dot")
   end
   return true
end

function gvversion()
   return io.popen('dot -V 2>&1 | cut -d " " -f 5'):read()
end
--------------------------------------------------------------------------------
-- Partially overload C-defined metamethods
-- Note: keep this function behind the overloading ones
--------------------------------------------------------------------------------
local function overload(g)
  local mt = getmetatable(g)
  if mt.overloaded == true then return end
  mt.__edge = mt.edge
  mt.__node = mt.node
  mt.__subgraph = mt.subgraph
  mt.edge = _edge
  mt.node = _node
  mt.subgraph = _subgraph
  mt.cluster = _cluster
  mt.record = _record
  mt._findedge = mt.findedge
  mt.findedge = findedge
  mt.findnode = findnode
  mt.findsubgraph = findsubgraph
  mt.showdotty = showdotty
  mt.show = show
  mt.overloaded = true
end

-- resolve forward declaration
_overload = overload

--------------------------------------------------------------------------------
-- Advanced implementation of graph.open() - Main entry
--------------------------------------------------------------------------------
function open(...)
  local arg = {...}
  local name
  local attr = {}
  local g
  if type(arg[1]) == "string" then
    -- Syntax 1: graph.open("NAME","directed", {graph={ATTR,..}, edge={ATTR,..}, node={ATTR,..}})
    name = arg[1]
    kind = arg[2]
    attr = arg[3] or {}
  
  elseif type(arg[1]) == "table" then
    -- Syntax 2: graph.open{"NAME", "kind", ATTR, ATTR}
    name = arg[1][1]
    kind = arg[1][2]
    for k,v in npairs(arg[1]) do
      if type(v) == "table" then
	attr[k] = v
      else
	attr.graph[k] = v
      end
    end
  end
  
  -- Create the graph and declare attributes
  g = _open(name, kind)
  g:declare(defattr)
  g:declare(attr)

  -- adjust methods
  overload(g)

  return g
end

--------------------------------------------------------------------------------
-- Advanced implementation of graph.read(file, doscan) - Main entry
--------------------------------------------------------------------------------
local function getedge(n, elist)
  for e in n:walkedges() do
    if not elist[e] then
      table.insert(elist, e)
    end
  end
end

local function getnode(g, nlist, elist)
  for n in g:walknodes() do
    if elist then
      getedge(n, elist)
    end
    table.insert(nlist, n)
  end
end

local function getsubg(g, glist)
  for sg in g:walk() do
    local t = {sg, graph = {}, node = {}, edge = {}}
    table.insert(glist, t)
    getsubg(sg, t.graph)
    getnode(sg, t.node, t.edge)
  end
end

function read(fname, doscan)
  local g, err = _read(fname)
  if not g then return g, err end
  overload(g)
  if doscan == true then
    local t = {
      g,
      graph={},
      node={},
      edge={}
    }
    getsubg(g, t.graph)
    getnode(g, t.node, t.edge)
    return g, t
  else
    return g
  end
end

--==============================================================================
-- Utilities to create a graph as Lua table.
-- Each of the following functions returns a constructor function for
-- the corresponding object. 
--==============================================================================

--------------------------------------------------------------------------------
-- Create a directed graph.
--------------------------------------------------------------------------------
local function _graph(t, typ)
  local name = t.name or t[1]
  if type(t[1]) == "string" then 
    table.remove(t, 1)
  end
  assert(name, "missing name")
  local g = open(name, typ)
  for k, v in npairs(t) do
    if type(v) == "table" then
      g:declare{[k] = v} 
    elseif type(v) == "string" then
      g[k] = v
    else
      error("invalid attribute type")
    end
  end
  for k,v in ipairs(t) do
    if type(v) == "function" then
      v(g)
    else
      error("invalid graph attribute")
    end
  end
  return g
end

function digraph(t)
  return _graph(t, "directed")
end

function strictdigraph(t)
  return _graph(t, "strictdirected")
end

function graph.graph(t)
  return _graph(t, "undirected")
end

function strictgraph(t)
  return _graph(t, "strict")
end

--------------------------------------------------------------------------------
-- Create subgraph
--------------------------------------------------------------------------------
function subgraph(t)
  return function(g) 
	   assert(type(t[1]) == "string", "missing subgraph name")
	   local sg = g:subgraph(t[1])
	   table.remove(t, 1)
	   for k, v in npairs(t) do
	     if type(v) == "string" then
	       sg[k] = v
	     else
	       error("invalid attribute type")
	     end
	   end
	   for k,v in ipairs(t) do
	     if type(v) == "function" then
	       v(sg)
	     else
	       error("invalid attribute type")
	     end
	   end
	   return sg
	 end
end

--------------------------------------------------------------------------------
-- Create cluster
--------------------------------------------------------------------------------
function cluster(t)
  assert(type(t[1]) == "string", "missing cluster name")
  t[1] = "cluster_"..t[1]
  return subgraph(t)
end

--------------------------------------------------------------------------------
-- Create node
--------------------------------------------------------------------------------
function node(t)
  return function(g) 
	   return g:node(t) 
	 end
end

--------------------------------------------------------------------------------
-- Create record
--------------------------------------------------------------------------------
function record(t)
  return function(g) 
	   return g:record(t) 
	 end
end
--------------------------------------------------------------------------------
-- Create edge
--------------------------------------------------------------------------------
function edge(t)
  return function(g) 
	   local p = {}
	   for k,v in pairs(t) do
	     if type(v) == "function" then
	       table.insert(p, v(g))
	     elseif type(v) == "string" or type(v) == "userdata" then
	       p[k] = v
	     else 
	       error("invalid parameter")
	     end
	   end
	   return g:edge(p) 
	 end
end

return graph
