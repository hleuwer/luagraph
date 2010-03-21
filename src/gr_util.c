/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006
 *
 * Utilities
 *
 * $Id: gr_util.c,v 1.1 2006-12-17 11:01:56 leuwer Exp $
\*=========================================================================*/

/*=========================================================================*\
 * Includes
\*=========================================================================*/
#include <string.h>
#include "lua.h"
#include "lauxlib.h"

#if 0
#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM < 501)
#include "compat-5.1.h"
#endif
#else
#include "compat-5.1.h"
#endif
#include "gr_graph.h"

/*=========================================================================*\
 * Functions
\*=========================================================================*/
/*
 * Query Lua stack for an object
 */
gr_object_t *toobject(lua_State *L, int narg, const char *type, int strict)
{
  gr_object_t *ud;
  if (type != NULL){
    ud = luaL_checkudata(L, narg, type);
  } else {
    ud = lua_touserdata(L, narg);
  }
  if (!ud){
    luaL_error(L, "bad argument #%d (valid userdata `%s' expected; null received)", 
	       narg, (type) ? type : "???");
    return NULL;
  }
  if (strict && (ud->p.p == NULL)){
    luaL_error(L, "bad argument #%d (valid userdata `%s' expected: invalid received)", 
	       narg, (type) ? type : "???");
    lua_error(L);
    return NULL;
  }
  return ud;
}

/*
 * Register an object in Lua registry.
 */
int set_object(lua_State *L, void *key)
{
  TRACE("set_object(): key=%p\n", key);
  lua_pushlightuserdata(L, key);       /* ud, key */
  lua_pushvalue(L, -2);                /* ud, key, ud */
  lua_rawset(L, LUA_REGISTRYINDEX);    /* ud */
  return 1;
}

/*
 * Delete an object from Lua registry.
 */
int del_object(lua_State *L, void *key)
{
  char *skey;
  TRACE("del_object(): key=%p\n", key);
  lua_pushlightuserdata(L, key);       /* ?, key */
  lua_pushnil(L);                      /* ?, key, nil */
  lua_rawset(L, LUA_REGISTRYINDEX);    /* ? */
  skey = agget(key, ".attrib");
  if (skey && (strlen(skey) != 0)) {
    lua_pushstring(L, skey);
    lua_pushnil(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
    agset(key, ".attrib", NULL);
    agstrfree(skey);
  }
  return 0;
}

/*
 * Get an object from Lua registry.
 */
int get_object(lua_State *L, void *key)
{
  TRACE("get_object():  key=%p\n", key);
  lua_pushlightuserdata(L, key);               /* ?, key */
  lua_rawget(L, LUA_REGISTRYINDEX);            /* ?, ud or nil */
  if (lua_isnil(L, -1)){                          
    lua_pushstring(L, "not found");            /* ?, nil, err */
    return 2;
  }
  return 1;                                    /* ?, ud */
}

/*
 * Get the value of a graphviz object attribute
 */
int getval(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, STRICT);
  char *key = (char *) luaL_checkstring(L, 2);
  char *value = agget(ud->p.p, key);
  if (!value || strlen(value) == 0){
    lua_pushnil(L);
    return 1;
  }
  lua_pushstring(L, value);
  return 1;
}

/*
 * Set a value to a graphviz object attribute
 */
static int setval(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, STRICT);
  char *key = (char *) luaL_checkstring(L, 2);
  char *value = (char *) luaL_checkstring(L, 3);
  return  agsafeset(ud->p.p, key, value, NULL);
}

/*
 * Generic object index metamethod handler
 * This closure has 2 upvalues:
 * 1. table with read members (this is the metatable)
 * 2. metatable.__metatable contains the methods
 * Lua stack: ud, key
 */
