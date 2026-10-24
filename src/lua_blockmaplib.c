// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2016-2023 by Iestyn "Monster Iestyn" Jealous.
// Copyright (C) 2016-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "p_local.h"
#include "r_main.h"  
#include "p_polyobj.h"
#include "lua_script.h"
#include "lua_libs.h"


static const char *const search_opt[] = {
	"objects",
	"lines",
	"polyobjs",
	NULL};






typedef UINT8 (*blockmap_func)(lua_State *, INT32, INT32, mobj_t *);

static boolean blockfuncerror = false;  


static UINT8 lib_searchBlockmap_Objects(lua_State *L, mobj_t *thing, mobj_t *mobj)
{
	lua_pushvalue(L, 1);  
	LUA_PushUserdata(L, thing, META_MOBJ);
	LUA_PushUserdata(L, mobj, META_MOBJ);
	if (lua_pcall(gL, 2, 1, 0)) {
		if (!blockfuncerror || cv_debug & DBG_LUA)
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
		lua_pop(gL, 1);
		blockfuncerror = true;
		return 0;  
	}
	if (!lua_isnil(gL, -1))
	{  
		if (lua_toboolean(gL, -1))
			return 2;  
		else
			return 1;  
	}
	lua_pop(gL, 1);
	if (P_MobjWasRemoved(thing))  
		return P_MobjWasRemoved(thing) ? 2 : 1;
	return 0;
}


static UINT8 lib_searchBlockmap_Lines(lua_State *L, INT32 x, INT32 y, mobj_t *thing)
{
	INT32 offset;
	const INT32 *list;  
	polymaplink_t *plink;  
	line_t *ld;

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return 0;

	offset = y*bmapwidth + x;


	plink = polyblocklinks[offset];

	while (plink)
	{
		polyobj_t *po = plink->po;

		if (po->validcount != validcount)  
		{
			size_t i;
			po->validcount = validcount;

			for (i = 0; i < po->numLines; ++i)
			{
				if (po->lines[i]->validcount == validcount)  
					continue;
				po->lines[i]->validcount = validcount;

				lua_pushvalue(L, 1);
				LUA_PushUserdata(L, thing, META_MOBJ);
				LUA_PushUserdata(L, po->lines[i], META_LINE);
				if (lua_pcall(gL, 2, 1, 0)) {
					if (!blockfuncerror || cv_debug & DBG_LUA)
						CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
					lua_pop(gL, 1);
					blockfuncerror = true;
					return 0;  
				}
				if (!lua_isnil(gL, -1))
				{  
					if (lua_toboolean(gL, -1))
						return 2;  
					else
						return 1;  
				}
				lua_pop(gL, 1);
				if (P_MobjWasRemoved(thing))
					return 2;
			}
		}
		plink = (polymaplink_t *)(plink->link.next);
	}

	offset = *(blockmap + offset);  


	for (list = blockmaplump + offset + 1; *list != -1; list++)
	{
		ld = &lines[*list];

		if (ld->validcount == validcount)
			continue;  

		ld->validcount = validcount;

		lua_pushvalue(L, 1);
		LUA_PushUserdata(L, thing, META_MOBJ);
		LUA_PushUserdata(L, ld, META_LINE);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!blockfuncerror || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			blockfuncerror = true;
			return 0;  
		}
		if (!lua_isnil(gL, -1))
		{  
			if (lua_toboolean(gL, -1))
				return 2;  
			else
				return 1;  
		}
		lua_pop(gL, 1);
		if (P_MobjWasRemoved(thing))
			return 2;
	}
	return 0;  
}


