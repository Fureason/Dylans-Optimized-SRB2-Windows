/*
** $Id: ldo.c,v 2.38.1.3 2008/01/18 22:31:22 roberto Exp $
** Stack and Call structure of Lua
** See Copyright Notice in lua.h
*/


#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#define ldo_c
#define LUA_CORE

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lundump.h"
#include "lvm.h"
#include "lzio.h"

#ifdef __GNUC__
 #ifndef FUNCNORETURN
  #define FUNCNORETURN __attribute__ ((noreturn))
 #endif
 #endif

#ifndef FUNCNORETURN
#define FUNCNORETURN
#endif

  
                                                          
                           
                                                          
  

#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif

 
struct lua_longjmp {
  struct lua_longjmp *previous;
  luai_jmpbuf b;
  volatile int status;   
};

#ifdef _MSC_VER
#pragma warning(default :  4324)
#endif

void luaD_seterrorobj (lua_State *L, int lerrcode, StkId oldtop) {
  switch (lerrcode) {
    case LUA_ERRMEM: {
      setsvalue2s(L, oldtop, luaS_newliteral(L, MEMERRMSG));
      break;
    }
    case LUA_ERRERR: {
      setsvalue2s(L, oldtop, luaS_newliteral(L, "error in error handling"));
      break;
    }
    case LUA_ERRSYNTAX:
    case LUA_ERRRUN: {
      setobjs2s(L, oldtop, L->top - 1);   
      break;
    }
  }
  L->top = oldtop + 1;
}


static void restore_stack_limit (lua_State *L) {
  lua_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  if (L->size_ci > LUAI_MAXCALLS) {   
    int inuse = cast_int(L->ci - L->base_ci);
    if (inuse + 1 < LUAI_MAXCALLS)   
      luaD_reallocCI(L, LUAI_MAXCALLS);
  }
}


static void resetstack (lua_State *L, int status) {
  L->ci = L->base_ci;
  L->base = L->ci->base;
  luaF_close(L, L->base);   
  luaD_seterrorobj(L, status, L->base);
  L->nCcalls = L->baseCcalls;
  L->allowhook = 1;
  restore_stack_limit(L);
  L->errfunc = 0;
  L->errorJmp = NULL;
}


FUNCNORETURN void luaD_throw (lua_State *L, int lerrcode) {
  if (L->errorJmp) {
    L->errorJmp->status = lerrcode;
    LUAI_THROW(L, L->errorJmp);
  }
  else {
    L->status = cast_byte(lerrcode);
    if (G(L)->panic) {
      resetstack(L, lerrcode);
      lua_unlock(L);
      G(L)->panic(L);
    }
    exit(EXIT_FAILURE);
  }
}


int luaD_rawrunprotected (lua_State *L, Pfunc f, void *ud) {
  struct lua_longjmp lj;
  lj.status = 0;
  lj.previous = L->errorJmp;   
  L->errorJmp = &lj;
  LUAI_TRY(L, &lj,
    (*f)(L, ud);
  );
  L->errorJmp = lj.previous;   
  return lj.status;
}

 


static void correctstack (lua_State *L, TValue *oldstack) {
  CallInfo *ci;
  GCObject *up;
  L->top = (L->top - oldstack) + L->stack;
  for (up = L->openupval; up != NULL; up = up->gch.next)
    gco2uv(up)->v = (gco2uv(up)->v - oldstack) + L->stack;
  for (ci = L->base_ci; ci <= L->ci; ci++) {
    ci->top = (ci->top - oldstack) + L->stack;
    ci->base = (ci->base - oldstack) + L->stack;
    ci->func = (ci->func - oldstack) + L->stack;
  }
  L->base = (L->base - oldstack) + L->stack;
}


void luaD_reallocstack (lua_State *L, int newsize) {
  TValue *oldstack = L->stack;
  int realsize = newsize + 1 + EXTRA_STACK;
  lua_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  luaM_reallocvector(L, L->stack, L->stacksize, realsize, TValue);
  L->stacksize = realsize;
  L->stack_last = L->stack+newsize;
  correctstack(L, oldstack);
}


