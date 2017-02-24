/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006, 01/2017
 *
 * Node related functionality.
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

/*=========================================================================* \
 * Defines
\*=========================================================================*/

/*=========================================================================*\
 * Prototypes
\*=========================================================================*/
static int gr_nameof(lua_State *L);
static int gr_id(lua_State *L);
static int gr_equal(lua_State *L);
static int gr_delete(lua_State *L);
static int gr_degree(lua_State *L);
static int gr_graphof(lua_State *L);
static int gr_edge(lua_State *L);
static int gr_nextedge(lua_State *L);
static int gr_walkedges(lua_State *L);
static int gr_nextinput(lua_State *L);
static int gr_walkinputs(lua_State *L);
static int gr_nextoutput(lua_State *L);
static int gr_walkoutputs(lua_State *L);
static int gr_info(lua_State *L);
static int gr_tostring(lua_State *L);

/*=========================================================================*\
 * Data
\*=========================================================================*/
static const luaL_Reg reg_methods[] = {
  {"delete", gr_delete},
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
  {"info", gr_info},
  {NULL, NULL}
};

static const luaL_Reg reg_rmembers[] = {
  {"id", gr_id},
  {"name", gr_nameof},
  {"status", gr_status},
  {"graph", gr_graphof},
  {NULL, NULL}
};

