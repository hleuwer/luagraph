/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006
 *
 * Graph related functionality.
 *
 * $Id: gr_graph.c,v 1.2 2007-01-10 22:28:09 leuwer Exp $
\*=========================================================================*/

/*=========================================================================*\
 * Includes
\*=========================================================================*/
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM < 501)
#include "compat-5.1.h"
#endif

#include "gr_graph.h"
#include "graphviz/cdt.h"

/*=========================================================================*\
 * Defines
\*=========================================================================*/
#define MYNAME "graph"
#define MYVERSION VERSION
#define MYGVVERSION GVVERSION
#define MYCOPYRIGHT "Copyright (C) 2006-2007, Herbert Leuwer "
#define MYDESCRIPTION "LuaGRAPH is a library for creating, manipulating and rendering GRAPHs (based on the GRAPHVIZ library)."
/*=========================================================================*\
 * Prototypes
\*=========================================================================*/
static int gr_open(lua_State *L);
static int gr_close(lua_State *L);
static int gr_nameof(lua_State *L);
static int gr_write(lua_State *L);
static int gr_read(lua_State *L);
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
static int gr_insert(lua_State *L);
static int gr_node(lua_State *L);
static int gr_edge(lua_State *L);
static int gr_findedge(lua_State *L);
static int gr_idnode(lua_State *L);
static int gr_rename(lua_State *L);
static int gr_nextnode(lua_State *L);
static int gr_walknodes(lua_State *L);
static int gr_graphof(lua_State *L);
static int gr_contains(lua_State *L);
static int gr_layout(lua_State *L);
static int gr_render(lua_State *L);

/*=========================================================================*\
 * Data
\*=========================================================================*/
/*
 * Base library functions
 */
static const luaL_reg funcs[] = {
  {"open", gr_open},
  {"close", gr_close},
  {"read", gr_read},
  {"equal", gr_equal},
  {NULL, NULL}
};

/*
 * Graph object methods
 */
static const luaL_reg reg_methods[] = {
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
  {"insert", gr_insert},
  {"node", gr_node},
  {"edge", gr_edge},
  {"findedge", gr_findedge},
  {"idnode", gr_idnode},
  {"rename", gr_rename},
  {"nextnode", gr_nextnode},
  {"walknodes", gr_walknodes},
  {"type", get_object_type},
  {"contains", gr_contains},
  {"layout", gr_layout},
  {"render", gr_render},
  {"rawget", getval},
  {NULL, NULL}
};

/*
 * Read-only members
 */
static const luaL_reg reg_rmembers[] = {
  {"nnodes", gr_nnodes},
  {"nedges", gr_nedges},
  {"name", gr_nameof},
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
static const luaL_reg reg_metamethods[] = {
  {"__gc", gr_close},
  {"__eq", gr_equal},
  {"__newindex", object_newindex_handler},
  {NULL, NULL}
};

/*=========================================================================*\
 * Functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
 * Utility Functions
\*-------------------------------------------------------------------------*/
#define new_graph(L)  new_object(L, "graph", reg_rmembers, reg_methods,\
                                 reg_metamethods, object_index_handler)

/*-------------------------------------------------------------------------*\
 * Create a new graph
 * Returns graph userdata.
 * Example:
 * g, err = graph.open(name, kind)
\*-------------------------------------------------------------------------*/
static int gr_open(lua_State *L)
{
  int kind = 0;
  gr_graph_t *ud;

  char *name = (char *) luaL_checkstring(L, 1);
  char *skind = (char *) luaL_optstring(L, 2, "directed");
  if (!strcmp(skind, "directed"))
    kind = AGDIGRAPH;
  else if (!strcmp(skind, "strictdirected"))
    kind = AGDIGRAPHSTRICT;
  else if (!strcmp(skind, "undirected"))
    kind = AGRAPH;
  else if (!strcmp(skind, "strictundirected"))
    kind = AGRAPHSTRICT;
  else {
    luaL_error(L, "invalid graph attribute");
  }

  /* Create a userdata */
  ud = lua_newuserdata(L, sizeof(gr_graph_t));   /* ud */
  if (!(ud->g = agopen(name, kind))){
    lua_pushnil(L);
    lua_pushstring(L, "open failed");
    return 2;
  }
  ud->name = strdup(name);
  ud->type = AGGRAPH;

  /* We need a few default fields */
  if (!agedgeattr(ud->g, "label", "") ||
      !agraphattr(ud->g, ".attrib", "") ||
      !agedgeattr(ud->g, ".attrib", "") ||
      !agnodeattr(ud->g, ".attrib", "")){
    luaL_error(L, "declaration failed");
    return 0;
  }
      
  ud->gvc = NULL;
  set_object(L, ud->g);
  return new_graph(L);
}