int object_index_handler(lua_State *L)
{
  gr_object_t *ud;
  char *skey;
  /* Read member lookup in first upvalue */
  lua_pushvalue(L, 2);                      /* ud, key, key */
  lua_rawget(L, lua_upvalueindex(1));       /* ud, key, getfunc (member) or nil (method) */
  if (!lua_isfunction(L, -1)){
    /* Member not found - try methods */
    lua_pop(L, 1);                          /* ud, key */
    lua_pushvalue(L, 2);                    /* ud, key, key */
    lua_gettable(L, lua_upvalueindex(2));   /* ud, key, method */
    if (lua_isnil(L, -1)){
      /* Try generic get method */
      if (lua_isstring(L, 2)){
	lua_settop(L, 2);                     /* ud, key */
	lua_pushcfunction(L, getval);         /* ud, key, gr_get */
	lua_pushvalue(L, 1);                  /* ud, key, gr_get, ud */
	lua_pushvalue(L, 2);                  /* ud, key, gr_get, ud, key */
	lua_call(L, 2, 1);                    /* ud, key, value or nil */
      }
      if (lua_isnil(L, -1)){
	/* Try generic storage */
	lua_settop(L, 2);                      /* ud, key */
	ud = toobject(L, 1, NULL, STRICT);
	skey = agget(ud->p.p, ".attrib");
	if (skey && strlen(skey) > 0){
	  lua_pushstring(L, skey);            /* ud, key, skey */
	  lua_rawget(L, LUA_REGISTRYINDEX);   /* ud, key, stab */
	  lua_pushvalue(L, 2);                /* ud, key, stab, key */
	  lua_rawget(L, -2);                  /* ud, key, stab, value */
	  lua_remove(L, -2);                  /* ud, key, value */
	} else
	  lua_pushnil(L);                     /* ud, key, nil */
      }
    }
    return 1;
  }
  /* Call the member's access function */
  lua_pushvalue(L, 1);                      /* ud, key, getfunc, ud */
  lua_pushvalue(L, 2);                      /* ud, key, getfunc, ud, key */
  lua_call(L, 2, 1);                        /* ud, key, value */
  return 1;
}

/*
 * Generic object newindex metamethod handler.
 * Lua Stack: ud, key, value
 */
int object_newindex_handler(lua_State *L)
{
  char sskey[16], *skey;
  if ((!lua_isstring(L, 2)) || (!lua_isstring(L, 3))){
    gr_object_t *ud = toobject(L, 1, NULL, STRICT);
    skey = agget(ud->p.p, ".attrib");
    if (!skey || (strlen(skey) == 0)){
      /* Let's create an attrib table on the fly if none exists */
      sprintf(sskey, "%p", ud->p.p);
      skey = agstrdup(sskey);
      agset(ud->p.p, ".attrib", skey);
      lua_pushstring(L, skey);             /* ud, key, value, skey */
      lua_newtable(L);                     /* ud, key, value, skey, stab */
      lua_rawset(L, LUA_REGISTRYINDEX);    /* ud, key, value, */
    }
    lua_pushstring(L, skey);          /* ud, key, value, skey */
    lua_rawget(L, LUA_REGISTRYINDEX); /* ud, key, value, stab */
    lua_pushvalue(L, 2);              /* ud, key, value, stab, key */
    lua_pushvalue(L, 3);              /* ud, key, value, stab, key, value */
    lua_rawset(L, -3);                /* ud, key, value, stab */
    lua_pop(L, -1);                   /* ud, key, value */
    return 0;
  }
  return setval(L);
}

/*
 * Create the graph object with member and method access.
 * Lua entry stack: ud
 * Lua exit stack:  ud
 */
