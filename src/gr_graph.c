/*=======================rap==================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006, 30-12-2016, 01/2017
 *
 * Graph related functionality.
 *
\*=========================================================================*/

/*=========================================================================*\
 * Includes
\*=========================================================================*/
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "gr_graph.h"
#include "graphviz/cdt.h"
#include "graphviz/gvplugin.h"

/*=========================================================================*\
 * Defines
\*=========================================================================*/
#define MYNAME "graph"
#define MYVERSION "2.0.0"
#if defined(_WIN32) || defined(WIN32)
#define MYGVVERSION "2.38"
#define MYSYSTEM "Win32"
#else
#define MYSYSTEM "other"
#define MYGVVERSION GVVERSION
#endif
#define MYCOPYRIGHT "Copyright (C) 2006-2017, Herbert Leuwer "
#define MYDESCRIPTION "LuaGRAPH is a library for creating, manipulating and rendering GRAPHs (based on the GRAPHVIZ library cgraph)."

/*=========================================================================*\
 * Prototypes
\*=========================================================================*/
static int gr_open(lua_State *L);
static int gr_close(lua_State *L);
static int gr_nameof(lua_State *L);
static int gr_write(lua_State *L);
static int gr_read(lua_State *L);
static int gr_memread(lua_State *L);
static int gr_isstrict(lua_State *L);
static int gr_isdirected(lua_State *L);
static int gr_nnodes(lua_State *L);
static int gr_nedges(lua_State *L);
static int gr_subgraph(lua_State *L);
static int gr_root(lua_State *L);
static int gr_parent(lua_State *L);
static int gr_equal(lua_State *L);
static int gr_getnext(lua_State *L);
static int gr_walk(lua_State *L);
static int gr_id(lua_State *L);
static int gr_isroot(lua_State *L);
static int gr_getgraphattr(lua_State *L);
static int gr_getnodeattr(lua_State *L);
static int gr_getedgeattr(lua_State *L);
static int gr_getattr(lua_State *L);
static int gr_setgraphattr(lua_State *L);
static int gr_setnodeattr(lua_State *L);
static int gr_setedgeattr(lua_State *L);
static int gr_setattr(lua_State *L);
static int gr_delete(lua_State *L);
static int gr_node(lua_State *L);
static int gr_edge(lua_State *L);
static int gr_findedge(lua_State *L);
static int gr_idnode(lua_State *L);
static int gr_nextnode(lua_State *L);
static int gr_walknodes(lua_State *L);
static int gr_graphof(lua_State *L);
static int gr_contains(lua_State *L);
static int gr_layout(lua_State *L);
static int gr_freelayout(lua_State *L);
static int gr_render(lua_State *L);
static int gr_plugins(lua_State *L);
static int gr_tostring(lua_State *L);

/*=========================================================================*\
 * Data
\*=========================================================================*/
GVC_t *gvc = NULL;

#ifdef USE_BUILTIN_PLUGINS
extern gvplugin_library_t gvplugin_dot_layout_LTX_library;
extern gvplugin_library_t gvplugin_neato_layout_LTX_library;
extern gvplugin_library_t gvplugin_gd_LTX_library;
extern gvplugin_library_t gvplugin_pango_LTX_library;
#if 0
extern gvplugin_library_t gvplugin_webp_LTX_library;
#endif
extern gvplugin_library_t gvplugin_core_LTX_library;

lt_symlist_t lt_preloaded_symbols[] = {
  	{ "gvplugin_dot_layout_LTX_library", (void*)(&gvplugin_dot_layout_LTX_library) },
	{ "gvplugin_neato_layout_LTX_library", (void*)(&gvplugin_neato_layout_LTX_library) },
	{ "gvplugin_pango_LTX_library", (void*)(&gvplugin_pango_LTX_library) },
#if 0
	{ "gvplugin_webp_LTX_library", (void*)(&gvplugin_webp_LTX_library) },
#endif
	{ "gvplugin_gd_LTX_library", (void*)(&gvplugin_gd_LTX_library) },
	{ "gvplugin_core_LTX_library", (void*)(&gvplugin_core_LTX_library) },
	{ 0, 0 }
};
#else
  lt_symlist_t lt_preloaded_symbols[] = {
  { 0, 0 }
};
#endif
/*
 * Base library functions
 */
static const luaL_Reg funcs[] = {
  {"open", gr_open},
  {"close", gr_close},
  {"read", gr_read},
  {"memread", gr_memread},
  {"equal", gr_equal},
  {NULL, NULL}
};

/*
 * Graph object methods
 */
static const luaL_Reg reg_methods[] = {
  {"close", gr_close},
  {"write", gr_write},
  {"subgraph", gr_subgraph},
  {"getnext", gr_getnext},
  {"nextgraph", gr_getnext},
  {"walk", gr_walk},
  {"walkgraphs", gr_walk},
  {"getgraphattr", gr_getgraphattr},
  {"getnodeattr", gr_getnodeattr},
  {"getedgeattr", gr_getedgeattr},
  {"getattr", gr_getattr},
  {"defaults", gr_getattr},
  {"setgraphattr", gr_setgraphattr},
  {"setnodeattr", gr_setnodeattr},
  {"setedgeattr", gr_setedgeattr},
  {"setattr", gr_setattr},
  {"declare", gr_setattr},
  {"delete", gr_delete},
  {"node", gr_node},
  {"edge", gr_edge},
  {"findedge", gr_findedge},
  {"idnode", gr_idnode},
  {"nextnode", gr_nextnode},
  {"walknodes", gr_walknodes},
  {"type", get_object_type},
  {"contains", gr_contains},
  {"layout", gr_layout},
  {"freelayout", gr_freelayout},
  {"render", gr_render},
  {"rawget", getval},
  {NULL, NULL}
};

/*
 * Read-only members
 */