/*-------------------------------------------------------------------------*\
 * Method: g.close(self)
 * Close a graph; also used as __gc metamethod.
 * Returns -1 if graph does not exit, 1 if graph has already been closed.
 * Example:
 * rv, err = graph.close(g)
 * rv, err = g:close()
\*-------------------------------------------------------------------------*/
static int gr_close(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, NONSTRICT);

  if (ud->g != NULL){
    Agraph_t *sg;
    Agnode_t *sn, *nn;
    int ix;

    /* Clean-up layout and rendering */
    if (ud->gvc){
      gvFreeLayout(ud->gvc, ud->g);
      gvFreeContext(ud->gvc);
      ud->gvc = NULL;
    }
    /* Recursively delete all assoicated subgraphs */
    if (AG_IS_METAGRAPH(ud->g) == FALSE){
      int flag;
      Agraph_t *meta;
      Agedge_t *e, *f;
      meta = ud->g->meta_node->graph;
      do {
	flag = FALSE;
	for (e = agfstout(meta, ud->g->meta_node); e; e = f){
	  f = agnxtout(meta, e);
	  if (agnxtin(meta, agfstin(meta, e->head)) == NULL){
	    flag = TRUE;
	    sg = agusergraph(e->head);
	    ix = get_object(L, sg);           /* ud, sg */
	    if (!(lua_isnil(L, -ix))){
	      TRACE("g:close(): subgraph: ud=%p '%s' g=%p type=%d\n", 
		    (void *) lua_touserdata(L, -ix), agnameof(sg), (void *)sg,
		    AGTYPE(sg));
	      lua_pushcfunction(L, gr_close); /* ud, sg, func */
	      lua_pushvalue(L , -2);          /* ud, sg, func, sg */
	      lua_call(L, 1, LUA_MULTRET);    /* ud, sg */
	      lua_pop(L, 1);                  /* ud */
	    } else {
	      lua_pop(L, 2);
	    }
	  }
	}
      } while (flag);
    }

    /* Delete all associated nodes - only from root graph, because subgraphs may
       refer to nodes in another subgraph */
    if (ud->g == ud->g->root){
      sn = agfstnode(ud->g);
      while (sn){
	nn = agnxtnode(ud->g, sn);                   
	ix = get_object(L, sn);                      /* ud, sn */
	if (!(lua_isnil(L, -ix))){
	  TRACE("g:close(): subnode: ud=%p'%s' id=0x%x n=%p\n", 
		(void *) ud, agnameof(sn), AGID(sn), (void *)sn);
	  lua_pushcfunction(L, gr_delete_node); /* ud, sn, func */
	  lua_pushvalue(L, -2);                 /* ud, sn, func, sn */
	  lua_call(L, 1, LUA_MULTRET);          /* ud, sn */
	  lua_pop(L, 1);                        /* ud */
	} else {
	  lua_pop(L, 2);
	}
	sn = nn;
      }
    }

    /* Delete the graph */
    TRACE("g:close(): graph: ud=%p '%s' g=%p type=%d\n", 
	  (void *) ud, agnameof(ud->g), (void *)ud->g, AGTYPE(ud->g));
    del_object(L, ud->g);
#if 0
    if (ud->gvc){
      gvFreeLayout(ud->gvc, ud->g);
      gvFreeContext(ud->gvc);
      ud->gvc = NULL;
    }
#endif
    agclose(ud->g);
    ud->g = NULL;
    if (ud->name){
      free(ud->name);
      ud->name = NULL;
    }
  } else {
    TRACE("g:close(): graph: ud=%p already closed\n", (void *) ud);
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

/*-------------------------------------------------------------------------*\
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
  if (!(ud->g = agread(fin))){
    fclose(fin);
    lua_pushnil(L);
    lua_pushstring(L, "agread failed");
    return 2;
  }
  fclose(fin);
  ud->name = strdup(agnameof(ud->g));
  ud->type = AGGRAPH;
  ud->gvc = NULL;
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
  if ((g = agfindsubg(ud->g, name)) != NULL){
    /* Yes - return corresponing userdata */
    rv = get_object(L, g);
    if (lua_isnil(L, -rv)){
      sg = lua_newuserdata(L, sizeof(gr_graph_t));
      sg->g = g;
      sg->name = strdup(name);
      sg->type = AGGRAPH;
      sg->gvc = NULL;
      set_object(L, g);
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
    if (!(sg->g = agsubg(ud->g, name))){
      lua_pushnil(L);
      lua_pushstring(L, "agsubg failed");
      return 2;
    }
    if (name)
      sg->name = strdup(name);
    sg->type = AGGRAPH;
    sg->gvc = NULL;
    set_object(L, sg->g);
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
  Agedge_t *e = agfstin(ud->g->meta_node->graph, ud->g->meta_node);
  Agnode_t *tail = e->tail;
  Agraph_t *g = agusergraph(tail);
  return get_object(L, g);
}

/*-------------------------------------------------------------------------*\
 * Method: g = getnext(self, prev)
 * Retrieves next subgraph in a graph hierarchy.
 * Returns the successor of the given graph. With nil as prev the first
 * successor is returned. 
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
    g = agfstsubg(ud->g, &ud->lastedge);
  } else {
    ud_sg = tograph(L, 2, STRICT);
    g = agnxtsubg(ud->g, &ud->lastedge);
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
      ud_sg->type = AGGRAPH;
      ud_sg->gvc = NULL;
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
  lua_pushcfunction(L, gr_getnext);      /* ud, getnext */
  lua_pushvalue(L, -2);                  /* ud, getnext, g */
  lua_pushnil(L);                        /* ud, getnext, g, nil */
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
  lua_pushnumber(L, AGID(ud->g->meta_node));
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
  lua_pushboolean(L, agisarootobj(ud->g));
  return 1;
}