static UINT8 lib_searchBlockmap_PolyObjs(lua_State *L, INT32 x, INT32 y, mobj_t *thing)
{
	INT32 offset;
	polymaplink_t *plink;  

	if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
		return 0;

	offset = y*bmapwidth + x;


	plink = polyblocklinks[offset];

	while (plink)
	{
		polyobj_t *po = plink->po;

		if (po->validcount != validcount)  
		{
			po->validcount = validcount;

			lua_pushvalue(L, 1);
			LUA_PushUserdata(L, thing, META_MOBJ);
			LUA_PushUserdata(L, po, META_POLYOBJ);
			if (lua_pcall(gL, 2, 1, 0)) {
				if (!blockfuncerror || cv_debug & DBG_LUA)
					CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
				lua_pop(gL, 1);
				blockfuncerror = true;
				return 0;  
			}
			if (!lua_isnil(gL, -1))
			{  
				if (lua_toboolean(gL, -1))
					return 2;  
				else
					return 1;  
			}
			lua_pop(gL, 1);
			if (P_MobjWasRemoved(thing))
				return 2;
		}
		plink = (polymaplink_t *)(plink->link.next);
	}

	return 0;  
}






static int lib_searchBlockmap(lua_State *L)
{
	int searchtype = luaL_checkoption(L, 1, "objects", search_opt);
	int n;
	mobj_t *mobj;
	INT32 xl, xh, yl, yh, bx, by;
	fixed_t x1, x2, y1, y2;
	boolean retval = true;
	UINT8 funcret = 0;
	blockmap_func searchFunc;
	boolean searchObjects = false;

	lua_remove(L, 1);  
	luaL_checktype(L, 1, LUA_TFUNCTION);

	switch (searchtype)
	{
		case 0:  
		default:
			searchObjects = true;
			searchFunc = NULL;
			break;
		case 1:  
			searchFunc = lib_searchBlockmap_Lines;
			break;
		case 2:  
			searchFunc = lib_searchBlockmap_PolyObjs;
			break;
	}


	mobj = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");

	n = lua_gettop(L);

	if (n > 2)  
	{
		if (n < 6)
			return luaL_error(L, "arguments 4 to 6 not all given (expected 4 fixed-point integers)");

		x1 = luaL_checkfixed(L, 3);
		x2 = luaL_checkfixed(L, 4);
		y1 = luaL_checkfixed(L, 5);
		y2 = luaL_checkfixed(L, 6);
	}
	else  
	{
		x1 = mobj->x - mobj->radius;
		x2 = mobj->x + mobj->radius;
		y1 = mobj->y - mobj->radius;
		y2 = mobj->y + mobj->radius;
	}
	lua_settop(L, 2);  

	xl = (unsigned)(x1 - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(x2 - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(y1 - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(y2 - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	blockfuncerror = false;  

	if (!searchObjects) {
		validcount++;

		for (bx = xl; bx <= xh; bx++)
			for (by = yl; by <= yh; by++)
			{
				funcret = searchFunc(L, bx, by, mobj);


				if (funcret == 2) {  
					lua_pushboolean(L, false);  
					return 1;
				}
				else if (funcret == 1)  
					retval = false;  


				if (P_MobjWasRemoved(mobj)) {  
					lua_pushboolean(L, false);  
					return 1;
				}
			}
	}
	else {
		bthingit_t *it = P_NewBlockThingsIterator(xl, yl, xh, yh);
		if (!it) {
			lua_pushboolean(L, false);
			return 1;
		}

		mobj_t *itmobj = NULL;

		do
		{
			itmobj = P_BlockThingsIteratorNext(it, false);
			if (itmobj)
			{
				if (mobj == itmobj)
					continue;  

				funcret = lib_searchBlockmap_Objects(L, mobj, itmobj);
				if (funcret == 2 || P_MobjWasRemoved(mobj)) {
					retval = false;
					break;
				}
				else if (funcret == 1)
					retval = false;
			}
		}
		while (itmobj != NULL);

		P_FreeBlockThingsIterator(it);
	}

	lua_pushboolean(L, retval);
	return 1;
}

int LUA_BlockmapLib(lua_State *L)
{
	lua_register(L, "searchBlockmap", lib_searchBlockmap);
	return 0;
}
