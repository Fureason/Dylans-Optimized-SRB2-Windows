/*
** $Id: lmem.c,v 1.70.1.1 2007/12/27 13:02:25 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/


#include <stddef.h>

#define lmem_c
#define LUA_CORE

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"



  
                              
                                                                     
                                                     
  
                                                   
  
                                                             
  
                                              
                                                      
                                                      
                                                
  
                                                                   
                                                              
  



#define MINSIZEARRAY	4


void *luaM_growaux_ (lua_State *L, void *block, int *size, size_t size_elems,
                     int limit, const char *errormsg) {
  void *newblock;
  int newsize;
  if (*size >= limit/2) {   
    if (*size >= limit)   
      luaG_runerror(L, errormsg);
    newsize = limit;   
  }
  else {
    newsize = (*size)*2;
    if (newsize < MINSIZEARRAY)
      newsize = MINSIZEARRAY;   
  }
  newblock = luaM_reallocv(L, block, *size, newsize, size_elems);
  *size = newsize;   
  return newblock;
}


void *luaM_toobig (lua_State *L) {
  luaG_runerror(L, "memory allocation error: block too big");
  return NULL;   
}



  
                              
  
void *luaM_realloc_ (lua_State *L, void *block, size_t osize, size_t nsize) {
  global_State *g = G(L);
  lua_assert((osize == 0) == (block == NULL));
  block = (*g->frealloc)(g->ud, block, osize, nsize);
  if (block == NULL && nsize > 0)
    luaD_throw(L, LUA_ERRMEM);
  lua_assert((nsize == 0) == (block == NULL));
  g->totalbytes = (g->totalbytes - osize) + nsize;
  return block;
}