static const luaL_Reg reg_rmembers[] = {
  {"nnodes", gr_nnodes},
  {"nedges", gr_nedges},
  {"name", gr_nameof},
  {"status", gr_status},
  {"root", gr_root},
  {"parent", gr_parent},
  {"isstrict", gr_isstrict},
  {"isdirected", gr_isdirected},
  {"isroot", gr_isroot},
  {"graph", gr_graphof},
  {"id", gr_id},
  {NULL, NULL}
};

/*
 * Standard metamethods
 */
static const luaL_Reg reg_metamethods[] = {
  {"__gc", gr_collect},
  {"__eq", gr_equal},
  {"__newindex", object_newindex_handler},
  {"__tostring", gr_tostring},
  {NULL, NULL}
};

/*
 * Event callback functions - used for registring/unregstring proxy objects
 * in Lua registry.
 */
static const struct Agcbdisc_s disc = {
  { cb_insert, cb_modify, cb_delete}, /* graph callbacks */
  { cb_insert, cb_modify, cb_delete}, /* node callbacks */
  { cb_insert, cb_modify, cb_delete}, /* edge callbacks */
};

/*=========================================================================*\
 * Functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
 * Utility Functions
\*-------------------------------------------------------------------------*/

/*
 * Create a new Lua userdata object and configure metatable.
 */
static int new_graph(lua_State *L){
  return   new_object(L, "graph", reg_rmembers, reg_methods,
                      reg_metamethods, object_index_handler);
}

#define DEMAND_LOADING (1)

/*
 * Layout a graph using given engine.
 */
static int gv_layout(Agraph_t *g, const char *engine)
{
  int rv = gvLayout(gvc, g, engine);
  if (rv != 0)
    return GR_ERROR;
  return GR_SUCCESS;
}

static int gv_free_layout(Agraph_t *g)
{
  int rv = gvFreeLayout(gvc, g);
  if (rv != 0)
    return GR_ERROR;
  return GR_SUCCESS;
}

/*
 * Render layouted graph into a file given by file handle
 * using given format.
 */
static int gv_render(Agraph_t *g, const char *fmt, FILE *fout)
{
int rv = gvRender(gvc, g, fmt, fout);
  if (rv != 0)
    return GR_ERROR;
  return GR_SUCCESS;      
}
/*
 * Render layouted graph into a file given by file name using given
 * format.
 */
static int gv_render_file(Agraph_t *g, const char *fmt, const char *fname)
{
  int rv = gvRenderFilename(gvc, g, fmt, fname);
  if (rv != 0)
    return GR_ERROR;
  return GR_SUCCESS;
}

/*
 * Gets attributes for a specific object type into a table rt.
 * If no attributes are defined, the function returns an empty table.
 * Lua entry stack: ud
 * Lua exit stack:  ud, rt
 */
static int getattr(lua_State *L, int kind)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  Agsym_t *sym = NULL;

  lua_newtable(L);    /* rt */
  switch (kind){
  case AGRAPH:
  case AGNODE:
  case AGEDGE:
    while ((sym = agnxtattr(ud->g, kind, sym)) != NULL){
      lua_pushstring(L, sym->name);    /* rt, key */
      lua_pushstring(L, sym->defval);  /* rt, key, value */
      lua_settable(L, -3);             /* rt */
    }
  return 1;
    break;
  default:
    return luaL_error(L, "invalid kind");
    break;
  }
}

/*
  * Sets attributes for a specific object type from a table.
  * Lua entry stack: ud, t
  * Lua exit stack:  ud, t
  */
static int setattr(lua_State *L, int kind)
{
  int n = 0;
  char *key;
  char *value;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  
  /* Parameter check */
  luaL_checktype(L, -1, LUA_TTABLE);  
  
  /* traverse through all key/value pairs of given table */
  lua_pushnil(L);                /* t, nil */
  while (lua_next(L, -2)){       /* t, key, val */
    if (!lua_isstring(L, -2)){
      lua_pop(L, 2);             /* t */
      luaL_error(L, "invalid key");
      return 0;
    }
    key = (char *) lua_tostring(L, -2);
    value = (char *) lua_tostring(L, -1);
    if (value == NULL){
      lua_pop(L,2); 
      luaL_error(L, "invalid value");
      return 0;
    }
    lua_pop(L, 1); /* t, key */
    n++;
    agattr(ud->g, kind, key, value);
  }
  lua_pushnumber(L, n);
  return 1;
}

/*-------------------------------------------------------------------------* \
 * Function: g, err = graph.open(name [,kind])
 * Create a new graph
 * Returns graph userdata.
 * Example:
 * g, err = graph.open(name[, kind])
 \*-------------------------------------------------------------------------*/
static int gr_open(lua_State *L)
{
  gr_graph_t *ud;
  Agdesc_t kind = Agdirected;
  char *name = (char *) luaL_checkstring(L, 1);
  char *skind = (char *) luaL_optstring(L, 2, "directed");
  
  if (!strcmp(skind, "directed")) {
    kind = Agdirected;
  } else if (!strcmp(skind, "strictdirected")){
    kind = Agstrictdirected;
  } else if (!strcmp(skind, "undirected")) {
    kind = Agundirected;
  } else if (!strcmp(skind, "strictundirected")) {
    kind = Agstrictundirected;
  } else {
    luaL_error(L, "invalid graph attribute");
  }

  /* Create a userdata */
  ud = lua_newuserdata(L, sizeof(gr_graph_t));    /* ud */
  if (!(ud->g = agopen(name, kind, &AgDefaultDisc))){
    lua_pushnil(L);
    lua_pushstring(L, "open failed");
    return 2;
  }
  ud->name = strdup(name);
  ud->type = AGRAPH;
  ud->status = ALIVE;
  /* We need a few default fields */
  if (!agattr(ud->g, AGEDGE, "label", "") ||
      !agattr(ud->g, AGRAPH, "__attrib__", "") ||
      !agattr(ud->g, AGEDGE, "__attrib__", "") ||
      !agattr(ud->g, AGNODE, "__attrib__", "")){
    luaL_error(L, "declaration failed");
    return 0;
  }

  /* We set callbacks only in root graph */
  if (ud->g == agroot(ud->g)){
    agpushdisc(ud->g, (struct Agcbdisc_s *)&disc, L);
  }

  set_object(L, ud->g);
  return new_graph(L);
}

