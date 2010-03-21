/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006
 *
 * Edge related functionality.
 *
 * $Id: gr_edge.c,v 1.1 2006-12-17 11:01:57 leuwer Exp $
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
static int gr_equal(lua_State *L);
static int gr_nameof(lua_State *L);
static int gr_id(lua_State *L);
static int gr_delete(lua_State *L);
static int gr_head(lua_State *L);
static int gr_tail(lua_State *L);
static int gr_graphof(lua_State *L);

/*=========================================================================*\
 * Data
\*=========================================================================*/

/*
 * Edge object methods
 */
static const luaL_reg reg_methods[] = {
  {"delete", gr_delete},
  {"type", get_object_type},
  {"rawget", getval},
  {NULL, NULL}
};

/*
 * Read-only members
 */
static const luaL_reg reg_rmembers[] = {
  {"head", gr_head},
  {"tail", gr_tail},
  {"name", gr_nameof},
  {"id", gr_id},
  {"graph", gr_graphof},
  {NULL, NULL}
};

/*
 * Standard metamethods
 */
static const luaL_reg reg_metamethods[] = {
  {"__eq", gr_equal},
  {"__gc", gr_delete},
  {"__newindex", object_newindex_handler},
  {NULL, NULL}
};

/*=========================================================================*\
 * Functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
 * Utility Functions
\*-------------------------------------------------------------------------*/
int new_edge(lua_State *L){
  return new_object(L, "edge", reg_rmembers, reg_methods, reg_metamethods, 
		    object_index_handler);
}

/*-------------------------------------------------------------------------*\
 * Metametho: __eq [boolean]
 * Compare two edges.
 * Example:
 * e1 == e2 or e1 ~= e2 with metamethods
\*-------------------------------------------------------------------------*/
static int gr_equal(lua_State *L)
{
  gr_edge_t *ud1 = toedge(L, 1, STRICT);
  gr_edge_t *ud2 = toedge(L, 2, STRICT);
  lua_pushboolean(L, (AGID(ud1->e) == AGID(ud2->e)));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: name [string]
 * Provides the name of an edge.
 * Example:
 * name = e.name
\*-------------------------------------------------------------------------*/
static int gr_nameof(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  lua_pushstring(L, ud->name);
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Property: id [number]
 * Get an edge's id.
 * Example:
 * n = e.id
\*-------------------------------------------------------------------------*/
static int gr_id(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  lua_pushnumber(L, AGID(ud->e));
  return 1;
}

/*-------------------------------------------------------------------------*\
 * Method: e.delete(self)
 * Delete an edge.
 * Returns non-nil on success.
 * Example:
 * rv, err = n:delete()
\*-------------------------------------------------------------------------*/
static int gr_delete(lua_State *L)
{
  int rv = -1;
  gr_edge_t *ud = toedge(L, 1, NONSTRICT);
  Agraph_t *g;
  if (ud->e != NULL){
    g = ud->e->tail->graph;
    TRACE("e.delete(): edge: ud=%p '%s' id=0x%x e=%p\n", 
	   (void *) ud, ud->name, AGID(ud->e), (void *)ud->e);
    del_object(L, ud->e);
    agdelete(g, ud->e);
    ud->e = NULL;
    if (ud->name){
      free(ud->name);
      ud->name = NULL;
    }
  } else {
    TRACE("e:delete(): ud=%p already closed\n", (void *)ud);
  }
  lua_pushnumber(L, rv);
  return 1;
}

/*
 * Provided for external access to gr_delete function.
 */
int gr_delete_edge(lua_State *L)
{
  return gr_delete(L);
}

/*-------------------------------------------------------------------------*\
 * Methods: n, err = e.head(self) and n, err = e.tail(self)
 * Retrieves head an tail node of an edge.
 * Returns node userdata on success.
 * Example:
 * head, err = e:head()
 * tail ,err = e.tail()
\*-------------------------------------------------------------------------*/
static int gr_head(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  Agnode_t *n = ud->e->head;
  if (n == NULL){
    lua_pushnil(L);
    return 1;
  } else
    return get_object(L, n);
}

static int gr_tail(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  Agnode_t *n = ud->e->tail;
  if (n == NULL){
    lua_pushnil(L);
    return 1;
  } else
    return get_object(L, n);
}

/*-------------------------------------------------------------------------*\
 * Determines the graph to which of a node belongs.
 * Returns graph userdata.
 * Example:
 * rv = n.graph
 * Methods: n, err = e.head(self) and n, err = e.tail(self)
\*-------------------------------------------------------------------------*/
static int gr_graphof(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  gr_node_t *udh;
  Agnode_t *head, *tail;
  Agraph_t *g;
  int rv;
  head = ud->e->head;
  tail = ud->e->tail;
  rv = get_object(L, head);
  udh = tonode(L, -rv, STRICT);
  g = udh->subg;
  lua_pop(L, rv);
  return get_object(L, g);
}