int new_object(lua_State *L, const char *kind, const luaL_reg *reg_rmembers, 
	       const luaL_reg *reg_methods, const luaL_reg *reg_metamethods,
	       index_handler_t *index_handler)
{
  int methods, metatable, rv;

  /* Put methods in a table */
  lua_newtable(L);                         /* ud, mtab */
  register_metainfo(L, reg_methods);
  methods = lua_gettop(L);

  /* Put metamethods in a new metatable */
  luaL_newmetatable(L, kind);             /* ud, mtab, mt */

  register_metainfo(L, reg_metamethods);
  metatable = lua_gettop(L);

  /* Keep a reference to methods in the new metatable */
  lua_pushstring(L, "__metatable");        /* ud, mtab, mt, "__metatable" */
  lua_pushvalue(L, methods);               /* ud, mtab, mt, "__metatable", mtab */
  lua_rawset(L, -3);                       /* ud, mtab, mt */

  lua_pushstring(L, "__index");            /* ud, mtab, mt, "__index" */

  /* First upvalue: metatable => members access */
  lua_pushvalue(L, metatable);             /* ud, mtab, mt, "__index", mt(upv 1) */

  /* Put members into the metatable */
  register_metainfo(L, reg_rmembers);     

  /* Second upvalue, methods => methods access */
  lua_pushvalue(L, methods);              /* ud, mtab, mt, "__index", mt(upv 1), mtab(upv 2) */

  /* Set index_handler with 2 upvalues as __index metamethod */
  lua_pushcclosure(L, index_handler, 2);  /* ud, mtab, mt, "__index", func */
  lua_rawset(L, metatable);               /* ud, mtab, mt */

  rv = lua_setmetatable(L, -3);           /* ud, mtab */
  lua_pop(L,1);                           /* ud */

  return 1;
}

/*
 * Check an object type.
 */
void *chk_object(lua_State *L, void *obj)
{
  gr_object_t *ud = (gr_object_t *) obj;
  if (ud->p.p == NULL){
    lua_pushfstring(L, "invalid userdata of type `%s' detected", 
		    ud->p.type == AGGRAPH ? "graph" :
		    ud->p.type == AGNODE ? "node" :
		    ud->p.type == AGEDGE ? "edge" : "unknown");
    lua_error(L);
    return NULL;
  }
  return obj;
}

/*-------------------------------------------------------------------------*\
 * Generic method: type = obj.type(self)
 * Returns type of a graph object.
 * Returns the type as string: 'graph', 'node' or 'edge'.
 * Example:
 * type = n:type()
\*-------------------------------------------------------------------------*/
int get_object_type(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, STRICT);
  lua_pushstring(L, AGTYPE(ud->p.p) == AGGRAPH ? "graph" :
		 AGTYPE(ud->p.p) == AGNODE ? "node" :
		 AGTYPE(ud->p.p) == AGEDGE ? "edge" : "unkown");
  return 1;
}

#ifdef FINISHME
void insert_callback(Agobj_t *obj, void *arg)
{
  gr_cbrecord_t *cbrec = (gr_cbrecord_t *)aggetrec(obj, "lua_State", FALSE);
  lua_State *L = cbrec->L;
}

void delete_callback(Agobj_t *obj, void *arg)
{
}

void update_callback(Agobj_t *obj, void *arg, Agsym_t *sym)
{
  gr_cbrecord_t *cbrec = (gr_cbrecord_t *)aggetrec(obj, "lua_State", FALSE);
  lua_State *L = cbrec->L;
  char *key = sym->name;
  char *value = agxget(obj, sym);
  gr_object_t *ud = get_object(L, obj);         /* ud */
  if (!obj){
    luaL_error(L, "not found");
  }
  lua_pushlightuserdata(L, &ud->p.cb.mod);           /* ud, key */
  lua_rawget(L, LUA_REGISTRYINDEX);             /* ud, func */
  lua_pushvalue(L, -2);                         /* ud, func, ud */
  lua_pushstring(L, key);                       /* ud, func, ud, key */
  lua_pushstring(L, value);                     /* ud, func, ud, key, value */
  lua_call(L, 3, 0);                            /* ud */
  lua_pop(L, 1);
}
#endif

/*
 * Some homemade extension to libgraph.
 */
Agraph_t *agfstsubg(Agraph_t *g, Agedge_t **lastedge)
{
  Agedge_t *e;
  Agraph_t *meta = g->meta_node->graph;
  e = agfstout(meta, g->meta_node);
  *lastedge = e;
  if (!e)
    return NULL;
  return agusergraph(e->head);
}

Agraph_t *agnxtsubg(Agraph_t *g, Agedge_t **lastedge)
{
  Agedge_t *e;
  Agraph_t *meta = g->meta_node->graph;
  e = agnxtout(meta, *lastedge);
  *lastedge = e;
  if (!e)
    return NULL;
  return agusergraph(e->head);
}