/*-------------------------------------------------------------------------*\
 * Method: g.close(self)
 * Close a graph
 * Returns always 0.
 * Example:
 * rv = graph.close(g)
 * rv = g:close()
\*-------------------------------------------------------------------------*/
static int gr_close(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);

  if (ud->g != NULL){

    if (ud->status == ALIVE) {
      /* Delete the graph, if it still exists */
      TRACE("   g:close(): graph: ud=%p '%s' ptr=%p type=%d (%s %d)\n", 
            (void *) ud, agnameof(ud->g), (void *)ud->g, AGTYPE(ud->g), __FILE__, __LINE__);
      agclose(ud->g);
    }
  } else {
    TRACE("   g:close(): graph: ud=%p already closed (%s %d)\n", (void *) ud, __FILE__, __LINE__);
  }
  lua_pushnumber(L, 0);
  return 1;

}

/*-------------------------------------------------------------------------*\
 * Property: name [string]
 * Returns the name of a graph.
 * Example:
 * name = g.name
\*-------------------------------------------------------------------------*/
static int gr_nameof(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  if (ud->status != ALIVE){
    luaL_error(L, "deleted");
    return 0;
  }
  lua_pushstring(L, agnameof(ud->g));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: isstrict [boolean]
 * Returns true is graph is strict, otherwise it returns false.
 * Example:
 * rv = g.isstrict
\*-------------------------------------------------------------------------*/
static int gr_isstrict(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  lua_pushboolean(L, agisstrict(ud->g));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: isdirected [boolean]
 * Returns true if the graph is directed, otherwise it returns false
 * Example:
 * rv = g.isdirected 
\*-------------------------------------------------------------------------*/
static int gr_isdirected(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  lua_pushboolean(L, agisdirected(ud->g));
  return 1;
}

/*-------------------------------------------------------------------------* \
 * Read a graph from a string
 * Returns graph userdata.
 * Example:
 * g, err = graph.memread(str)
\*-------------------------------------------------------------------------*/
static int gr_memread(lua_State *L)
{
  gr_graph_t *ud;
  char *str = (char *)lua_tostring(L, 1);

  ud = lua_newuserdata(L, sizeof(gr_graph_t));
  if (!(ud->g = agmemread(str))){
    lua_pushnil(L);
    lua_pushstring(L, "agread failed");
    return 2;
  }
  ud->name = strdup(agnameof(ud->g));
  ud->type = AGRAPH;
  ud->status = ALIVE;
  
  /* We set callbacks only in root graph */
  if (ud->g == agroot(ud->g)){
    agpushdisc(ud->g, (struct Agcbdisc_s *)&disc, L);
  }
  set_object(L, ud->g);
  return new_graph(L);
}

/*-------------------------------------------------------------------------* \
 * Read a graph from a file; "stdin" reads from STDIN.
 * Returns graph userdata.
 * Example:
 * g, err = graph.read(filename)
\*-------------------------------------------------------------------------*/
static int gr_read(lua_State *L)
{
  gr_graph_t *ud;
  FILE *fin;
  char *fname = (char *)luaL_optstring(L, 1, "stdin");

  if (!strcmp(fname, "stdin")){
    fin = stdin;
  } else {
    fin = fopen(fname, "r");
  }
  if (!fin){
    lua_pushnil(L);
    lua_pushstring(L, "fopen failed");
    return 2;
  }
  /* Create a userdata */
  ud = lua_newuserdata(L, sizeof(gr_graph_t));
  if (!(ud->g = agread(fin, NIL(Agdisc_t*)))){
    fclose(fin);
    lua_pushnil(L);
    lua_pushstring(L, "agread failed");
    return 2;
  }
  fclose(fin);
  ud->name = strdup(agnameof(ud->g));
  ud->type = AGRAPH;
  ud->status = ALIVE;
  
  /* We set callbacks only in root graph */
  if (ud->g == agroot(ud->g)){
    agpushdisc(ud->g, (struct Agcbdisc_s *)&disc, L);
  }
  set_object(L, ud->g);
  return new_graph(L);
}

/*-------------------------------------------------------------------------*\
 * Method: g.write(self, filename) 
 * Write a graph into a file.
 * Returns 1 on success, nil plus error message on failure.
 * rv, err = g:write(filename)
\*-------------------------------------------------------------------------*/
static int gr_write(lua_State *L)
{
  int rv;
  FILE *fout;
  int stdflag = 0;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *fname = (char*) luaL_optstring(L, 2, "__std__");

  if (!strcmp(fname, "__std__")){
    stdflag = 1;
    fout = stdout;
  } else {
    fout = fopen(fname, "w+");
  }
  if (!fout){
    lua_pushnil(L);
    lua_pushstring(L, "fopen failed");
    return 2;
  }
  rv = agwrite(ud->g, fout);
  if (rv){
    if (!stdflag)
      fclose(fout);
    lua_pushnil(L);
    lua_pushstring(L, "agwrite failed");
    return 2;
  }
  if (!stdflag)
    fclose(fout);
  lua_pushnumber(L, rv);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property:nnodes [number]
 * Provides the numebr of edges of a graph
 * Example:
 * rv = g.nnodes
\*-------------------------------------------------------------------------*/
static int gr_nnodes(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  lua_pushnumber(L, agnnodes(ud->g));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: nedges [number]
 * Provides the number of edges of a graph.
 * Example:
 * rv = g.nedges
\*-------------------------------------------------------------------------*/
static int gr_nedges(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  lua_pushnumber(L, agnedges(ud->g));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Metamethod: __eq [boolean]
 * Compare two graphs
 * Example:
 * g == h or g != h with metamethods
\*-------------------------------------------------------------------------*/
static int gr_equal(lua_State *L)
{
  gr_graph_t *ud1 = tograph(L, 1, STRICT);
  gr_graph_t *ud2 = tograph(L, 2, STRICT);
  lua_pushboolean(L, (ud1->g == ud2->g));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: g.subgraph(self, name, nocreate)
 * Find or create a subgraph. The optional flag nocreate=true inhibits creation.
 * Returns graph userdata.
 * Example:
 * sg, err = g:subgraph(name)
\*-------------------------------------------------------------------------*/
static int gr_subgraph(lua_State *L)
{
  int rv;
  Agraph_t *g;
  gr_graph_t *sg; 
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *name = (char *) luaL_checkstring(L, 2);
  
  /* Check wheter the subgraph already exists */
  if ((g = agsubg(ud->g, name, 0)) != NULL){

    /* Yes - return corresponing userdata */
    rv = get_object(L, g);
    if (lua_isnil(L, -rv)){
      sg = lua_newuserdata(L, sizeof(gr_graph_t));
      sg->g = g;
      sg->name = strdup(name);
      sg->type = AGRAPH;
      sg->status = ALIVE;
      return new_graph(L);
    } else 
      return rv;
  } else {
    /* No - Create a new one  */
    if (lua_toboolean(L, 3)){
      lua_pushnil(L);
      lua_pushstring(L, "graph not found");
      return 2;
    }
    sg = lua_newuserdata(L, sizeof(gr_graph_t));
    if (!(sg->g = agsubg(ud->g, name, 1))){
      lua_pushnil(L);
      lua_pushstring(L, "agsubg failed");
      return 2;
    }
    if (name)
      sg->name = strdup(name);
    sg->type = AGRAPH;
    sg->status = ALIVE;
    return new_graph(L);
  }
}

/*-------------------------------------------------------------------------*\
 * Property: root [userdata]
 * Get root of graph.
 * Example:
 * sg, err = g.root
\*-------------------------------------------------------------------------*/
static int gr_root(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  Agraph_t *g;

  if (!(g = agroot(ud->g))){
    lua_pushnil(L);
    lua_pushstring(L, "agroot failed");
  }
  lua_pop(L, 1);                  /* empty */
  return get_object(L, g);        /* root */
}

/*-------------------------------------------------------------------------*\
 * Property: parent [userdata]
 * Get parent of graph.
 * Example:
 * sg, err = g.parent
\*-------------------------------------------------------------------------*/
static int gr_parent(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  Agraph_t *g = agparent(ud->g);
  
  return get_object(L, g);
}

/*-------------------------------------------------------------------------*\
 * Method: g = getnext(self, prev)
 * Retrieves next subgraph in a graph hierarchy.
 * Returns the successor of the given graph. With nil for parameter prev the 
 * first successor is returned. 
 * Example:
 * first = g:getnext(nil)
 * second = g:getnext(first)
 * third = g:getnext(second)
\*-------------------------------------------------------------------------*/
static int gr_getnext(lua_State *L)
{
  int rv;
  Agraph_t *g;
  gr_graph_t *ud_sg;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  if (lua_isnil(L, 2)){
    g = agfstsubg(ud->g);
  } else {
    ud_sg = tograph(L, 2, STRICT);
    g = agnxtsubg(ud_sg->g);
  }
  if (!g){
    lua_pushnil(L);
    return 1;
  } else {
    rv = get_object(L, g);
    if (rv == 1)
      return rv;
    else {
      lua_pop(L, rv);
      ud_sg = lua_newuserdata(L, sizeof(gr_graph_t));
      ud_sg->g = g;
      ud_sg->name = strdup(agnameof(g));
      ud_sg->type = AGRAPH;
      ud_sg->status = ALIVE;
      set_object(L, g);
      return new_graph(L);
    }
  } 
}

/*-------------------------------------------------------------------------*\
 * Iterator: g.walk(self)
 * Iterator over all subgraphs of the given graph.
 * Example:
 * for g in g:walk() do ... end
\*-------------------------------------------------------------------------*/
static int gr_walk(lua_State *L)
{
  lua_pushcfunction(L, gr_getnext);       /* ud, getnext - iterator func */
  lua_pushvalue(L, -2);                   /* ud, getnext, g - state */
  lua_pushnil(L);                         /* ud, getnext, g, nil - initial param = nil*/
  return 3;
}

/*-------------------------------------------------------------------------*\
 * Property: id [number]
 * Get a graph's id.
 * Example:
 * n = g.id
\*-------------------------------------------------------------------------*/
static int gr_id(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  lua_pushnumber(L, AGID(ud->g));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: isroot [boolean]
 * Is given graph a root ?
 * Example:
 * b = g.isroot
\*-------------------------------------------------------------------------*/
static int gr_isroot(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  lua_pushboolean(L, agroot(ud->g) == ud->g);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: t, err = g.getgraphattr(self), 
 *         t, err = g.getnodeattr(self), 
 *         t, err = g.getedgeattr(self)
 * Reads default attributes into a Lua table.
 * Example:
 * tab, err = g:getnodeattr()
\*-------------------------------------------------------------------------*/
static int gr_getgraphattr(lua_State *L)
{
  return getattr(L, AGRAPH);
}

static int gr_getnodeattr(lua_State *L)
{
  return getattr(L, AGNODE);
}

static int gr_getedgeattr(lua_State *L)
{
  return getattr(L, AGEDGE);
}

/*-------------------------------------------------------------------------*\
 * Method: t, err = g.getattr(self)
 * Reads declarations/default attributes into a nested Lua table for format:
 * t = {graph={name=val, ...}, node={name=val}, edge={name=val}}
 * Example:
 * tab, err = g:getattr()
\*-------------------------------------------------------------------------*/
static int gr_getattr(lua_State *L)
{
  lua_newtable(L);                     /* ud, t */
  
  lua_pushstring(L, "graph");          /* ud, t, key */
  getattr(L, AGRAPH);                  /* ud, t, key, gt */
  lua_settable(L, -3);                 /* ud, t */

  lua_pushstring(L, "node");           /* ud, t, key */
  getattr(L, AGNODE);                  /* ud, t, key, nt, n */
  lua_settable(L, -3);                 /* ud, t */

  lua_pushstring(L, "edge");           /* ud, t, key */
  getattr(L, AGEDGE);                  /* ud, t, key, et, n */
  lua_settable(L, -3);                 /* ud, t */
  
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: n, err = g.setgraphattr(self, t), 
 *         n, err = g.setnodeattr(self, t), 
 *         n, err = g.setedgeattr(self, t)
 * Sets default attributes from a Lua table t.
 * Returns the number of attributes set.
 * Example:
 * rv, err = g:setnodeattr{shape="box", width=12}
\*-------------------------------------------------------------------------*/
static int gr_setgraphattr(lua_State *L)
{
  return setattr(L, AGRAPH);
}

static int gr_setnodeattr(lua_State *L)
{
  return setattr(L, AGNODE);
}

static int gr_setedgeattr(lua_State *L)
{
  return setattr(L, AGEDGE);
}

/*-------------------------------------------------------------------------* \
 * Method: n, err = g.setattr(self, t)
 * Sets default attributes from a nested Lua table of format: 
 * see g.getattr(self) above. 
 * Returns the number of attributes set.
 * Example:
 * rv, err = g:setattr{graph={...}, node={shape="box"}, edge={...}} 
\*-------------------------------------------------------------------------*/
static int gr_setattr(lua_State *L)
{
  int n = 0;
  int rv;
  
  /* 1. Retrieve object specific attribute table from attribute table. */
  lua_pushstring(L, "graph");            /* ud, t, key */
  lua_gettable(L, -2);                   /* ud, t, gt */
  if (!lua_isnil(L, -1)){
    /* 2. Set/declare all object specific attribute */ 
    rv = setattr(L, AGRAPH);            /* ud, t, gt, n */
    n += (int) lua_tonumber(L, -rv);
    lua_pop(L, 2);                       /* ud, t */
  } else
    lua_pop(L, 1);                       /* ud, t */

  /* 1. ... see above */
  lua_pushstring(L, "node");             /* ud, t, key */
  lua_gettable(L, -2);                   /* ud, t, nt */
  if (!lua_isnil(L, -1)){
    /* 2. ... see above */
    rv = setattr(L, AGNODE);             /* ud, t, nt, n */
    n += (int) lua_tonumber(L, -rv);
    lua_pop(L, 2);                       /* ud, t */
  } else
    lua_pop(L, 1);                       /* ud, t */

  /* 1. ... see above */
  lua_pushstring(L, "edge");             /* ud, t, key */
  lua_gettable(L, -2);                   /* ud, t, et */
  if (!lua_isnil(L, -1)){
    /* 2. ... see above */
    rv = setattr(L, AGEDGE);             /* ud, t, et, n */
    n += (int) lua_tonumber(L, -rv);
    lua_pop(L, 2);                       /* ud, t */
  } else
    lua_pop(L, 1);

  lua_pushnumber(L, n);                  /* ud, t, n */
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: g.delete(self, obj)
 * Deletes an object from the given graph.
 * Returns non-nil.
 * Example:
 * rv, err = g:delete(n)
\*-------------------------------------------------------------------------*/
static int gr_delete(lua_State *L)
{
  gr_object_t *obj = toobject(L, 2, NULL, STRICT);
  TRACE("   g.delete(): obj: ud=%p ptr=%p type=%d (%s %d)\n", 
        (void *) obj, (void *) obj->p.p, AGTYPE(obj->p.p), __FILE__, __LINE__);
  
  switch(AGTYPE(obj->p.p)){
  case AGRAPH:
    TRACE("   g.delete(): graph: ud=%p '%s' g=%p (%s %d)\n",
	  (void *)obj, agnameof(obj->g.g), (void *)obj->g.g, __FILE__, __LINE__);
    lua_pushcfunction(L, gr_close);  /* ud, obj, func */
    lua_pushvalue(L, 2);             /* ud, obj, func, obj */
    lua_call(L, 1, 1);               /* ud, obj, result */
    lua_pop(L, 1);                   /* ud, obj */
    break;
  case AGNODE:
    TRACE("   g:delete(): node: ud=%p '%s' g=%p (%s %d)\n",
	  (void *)obj, agnameof(obj->n.n), (void *)obj->n.n, __FILE__, __LINE__);
    lua_pushcfunction(L, gr_delete_node);
    lua_pushvalue(L, 2);             /* ud, obj, func, obj */
    lua_call(L, 1, 1);               /* ud, obj, result */
    lua_pop(L, 1);                   /* ud, obj */
    break;
  case AGEDGE:
    TRACE("   g:delete(): edge: ud=%p '%s' g=%p (%s %d)\n",
	  (void *)obj, obj->p.name, (void *)obj->e.e, __FILE__, __LINE__);
    lua_pushcfunction(L, gr_delete_edge);
    lua_pushvalue(L, 2);             /* ud, obj, func, obj */
    lua_call(L, 1, 1);               /* ud, obj, result */
    lua_pop(L, 1);                   /* ud, obj */
    break;
  }
  lua_pushnumber(L, 0);
  return 1;
}

/*
 * A simple wrapper for usage of gr_node in other files.
 */
int gr_create_node(lua_State *L)
{
  return gr_node(L);
}

/*-------------------------------------------------------------------------* \
 * Method: n, err = g.node(self [, name[, nocreate]])
 * Finds or creates a node of given name. If name is nil autoname 'node@ID'
 * is used. The optional flag nocreate=true inhibits auto-creation.
 * Returns userdata.
 * Examples:
 * n, err = g:node("node-1", true)
 *  - Node will not be created, if it does not exists.
 * n, err = g:node("node2")
 *  - Node will be created, if it does not exist.
 * n, err = g:node()
 *  - 
\*-------------------------------------------------------------------------*/
static int gr_node(lua_State *L)
{
  Agnode_t *n;
  gr_node_t *node;                 
  int rv;
  char *name;
  
  /* param 1: graph = self */
  gr_graph_t *ud = tograph(L, 1, STRICT);

  name = (char *) luaL_checkstring(L, 2);          /* ud, name */

  if (name && (n = agnode(ud->g, name, 0)) != NULL){
    rv = get_object(L, n);                           /* ud, name, node/nil */
    if (lua_isnil(L, -rv)){
      lua_pop(L, rv);                                /* ud, name */
      /* Node not yet registered */
      node = lua_newuserdata(L, sizeof(gr_node_t));  /* ud, name, node */
      node->n = n;
      node->name = strdup(agnameof(n));
      node->type = AGNODE;
      node->status = ALIVE;
      return new_node(L);                            
    } else 
      /* Node already registered */
      return rv;                                     /* ud, name, node */
  } else {
    /* Node does not exist */
    if (lua_toboolean(L, 3)){
      /* auto-creation forbidden: return an error */
      lua_pushnil(L);
      lua_pushstring(L, "node not found");
      return 2;
    }
    /* Enforce creation with given name or an intermediate name '__tempname__' */
    node = lua_newuserdata(L, sizeof(gr_node_t));
    if ( (name && (node->n = agnode(ud->g, name, 1)) == NULL)){
      luaL_error(L, "agnode failed");
      return 0;
    }
    node->name = strdup(name);
    node->type = AGNODE;
    node->status = ALIVE;
    return new_node(L);
  }
}

/*-------------------------------------------------------------------------* \
 * Method: e, tail, head = g.edge(self, tail, head, label, nocreate)
 * Finds or creates an edge from tail to head with label. Parameter label 
 * is optional.
 * The optional flag nocreate inhibits auto-creation of the edge.
 * Any node given by name is implicitly created and it's userdata returned
 * as additional results - even if nocreate is not set.
 * Userdata proxy object is registered on the fly, if not yet available.
 * Examples:
 * e, tail, head = g:edge(n1, "node@2", "n1 => n2"[, false|true])
 * e, tail, head = g:edge(n1, n2)
 * e, tail, head = g:edge("node@1", n2, nil, true)
\*-------------------------------------------------------------------------*/
static int gr_edge(lua_State *L)
{
  Agedge_t *e;
  gr_edge_t *edge;                 
  gr_node_t *tail, *head;
  int rv;
  char ename[32];
  char *label;
  int head_created = 0;
  int tail_created = 0;
  
  /* param 1: graph = self */
  gr_graph_t *ud = tograph(L, 1, STRICT);
  
  /* param 2: tail node */
  if (lua_isuserdata(L, 2) == 1) {
    tail = tonode(L, 2, STRICT);
  } else {
    /* Create a node given only by name */
    lua_pushcfunction(L, gr_node);             /* ud, ntail, (n)head, [label], func */
    lua_pushvalue(L, 1);                       /* ud, ntail, (n)head, [label], func, ud */ 
    lua_pushstring(L, (const char *) luaL_checkstring(L, 2));
    /* g.node(self, name, flag) */
    lua_call(L, 2, 1);                         /* ud, ntail, (n)head, (label), tail */
    if (lua_isnil(L, -1)){
      return luaL_error(L, "tailnode create failed");
    }

    tail = tonode(L, -1, STRICT);
    lua_pop(L, 1);                             /* ud, ntail, (n)head, (label) */
    tail_created = 1;
  }
  /* param 3: head node */
  if (lua_isuserdata(L, 3))
    head = tonode(L, 3, STRICT);
  else {
    /* Create a node given only by name */
    lua_pushcfunction(L, gr_node);             /* ud, ntail, (n)head, [label], func */
    lua_pushvalue(L, 1);                       /* ud, ntail, (n)head, [label], func, ud */ 
    lua_pushstring(L, (const char *) luaL_checkstring(L, 3));
    /* g.node(self, name, flag) */
    lua_call(L, 2, 1);                         /* ud, ntail, (n)head, (label), head */
    if (lua_isnil(L, -1)){
      del_object(L, tail);
      return luaL_error(L, "headnode create failed");
    }
    head = tonode(L, -1, STRICT);
    lua_pop(L, 1);                             /* ud, ntail, (n)head, (label) */
    head_created = 1;
  }

  /* param 4: label */
  label = (char *) luaL_optstring(L, 4, NULL);   /* ud, tail, head */

  /* Check whether edge already exists - only required for strict graphs */
  if ((e = agedge(ud->g, tail->n, head->n, NULL, 0)) != NULL){
    TRACE("   gr_edge(): edge '%s' exists force=%d (%s %d)\n", agnameof(e), (int) lua_toboolean(L, 5), __FILE__, __LINE__);
    /* Edge exists */
    if (agisstrict(ud->g)) {
      /* strict directed graph: give edge a new label */
      agsafeset(e, "label", label, NULL);
    } 
    rv = get_object(L, e);
    if (lua_isnil(L, -rv)){
      rv = get_object(L, agopp(e));
      if (lua_isnil(L, -rv))
        return rv;
    }
    lua_pushlightuserdata(L, tail);
    lua_pushlightuserdata(L, head);
    return 3;
  }
  /* Edge doees not exist */
  if (lua_isboolean(L, 5)){
    /* creation of edge forbidden: nocreate flag is set */
    if (tail_created)
      del_object(L, tail->n);
    if (head_created)
      del_object(L, head->n);
    lua_pushnil(L);
    lua_pushstring(L, "edge not found");
    return 2;
  }
  /* Enforce creation */
  edge = lua_newuserdata(L, sizeof(gr_edge_t));
  if (agroot(tail->n) != agroot(head->n)){
    luaL_error(L, "nodes in different graphs");
    return 0;
  }
  sprintf(ename, "edge@%u", newid());
  if (!(edge->e = agedge(ud->g, tail->n, head->n, ename, 1))){
    /* creation failed */
    if (tail_created)
      del_object(L, tail->n);
    if (head_created)
      del_object(L, head->n);
    lua_pushnil(L);
    lua_pushstring(L, "agedge failed");
    return 2;
  }
  TRACE("   gr_edge(): ud=%p '%s' l=%p g=%p e=%p tail=%p head=%p force=%d (%s %d)\n",
        (void *) edge, label, (void*) label, (void *)ud->g, (void *)edge->e,
        (void *)tail->n, (void *)head->n, (int) lua_toboolean(L, 5), __FILE__, __LINE__);
  if (label)
    agsafeset(edge->e, "label", label, NULL);
  edge->name = strdup(ename);
  edge->type = AGEDGE;
  edge->status = ALIVE;
  new_edge(L);
  lua_pushlightuserdata(L, tail);
  lua_pushlightuserdata(L, head);
  return 3;
}

/*-------------------------------------------------------------------------*\
 * Method: n, err = g.findedge(self, tail, head [, name])
 * Finds an edge of given name between given nodes. 
 * If multiple edges exist in non-directed graph, onle one of them is 
 * retrieved, if name is not specified.
 * Returns proxy userdata.
 * Example:
 * e, tail, head = g:findedge(n1, "node@2", "edge@122")
\*-------------------------------------------------------------------------*/
static int gr_findedge(lua_State *L)
{
  Agedge_t *e;
  gr_edge_t *edge;                 
  int rv;
  char sbuf[32];
  gr_graph_t *ud = tograph(L, 1, STRICT);
  gr_node_t *tail = tonode(L, 2, STRICT);
  gr_node_t *head = tonode(L, 3, STRICT);
  char *name = (char *) luaL_optstring(L, 4, NULL); /* ud, tail, head, label */
  if ((e = agedge(ud->g, tail->n, head->n, name, 0)) != NULL){
    rv = get_object(L, e);                       /* ud, tail, head, (label), e/nil */
    if (lua_isnil(L, -rv)){
      rv = get_object(L, agopp(e));              /* ud, tail, head, (label), e/nil, nil/e */
      if (lua_isnil(L, -rv)){
        lua_pop(L, rv);                          /* ud, tail, head, (label) */
        /* Edge not yet registered */
        edge = lua_newuserdata(L, sizeof(gr_edge_t));  /* ud, peer, name, edge */
        edge->e = e;
        sprintf(sbuf, "edge@%lu", (unsigned long) AGID(e));
        edge->name = strdup(sbuf);
        edge->type = AGEDGE;
        edge->status = ALIVE;
        set_object(L, e);                              /* ud, peer, name, edge */
        new_edge(L);
        lua_pushlightuserdata(L, tail);
        lua_pushlightuserdata(L, head);
        return 3;
      } else {
        /* Edge already registered */
        lua_pushlightuserdata(L, tail);
        lua_pushlightuserdata(L, head);
        return rv + 2;                                     /* ud, peer, name, edge */
      }
    } else {
      /* Edge already registered */
      lua_pushlightuserdata(L, tail);
      lua_pushlightuserdata(L, head);
      return rv + 2;                                     /* ud, peer, name, edge */
    }
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "edge not found");
    return 2;
  }
}
/*-------------------------------------------------------------------------*\
 * Method: n, err = g.idnode(self, id)
 * Finds a node by id.
 * Returns userdata.
 * Example:
 * n, err = g:idnode(18)
\*-------------------------------------------------------------------------*/
static int gr_idnode(lua_State *L)
{
  Agnode_t *n;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  unsigned long id = (unsigned long) luaL_checknumber(L, 2);
  if ((n = agidnode(ud->g, id, 0)) != NULL){
    return get_object(L, n);
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "agidnode failed");
    return 2;
  }
}

/*-------------------------------------------------------------------------*\
 * Method: n = nextnode(self, prev)
 * Retrieves next node in a graph.
 * Returns the successor of the given node. With nil as parameter prev the 
 * first node is returned. 
 * Example:
 * first = g:nextnode(nil)
 * second = g:nextnode(first)
 * third = g:nextnode(second)
\*-------------------------------------------------------------------------*/
static int gr_nextnode(lua_State *L)
{
  int rv;
  Agnode_t *n;
  gr_node_t *ud_n;
  gr_graph_t *ud_g = tograph(L, 1, STRICT);
  if (lua_isnil(L, 2))
    n = agfstnode(ud_g->g);
  else {
    ud_n = tonode(L, 2, STRICT);
    n = agnxtnode(ud_g->g, ud_n->n);
  }
  if (!n){
    /* no more nodes */
    lua_pushnil(L);
    return 1;
  } else {
    /* Check whether userdata exists .. */
    rv = get_object(L, n);
    if (rv == 1)
      /* .. yes: return it */
      return rv;
    else {
      /* .. no: create it */
      lua_pop(L, rv);
      ud_n = lua_newuserdata(L, sizeof(gr_node_t)); 
      ud_n->n = n;
      ud_n->name = strdup(agnameof(n));
      ud_n->type = AGNODE;
      ud_n->status = ALIVE;
      set_object(L, n);
      return new_node(L);
    }
  }
}

/*-------------------------------------------------------------------------*\
 * Iterator: walknodes()
 * Iterator over all nodes of a given graph.
 * Example:
 * for n in g:walknodes() do ... end
\*-------------------------------------------------------------------------*/
static int gr_walknodes(lua_State *L)
{
  lua_pushcfunction(L, gr_nextnode);     /* ud, nextnode */
  lua_pushvalue(L, -2);                  /* ud, nextnode, g */
  lua_pushnil(L);                        /* ud, nextnode, g, nil */
  return 3;
}

/*-------------------------------------------------------------------------*\
 * Property: g.graph [userdata]
 * Retrieves the graph to which given subgraph belongs. Trivial because it 
 * returns itself.
 * Returns graph userdata.
 * Example:
 * g, err = g.graph
\*-------------------------------------------------------------------------*/
int gr_graphof(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  Agraph_t *g = agraphof(ud->g);
  if (g == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "no graph");
    return 2;
  }
  return get_object(L, ud->g);
}

/*-------------------------------------------------------------------------*\
 * Method: rv = g.contains(self, obj)
 * Returns true if graph g contains object obj.
 * Example:
 * b = g:contains(n)
\*-------------------------------------------------------------------------*/
int gr_contains(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  gr_object_t *obj = toobject(L, 2, NULL, STRICT);
  int rv = agcontains(ud->g, obj->p.p);
  if (rv && AGTYPE(obj->p.p) == AGNODE){
    if (agroot(obj->n.n) == agroot(ud->g))
      lua_pushboolean(L, TRUE);
    else
      lua_pushboolean(L, FALSE);
  } else 
    lua_pushboolean(L, rv);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: rv = g.layout(self, fmt)
 * Layout the given graph in the specified format/algorithm.
 * Example:
 * b = g:layout("dot")
\*-------------------------------------------------------------------------*/
static int gr_layout(lua_State *L)
{
  int rv;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *fmt = (char *) luaL_optstring(L, 2, "dot");

  if (!strcmp(fmt, "dot") ||
      !strcmp(fmt, "neato") ||
      !strcmp(fmt, "nop") ||
      !strcmp(fmt, "nop2") ||
      !strcmp(fmt, "twopi") ||
      !strcmp(fmt, "fdp") ||
      !strcmp(fmt, "circo")){

    if ((rv = gv_layout(ud->g, fmt)) != GR_SUCCESS){
      luaL_error(L, "layout error: %d", rv);
      return 0;
    }
    lua_pushvalue(L, rv);
    return 1;
  } else {
    luaL_error(L, "invalid layout format '%s'", fmt);
    return 0;
  }
}

/*-------------------------------------------------------------------------*\
 * Method: rv = g.freelayout(self, fmt)
 * Free a layout
 * Example:
 * b = g:freelayout()
\*-------------------------------------------------------------------------*/
static int gr_freelayout(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  if (ud->g)
    gv_free_layout(ud->g);
  else {
    lua_pushnil(L);
    lua_pushstring(L, "invalid graph");
    return 2;
  }
  lua_pushnumber(L, 0);
  return 1;
}
/*-------------------------------------------------------------------------*\
 * Method: rv = g.render(self, rfmt, file, lfmt)
 * Render the given graph in the specified format.
 * Example:
 * b = g:render("pdf")
\*-------------------------------------------------------------------------*/
static int gr_render(lua_State *L)
{
  int rv;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *rfmt = (char *) luaL_optstring(L, 2, "plain");
  char *fname = (char *) luaL_optstring(L, 3, NULL);
  char *lfmt = (char *) luaL_optstring(L, 4, NULL);
  if (gvc == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "layout missing");
    return 2;
  }
  if (lfmt)
    gv_layout(ud->g, lfmt);
  if (fname)
    rv = gv_render_file(ud->g, rfmt, fname);
  else
    rv = gv_render(ud->g, rfmt, stdout);
  if (lfmt)
    gv_free_layout(ud->g);
  if (rv != GR_SUCCESS){
    lua_pushnil(L);
    lua_pushstring(L, "gvRender failed");
    return 2;
  }
  lua_pushnumber(L, rv);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: list, count = graph.plugins(type)
 * Retrieve available plugins for layout or rendering.
 * type: layout | render 
 * Example:
 * tab, err  = g:layout("layout")
\*-------------------------------------------------------------------------*/
static int gr_plugins(lua_State *L)
{
  int i, count;
  char **list;

  char *kind = (char *) luaL_optstring(L, 1, "render");
  list = gvPluginList(gvc, kind, &count, NULL);
  if (list == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "no plugins");
    return 2;
  }

  lua_newtable(L);                 /* t */
  for (i = 0; i < count; i++){
    lua_pushnumber(L, i+1);        /* t, index */
    lua_pushstring(L, list[i]);    /* t, index, value */
    lua_settable(L, -3);           /* t */
  }
  lua_pushnumber(L, count);
  for (i = 0; i < count; i++)
    free(list[i]);
  free(list);
  return 2;
}
/*-------------------------------------------------------------------------*\
 * Module initialization
\*-------------------------------------------------------------------------*/
LUALIB_API int luaopen_graph_core(lua_State *L) {
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, MYNAME);
  register_metainfo(L, funcs);
  lua_pushliteral(L, "version");
  lua_pushliteral(L, MYVERSION);
  lua_rawset(L, -3);
  lua_pushliteral(L, "_VERSION");
  lua_pushliteral(L, MYVERSION);
  lua_rawset(L, -3);
  lua_pushliteral(L, "_GVVERSION");
  lua_pushliteral(L, MYGVVERSION);
  lua_rawset(L, -3);
  lua_pushliteral(L, "_COPYRIGHT");
  lua_pushliteral(L, MYCOPYRIGHT);
  lua_rawset(L, -3);
  lua_pushliteral(L, "_DESCRIPTION");
  lua_pushliteral(L, MYDESCRIPTION);
  lua_rawset(L, -3);
  lua_pushliteral(L, "_SYSTEM");
  lua_pushliteral(L, MYSYSTEM);
  lua_rawset(L, -3);
  lua_pushliteral(L, "plugins");
  lua_pushcfunction(L, gr_plugins);
  lua_rawset(L, -3);
  if ((gvc = gvContextPlugins(lt_preloaded_symbols, DEMAND_LOADING)) == NULL){
    return luaL_error(L, "cannot load plugins");
  }
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Metamethod: __tostring [boolean]
 * Return a string presentation of a graph object.
 * Example:
 * g == h or g != h with metamethods
\*-------------------------------------------------------------------------*/
static int gr_tostring(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, NONSTRICT);
  lua_pushfstring(L, "graph: %p (%s)", ud, ud->p.status == ALIVE ? "alive" : "dead");
  return 1;
}
