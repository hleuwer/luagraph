/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006, 01/2017
 *
 * Edge related functionality.
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
static int gr_info(lua_State *L);
static int gr_graphof(lua_State *L);
static int gr_tostring(lua_State *L);

/*=========================================================================*\
 * Data
\*=========================================================================*/

/*
 * Edge object methods
 */
static const luaL_Reg reg_methods[] = {
  {"delete", gr_delete},
  {"type", get_object_type},
  {"rawget", getval},
  {"info", gr_info},
  {NULL, NULL}
};

/*
 * Read-only members
 */
static const luaL_Reg reg_rmembers[] = {
  {"head", gr_head},
  {"tail", gr_tail},
  {"name", gr_nameof},
  {"id", gr_id},
  {"graph", gr_graphof},
  {"status", gr_status},
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
  lua_pushboolean(L, ageqedge(ud1->e, ud2->e));
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
  if (ud->status != ALIVE)
    luaL_error(L, "deleted");
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
    g = agroot(ud->e);
    if (ud->status == ALIVE){
      TRACE("   e.delete(): deleting edge '%s' ud=%p id=0x%lx e=%p (%s %d)\n", 
            agnameof(ud->e), (void *) ud, (unsigned long) AGID(ud->e), (void *)ud->e, __FILE__, __LINE__);
      agdeledge(g, ud->e);
    }
  } else {
    TRACE("   e.delete(): ud=%p already closed (%s %d)\n", (void *)ud, __FILE__, __LINE__);
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
 * Print info about a node to stdout.
 * Example:
 * e:info()
\*-------------------------------------------------------------------------*/
static int gr_info(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  Agnode_t *head = aghead(ud->e);
  Agnode_t *tail = agtail(ud->e);
  Agedge_t *cedge = agopp(ud->e);
  Agnode_t *chead = aghead(cedge);
  Agnode_t *ctail = agtail(cedge);
  printf("INFO EDGE:\n");
  printf("  label: '%s' '%s'\n", agnameof(ud->e), ud->name);
  printf("  ptr : %p\n", (void *)ud->e);
  printf("  name: %s\n", agnameof(ud->e));
  printf("  head: %s\n", agnameof(head));
  printf("  tail: %s\n", agnameof(tail));
  printf("  cptr: %p\n", (void*)cedge); 
  printf("  cname: %s\n", agnameof(cedge));
  printf("  chead: %s\n", agnameof(chead));
  printf("  ctail: %s\n", agnameof(ctail));
  return 0;
}

/*-------------------------------------------------------------------------* \
 * Methods: n, err = e.head(self) and n, err = e.tail(self)
 * Retrieves head and tail node of an edge.
 * Returns node userdata on success.
 * Example:
 * head, err = e:head()
 * tail ,err = e.tail()
\*-------------------------------------------------------------------------*/
static int gr_head(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  Agnode_t *n = aghead(ud->e);
  if (n == NULL){
    lua_pushnil(L);
    return 1;
  } else
    return get_object(L, n);
}

static int gr_tail(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  Agnode_t *n = agtail(ud->e);
  if (n == NULL){
    lua_pushnil(L);
    return 1;
  } else
    return get_object(L, n);
}

/*-------------------------------------------------------------------------*\
 * Determines the graph to which of a edge belongs.
 * Returns graph userdata.
 * Example:
 * rv = n.graph
 * Methods: n, err = e.head(self) and n, err = e.tail(self)
\*-------------------------------------------------------------------------*/
static int gr_graphof(lua_State *L)
{
  gr_edge_t *ud = toedge(L, 1, STRICT);
  Agraph_t *g = agraphof(ud->e);
  if (g == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "no graph");
    return 2;
  }
  return get_object(L, g);
}

static int gr_tostring(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, NONSTRICT);
  lua_pushfstring(L, "edge: %p (%s)", ud, ud->p.status == ALIVE ? "alive" : "dead");
  return 1;
}