void luaD_reallocCI (lua_State *L, int newsize) {
  CallInfo *oldci = L->base_ci;
  luaM_reallocvector(L, L->base_ci, L->size_ci, newsize, CallInfo);
  L->size_ci = newsize;
  L->ci = (L->ci - oldci) + L->base_ci;
  L->end_ci = L->base_ci + L->size_ci - 1;
}


void luaD_growstack (lua_State *L, int n) {
  if (n <= L->stacksize)   
    luaD_reallocstack(L, 2*L->stacksize);
  else
    luaD_reallocstack(L, L->stacksize + n);
}


static CallInfo *growCI (lua_State *L) {
  if (L->size_ci > LUAI_MAXCALLS)   
    luaD_throw(L, LUA_ERRERR);
  else {
    luaD_reallocCI(L, 2*L->size_ci);
    if (L->size_ci > LUAI_MAXCALLS)
      luaG_runerror(L, "stack overflow");
  }
  return ++L->ci;
}


void luaD_callhook (lua_State *L, int event, int line) {
  lua_Hook hook = L->hook;
  if (hook && L->allowhook) {
    ptrdiff_t top = savestack(L, L->top);
    ptrdiff_t ci_top = savestack(L, L->ci->top);
    lua_Debug ar;
    ar.event = event;
    ar.currentline = line;
    if (event == LUA_HOOKTAILRET)
      ar.i_ci = 0;   
    else
      ar.i_ci = cast_int(L->ci - L->base_ci);
    luaD_checkstack(L, LUA_MINSTACK);   
    L->ci->top = L->top + LUA_MINSTACK;
    lua_assert(L->ci->top <= L->stack_last);
    L->allowhook = 0;   
    lua_unlock(L);
    (*hook)(L, &ar);
    lua_lock(L);
    lua_assert(!L->allowhook);
    L->allowhook = 1;
    L->ci->top = restorestack(L, ci_top);
    L->top = restorestack(L, top);
  }
}


static StkId adjust_varargs (lua_State *L, Proto *p, int actual) {
  int i;
  int nfixargs = p->numparams;
  Table *htab = NULL;
  StkId base, fixed;
  for (; actual < nfixargs; ++actual)
    setnilvalue(L->top++);
#if defined(LUA_COMPAT_VARARG)
  if (p->is_vararg & VARARG_NEEDSARG) {  
    int nvar = actual - nfixargs;   
    lua_assert(p->is_vararg & VARARG_HASARG);
    luaC_checkGC(L);
    htab = luaH_new(L, nvar, 1);   
    for (i=0; i<nvar; i++)   
      setobj2n(L, luaH_setnum(L, htab, i+1), L->top - nvar + i);
     
    setnvalue(luaH_setstr(L, htab, luaS_newliteral(L, "n")), cast_num(nvar));
  }
#endif
   
  fixed = L->top - actual;   
  base = L->top;   
  for (i=0; i<nfixargs; i++) {
    setobjs2s(L, L->top++, fixed+i);
    setnilvalue(fixed+i);
  }
   
  if (htab) {
    sethvalue(L, L->top++, htab);
    lua_assert(iswhite(obj2gco(htab)));
  }
  return base;
}


static StkId tryfuncTM (lua_State *L, StkId func) {
  const TValue *tm = luaT_gettmbyobj(L, func, TM_CALL);
  StkId p;
  ptrdiff_t funcr = savestack(L, func);
  if (!ttisfunction(tm))
    luaG_typeerror(L, func, "call");
   
  for (p = L->top; p > func; p--) setobjs2s(L, p, p-1);
  incr_top(L);
  func = restorestack(L, funcr);   
  setobj2s(L, func, tm);   
  return func;
}



#define inc_ci(L) \
  ((L->ci == L->end_ci) ? growCI(L) : \
   (condhardstacktests(luaD_reallocCI(L, L->size_ci)), ++L->ci))


