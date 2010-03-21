/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006
 *
 * Node related functionality.
 *
 * $Id: gr_node.c,v 1.1 2006-12-17 11:01:56 leuwer Exp $
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
/*=========================================================================*\
 * Defines
\*=========================================================================*/

/*=========================================================================*\
 * Prototypes
\*=========================================================================*/
static int gr_nameof(lua_State *L);
static int gr_id(lua_State *L);
static int gr_equal(lua_State *L);
static int gr_delete(lua_State *L);
static int gr_rename(lua_State *L);
static int gr_degree(lua_State *L);
static int gr_graphof(lua_State *L);
static int gr_edge(lua_State *L);
static int gr_nextedge(lua_State *L);
static int gr_walkedges(lua_State *L);
static int gr_nextinput(lua_State *L);
static int gr_walkinputs(lua_State *L);
static int gr_nextoutput(lua_State *L);
static int gr_walkoutputs(lua_State *L);

/*=========================================================================*\
 * Data
\*=========================================================================*/
static const luaL_reg reg_methods[] = {
  {"delete", gr_delete},
  {"rename", gr_rename},
  {"degree", gr_degree},
  {"edge", gr_edge},
  {"nextedge", gr_nextedge},
  {"walkedges", gr_walkedges},
  {"nextinput", gr_nextinput},
  {"walkinputs", gr_walkinputs},
  {"nextoutput", gr_nextoutput},
  {"walkoutputs", gr_walkoutputs},
  {"type", get_object_type},
  {"rawget", getval},
  {NULL, NULL}
};

static const luaL_reg reg_rmembers[] = {
  {"id", gr_id},
  {"name", gr_nameof},
  {"graph", gr_graphof},
  {NULL, NULL}
};

static const luaL_reg reg_metamethods[] = {
  {"__eq", gr_equal},
  {"__gc", gr_delete},
  {"__newindex", object_newindex_handler},
  {"__concat", gr_edge},
  {"__add", gr_edge},
  {NULL, NULL}
};
/*=========================================================================*\
 * Functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
 * Utility Functions
\*-------------------------------------------------------------------------*/
int new_node(lua_State *L){
  return new_object(L, "node", reg_rmembers, reg_methods, reg_metamethods, 
		    object_index_handler);
}

