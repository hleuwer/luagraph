/*=========================================================================*\
 * LuaGRAPH toolkit
 * Graph support for Lua.
 * Herbert Leuwer
 * 30-7-2006, 01/2017
 *
 * Utilities
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
 * Functions
\*=========================================================================*/
static unsigned int localid = 0;

unsigned int newid(void){
  localid = localid + 1;
  return localid;
}

/*
 * Convert numerical kind indication into readable text string.
 */
char *agobjkindstr(struct Agobj_s *obj)
{
  int kind = agobjkind(obj);
  return kind == AGRAPH ? "graph" : kind == AGNODE ? "node" : kind == AGEDGE ? "edge" : "unknown";
}

/*
 * Insertion callback: called when an object is inserted by cgraph.
 * Used to register proxy object on top of stack in Lua registry using cgraph object as key.
 */
void cb_insert(struct Agraph_s *g, struct Agobj_s *obj, void *L)
{
  /* register user data on top of stack */
  TRACE("   cb_insert(): register object '%s' type: %s\n", agnameof(obj), agobjkindstr(obj));
  set_object((lua_State *)L, (void *) obj);
}

/*
 * Deletion callback: called when an object is inserted by cgraph.
 * De-register proxy object.
 */
void cb_delete(Agraph_t *g, Agobj_t *obj, void *L)
{
  char *skey;

  TRACE("   cb_delete() : unregister object '%s' kind=%s ptr=%p\n", agnameof(obj),
        agobjkind(obj) == AGRAPH ? "graph" : agobjkind(obj) == AGNODE ? "node" :
        agobjkind(obj) == AGEDGE ? "edge" : "unknown", obj);
  skey = agget(obj, "__attrib__");
  if (skey && (strlen(skey) != 0)) {
    printf("   cb_delete(): nulling attrib\n");
    lua_pushstring(L, skey);
    lua_pushnil(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
    agset(obj, "__attrib__", NULL);
    agstrfree(agroot(obj), skey);
  }
  del_object(L, obj);
}

/*
 * Modification callback. Not used by LuaGRAPH
 */ 
void cb_modify(Agraph_t *g, Agobj_t *obj, void *L, Agsym_t *sym)
{
  TRACE("   cb_modify(): modify object '%s'\n", agnameof(obj));
}

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
    luaL_error(L, "bad argument %d (valid userdata `%s' expected; null received)", 
	       narg, (type) ? type : "???");
    return NULL;
  }
    if (strict && (ud->p.p == NULL)){
      luaL_error(L, "bad argument #%d (valid userdata `%s' %d expected: invalid received)", 
	       narg, (type) ? type : "???", ud->p.status);
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
  TRACE("   set_object(): key=%p (%s %d)\n", key, __FILE__, __LINE__);
  lua_pushlightuserdata(L, key);       /* ud, key */
  lua_pushvalue(L, -2);                /* ud, key, ud */
  lua_rawset(L, LUA_REGISTRYINDEX);    /* ud */
  return 1;
}

int set_status(lua_State *L, void *key, int status)
{
  gr_object_t *ud;
  
  lua_pushlightuserdata(L, key);       /* ?, key */
  lua_rawget(L, LUA_REGISTRYINDEX);
  ud = lua_touserdata(L, -1);
  if (ud)
    ud->p.status = status;
  return 0;
}
/*
 * Delete an object from Lua registry.
 */
int del_object(lua_State *L, void *key)
{
  TRACE("   del_object(): key=%p '%s' (%s %d)\n", key, agnameof(key), __FILE__, __LINE__);
  set_status(L, key, DEAD);
  lua_pushlightuserdata(L, key);
  lua_pushnil(L);                      /* ?, key, nil */
  lua_rawset(L, LUA_REGISTRYINDEX);    /* ? */
  return 0;
}

/*-------------------------------------------------------------------------*\
 * Property: status [string]
 * Returns the string "alive" or "dead" for the given graph.
 * Use to test whether a reference to a graph object kept in a lua variable
 * is still alive.
 * Example:
 * b = g.status
\*-------------------------------------------------------------------------*/
int gr_status(lua_State *L)
{
  gr_object_t *ud = toobject(L, 1, NULL, NONSTRICT);
  if (ud){
    if (ud->p.status == ALIVE)
      lua_pushstring(L, "alive");
    else
      lua_pushstring(L, "dead");
  } else {
    lua_pushstring(L, "???");
  }
  return 1;
}

int gr_collect(lua_State *L)
{
  gr_object_t *ud = lua_touserdata(L, 1);
  TRACE("   gr_collect(): %s ud=%p ptr=%p\n", ud->p.name, ud, ud->p.p);
  if (ud->p.name)
    free(ud->p.name);
  ud->p.p = NULL;
  return 0;
}

/*
 * Get an object from Lua registry.
 */
int get_object(lua_State *L, void *key)
{
  gr_object_t *ud;
  lua_pushlightuserdata(L, key);               /* ?, key */
  lua_rawget(L, LUA_REGISTRYINDEX);            /* ?, ud or nil */
  ud = lua_touserdata(L, -1);
  if (ud == NULL){                          
    TRACE("   get_object(): key = %p not found (%s %d)\n", key, __FILE__, __LINE__);
    lua_pushstring(L, "object not found in registry");            /* ?, nil, err */
    return 2;
  }
  TRACE("   get_object(): key=%p ud=%p '%s' (%s %d)\n", key, ud, ud->p.name, __FILE__,__LINE__);
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
  //  return agset(ud->p.p, key, value);
  return  agsafeset(ud->p.p, key, value, "");
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
	skey = agget(ud->p.p, "__attrib__");
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
  TRACE("   newindex(): key='%s' value='%s'\n", lua_tostring(L, 2), lua_tostring(L, 3));
  if ((!lua_isstring(L, 2)) || (!lua_isstring(L, 3))){
    gr_object_t *ud = toobject(L, 1, NULL, STRICT);
    skey = agget(ud->p.p, "__attrib__");
    if (!skey || (strlen(skey) == 0)){
      /* Let's create an attrib table on the fly if none exists */
      sprintf(sskey, "%p", ud->p.p);
      skey = agstrdup(agroot(ud->p.p), sskey);
      agset(ud->p.p, "__attrib__", skey);
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
int new_object(lua_State *L, const char *kind, const luaL_Reg *reg_rmembers, 
	       const luaL_Reg *reg_methods, const luaL_Reg *reg_metamethods,
	       index_handler_t *index_handler)
{
  int methods, metatable;
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

  lua_setmetatable(L, -3);           /* ud, mtab */
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
		    ud->p.type == AGRAPH ? "graph" :
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
  lua_pushstring(L, AGTYPE(ud->p.p) == AGRAPH ? "graph" :
		 AGTYPE(ud->p.p) == AGNODE ? "node" :
		 AGTYPE(ud->p.p) == AGEDGE ? "edge" : "unkown");
  return 1;
}