static const luaL_Reg reg_metamethods[] = {
  {"__gc", gr_collect},
  {"__eq", gr_equal},
  {"__newindex", object_newindex_handler},
  {"__concat", gr_edge},
  {"__add", gr_edge},
  {"__tostring", gr_tostring},
  {NULL, NULL}
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
  if (ud->status != ALIVE){
    luaL_error(L, "deleted");
    return 0;
  }  
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
 * Write info about a node to stdout.
 * Example:
 * n:info()
\*-------------------------------------------------------------------------*/
static int gr_info(lua_State *L)
{
  Agraph_t  *g;
  gr_node_t *ud = tonode(L, 1, STRICT);
  Agedge_t *se;
  Agsym_t *sym;
  
  g = agraphof(ud->n);
  printf("INFO NODE '%s' '%s' id=%lu seq=%d\n", agnameof(ud->n), ud->name, (unsigned long) AGID(ud->n), AGSEQ(ud->n));
  printf("  ptr: %p\n", ud->n);
  printf("  Symbols:\n");
  se = agfstout(g, ud->n);
  sym=0;
  while ((sym = agnxtattr(g,AGNODE,sym))!=NULL)
         printf("    %s = '%s'\n",sym->name,sym->defval);
#if 0
  printf("  Out edges: d-out=%d u-out=%d\n", agdegree(g, ud->n, 0, 1), agcountuniqedges(g, ud->n, 0, 1));
#endif
  while (se) {
    printf("    name: '%s', head: '%s', tail: '%s' id=%lud, seq=%d %p\n",
           agnameof(se), agnameof(aghead(se)), agnameof(agtail(se)), (unsigned long) AGID(se), AGSEQ(se), (void*)se);
    se = agnxtout(g, se);
  }
#if 0
  printf("  In edges: d-in=%d u-in=%d\n", agdegree(g, ud->n, 1, 0), agcountuniqedges(g, ud->n, 1, 0));
#endif
  se = agfstin(g, ud->n);
  while (se) {
    printf("    name: '%s', head: '%s', tail: '%s' îd=%lu seq=%d %p\n",
           agnameof(se), agnameof(aghead(se)), agnameof(agtail(se)), (unsigned long) AGID(se), AGSEQ(se), (void*)se);
    se = agnxtin(g, se);
  }
#if 0
  printf("  Edges: d-io=%d u-io=%d\n", agdegree(g, ud->n, 1, 1), agcountuniqedges(g, ud->n, 1, 1));
#endif  
  se = agfstedge(g, ud->n);
  while (se) {
    printf("    name: '%s', head: '%s', tail: '%s' id=%lud seq=%d %p\n",
           agnameof(se), agnameof(aghead(se)), agnameof(agtail(se)), (unsigned long) AGID(se), AGSEQ(se), (void*)se);
    se = agnxtedge(g, se, ud->n);
  }
  return 0;
}

/*-------------------------------------------------------------------------* \
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
    g = agraphof(ud->n);
    if (ud->status == ALIVE){
      TRACE("   n.delete(): deleting node '%s' ud=%p id=0x%0lx (%s %d) \n", 
            agnameof(ud->n), (void *) ud, (unsigned long) AGID(ud->n), __FILE__, __LINE__);
      agdelnode(g, ud->n);
    }
  } else {
    TRACE("   n:delete(): ud=%p already closed (%s %d)\n", (void *)ud, __FILE__, __LINE__);
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
  g = agroot(ud->n);
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
  gr_node_t *ud = tonode(L, 1, STRICT);
  Agraph_t *g = agraphof(ud->n);
  if (g == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "no graph");
    return 2;
  }
  return get_object(L, g);
}

/*-------------------------------------------------------------------------*\
 * Method: e, tail, head = n.edge(self, node, label, flag)
 * Finds or creates an edge. The given node is the tail of the edge.
 * The created node is the the edge.
 * Label is optional.
 * The optional flag nocreate=true inhibits auto-creation of the edge.
 * Any node given by name is implicitly created and it's userdata returned
 * as additional results - even if nocreate is not set.
 * Example:
 * e, tail, head = n.edge(head, "edge-1", true | false)
 * e, tail, head = e:node("headname", "edge-1", true | false)
\*-------------------------------------------------------------------------*/
static int gr_edge(lua_State *L)
{
  Agedge_t *e;
  gr_edge_t *edge;                 
  int rv;
  char *label;
  char ename[32];
  Agraph_t *g;
  gr_node_t *head;
  gr_node_t *tail = tonode(L, 1, STRICT);

  g = agroot(tail->n);
  
  if (lua_isuserdata(L, 2))
    head = tonode(L, 2, STRICT);
  else {
    lua_pushcfunction(L, gr_create_node);             /* tail, nhead, (label), (nocreate), func */
    get_object(L, agroot(tail->n));                   /* tail, nhead, (label), (nocreate), func, graph */
    lua_pushvalue(L, 2);                              /* ... func, graph, nhead */
    if (lua_isboolean(L, 4))                                  
      lua_pushvalue(L, 4);                            /* ... func, graph, nhead, (nocreate) */
    else
      lua_pushboolean(L, 0);                          /* ... func, graph, nhead, false */
    lua_call(L, 3,  1);                              /* tail, nhead, (label), head */
    if (lua_isnil(L, -1))
      return  2;
    head = tonode(L, -1, STRICT);
    lua_pop(L,1);                                    /* tail, nhead, (label) */
  }
  
  g = agroot(tail->n);
  if (g != agroot(head->n)){
    luaL_error(L, "head/tail not in same graph");
  }
  label = (char *) luaL_optstring(L, 3, NULL);         /* ud, peer, name, (flag) */
  if ((e = agedge(g, tail->n, head->n, label, 0)) != NULL){
    rv = get_object(L, e);                           /* ud, peer, name, (flag), edge */
    if (lua_isnil(L, -rv)){
      /* not yet registered */
      lua_pop(L, rv);                                /* ud, peer, name, (flag) */ 
      edge = lua_newuserdata(L, sizeof(gr_edge_t));  /* ud, peer, name, (flag), edge */
      edge->e = e;
      if (label)
	agset(e, "label", label);
      edge->name = strdup(agnameof(e));
      edge->type = AGEDGE;
      edge->status = ALIVE;
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
    sprintf(ename, "edge@%u", newid());
    if ((edge->e = agedge(g, tail->n, head->n, ename, 1)) == NULL){
      luaL_error(L, "agedge failed");
      return 0;
    }
    if (label)
      agset(edge->e, "label", label);
    edge->name = strdup(agnameof(edge->e));
    edge->type = AGEDGE;
    edge->status = ALIVE;
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
  g = agroot(ud_n->n);

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
      sprintf(sbuf, "edge@%lu", (unsigned long) AGID(e));
      ud_e->name = strdup(sbuf);
      ud_e->type = AGEDGE;
      ud_e->status = ALIVE;
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
  Agraph_t *g = agroot(ud_n->n);

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
      sprintf(sbuf, "edge@%lu", (unsigned long) AGID(e));
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

/*-------------------------------------------------------------------------*\
 * Metamethod: __tostring [boolean]
 * Return a string presentation of a graph object.
 * Example:
 * g == h or g != h with metamethods
\*-------------------------------------------------------------------------*/
static int gr_tostring(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, NONSTRICT);
  lua_pushfstring(L, "node: %p (%s)", ud, ud->p.status == ALIVE ? "alive" : "dead");
  return 1;
}