/*
 * Gets attributes for a specific object type into a table rt.
 * If no attributes are defined, the function returns an empty table.
 * Lua entry stack: ud
 * Lua exit stack:  ud, rt
 */
static int getattr(lua_State *L, int kind)
{
  Agdict_t *dict;
  int i;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  Agsym_t *sym = NULL;

  lua_newtable(L);    /* rt */
  switch (kind){
  case AGGRAPH:
    dict = ud->g->univ->globattr;
    break;
  case AGNODE:
    dict = ud->g->univ->nodeattr;
    break;
  case AGEDGE:
    dict = ud->g->univ->edgeattr;
    break;
  default:
    dict = NULL; /* avoid warning */
    luaL_error(L, "invalid kind");
    break;
  }
  for (i = 0; dict->list && (sym = dict->list[i]) != NULL; i++){
    lua_pushstring(L, sym->name);
    lua_pushstring(L, sym->value);
    lua_settable(L, -3);
    if (ud->g != ud->g->root){
      char *value = NULL;
      switch(kind){
      case AGNODE:
	value = agget(ud->g->proto->n, sym->name);
	break;
      case AGEDGE:
	value = agget(ud->g->proto->e, sym->name);
	break;
      }
      if (value){
	lua_pushstring(L, sym->name);
	lua_pushstring(L, value);
	lua_settable(L, -3);
      }
    }
  }
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
  return getattr(L, AGGRAPH);
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
  getattr(L, AGGRAPH);                 /* ud, t, key, gt */
  lua_settable(L, -3);                 /* ud, t */

  lua_pushstring(L, "node");           /* ud, t, key */
  getattr(L, AGNODE);                  /* ud, t, key, nt, n */
  lua_settable(L, -3);                 /* ud, t */

  lua_pushstring(L, "edge");           /* ud, t, key */
  getattr(L, AGEDGE);                  /* ud, t, key, et, n */
  lua_settable(L, -3);                 /* ud, t */
  
  return 1;
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
  Agsym_t *sym = NULL;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  luaL_checktype(L, -1, LUA_TTABLE);
  
  if (ud->g != ud->g->root){
#if 0
    lua_pushnil(L);
    luaL_error(L, "invalid graph");
    return 0;
#else
    lua_pushnil(L);
    while (lua_next(L, -2)){   /* t, key, val */
      if (!lua_isstring(L, -2)){
	luaL_error(L, "invalid key");
	return 0;
      }
      key = (char *) lua_tostring(L, -2);
      value = (char *) lua_tostring(L, -1);
      if (value == NULL){
	lua_pop(L, 2);
	luaL_error(L, "invalid value");
	return 0;
      }
      switch(kind){
      case AGNODE:
	agset(ud->g->proto->n, key, value);
	break;
      case AGEDGE:
	agset(ud->g->proto->e, key, value);
	break;
      }
      lua_pop(L, 1);
      n++;
    }
    lua_pushnumber(L, n);
    return 1;
#endif
  }

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
    switch(kind){
    case AGGRAPH:
      sym = agraphattr(ud->g, key, value); 
      if (!sym){luaL_error(L, "aggraphsattr failed"); return 0;}
      break;
    case AGNODE:
      sym = agnodeattr(ud->g, key, value);
      if (!sym){luaL_error(L, "agnodeattr failed"); return 0;}
      break;
    case AGEDGE:
      sym = agedgeattr(ud->g, key, value);
      if (!sym){luaL_error(L, "agedgeattr failed"); return 0;}
      break;
    }
    lua_pop(L, 1);               /* t, key */
    n++;
  }
  lua_pushnumber(L, n);
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
  return setattr(L, AGGRAPH);
}

