#ifndef GR_GRAPH_INCL
#define GR_GRAPH_INCL

/* 
 * Userdata in graph objects
 */
#ifdef DELETEME
struct Agraphinfo_s {
};
struct Agnodeinfo_s {
};
struct Agedgeinfo_s {
};
typedef struct Agraphinfo_s Agraphinfo_t;
typedef struct Agnodeinfo_s Agnodeinfo_t;
typedef struct Agedgeinfo_s Agedgeinfo_t;
#endif
#include "graphviz/gvcext.h"
#include "graphviz/gvc.h"
#include "graphviz/graph.h"
/* 
 * Set to 1 to track garbage collection 
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

/*
 * Userdata checking options
 */
#define STRICT (1)
#define NONSTRICT (0)

/*
 * Some compatibility defines between agraph and graph 
 */
#ifndef AGTYPE
  #define AGTYPE(obj) agobjkind(obj)
#endif
#ifndef AGID
  #define AGID(obj) ((obj)->id)
#endif
#ifndef agnameof
  #define agnameof(obj) ((obj)->name)
#endif
#define agisstrict(obj) (AG_IS_STRICT(obj) != 0)
#ifndef agisdirected
  #define agisdirected(obj) (AG_IS_DIRECTED(obj) != 0)
#endif
#ifndef agroot
  #define agroot(obj) ((obj)->root)
#endif
#ifndef agisarootobj
  #define agisarootobj(obj) ((obj)->root == obj)
#endif
#ifndef agrename
  #define agrename(obj, nam) (obj)->name = (nam)
#endif
Agraph_t *agfstsubg(Agraph_t *g, Agedge_t **lastedge);
Agraph_t *agnxtsubg(Agraph_t *g, Agedge_t **lastedge);
Agnode_t *agidnode(Agraph_t *g, int index);
/*
 * Our representation of graph objects
 */
struct gr_graph_s {
  int type;
  Agraph_t *g;
  char *name; /* only for debugging */
  Agedge_t *lastedge;
};
typedef struct gr_graph_s gr_graph_t;

struct gr_node_s {
  int type;
  Agnode_t *n;
  char *name; /* only for debugging */
  Agraph_t *subg;
};
typedef struct gr_node_s gr_node_t;

struct gr_edge_s {
  int type;
  Agedge_t *e;
  char *name; /* only for debugging */
  Agraph_t *subg;
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
  } p;
};
typedef union gr_object_s gr_object_t;

struct gr_callback_s {
  int typ;
  lua_CFunction func;
};
typedef struct gr_callback_s gr_callback_t;

typedef int (index_handler_t)(lua_State *L);

/* 
 * Userdata to/from graph object conversion, retrival and creation
 */
gr_object_t *toobject(lua_State *L, int narg, const char *type, int strict);
#define tograph(L, m, s) (&toobject(L, m, "graph", s)->g)
#define tonode(L, m, s) (&toobject(L, m, "node", s)->n)
#define toedge(L, m, s) (&toobject(L, m, "edge", s)->e)
#define register_metainfo(L, f) luaL_openlib(L, NULL, f, 0)
int set_object(lua_State *L, void *key);
int get_object(lua_State *L, void *key);
int del_object(lua_State *L, void *key);

/*
 * Graph object creation
 */
int new_object(lua_State *L, const char *kind, const luaL_reg *reg_rmembers, 
	       const luaL_reg *reg_methods, const luaL_reg *reg_metamethods,
	       index_handler_t *index_handler);
int new_edge(lua_State *L);
int new_node(lua_State *L);
int new_graph(lua_State *L);

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

void insert_callback(void *obj, void *arg);
void delete_callback(void *obj, void *arg);
void update_callback(void *obj, void *arg, Agsym_t *sym);

#endif