/*-------------------------------------------------------------------------*\
 * Metamethod: __eq [boolean]
 * Compare two nodes.
 * Example:
 * n1 == n2 or n1 ~= n2 with metamethods
\*-------------------------------------------------------------------------*/
static int gr_equal(lua_State *L)
{
  gr_node_t *ud1 = tonode(L, 1, STRICT);
  gr_node_t *ud2 = tonode(L, 2, STRICT);
  lua_pushboolean(L, (AGID(ud1->n) == AGID(ud2->n)));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: name [string]
 * Provides the name of a node.
 * Example:
 * name = g.name
\*-------------------------------------------------------------------------*/
static int gr_nameof(lua_State *L)
{
  gr_node_t *ud = tonode(L, 1, STRICT);
  lua_pushstring(L, agnameof(ud->n));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: id [number]
 * Get a node's id.
 * Example:
 * n = n.id
\*-------------------------------------------------------------------------*/
static int gr_id(lua_State *L)
{
  gr_node_t *ud = tonode(L, 1, STRICT);
  lua_pushnumber(L, AGID(ud->n));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: n.delete(self)
 * Delete a node. All associated edges are deleted as well.
 * Returns non-nil on success.
 * Example:
 * rv, err = n:delete(h)
\*-------------------------------------------------------------------------*/
static int gr_delete(lua_State *L)
{
  Agraph_t *g;
  gr_node_t *ud = tonode(L, 1, NONSTRICT);
  if (ud->n != NULL){
    /* Delete all associated edges with tail on this node */
    Agedge_t *se, *ne;
    int ix;
    g = ud->n->graph;
    se = agfstedge(g, ud->n);
    while (se){
      ne = agnxtedge(g, se, ud->n);
      ix = get_object(L, se);         /* ud, se */
      if (!(lua_isnil(L, -ix))){
	TRACE("n:delete(): closing subedge: ud=%p 'edge@%d' id=0x%0x e=%p\n", 
	       (void *) lua_touserdata(L, -ix), AGID(se), AGID(se), (void *)se);
	lua_pushcfunction(L, gr_delete_edge); /* ud, se, func */
	lua_pushvalue(L, -2);                 /* ud, se, func, se */
	lua_call(L, 1, LUA_MULTRET);          /* ud, se */
	lua_pop(L, 1);                        /* ud */
      } else {
	lua_pop(L, 2);                        /* ud */
      }
      se = ne;
    }
    TRACE("n:delete(): ud=%p '%s' id=0x%0x \n", 
	  (void *) ud, agnameof(ud->n), AGID(ud->n));
    del_object(L, ud->n);
    agdelete(g, ud->n);
    ud->n = NULL;
    if (ud->name){
      free(ud->name);
      ud->name = NULL;
    }
  } else {
    TRACE("n:delete(): ud=%p already closed\n", (void *)ud);
  }
  lua_pushnumber(L, 0);
  return 1;
}

/*
 * Provided for external access to gr_delete function.
 */
int gr_delete_node(lua_State *L)
{
  return gr_delete(L);
}

/*-------------------------------------------------------------------------*\
 * Method: n.rename(self, name)
 * Renames a graph. Null name will assign an auto-name 'node@ID'.
 * Returns old name.
 * Example:
 * oldname, err = n:rename("NODENEWNAME") or n:rename()
\*-------------------------------------------------------------------------*/
static int gr_rename(lua_State *L)
{
  char sbuf[32];
  gr_node_t *ud = tonode(L, 1, STRICT);
  char *name = (char *) lua_tostring(L, 2);
  char *oldname = agnameof(ud->n);
  if (!name){
    sprintf(sbuf, "node@%d", AGID(ud->n));
    agrename(ud->n, agstrdup(sbuf));
    free(ud->name);
    ud->name = strdup(sbuf);
  } else {
    agrename(ud->n, agstrdup(name));
    free(ud->name);
    ud->name = strdup(name);
  }
  lua_pushstring(L, oldname);
  agstrfree(oldname);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: n.degree(self, what)
 * Determines degree of a node. 
 * Returns number of in-edges (what='*i'), out-edges (what='*o') or the sum 
 * of all edges (what='*a') to/from/of a node.
 * Example:
 * rv, err = n:degree("*i")
\*-------------------------------------------------------------------------*/
int gr_degree(lua_State *L)
{
  int count = 0;
  Agedge_t *e;
  Agraph_t *g;
  Agnode_t *n;
  gr_node_t *ud = tonode(L, 1, STRICT);
  char *flag = (char *) luaL_optstring(L, 2, "*a");
  int indeg = TRUE;
  int outdeg = TRUE;
  n = ud->n;
  g = ud->n->graph;
  if (*flag != '*'){
    luaL_error(L, "invalid format specifier");
    return 0;
  }
  switch(*(flag+1)){
  case 'i': outdeg = FALSE; break;
  case 'o': indeg = FALSE; break;
  }
  if (indeg){
    for (e = agfstin(g, n); e; e = agnxtin(g, e)){
      count++;
    }
  }
  if (outdeg){
    for (e = agfstout(g, n); e; e = agnxtout(g, e)){
      count++;
    }
  }
  lua_pushnumber(L, count);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: n.graph
 * Determines the graph to which of a node belongs.
 * Returns graph userdata.
 * Example:
 * rv = n.graph
\*-------------------------------------------------------------------------*/
int gr_graphof(lua_State *L)
{
  int rv;
  gr_node_t *ud = tonode(L, 1, STRICT);
  //  Agraph_t *g = ud->n->u.subg;
  Agraph_t *g = ud->subg;
  if (g == NULL){
    lua_pushnil(L);
    return 1;
  }
  rv = get_object(L, g);
  if (rv == 2){
    lua_error(L);
    return 0;
  } else {
    return rv;
  }
}

/*-------------------------------------------------------------------------*\
 * Method: e, tail, head = n.edge(self, node, label, flag)
 * Finds or creates an edge. The given node becomes the tail of the edge.
 * Label is optional.
 * The optional flag nocreate=true inhibits auto-creation.
 * Any node given by name is implicitly created and it's userdata returned
 * as additional results - even if nocreate is not set.
 * Example:
 * e, tail, head = e:node(n2, "edge-1")
 * e, err = g:edge(...)
\*-------------------------------------------------------------------------*/
static int gr_edge(lua_State *L)
{
  Agedge_t *e;
  gr_edge_t *edge;                 
  int rv;
  char *label;
  char sbuf[32];
  Agraph_t *g;
  gr_node_t *head;
  gr_node_t *tail = tonode(L, 1, STRICT);
  if (lua_isuserdata(L, 2))
    head = tonode(L, 2, STRICT);
  else {
    /* Create a node given by name */
    lua_pushcfunction(L, gr_create_node);            /* tail, nhead, (label), func */
    get_object(L, tail->n->graph);                   /* tail, nhead, (label), func, graph */
    lua_pushstring(L, (const char *) luaL_checkstring(L, 2)); /* ... func, graph, nhead */
    if (lua_isboolean(L, 4))
      lua_pushvalue(L, 4);
    else
      lua_pushboolean(L, 0);
    /* g.node(self, name, flag) */
    lua_call(L, 3,  1);                              /* tail, nhead, (label), head */ 
    head = tonode(L, -1, STRICT);
    lua_pop(L,1);                                    /* tail, nhead, (label) */
  }

  g = tail->n->graph;
  if (tail->n->graph != head->n->graph){
    luaL_error(L, "head/tail not in same graph");
  }
  label = (char *) luaL_optstring(L, 3, "");         /* ud, peer, name, (flag) */
  if ((e = agfindedge(g, tail->n, head->n)) != NULL){
    /* Edge exists */
    rv = get_object(L, e);                           /* ud, peer, name, (flag), edge */
    if (lua_isnil(L, -rv)){
      lua_pop(L, rv);                                /* ud, peer, name, (flag) */ 
      /* Edge not yet registered */
      edge = lua_newuserdata(L, sizeof(gr_edge_t));  /* ud, peer, name, (flag), edge */
      edge->e = e;
      if (strlen(label) > 0)
	agset(e, "label", label);
      sprintf(sbuf, "edge@%d", AGID(e));
      edge->name = strdup(sbuf);
      edge->type = AGEDGE;
      set_object(L, e);              /* ud, peer, name, (flag), edge */
      new_edge(L);
      lua_pushlightuserdata(L, tail);
      lua_pushlightuserdata(L, head);     /* ud, peer, name, (flag), edge, tail, head */
      return 3;
    } else {
      /* Edge already registered */
      lua_pushlightuserdata(L, tail);
      lua_pushlightuserdata(L, head);
      return rv + 2;                 /* ud, peer, name, (flag), edge, tail, head */
    }
  } else {
    /* Edge does not exist */
    if (lua_toboolean(L, 4)){
      lua_pushnil(L);
      lua_pushstring(L, "edge not found");
      return 2;
    }
    edge = lua_newuserdata(L, sizeof(gr_edge_t));
    if (!(edge->e = agedge(g, tail->n, head->n))){
      luaL_error(L, "agedge failed");
      return 0;
    }
    if (strlen(label) > 0)
      agset(edge->e, "label", label);
    sprintf(sbuf, "edge@%d", AGID(edge->e));
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
 * Method: e = nextedge(self, prev)
 * Retrieves next edge of a node.
 * Returns the next edge of the given node. With nil as prev the first
 * edge is returned. 
 * Example:
 * first = n:nextedge(nil)
 * second = n:nextedge(first)
 * third = n:nextedge(second)
\*-------------------------------------------------------------------------*/
static int gr_nextedge(lua_State *L)
{
  int rv;
  char sbuf[32];
  Agraph_t *g;
  Agedge_t *e;
  gr_edge_t *ud_e;
  gr_node_t *ud_n = tonode(L, 1, STRICT);
  g = ud_n->n->graph;

  if (lua_isnil(L, 2)){
    e = agfstedge(g, ud_n->n);
  } else {
    ud_e = toedge(L, 2, STRICT);
    e = agnxtedge(g, ud_e->e, ud_n->n);
  }
  if (!e){
    /* no more edges */
    lua_pushnil(L);
    return 1;
  } else {
    /* Check whether userdata exists .. */
    rv = get_object(L, e);
    if (rv == 1){
      /* .. yes: return it */
      return rv;
    } else {
      /* .. no: create it */
      lua_pop(L, rv);
      ud_e = lua_newuserdata(L, sizeof(gr_edge_t)); 
      ud_e->e = e;
      sprintf(sbuf, "edge@%d", AGID(e));
      ud_e->name = strdup(sbuf);
      ud_e->type = AGEDGE;
      set_object(L, e);
      return new_edge(L);
    }
  }
}

/*-------------------------------------------------------------------------*\
 * Iterator: walkedges()
 * Iterator over all edges of a given node.
 * Example:
 * for n in g:walkedges() do ... end
\*-------------------------------------------------------------------------*/
static int gr_walkedges(lua_State *L)
{
  lua_pushcfunction(L, gr_nextedge);     /* ud, nextedge */
  lua_pushvalue(L, -2);                  /* ud, nextedge, n */
  lua_pushnil(L);                        /* ud, nextedge, n, nil */
  return 3;
}

/*-------------------------------------------------------------------------*\
 * Method: e = nextinput(self, prev)
 *         e = nextoutput(self, prev)
 * Same behaviour as nextedge, but only input or output edges are returned.
 * Example:
 * first = n:nextinput(nil)
 * second = n:nextinput(first)
 * third = n:nextinput(second)
\*-------------------------------------------------------------------------*/
typedef Agedge_t* (edge_first_iter_t)(Agraph_t *g, Agnode_t *n);
typedef Agedge_t* (edge_next_iter_t)(Agraph_t *g, Agedge_t *e);

static int gr_nextinout(lua_State *L, edge_first_iter_t *fst, edge_next_iter_t *nxt)
{
  int rv;
  Agedge_t *e;
  char sbuf[32];
  gr_edge_t *ud_e;
  gr_node_t *ud_n = tonode(L, 1, STRICT);
  Agraph_t *g = ud_n->n->graph;

  if (lua_isnil(L, 2))
    e = fst(g, ud_n->n);
  else {
    ud_e = toedge(L, 2, STRICT);
    e = nxt(g, ud_e->e);
  }
  if (!e){
    /* no more nodes */
    lua_pushnil(L);
    return 1;
  } else {
    /* Check whether userdata exists .. */
    rv = get_object(L, e);
    if (rv == 1)
      /* .. yes: return it */
      return rv;
    else {
      /* .. no: create it */
      lua_pop(L, rv);
      ud_e = lua_newuserdata(L, sizeof(gr_edge_t)); 
      ud_e->e = e;
      sprintf(sbuf, "edge@%d", AGID(e));
      ud_e->name = strdup(sbuf);
      ud_e->type = AGEDGE;
      set_object(L, e);
      return new_edge(L);
    }
  }
}

static int gr_nextinput(lua_State *L)
{
  return gr_nextinout(L, agfstin, agnxtin);
}

static int gr_nextoutput(lua_State *L)
{
  return gr_nextinout(L, agfstout, agnxtout);
}
/*-------------------------------------------------------------------------*\
 * Iterator: e = n.walkinputs(self)
 * Iterator over all input edges of a given node.
 * Example:
 * for e in n:walkinputs() do ... end
\*-------------------------------------------------------------------------*/
static int gr_walkinputs(lua_State *L)
{
  lua_pushcfunction(L, gr_nextinput);    /* ud, nextedge */
  lua_pushvalue(L, -2);                  /* ud, nextedge, n */
  lua_pushnil(L);                        /* ud, nextedge, n, nil */
  return 3;
}

/*-------------------------------------------------------------------------*\
 * Iterator: e = walkoutputs(self)
 * Iterator over all input edges of a given node.
 * Example:
 * for e in n:walkoutputs() do ... end
\*-------------------------------------------------------------------------*/
static int gr_walkoutputs(lua_State *L)
{
  lua_pushcfunction(L, gr_nextoutput);    /* ud, nextedge */
  lua_pushvalue(L, -2);                  /* ud, nextedge, n */
  lua_pushnil(L);                        /* ud, nextedge, n, nil */
  return 3;
}