static int gr_setnodeattr(lua_State *L)
{
  return setattr(L, AGNODE);
}

static int gr_setedgeattr(lua_State *L)
{
  return setattr(L, AGEDGE);
}

/*-------------------------------------------------------------------------*\
 * Method: n, err = g.setattr(self, t)
 * Sets default attributes from a nested Lua table of format: see g.getattr(self)
 * above. 
 * Returns the number of attributes set.
 * Example:
 * rv, err = g:setattr{graph={...}, node={shape="box"}, edge={...}} 
\*-------------------------------------------------------------------------*/
static int gr_setattr(lua_State *L)
{
  int n = 0;
  int rv;
  lua_pushstring(L, "graph");            /* ud, t, key */
  lua_gettable(L, -2);                   /* ud, t, gt */
  if (!lua_isnil(L, -1)){
    rv = setattr(L, AGGRAPH);            /* ud, t, gt, n */
    n += lua_tonumber(L, -rv);
    lua_pop(L, 2);                       /* ud, t */
  } else
    lua_pop(L, 1);                       /* ud, t */

  lua_pushstring(L, "node");             /* ud, t, key */
  lua_gettable(L, -2);                   /* ud, t, nt */
  if (!lua_isnil(L, -1)){
    rv = setattr(L, AGNODE);             /* ud, t, nt, n */
    n += lua_tonumber(L, -rv);
    lua_pop(L, 2);                       /* ud, t */
  } else
    lua_pop(L, 1);                       /* ud, t */

  lua_pushstring(L, "edge");             /* ud, t, key */
  lua_gettable(L, -2);                   /* ud, t, et */
  if (!lua_isnil(L, -1)){
    rv = setattr(L, AGEDGE);             /* ud, t, et, n */
    n += lua_tonumber(L, -rv);
    lua_pop(L, 2);                       /* ud, t */
  } else
    lua_pop(L, 1);

  lua_pushnumber(L, n);                  /* ud, t, n */
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: rv, err = g.delete(self, obj)
 * Deletes an object from the given graph.
 * Returns non-nil.
 * Example:
 * rv, err = g:delete(h)
\*-------------------------------------------------------------------------*/
static int gr_delete(lua_State *L)
{
  gr_object_t *obj = toobject(L, 2, NULL, STRICT);
  TRACE("g:delete(): obj: ud=%p x=%p type=%d\n", 
	 (void *) obj, (void *) obj->p.p, AGTYPE(obj->p.p));

  switch(AGTYPE(obj->p.p)){
  case AGGRAPH:
    TRACE("g:delete(): graph: ud=%p '%s' g=%p\n",
	  (void *)obj, agnameof(obj->g.g), (void *)obj->g.g);
    lua_pushcfunction(L, gr_close);  /* ud, obj, func */
    lua_pushvalue(L, 2);             /* ud, obj, func, obj */
    lua_call(L, 1, 1);               /* ud, obj, result */
    lua_pop(L, 1);                   /* ud, obj */
    break;
  case AGNODE:
    TRACE("g:delete(): node: ud=%p '%s' g=%p\n",
	  (void *)obj, agnameof(obj->n.n), (void *)obj->n.n);
    lua_pushcfunction(L, gr_delete_node);
    lua_pushvalue(L, 2);             /* ud, obj, func, obj */
    lua_call(L, 1, 1);               /* ud, obj, result */
    lua_pop(L, 1);                   /* ud, obj */
    break;
  case AGEDGE:
    TRACE("g:delete(): edge: ud=%p '%s' g=%p\n",
	  (void *)obj, obj->p.name, (void *)obj->e.e);
    lua_pushcfunction(L, gr_delete_edge);
    lua_pushvalue(L, 2);             /* ud, obj, func, obj */
    lua_call(L, 1, 1);               /* ud, obj, result */
    lua_pop(L, 1);                   /* ud, obj */
    break;
  }
  lua_pushnumber(L, 0);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: rv, err = g.insert(self, obj)
 * Inserts an existing object into a graph.
 * Returns non-nil.
 * Example:
 * rv, err = g:insert(n)
\*-------------------------------------------------------------------------*/
static int gr_insert(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  gr_object_t *obj = toobject(L, 2, NULL, STRICT);
  aginsert(ud->g, (void *)(obj->p.p));
  lua_pushnumber(L, 0);
  return 1;
}
/*-------------------------------------------------------------------------*\
 * Method: n, err = g.node(self, name, nocreate)
 * Finds or creates a node of given name. If name is nil autoname 'node@ID'
 * is used. The optional flag nocreate=true inhibits auto-creation.
 * Returns userdata.
 * Example:
 * rv, err = g:node("node-1", false)
\*-------------------------------------------------------------------------*/
static int gr_node(lua_State *L)
{
  Agnode_t *n;
  gr_node_t *node;                 
  int rv;

  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *name = (char *) lua_tostring(L, 2);          /* ud, name */
  if (name && (n = agfindnode(ud->g, name)) != NULL){
    /* Node exists */
    rv = get_object(L, n);                           /* ud, name, node/nil */
    if (lua_isnil(L, -rv)){
      lua_pop(L, rv);                                /* ud, name */
      /* Node not yet registered */
      node = lua_newuserdata(L, sizeof(gr_node_t));  /* ud, name, node */
      node->n = n;
      node->name = strdup(agnameof(n));
      node->type = AGNODE;
      //      n->u.subg = ud->g;
      node->subg = ud->g;
      set_object(L, n);                              /* ud, name, node */
      return new_node(L);                            
    } else 
      /* Node already registered */
      return rv;                                     /* ud, name, node */
  } else {
    /* Node does not exist */
    if (lua_toboolean(L, 3)){
      lua_pushnil(L);
      lua_pushstring(L, "node not found");
      return 2;
    }
    /* Enforce creation */
    node = lua_newuserdata(L, sizeof(gr_node_t));
    if ( (name && (node->n = agnode(ud->g, name)) == NULL) ||
	 (!name && (node->n = agnode(ud->g, "__tempname__")) == NULL)){
      luaL_error(L, "agnode failed");
      return 0;
    }
    if (name)
      node->name = strdup(name);
    else {
      char sbuf[32];
      sprintf(sbuf, "node@%d", AGID(node->n));
      agstrfree(node->n->name);
      node->n->name = agstrdup(sbuf);
      node->name = strdup(sbuf);
    }
    node->type = AGNODE;
    //    node->n->u.subg = ud->g;
    node->subg = ud->g;
    set_object(L, node->n);
    return new_node(L);
  }
}

int gr_create_node(lua_State *L)
{
  return gr_node(L);
}
/*-------------------------------------------------------------------------*\
 * Method: e, tail, head = g.edge(self, tail, head, label, nocreate)
 * Finds or creates an edge from tail to head with label. Label is optional.
 * The optional flag nocreate=true inhibits auto-creation.
 * Any node given by name is implicitly created and it's userdata returned
 * as additional results - even if nocreate is not set.
 * Example:
 * e, tail, head = g:edge(n1, "node@2", "n1 => n2")
 * e, err = g:edge(...)
\*-------------------------------------------------------------------------*/
static int gr_edge(lua_State *L)
{
  Agedge_t *e;
  gr_edge_t *edge;                 
  int rv;
  char sbuf[32];
  gr_node_t *tail, *head;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  if (lua_isuserdata(L, 2)) 
    tail = tonode(L, 2, STRICT);
  else {
    /* Create a node given only by name */
    lua_pushcfunction(L, gr_node);             /* ud, ntail, (n)head, [label], func */
    lua_pushvalue(L, 1);                       /* ud, ntail, (n)head, [label], func, ud */ 
    lua_pushstring(L, (const char *) luaL_checkstring(L, 2));
    /* g.node(self, name, flag) */
    lua_call(L, 2, 1);                         /* ud, ntail, (n)head, (label), tail */
    if (lua_isnil(L, -1)){
      luaL_error(L, "tailnode create failed");
      return 0;
    }
    tail = tonode(L, -1, STRICT);
    lua_pop(L, 1);                             /* ud, ntail, (n)head, (label) */
  }
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
      luaL_error(L, "headnode create failed");
      return 0;
    }
    head = tonode(L, -1, STRICT);
    lua_pop(L, 1);                             /* ud, ntail, (n)head, (label) */
  }

  char *label = (char *) luaL_optstring(L, 4, "");   /* ud, tail, head */
  if (AG_IS_STRICT(ud->g) && (e = agfindedge(ud->g, tail->n, head->n)) != NULL){
    /* Edge exists */
    rv = get_object(L, e);                           /* ud, tail, head, edge|nil */
    if (lua_isnil(L, -rv)){
      lua_pop(L, rv);                                /* ud, tail, head */
      /* Edge not yet registered */
      edge = lua_newuserdata(L, sizeof(gr_edge_t));  /* ud, peer, name, edge */
      edge->e = e;
      sprintf(sbuf, "edge@%d", AGID(e));
      edge->name = strdup(sbuf);
      if (strlen(label) > 0)
	agset(e, "label", label);
      edge->type = AGEDGE;
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
    if (lua_toboolean(L,5)){
      del_object(L, tail->n);
      del_object(L, head->n);
      lua_pushnil(L);
      lua_pushstring(L, "edge not found");
      return 2;
    }
    /* Enforce creation */
    edge = lua_newuserdata(L, sizeof(gr_edge_t));
    if (tail->n->graph != head->n->graph){
      luaL_error(L, "nodes in different graphs");
      return 0;
    }
#if 0
    if (AG_IS_STRICT(ud->g) && (tail->n == head->n)){
      luaL_error(L, "strict graph forbids self-edges");
      return 0;
    }
#endif
    if (!(edge->e = agedge(ud->g, tail->n, head->n))){
      del_object(L, tail->n);
      del_object(L, head->n);
      lua_pushnil(L);
      lua_pushstring(L, "agedge failed");
      return 2;
    }
    if (strlen(label) > 0)
      agset(edge->e, "label", label);
    sprintf(sbuf, "edge@%d", edge->e->id);
    edge->name = strdup(sbuf);
    edge->type = AGEDGE;
    set_object(L, edge->e);
    new_edge(L);
    lua_pushlightuserdata(L, tail);
    lua_pushlightuserdata(L, head);
    return 3;
  }
}

/*-------------------------------------------------------------------------*\
 * Method: n, err = g.findedge(self, tail, head, label)
 * Finds an edge
 * Returns userdata.
 * Example:
 * e, tail, head = g:findedge(n1, "node@2", "n1 => n2")
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
  char *label = (char *) luaL_optstring(L, 4, NULL); /* ud, tail, head, label */
  if ((e = agfindedge(ud->g, tail->n, head->n)) != NULL){
    if ( !label  || !strcmp(agget(e, "label"), label)){
      rv = get_object(L, e);
      if (lua_isnil(L, -rv)){
	lua_pop(L, rv);                                /* ud, tail, head */
	/* Edge not yet registered */
	edge = lua_newuserdata(L, sizeof(gr_edge_t));  /* ud, peer, name, edge */
	edge->e = e;
	sprintf(sbuf, "edge@%d", AGID(e));
	edge->name = strdup(agget(e, "label"));
	edge->type = AGEDGE;
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
      /* Edge found but given label doesn't fit */
      lua_pushnil(L);
      lua_pushstring(L, "edge not found (invalid label)");
      return 2;
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
  if ((n = agidnode(ud->g, id)) != NULL){
    return get_object(L, n);
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "agidnode failed");
    return 2;
  }
}

/*-------------------------------------------------------------------------*\
 * Method: g.rename(self, name)
 * Renames a graph. Null name is NOT allowed.
 * Returns oldname.
 * Example:
 * oldname, err = g:rename("GNEWNAME")
\*-------------------------------------------------------------------------*/
static int gr_rename(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *name = (char *) luaL_checkstring(L, 2);
  char *oldname = agnameof(ud->g);
  agrename(ud->g, agstrdup(name));
  lua_pushstring(L, oldname);
  agstrfree(oldname);
  return 1;
}
/*-------------------------------------------------------------------------*\
 * Method: n = nextnode(self, prev)
 * Retrieves next node in a graph.
 * Returns the successor of the given node. With nil as prev the first
 * node is returned. 
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
 * Retrieves the graph to which given subgraph belongs. Trivial because it returns itself.
 * Returns graph userdata.
 * Example:
 * g, err = g.graph
\*-------------------------------------------------------------------------*/
int gr_graphof(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  return get_object(L, ud->g);
}

/*-------------------------------------------------------------------------*\
 * Method: rv = g.contains(self, obj)
 * Returns true if graph g contains object obj.
 * Returns boolean.
 * Example:
 * b = g.contains(n)
\*-------------------------------------------------------------------------*/
int gr_contains(lua_State *L)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  gr_object_t *obj = toobject(L, 2, NULL, STRICT);
  int rv = agcontains(ud->g, obj->p.p);
  if (rv && AGTYPE(obj->p.p) == AGNODE){
    if (obj->n.n->graph == ud->g->root)
      lua_pushboolean(L, TRUE);
    else
      lua_pushboolean(L, FALSE);
  } else 
    lua_pushboolean(L, rv);
  return 1;
}

#if DELETEME
static int getproto(lua_State *L, int kind)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);
  gr_node_t *node;
  gr_edge_t *edge;
  switch(kind){
  case AGNODE:
    Agnode_t *n = ud->g->proto->n;
    rv = get_object(L, n);                           /* ud, node, (nil) */
    if (lua_isnil(L, -rv)){
      lua_pop(L, 2);                                 /* ud */
      node = lua_newuserdata(L, sizeof(gr_node_t));  /* ud, udnode */
      node->n = n;
      node->name = strdup(agnameof(n));
      node->type = AGNODE;
      node->subg = ud->g;
      set_object(L, n);
      rv = new_node(L);
    } else 
      return rv;                                     /* ud, udnode */
    break;

  case AGEDGE:
    Agedge_t *e = ud->g->proto->e;
    rv = get_object(L, e);                           /* ud, node, (nil) */
    if (lua_isnil(L, -rv)){
      lua_pop(L, 2);                                 /* ud */
      edge = lua_newuserdata(L, sizeof(gr_edge_t));  /* ud, udnode */
      edge->e = e;
      edge-> = strdup(agnameof(n));
      node->type = AGNODE;
      node->subg = ud->g;
      set_object(L, n);
      rv = new_node(L);
    break;
  }

}