int luaD_precall (lua_State *L, StkId func, int nresults) {
  LClosure *cl;
  ptrdiff_t funcr;
  if (!ttisfunction(func))  
    func = tryfuncTM(L, func);   
  funcr = savestack(L, func);
  cl = &clvalue(func)->l;
  L->ci->savedpc = L->savedpc;
  if (!cl->isC) {   
    CallInfo *ci;
    StkId st, base;
    Proto *p = cl->p;
    luaD_checkstack(L, p->maxstacksize);
    func = restorestack(L, funcr);
    if (!p->is_vararg) {   
      base = func + 1;
      if (L->top > base + p->numparams)
        L->top = base + p->numparams;
    }
    else {   
      int nargs = cast_int(L->top - func) - 1;
      base = adjust_varargs(L, p, nargs);
      func = restorestack(L, funcr);   
    }
    ci = inc_ci(L);   
    ci->func = func;
    L->base = ci->base = base;
    ci->top = L->base + p->maxstacksize;
    lua_assert(ci->top <= L->stack_last);
    L->savedpc = p->code;   
    ci->tailcalls = 0;
    ci->nresults = nresults;
    for (st = L->top; st < ci->top; st++)
      setnilvalue(st);
    L->top = ci->top;
    if (L->hookmask & LUA_MASKCALL) {
      L->savedpc++;   
      luaD_callhook(L, LUA_HOOKCALL, -1);
      L->savedpc--;   
    }
    return PCRLUA;
  }
  else {   
    CallInfo *ci;
    int n;
    luaD_checkstack(L, LUA_MINSTACK);   
    ci = inc_ci(L);   
    ci->func = restorestack(L, funcr);
    L->base = ci->base = ci->func + 1;
    ci->top = L->top + LUA_MINSTACK;
    lua_assert(ci->top <= L->stack_last);
    ci->nresults = nresults;
    if (L->hookmask & LUA_MASKCALL)
      luaD_callhook(L, LUA_HOOKCALL, -1);
    lua_unlock(L);
    n = (*curr_func(L)->c.f)(L);   
    lua_lock(L);
    if (n < 0)   
      return PCRYIELD;
    else {
      luaD_poscall(L, L->top - n);
      return PCRC;
    }
  }
}


static StkId callrethooks (lua_State *L, StkId firstResult) {
  ptrdiff_t fr = savestack(L, firstResult);   
  luaD_callhook(L, LUA_HOOKRET, -1);
  if (f_isLua(L->ci)) {   
    while ((L->hookmask & LUA_MASKRET) && L->ci->tailcalls--)  
      luaD_callhook(L, LUA_HOOKTAILRET, -1);
  }
  return restorestack(L, fr);
}


int luaD_poscall (lua_State *L, StkId firstResult) {
  StkId res;
  int wanted, i;
  CallInfo *ci;
  if (L->hookmask & LUA_MASKRET)
    firstResult = callrethooks(L, firstResult);
  ci = L->ci--;
  res = ci->func;   
  wanted = ci->nresults;
  L->base = (ci - 1)->base;   
  L->savedpc = (ci - 1)->savedpc;   
   
  for (i = wanted; i != 0 && firstResult < L->top; i--)
    setobjs2s(L, res++, firstResult++);
  while (i-- > 0)
    setnilvalue(res++);
  L->top = res;
  return (wanted - LUA_MULTRET);   
}


  
                                                                     
                                                            
                                                                           
                     
  
void luaD_call (lua_State *L, StkId func, int nResults) {
  if (++L->nCcalls >= LUAI_MAXCCALLS) {
    if (L->nCcalls == LUAI_MAXCCALLS)
      luaG_runerror(L, "C stack overflow");
    else if (L->nCcalls >= (LUAI_MAXCCALLS + (LUAI_MAXCCALLS>>3)))
      luaD_throw(L, LUA_ERRERR);   
  }
  if (luaD_precall(L, func, nResults) == PCRLUA)   
    luaV_execute(L, 1);   
  L->nCcalls--;
  luaC_checkGC(L);
}


static void resume (lua_State *L, void *ud) {
  StkId firstArg = cast(StkId, ud);
  CallInfo *ci = L->ci;
  if (L->status == 0) {   
    lua_assert(ci == L->base_ci && firstArg > L->base);
    if (luaD_precall(L, firstArg - 1, LUA_MULTRET) != PCRLUA)
      return;
  }
  else {   
    lua_assert(L->status == LUA_YIELD);
    L->status = 0;
    if (!f_isLua(ci)) {   
       
      lua_assert(GET_OPCODE(*((ci-1)->savedpc - 1)) == OP_CALL ||
                 GET_OPCODE(*((ci-1)->savedpc - 1)) == OP_TAILCALL);
      if (luaD_poscall(L, firstArg))   
        L->top = L->ci->top;   
    }
    else   
      L->base = L->ci->base;
  }
  luaV_execute(L, cast_int(L->ci - L->base_ci));
}


