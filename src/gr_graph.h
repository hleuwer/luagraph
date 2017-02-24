#ifndef GR_GRAPH_INCL
#define GR_GRAPH_INCL

#include "lua.h"
#include "lauxlib.h"

/* 
 * Userdata in graph objects
 */
#include "graphviz/gvcext.h"
#include "graphviz/gvc.h"
#include "graphviz/cgraph.h"

/* 
 * Set DOTRACE to 1 to track internal operations
 */
#define DOTRACE 0
#if DOTRACE == 1
#define TRACE(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define TRACE(fmt, ...)
#endif

/*
 * Just to be sure we have it
 */
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#define ALIVE (1)
#define DEAD  (0)

#define GR_SUCCESS (0)
#define GR_ERROR (1)

/*
 * Userdata checking options
 */
#define STRICT (1)
#define NONSTRICT (0)

/*
 * LuaGRAPH's representation of graph objects
 */
struct gr_graph_s {
  int type;
  Agraph_t *g;
  char *name; /* only for debugging */
  int status;
  Agedge_t *lastedge;
};
typedef struct gr_graph_s gr_graph_t;

struct gr_node_s {
  int type;
  Agnode_t *n;
  char *name; /* only for debugging */
  int status;
};
typedef struct gr_node_s gr_node_t;

struct gr_edge_s {
  int type;
  Agedge_t *e;
  char *name; /* only for debugging */
  int status;
};
typedef struct gr_edge_s gr_edge_t;

union gr_object_s {
  int type;
  gr_graph_t g;
  gr_node_t n;
  gr_edge_t e;
  struct {
    int type;
    void *p;
    char *name; /* only for debugging */
    int status;
  } p;
};
typedef union gr_object_s gr_object_t;

/* callback structure */
struct gr_callback_s {
  int typ;
  lua_CFunction func;
};
typedef struct gr_callback_s gr_callback_t;

/* handler for __index metamethod */
typedef int (index_handler_t)(lua_State *L);

/*
 * Callback functions
 */
void cb_insert(struct Agraph_s *g, struct Agobj_s *obj, void *L);
void cb_delete(Agraph_t *g, Agobj_t *obj, void *L);
void cb_modify(Agraph_t *g, Agobj_t *obj, void *L, Agsym_t *sym);

/*
 * Helper for auto naming.
 */
unsigned int newid(void);

/* 
 * Userdata to/from graph object conversion, retrival and creation
 */
gr_object_t *toobject(lua_State *L, int narg, const char *type, int strict);
#define tograph(L, m, s) (&toobject(L, m, "graph", s)->g)
#define tonode(L, m, s) (&toobject(L, m, "node", s)->n)
#define toedge(L, m, s) (&toobject(L, m, "edge", s)->e)
#if LUA_VERSION_NUM == 502
#define register_metainfo(L, f) luaL_setfuncs(L, f, 0)
#else
#define register_metainfo(L, f) luaL_register(L, NULL, f)
#endif

int set_object(lua_State *L, void *key);
int get_object(lua_State *L, void *key);
int del_object(lua_State *L, void *key);

/*
 * Graph object creation
 */
int new_object(lua_State *L, const char *kind, const luaL_Reg *reg_rmembers, 
	       const luaL_Reg *reg_methods, const luaL_Reg *reg_metamethods,
	       index_handler_t *index_handler);
int new_edge(lua_State *L);
int new_node(lua_State *L);

/*
 * __index, __newindex metamethod handlers
 */
int object_index_handler(lua_State *L);
int object_newindex_handler(lua_State *L);
int getval(lua_State *L);

/*
 * object delete/create methods
 */
int gr_delete_node(lua_State *L);
int gr_create_node(lua_State *L);
int gr_delete_edge(lua_State *L);

/*
 * Generic object member/method handlers
 */
int get_object_type(lua_State *L);

/*
 * generic function to be called in __gc metamethods of 
 * userdata objects.
 */
int gr_collect(lua_State *L);

/*
 * generic status property - for all objects.
 */
int gr_status(lua_State *L);

/*
 * Set the status of an object 1=alive 0=dead
 */
int set_status(lua_State *L, void *key, int status);
#endif