static int gr_getproto(lua_State *L)
{
  getproto(L, AGNODE);           /* ud, udnode */

  lua_pushvalue(L, -2);          /* ud, udnode, ud */
  getproto(L, AGEDGE);           /* ud, udnode, ud, udedge */
  lua_remove(L, -2);             /* ud, udnode, udedge */

  return 2;
}

#endif

static int gr_layout(lua_State *L)
{
  int rv;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *fmt = (char *) luaL_optstring(L, 2, "dot");
  if (ud->gvc){
    lua_pushnil(L);
    lua_pushstring(L, "layout already exists");
    return 2;
  }
  if (!strcmp(fmt, "dot") ||
      !strcmp(fmt, "neato") ||
      !strcmp(fmt, "nop") ||
      !strcmp(fmt, "nop2") ||
      !strcmp(fmt, "twopi") ||
      !strcmp(fmt, "fdp") ||
      !strcmp(fmt, "circo")){
    ud->gvc = gvContext();
    if ((rv = gvLayout(ud->gvc, ud->g, fmt)) != 0){
      gvFreeContext(ud->gvc);
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

static int gr_render(lua_State *L)
{
  int rv;
  gr_graph_t *ud = tograph(L, 1, STRICT);
  char *fmt = (char *) luaL_optstring(L, 2, "plain");
  char *fname = (char *) luaL_optstring(L, 3, NULL);
  if (ud->gvc == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "layout missing");
    return 2;
  }
  if (fname)
    rv = gvRenderFilename(ud->gvc, ud->g, fmt, fname);
  else
    rv = gvRender(ud->gvc, ud->g, fmt, stdout);
  if (rv != 0){
    gvFreeLayout(ud->gvc, ud->g);
    gvFreeContext(ud->gvc);
    lua_pushnil(L);
    lua_pushstring(L, "gvRender failed");
    return 2;
  }
  lua_pushnumber(L, rv);
  return 1;
}
#ifdef FINISHME
/*-------------------------------------------------------------------------*\
 * Callback functions
 * Example:
 * rv, err = g:nodecallback{insert=cbinsert, update=cbupdate, delete=cbdelete}
 * rv, err = g:setcallback{
 *   node = {
 *     insert=cbinsert, update=cbupdate, delete=cbdelete
 *   },
 *   edge = {
 *   }
 * }
\*-------------------------------------------------------------------------*/
static int setcallback(lua_State *L, const char *cbindex)
{
  gr_graph_t *ud = tograph(L, 1, STRICT);

  ud->cb = calloc(1, sizeof(Agcbdisc_t));

  /* Lua stack: ud, t */
  lua_pushstring(L, "node");                     /* ud, t, objkey */
  lua_gettable(L, -2);                           /* ud, t, tt */
  if (!lua_isnil(L, -1)){
    lua_pushstring(L, "modify");                 /* ud, t, tt, modkey */
    lua_gettable(L, -2);                         /* ud, t, tt, modfunc */
    if (!lua_isnil(L, -2)){
      lua_pushlightuserdata(L, &ud->cb.graph.mod); /* ud, t, tt, modfunc, key */
      lua_pushvalue(L, -2);                        /* ud, t, tt, modfunc, key, modfunc */
      lua_rawset(L, LUA_REGISTRYINDEX);            /* ud, t, tt, modfunc */
      lua_pop(L, 2);                               /* ud, t */
      ud->cb.graph.mod = update_callback;
    }
    lua_pushstring(L, "insert");                   /* ud, t, tt, inskey */
    lua_gettable(L, -2);                           /* ud, t, tt, insfunc */
    if (!lua_isnil(L, -2)){
      lua_pushlightuserdata(L, &ud->cb.graph.ins); /* ud, t, tt, insfunc, key */
      lua_pushvalue(L, -2);                        /* ud, t, tt, insfunc, key, insfunc */
      lua_rawset(L, LUA_REGISTRYINDEX);            /* ud, t, tt, insfunc */
      lua_pop(L, 2);                               /* ud, t */
      ud->cb.graph.ins = insert_callback;
    }
  } else {
  }
  if ((rv = agpushdisc(ud->g, ud->cb, TRUE)) != 0){
    luaL_error(L, "%s: cannot install `mod' callback function for nodes", "setcallback", "setcallback");
    return 0;
  }
}
#endif

/*-------------------------------------------------------------------------*\
 * Module initialization
\*-------------------------------------------------------------------------*/
LUALIB_API int luaopen_graph_core(lua_State *L) {
  luaL_openlib(L, MYNAME, funcs, 0);
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
  aginit();
  return 1;
}