static int resume_error (lua_State *L, const char *msg) {
  L->top = L->ci->base;
  setsvalue2s(L, L->top, luaS_new(L, msg));
  incr_top(L);
  lua_unlock(L);
  return LUA_ERRRUN;
}


LUA_API int lua_resume (lua_State *L, int nargs) {
  int status;
  lua_lock(L);
  if (L->status != LUA_YIELD && (L->status != 0 || L->ci != L->base_ci))
      return resume_error(L, "cannot resume non-suspended coroutine");
  if (L->nCcalls >= LUAI_MAXCCALLS)
    return resume_error(L, "C stack overflow");
  luai_userstateresume(L, nargs);
  lua_assert(L->errfunc == 0);
  L->baseCcalls = ++L->nCcalls;
  status = luaD_rawrunprotected(L, resume, L->top - nargs);
  if (status != 0) {   
    L->status = cast_byte(status);   
    luaD_seterrorobj(L, status, L->top);
    L->ci->top = L->top;
  }
  else {
    lua_assert(L->nCcalls == L->baseCcalls);
    status = L->status;
  }
  --L->nCcalls;
  lua_unlock(L);
  return status;
}


LUA_API int lua_yield (lua_State *L, int nresults) {
  luai_userstateyield(L, nresults);
  lua_lock(L);
  if (L->nCcalls > L->baseCcalls)
    luaG_runerror(L, "attempt to yield across metamethod/C-call boundary");
  L->base = L->top - nresults;   
  L->status = LUA_YIELD;
  lua_unlock(L);
  return -1;
}


int luaD_pcall (lua_State *L, Pfunc func, void *u,
                ptrdiff_t old_top, ptrdiff_t ef) {
  int status;
  unsigned short oldnCcalls = L->nCcalls;
  ptrdiff_t old_ci = saveci(L, L->ci);
  lu_byte old_allowhooks = L->allowhook;
  ptrdiff_t old_errfunc = L->errfunc;
  L->errfunc = ef;
  status = luaD_rawrunprotected(L, func, u);
  if (status != 0) {   
    StkId oldtop = restorestack(L, old_top);
    luaF_close(L, oldtop);   
    luaD_seterrorobj(L, status, oldtop);
    L->nCcalls = oldnCcalls;
    L->ci = restoreci(L, old_ci);
    L->base = L->ci->base;
    L->savedpc = L->ci->savedpc;
    L->allowhook = old_allowhooks;
    restore_stack_limit(L);
  }
  L->errfunc = old_errfunc;
  return status;
}



  
                              
  
struct SParser {   
  ZIO *z;
  Mbuffer buff;   
  const char *name;
};

static void f_parser (lua_State *L, void *ud) {
  int i;
  Proto *tf;
  Closure *cl;
  struct SParser *p = cast(struct SParser *, ud);
  int c = luaZ_lookahead(p->z);
  luaC_checkGC(L);
#ifdef LUA_ALLOW_BYTECODE
  tf = ((c == LUA_SIGNATURE[0]) ? luaU_undump : luaY_parser)(L, p->z,
                                                             &p->buff, p->name);
#else
  if (c == LUA_SIGNATURE[0])
		luaG_runerror(L, "invalid format, cannot load bytecode scripts");
  tf = luaY_parser(L, p->z, &p->buff, p->name);
#endif
  cl = luaF_newLclosure(L, tf->nups, hvalue(gt(L)));
  cl->l.p = tf;
  for (i = 0; i < tf->nups; i++)   
    cl->l.upvals[i] = luaF_newupval(L);
  setclvalue(L, L->top, cl);
  incr_top(L);
}


int luaD_protectedparser (lua_State *L, ZIO *z, const char *name) {
  struct SParser p;
  int status;
  p.z = z; p.name = name;
  luaZ_initbuffer(L, &p.buff);
  status = luaD_pcall(L, f_parser, &p, savestack(L, L->top), L->errfunc);
  luaZ_freebuffer(L, &p.buff);
  return status;
}
