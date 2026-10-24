// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "fastcmp.h"
#include "info.h"
#include "dehacked.h"
#include "deh_tables.h"
#include "deh_lua.h"
#include "p_mobj.h"
#include "p_local.h"
#include "z_zone.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_things.h"
#include "r_translation.h"
#include "r_draw.h"  
#include "doomstat.h"  

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"  
#include "lua_hook.h"  

boolean LUA_CallAction(enum actionnum actionnum, mobj_t *actor);
state_t *astate;

enum sfxinfo_read {
	sfxinfor_name = 0,
	sfxinfor_singular,
	sfxinfor_priority,
	sfxinfor_flags,  
	sfxinfor_caption,
	sfxinfor_skinsound
};
const char *const sfxinfo_ropt[] = {
	"name",
	"singular",
	"priority",
	"flags",
	"caption",
	"skinsound",
	NULL};

enum sfxinfo_write {
	sfxinfow_singular = 0,
	sfxinfow_priority,
	sfxinfow_flags,  
	sfxinfow_caption
};
const char *const sfxinfo_wopt[] = {
	"singular",
	"priority",
	"flags",
	"caption",
	NULL};

int actionsoverridden[NUMACTIONS][MAX_ACTION_RECURSION];






static int lib_getSprname(lua_State *L)
{
	UINT32 i;

	lua_remove(L, 1);  

	if (lua_isnumber(L, 1))
	{
		i = lua_tonumber(L, 1);
		if (i > NUMSPRITES)
			return 0;
		lua_pushlstring(L, sprnames[i], 4);
		return 1;
	}
	else if (lua_isstring(L, 1))
	{
		const char *name = lua_tostring(L, 1);
		i = R_GetSpriteNumByName(name);
		if (i != NUMSPRITES)
		{
			lua_pushinteger(L, i);
			return 1;
		}
	}
	return 0;
}


static int lib_sprnamelen(lua_State *L)
{
	lua_pushinteger(L, NUMSPRITES);
	return 1;
}






static int lib_getSpr2name(lua_State *L)
{
	playersprite_t i;

	lua_remove(L, 1);  

	if (lua_isnumber(L, 1))
	{
		i = lua_tonumber(L, 1);
		if (i >= free_spr2)
			return 0;
		lua_pushlstring(L, spr2names[i], 4);
		return 1;
	}
	else if (lua_isstring(L, 1))
	{
		const char *name = lua_tostring(L, 1);
		for (i = 0; i < free_spr2; i++)
			if (fastcmp(name, spr2names[i]))
			{
				lua_pushinteger(L, i);
				return 1;
			}
	}
	return 0;
}

static int lib_getSpr2default(lua_State *L)
{
	playersprite_t i;

	lua_remove(L, 1);  

	if (lua_isnumber(L, 1))
		i = lua_tonumber(L, 1);
	else if (lua_isstring(L, 1))
	{
		const char *name = lua_tostring(L, 1);
		for (i = 0; i < free_spr2; i++)
			if (fastcmp(name, spr2names[i]))
				break;
	}
	else
		return luaL_error(L, "spr2defaults[] invalid index");

	if (i >= free_spr2)
		return luaL_error(L, "spr2defaults[] index %d out of range (%d - %d)", i, 0, free_spr2-1);

	lua_pushinteger(L, spr2defaults[i]);
	return 1;
}

static int lib_setSpr2default(lua_State *L)
{
	playersprite_t i;
	UINT16 j = 0;

	if (hud_running)
		return luaL_error(L, "Do not alter spr2defaults[] in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter spr2defaults[] in CMD building code!");


#ifdef SETALLSPR2DEFAULTS
#define FIRSTMODIFY 0
#else
#define FIRSTMODIFY SPR2_FIRSTFREESLOT
	if (free_spr2 == SPR2_FIRSTFREESLOT)
		return luaL_error(L, "You can only modify the spr2defaults[] entries of sprite2 freeslots, and none are currently added.");
#endif

	lua_remove(L, 1);  

	if (lua_isnumber(L, 1))
		i = lua_tonumber(L, 1);
	else if (lua_isstring(L, 1))
	{
		const char *name = lua_tostring(L, 1);
		for (i = 0; i < free_spr2; i++)
		{
			if (fastcmp(name, spr2names[i]))
				break;
		}
		if (i == free_spr2)
			return luaL_error(L, "spr2defaults[] invalid index");
	}
	else
		return luaL_error(L, "spr2defaults[] invalid index");

	if (i < FIRSTMODIFY || i >= free_spr2)
		return luaL_error(L, "spr2defaults[] index %d out of range (%d - %d)", i, FIRSTMODIFY, free_spr2-1);
#undef FIRSTMODIFY

	if (lua_isnumber(L, 2))
		j = lua_tonumber(L, 2);
	else if (lua_isstring(L, 2))
	{
		const char *name = lua_tostring(L, 2);
		for (j = 0; j < free_spr2; j++)
		{
			if (fastcmp(name, spr2names[j]))
				break;
		}
		if (j == free_spr2)
			return luaL_error(L, "spr2defaults[] invalid set");
	}
	else
		return luaL_error(L, "spr2defaults[] invalid set");

	if (j >= free_spr2)
		return luaL_error(L, "spr2defaults[] set %d out of range (%d - %d)", j, 0, free_spr2-1);

	spr2defaults[i] = j;
	return 0;
}

static int lib_spr2namelen(lua_State *L)
{
	lua_pushinteger(L, free_spr2);
	return 1;
}






static int lib_getSpriteInfo(lua_State *L)
{
	UINT32 i = NUMSPRITES;
	lua_remove(L, 1);

	if (lua_type(L, 1) == LUA_TSTRING)
	{
		const char *name = lua_tostring(L, 1);
		INT32 spr = R_GetSpriteNumByName(name);
		if (spr == NUMSPRITES)
			return luaL_error(L, "unknown sprite name %s", name);
		i = spr;
	}
	else
		i = luaL_checkinteger(L, 1);

	if (i == 0 || i >= NUMSPRITES)
		return luaL_error(L, "spriteinfo[] index %d out of range (1 - %d)", i, NUMSPRITES-1);

	LUA_PushUserdata(L, &spriteinfo[i], META_SPRITEINFO);
	return 1;
}

#define FIELDERROR(f, e) luaL_error(L, "bad value for " LUA_QL(f) " in table passed to spriteinfo[] (%s)", e);
#define TYPEERROR(f, t1, t2) FIELDERROR(f, va("%s expected, got %s", lua_typename(L, t1), lua_typename(L, t2)))

static int PopPivotSubTable(spriteframepivot_t *pivot, lua_State *L, int stk, int idx)
{
	int okcool = 0;
	switch (lua_type(L, stk))
	{
		case LUA_TTABLE:
			lua_pushnil(L);
			while (lua_next(L, stk))
			{
				const char *key = NULL;
				lua_Integer ikey = -1;
				lua_Integer value = 0;

				switch (lua_type(L, stk+1))
				{
					case LUA_TSTRING:
						key = lua_tostring(L, stk+1);
						break;
					case LUA_TNUMBER:
						ikey = lua_tointeger(L, stk+1);
						break;
					default:
						FIELDERROR("pivot key", va("string or number expected, got %s", luaL_typename(L, stk+1)))
				}

				switch (lua_type(L, stk+2))
				{
					case LUA_TNUMBER:
						value = lua_tonumber(L, stk+2);
						break;
					case LUA_TBOOLEAN:
						value = (UINT8)lua_toboolean(L, stk+2);
						break;
					default:
						TYPEERROR("pivot value", LUA_TNUMBER, lua_type(L, stk+2))
				}

				if (ikey == 1 || (key && fastcmp(key, "x")))
					pivot[idx].x = (INT32)value;
				else if (ikey == 2 || (key && fastcmp(key, "y")))
					pivot[idx].y = (INT32)value;

				else if (ikey == 3 || (key && fastcmp(key, "rotaxis")))
					LUA_UsageWarning(L, "\"rotaxis\" is deprecated and will be removed.")
				else if (ikey == -1 && (key != NULL))
					FIELDERROR("pivot key", va("invalid option %s", key));
				okcool = 1;
				lua_pop(L, 1);
			}
			break;
		default:
			TYPEERROR("sprite pivot", LUA_TTABLE, lua_type(L, stk))
	}
	return okcool;
}

static int PopPivotTable(spriteinfo_t *info, lua_State *L, int stk)
{

	if (!lua_istable(L, stk))
		TYPEERROR("pivot table", LUA_TTABLE, lua_type(L, stk));

	lua_pushnil(L);





	while (lua_next(L, stk))
	{
		int idx = 0;
		const char *framestr = NULL;
		switch (lua_type(L, stk+1))
		{
			case LUA_TSTRING:
				framestr = lua_tostring(L, stk+1);
				idx = R_Char2Frame(framestr[0]);
				break;
			case LUA_TNUMBER:
				idx = lua_tonumber(L, stk+1);
				break;
			default:
				TYPEERROR("pivot frame", LUA_TNUMBER, lua_type(L, stk+1));
		}
		if ((idx < 0) || (idx >= MAXFRAMENUM))
			return luaL_error(L, "pivot frame %d out of range (0 - %d)", idx, MAXFRAMENUM - 1);

		if (PopPivotSubTable(info->pivot, L, stk+2, idx))
			info->available = true;
		lua_pop(L, 1);
	}

	return 0;
}

static int lib_setSpriteInfo(lua_State *L)
{
	spriteinfo_t *info;

	if (!lua_lumploading)
		return luaL_error(L, "Do not alter spriteinfo_t from within a hook or coroutine!");
	if (hud_running)
		return luaL_error(L, "Do not alter spriteinfo_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter spriteinfo_t in CMD building code!");

	lua_remove(L, 1);
	{
		UINT32 i = luaL_checkinteger(L, 1);
		if (i == 0 || i >= NUMSPRITES)
			return luaL_error(L, "spriteinfo[] index %d out of range (1 - %d)", i, NUMSPRITES-1);
		info = &spriteinfo[i];  
	}
	luaL_checktype(L, 2, LUA_TTABLE);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		lua_Integer i = 0;
		const char *str = NULL;
		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2);
		else
			str = luaL_checkstring(L, 2);

		if (i == 1 || (str && fastcmp(str, "pivot")))
		{

			if (lua_istable(L, 3))
				return PopPivotTable(info, L, 3);
			else
				FIELDERROR("pivot", va("%s expected, got %s", lua_typename(L, LUA_TTABLE), luaL_typename(L, -1)))
		}

		lua_pop(L, 1);
	}

	return 0;
}

#undef FIELDERROR
#undef TYPEERROR

static int lib_spriteinfolen(lua_State *L)
{
	lua_pushinteger(L, NUMSPRITES);
	return 1;
}


static int spriteinfo_get(lua_State *L)
{
	spriteinfo_t *sprinfo = *((spriteinfo_t **)luaL_checkudata(L, 1, META_SPRITEINFO));
	const char *field = luaL_checkstring(L, 2);

	I_Assert(sprinfo != NULL);


	if (fastcmp(field, "pivot"))
	{

		void **userdata = lua_newuserdata(L, sizeof(void *));
		*userdata = &sprinfo->pivot;
		luaL_getmetatable(L, META_PIVOTLIST);
		lua_setmetatable(L, -2);


		return 1;
	}
	else
		return luaL_error(L, LUA_QL("spriteinfo_t") " has no field named " LUA_QS, field);

	return 0;
}

static int spriteinfo_set(lua_State *L)
{
	spriteinfo_t *sprinfo = *((spriteinfo_t **)luaL_checkudata(L, 1, META_SPRITEINFO));
	const char *field = luaL_checkstring(L, 2);

	if (!lua_lumploading)
		return luaL_error(L, "Do not alter spriteinfo_t from within a hook or coroutine!");
	if (hud_running)
		return luaL_error(L, "Do not alter spriteinfo_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter spriteinfo_t in CMD building code!");

	I_Assert(sprinfo != NULL);

	lua_remove(L, 1);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	if (fastcmp(field, "pivot"))
	{

		if (lua_istable(L, 1))
			return PopPivotTable(sprinfo, L, 1);

		else if (lua_isuserdata(L, 1))
		{
			spriteframepivot_t *pivot = *((spriteframepivot_t **)luaL_checkudata(L, 1, META_PIVOTLIST));
			memcpy(&sprinfo->pivot, pivot, sizeof(spriteframepivot_t));
			sprinfo->available = true;  
		}
	}
	else
		return luaL_error(L, "Field %s does not exist in spriteinfo_t", field);

	return 0;
}

static int spriteinfo_num(lua_State *L)
{
	spriteinfo_t *sprinfo = *((spriteinfo_t **)luaL_checkudata(L, 1, META_SPRITEINFO));

	I_Assert(sprinfo != NULL);
	I_Assert(sprinfo >= spriteinfo);

	lua_pushinteger(L, (UINT32)(sprinfo-spriteinfo));
	return 1;
}


static int pivotlist_get(lua_State *L)
{
	void **userdata;
	spriteframepivot_t *framepivot = *((spriteframepivot_t **)luaL_checkudata(L, 1, META_PIVOTLIST));
	const char *field = luaL_checkstring(L, 2);
	UINT8 frame;

	frame = R_Char2Frame(field[0]);
	if (frame == 255)
		luaL_error(L, "invalid frame %s", field);


	userdata = lua_newuserdata(L, sizeof(void *));
	*userdata = &framepivot[frame];
	luaL_getmetatable(L, META_FRAMEPIVOT);
	lua_setmetatable(L, -2);


	return 1;
}

static int pivotlist_set(lua_State *L)
{

	spriteframepivot_t *pivotlist = *((spriteframepivot_t **)lua_touserdata(L, 1));

	const char *field = luaL_checkstring(L, 2);
	UINT8 frame;

	if (!lua_lumploading)
		return luaL_error(L, "Do not alter spriteframepivot_t from within a hook or coroutine!");
	if (hud_running)
		return luaL_error(L, "Do not alter spriteframepivot_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter spriteframepivot_t in CMD building code!");

	frame = R_Char2Frame(field[0]);
	if (frame == 255)
		luaL_error(L, "invalid frame %s", field);


	if (lua_istable(L, 3))
		return PopPivotSubTable(pivotlist, L, 3, frame);

	else if (lua_isuserdata(L, 3))
	{
		spriteframepivot_t *copypivot = *((spriteframepivot_t **)luaL_checkudata(L, 3, META_FRAMEPIVOT));
		memcpy(&pivotlist[frame], copypivot, sizeof(spriteframepivot_t));
	}

	return 0;
}

static int pivotlist_num(lua_State *L)
{
	lua_pushinteger(L, MAXFRAMENUM);
	return 1;
}

static int framepivot_get(lua_State *L)
{
	spriteframepivot_t *framepivot = *((spriteframepivot_t **)luaL_checkudata(L, 1, META_FRAMEPIVOT));
	const char *field = luaL_checkstring(L, 2);

	I_Assert(framepivot != NULL);

	if (fastcmp("x", field))
		lua_pushinteger(L, framepivot->x);
	else if (fastcmp("y", field))
		lua_pushinteger(L, framepivot->y);

	else if (fastcmp("rotaxis", field))
	{
		LUA_UsageWarning(L, "\"rotaxis\" is deprecated and will be removed.");
		lua_pushinteger(L, 0);
	}
	else
		return luaL_error(L, "Field %s does not exist in spriteframepivot_t", field);

	return 1;
}

static int framepivot_set(lua_State *L)
{
	spriteframepivot_t *framepivot = *((spriteframepivot_t **)luaL_checkudata(L, 1, META_FRAMEPIVOT));
	const char *field = luaL_checkstring(L, 2);

	if (!lua_lumploading)
		return luaL_error(L, "Do not alter spriteframepivot_t from within a hook or coroutine!");
	if (hud_running)
		return luaL_error(L, "Do not alter spriteframepivot_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter spriteframepivot_t in CMD building code!");

	I_Assert(framepivot != NULL);

	if (fastcmp("x", field))
		framepivot->x = luaL_checkinteger(L, 3);
	else if (fastcmp("y", field))
		framepivot->y = luaL_checkinteger(L, 3);

	else if (fastcmp("rotaxis", field))
		LUA_UsageWarning(L, "\"rotaxis\" is deprecated and will be removed.")
	else
		return luaL_error(L, "Field %s does not exist in spriteframepivot_t", field);

	return 0;
}

static int framepivot_num(lua_State *L)
{
	lua_pushinteger(L, 2);
	return 1;
}







static void A_Lua(mobj_t *actor)
{
	boolean found = false;
	I_Assert(actor != NULL);

	lua_settop(gL, 0);  
	lua_pushcfunction(gL, LUA_GetErrorMessage);


	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_STATEACTION);
	I_Assert(lua_istable(gL, -1));
	lua_pushlightuserdata(gL, astate);
	lua_rawget(gL, -2);
	I_Assert(lua_isfunction(gL, -1));
	lua_remove(gL, -2);  


	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_ACTIONS);
	lua_pushnil(gL);
	while (lua_next(gL, -2))
	{
		if (lua_rawequal(gL, -1, -4))
		{
			found = true;
			luaactions[luaactionstack] = lua_tostring(gL, -2);  
			++luaactionstack;
			lua_pop(gL, 2);  
			break;
		}
		lua_pop(gL, 1);
	}
	lua_pop(gL, 1);  

	LUA_PushUserdata(gL, actor, META_MOBJ);
	lua_pushinteger(gL, var1);
	lua_pushinteger(gL, var2);
	LUA_Call(gL, 3, 0, 1);

	if (found)
	{
		--luaactionstack;
		luaactions[luaactionstack] = NULL;
	}
}


static int lib_getState(lua_State *L)
{
	UINT32 i;
	lua_remove(L, 1);

	i = luaL_checkinteger(L, 1);
	if (i >= NUMSTATES)
		return luaL_error(L, "states[] index %d out of range (0 - %d)", i, NUMSTATES-1);
	LUA_PushUserdata(L, &states[i], META_STATE);
	return 1;
}


static int lib_setState(lua_State *L)
{
	state_t *state;
	lua_remove(L, 1);  
	{
		UINT32 i = luaL_checkinteger(L, 1);
		if (i >= NUMSTATES)
			return luaL_error(L, "states[] index %d out of range (0 - %d)", i, NUMSTATES-1);
		state = &states[i];  
	}
	luaL_checktype(L, 2, LUA_TTABLE);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	if (hud_running)
		return luaL_error(L, "Do not alter states in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter states in CMD building code!");


	memset(state,0,sizeof(state_t));
	state->tics = -1;

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		lua_Integer i = 0;
		const char *str = NULL;
		lua_Integer value;
		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2);
		else
			str = luaL_checkstring(L, 2);

		if (i == 1 || (str && fastcmp(str, "sprite"))) {
			value = luaL_checkinteger(L, 3);
			if (value < SPR_NULL || value >= NUMSPRITES)
				return luaL_error(L, "sprite number %d is invalid.", value);
			state->sprite = (spritenum_t)value;
		} else if (i == 2 || (str && fastcmp(str, "frame"))) {
			state->frame = (UINT32)luaL_checkinteger(L, 3);
		} else if (i == 3 || (str && fastcmp(str, "tics"))) {
			state->tics = (INT32)luaL_checkinteger(L, 3);
		} else if (i == 4 || (str && fastcmp(str, "action"))) {
			switch(lua_type(L, 3))
			{
			case LUA_TNIL:  
				state->action.acp1 = NULL;
				break;
			case LUA_TSTRING:  
				LUA_SetActionByName(state, lua_tostring(L, 3));
				break;
			case LUA_TUSERDATA:  
			{
				actionf_t *action = *((actionf_t **)luaL_checkudata(L, 3, META_ACTION));

				if (!action)
					return luaL_error(L, "not a valid action?");

				state->action = *action;
				state->action.acv = action->acv;
				state->action.acp1 = action->acp1;
				break;
			}
			case LUA_TFUNCTION:  
				lua_getfield(L, LUA_REGISTRYINDEX, LREG_STATEACTION);
				I_Assert(lua_istable(L, -1));
				lua_pushlightuserdata(L, state);  
				lua_pushvalue(L, 3);  
				lua_rawset(L, -3);  
				lua_pop(L, 1);  
				state->action.acp1 = (actionf_p1)A_Lua;  
				break;
			default:  
				return luaL_typerror(L, 3, "function");
			}
		} else if (i == 5 || (str && fastcmp(str, "var1"))) {
			state->var1 = (INT32)luaL_checkinteger(L, 3);
		} else if (i == 6 || (str && fastcmp(str, "var2"))) {
			state->var2 = (INT32)luaL_checkinteger(L, 3);
		} else if (i == 7 || (str && fastcmp(str, "nextstate"))) {
			value = luaL_checkinteger(L, 3);
			if (value < S_NULL || value >= NUMSTATES)
				return luaL_error(L, "nextstate number %d is invalid.", value);
			state->nextstate = (statenum_t)value;
		}
		lua_pop(L, 1);
	}
	return 0;
}


static int lib_statelen(lua_State *L)
{
	lua_pushinteger(L, NUMSTATES);
	return 1;
}

boolean LUA_SetLuaAction(void *stv, const char *action)
{
	state_t *st = (state_t *)stv;

	I_Assert(st != NULL);

	I_Assert(action != NULL);

	if (!gL)  
		return false;  




	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_ACTIONS);
	lua_getfield(gL, -1, action);

	if (lua_isnil(gL, -1))  
	{
		lua_pop(gL, 2);  
		return false;  
	}

	lua_getfield(gL, LUA_REGISTRYINDEX, LREG_STATEACTION);
	I_Assert(lua_istable(gL, -1));
	lua_pushlightuserdata(gL, stv);  
	lua_pushvalue(gL, -3);  
	lua_rawset(gL, -3);  
	lua_pop(gL, 1);  

	lua_pop(gL, 2);  
	st->action.acp1 = (actionf_p1)A_Lua;  
	return true;  
}

static UINT8 superstack[NUMACTIONS];
boolean LUA_CallAction(enum actionnum actionnum, mobj_t *actor)
{
	I_Assert(actor != NULL);

	if (actionsoverridden[actionnum][0] == LUA_REFNIL)
	{


		return false;
	}

	if (luaactionstack && fasticmp(actionpointers[actionnum].name, luaactions[luaactionstack-1]))
	{





		superstack[actionnum]++;

		if (superstack[actionnum] >= MAX_ACTION_RECURSION)
		{
			CONS_Alert(CONS_WARNING, "Max Lua super recursion reached! Cool it on calling super!\n");
			superstack[actionnum] = 0;
			return false;
		}
	}

	if (actionsoverridden[actionnum][superstack[actionnum]] == LUA_REFNIL)
	{



		if (superstack[actionnum])
		{

			superstack[actionnum]--;
		}

		return false;
	}


	lua_pushcfunction(gL, LUA_GetErrorMessage);


	lua_getref(gL, actionsoverridden[actionnum][superstack[actionnum]]);

	if (lua_isnil(gL, -1))  
	{
		lua_pop(gL, 2);  
		return false;  
	}

	if (luaactionstack >= MAX_ACTION_RECURSION)
	{
		CONS_Alert(CONS_WARNING, "Max Lua Action recursion reached! Cool it on the calling A_Action functions from inside A_Action functions!\n");
		lua_pop(gL, 2);  
		return true;
	}



	I_Assert(lua_isfunction(gL, -1));
	LUA_PushUserdata(gL, actor, META_MOBJ);
	lua_pushinteger(gL, var1);
	lua_pushinteger(gL, var2);

	luaactions[luaactionstack] = actionpointers[actionnum].name;
	++luaactionstack;

	LUA_Call(gL, 3, 0, -(2 + 3));
	lua_pop(gL, -1);  

	if (superstack[actionnum])
	{

		superstack[actionnum]--;
	}

	--luaactionstack;
	luaactions[luaactionstack] = NULL;
	return true;  
}


static int state_get(lua_State *L)
{
	state_t *st = *((state_t **)luaL_checkudata(L, 1, META_STATE));
	const char *field = luaL_checkstring(L, 2);
	lua_Integer number;

	if (fastcmp(field,"sprite"))
		number = st->sprite;
	else if (fastcmp(field,"frame"))
		number = st->frame;
	else if (fastcmp(field,"tics"))
		number = st->tics;
	else if (fastcmp(field,"action")) {
		const char *name;
		if (!st->action.acp1)  
			return 0;  
		if (st->action.acp1 == (actionf_p1)A_Lua) {  
			lua_getfield(L, LUA_REGISTRYINDEX, LREG_STATEACTION);
			I_Assert(lua_istable(L, -1));
			lua_pushlightuserdata(L, st);  
			lua_rawget(L, -2);  
			lua_remove(L, -2);  
			return 1;  
		}
		name = LUA_GetActionName(&st->action);  
		if (!name)  
			return 0;  


		lua_getglobal(L, name);  
		return 1;  
	} else if (fastcmp(field,"var1"))
		number = st->var1;
	else if (fastcmp(field,"var2"))
		number = st->var2;
	else if (fastcmp(field,"nextstate"))
		number = st->nextstate;
	else if (devparm)
		return luaL_error(L, LUA_QL("state_t") " has no field named " LUA_QS, field);
	else
		return 0;

	lua_pushinteger(L, number);
	return 1;
}


static int state_set(lua_State *L)
{
	state_t *st = *((state_t **)luaL_checkudata(L, 1, META_STATE));
	const char *field = luaL_checkstring(L, 2);
	lua_Integer value;

	if (hud_running)
		return luaL_error(L, "Do not alter states in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter states in CMD building code!");

	if (fastcmp(field,"sprite")) {
		value = luaL_checknumber(L, 3);
		if (value < SPR_NULL || value >= NUMSPRITES)
			return luaL_error(L, "sprite number %d is invalid.", value);
		st->sprite = (spritenum_t)value;
	} else if (fastcmp(field,"frame"))
		st->frame = (UINT32)luaL_checknumber(L, 3);
	else if (fastcmp(field,"tics"))
		st->tics = (INT32)luaL_checknumber(L, 3);
	else if (fastcmp(field,"action")) {
		switch(lua_type(L, 3))
		{
		case LUA_TNIL:  
			st->action.acp1 = NULL;
			break;
		case LUA_TSTRING:  
			LUA_SetActionByName(st, lua_tostring(L, 3));
			break;
		case LUA_TUSERDATA:  
		{
			actionf_t *action = *((actionf_t **)luaL_checkudata(L, 3, META_ACTION));

			if (!action)
				return luaL_error(L, "not a valid action?");

			st->action = *action;
			st->action.acv = action->acv;
			st->action.acp1 = action->acp1;
			break;
		}
		case LUA_TFUNCTION:  
			lua_getfield(L, LUA_REGISTRYINDEX, LREG_STATEACTION);
			I_Assert(lua_istable(L, -1));
			lua_pushlightuserdata(L, st);  
			lua_pushvalue(L, 3);  
			lua_rawset(L, -3);  
			lua_pop(L, 1);  
			st->action.acp1 = (actionf_p1)A_Lua;  
			break;
		default:  
			return luaL_typerror(L, 3, "function");
		}
	} else if (fastcmp(field,"var1"))
		st->var1 = (INT32)luaL_checknumber(L, 3);
	else if (fastcmp(field,"var2"))
		st->var2 = (INT32)luaL_checknumber(L, 3);
	else if (fastcmp(field,"nextstate")) {
		value = luaL_checkinteger(L, 3);
		if (value < S_NULL || value >= NUMSTATES)
			return luaL_error(L, "nextstate number %d is invalid.", value);
		st->nextstate = (statenum_t)value;
	} else
		return luaL_error(L, LUA_QL("state_t") " has no field named " LUA_QS, field);

	return 0;
}


static int state_num(lua_State *L)
{
	state_t *state = *((state_t **)luaL_checkudata(L, 1, META_STATE));
	lua_pushinteger(L, state-states);
	return 1;
}






static int lib_getMobjInfo(lua_State *L)
{
	UINT32 i;
	lua_remove(L, 1);

	i = luaL_checkinteger(L, 1);
	if (i >= NUMMOBJTYPES)
		return luaL_error(L, "mobjinfo[] index %d out of range (0 - %d)", i, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, &mobjinfo[i], META_MOBJINFO);
	return 1;
}


static int lib_setMobjInfo(lua_State *L)
{
	mobjinfo_t *info;
	lua_remove(L, 1);  
	{
		UINT32 i = luaL_checkinteger(L, 1);
		if (i >= NUMMOBJTYPES)
			return luaL_error(L, "mobjinfo[] index %d out of range (0 - %d)", i, NUMMOBJTYPES-1);
		info = &mobjinfo[i];  
	}
	luaL_checktype(L, 2, LUA_TTABLE);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	if (hud_running)
		return luaL_error(L, "Do not alter mobjinfo in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter mobjinfo in CMD building code!");


	memset(info,0,sizeof(mobjinfo_t));
	info->doomednum = -1;  
	info->spawnhealth = 1;  

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		lua_Integer i = 0;
		const char *str = NULL;
		lua_Integer value;
		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2);
		else
			str = luaL_checkstring(L, 2);

		if (i == 1 || (str && fastcmp(str,"doomednum")))
			info->doomednum = (INT32)luaL_checkinteger(L, 3);
		else if (i == 2 || (str && fastcmp(str,"spawnstate"))) {
			value = luaL_checkinteger(L, 3);
			if (value < S_NULL || value >= NUMSTATES)
				return luaL_error(L, "spawnstate number %d is invalid.", value);
			info->spawnstate = (statenum_t)value;
		} else if (i == 3 || (str && fastcmp(str,"spawnhealth")))
			info->spawnhealth = (INT32)luaL_checkinteger(L, 3);
		else if (i == 4 || (str && fastcmp(str,"seestate"))) {
			value = luaL_checkinteger(L, 3);
			if (value < S_NULL || value >= NUMSTATES)
				return luaL_error(L, "seestate number %d is invalid.", value);
			info->seestate = (statenum_t)value;
		} else if (i == 5 || (str && fastcmp(str,"seesound"))) {
			value = luaL_checkinteger(L, 3);
			if (value < sfx_None || value >= NUMSFX)
				return luaL_error(L, "seesound number %d is invalid.", value);
			info->seesound = (sfxenum_t)value;
		} else if (i == 6 || (str && fastcmp(str,"reactiontime")))
			info->reactiontime = (INT32)luaL_checkinteger(L, 3);
		else if (i == 7 || (str && fastcmp(str,"attacksound")))
			info->attacksound = luaL_checkinteger(L, 3);
		else if (i == 8 || (str && fastcmp(str,"painstate")))
			info->painstate = luaL_checkinteger(L, 3);
		else if (i == 9 || (str && fastcmp(str,"painchance")))
			info->painchance = (INT32)luaL_checkinteger(L, 3);
		else if (i == 10 || (str && fastcmp(str,"painsound")))
			info->painsound = luaL_checkinteger(L, 3);
		else if (i == 11 || (str && fastcmp(str,"meleestate")))
			info->meleestate = luaL_checkinteger(L, 3);
		else if (i == 12 || (str && fastcmp(str,"missilestate")))
			info->missilestate = luaL_checkinteger(L, 3);
		else if (i == 13 || (str && fastcmp(str,"deathstate")))
			info->deathstate = luaL_checkinteger(L, 3);
		else if (i == 14 || (str && fastcmp(str,"xdeathstate")))
			info->xdeathstate = luaL_checkinteger(L, 3);
		else if (i == 15 || (str && fastcmp(str,"deathsound")))
			info->deathsound = luaL_checkinteger(L, 3);
		else if (i == 16 || (str && fastcmp(str,"speed")))
			info->speed = luaL_checkfixed(L, 3);
		else if (i == 17 || (str && fastcmp(str,"radius")))
			info->radius = luaL_checkfixed(L, 3);
		else if (i == 18 || (str && fastcmp(str,"height")))
			info->height = luaL_checkfixed(L, 3);
		else if (i == 19 || (str && fastcmp(str,"dispoffset")))
			info->dispoffset = (INT32)luaL_checkinteger(L, 3);
		else if (i == 20 || (str && fastcmp(str,"mass")))
			info->mass = (INT32)luaL_checkinteger(L, 3);
		else if (i == 21 || (str && fastcmp(str,"damage")))
			info->damage = (INT32)luaL_checkinteger(L, 3);
		else if (i == 22 || (str && fastcmp(str,"activesound")))
			info->activesound = luaL_checkinteger(L, 3);
		else if (i == 23 || (str && fastcmp(str,"flags")))
			info->flags = (INT32)luaL_checkinteger(L, 3);
		else if (i == 24 || (str && fastcmp(str,"raisestate"))) {
			info->raisestate = luaL_checkinteger(L, 3);
		}
		lua_pop(L, 1);
	}
	return 0;
}


static int lib_mobjinfolen(lua_State *L)
{
	lua_pushinteger(L, NUMMOBJTYPES);
	return 1;
}

enum mobjinfo_e
{
	mobjinfo_doomednum,
	mobjinfo_spawnstate,
	mobjinfo_spawnhealth,
	mobjinfo_seestate,
	mobjinfo_seesound,
	mobjinfo_reactiontime,
	mobjinfo_attacksound,
	mobjinfo_painstate,
	mobjinfo_painchance,
	mobjinfo_painsound,
	mobjinfo_meleestate,
	mobjinfo_missilestate,
	mobjinfo_deathstate,
	mobjinfo_xdeathstate,
	mobjinfo_deathsound,
	mobjinfo_speed,
	mobjinfo_radius,
	mobjinfo_height,
	mobjinfo_dispoffset,
	mobjinfo_mass,
	mobjinfo_damage,
	mobjinfo_activesound,
	mobjinfo_flags,
	mobjinfo_raisestate,
};

const char *const mobjinfo_opt[] = {
	"doomednum",
	"spawnstate",
	"spawnhealth",
	"seestate",
	"seesound",
	"reactiontime",
	"attacksound",
	"painstate",
	"painchance",
	"painsound",
	"meleestate",
	"missilestate",
	"deathstate",
	"xdeathstate",
	"deathsound",
	"speed",
	"radius",
	"height",
	"dispoffset",
	"mass",
	"damage",
	"activesound",
	"flags",
	"raisestate",
	NULL,
};

static int mobjinfo_fields_ref = LUA_NOREF;


static int mobjinfo_get(lua_State *L)
{
	mobjinfo_t *info = *((mobjinfo_t **)luaL_checkudata(L, 1, META_MOBJINFO));
	enum mobjinfo_e field = Lua_optoption(L, 2, mobjinfo_doomednum, mobjinfo_fields_ref);

	I_Assert(info != NULL);
	I_Assert(info >= mobjinfo);

	switch (field)
	{
	case mobjinfo_doomednum:
		lua_pushinteger(L, info->doomednum);
		break;
	case mobjinfo_spawnstate:
		lua_pushinteger(L, info->spawnstate);
		break;
	case mobjinfo_spawnhealth:
		lua_pushinteger(L, info->spawnhealth);
		break;
	case mobjinfo_seestate:
		lua_pushinteger(L, info->seestate);
		break;
	case mobjinfo_seesound:
		lua_pushinteger(L, info->seesound);
		break;
	case mobjinfo_reactiontime:
		lua_pushinteger(L, info->reactiontime);
		break;
	case mobjinfo_attacksound:
		lua_pushinteger(L, info->attacksound);
		break;
	case mobjinfo_painstate:
		lua_pushinteger(L, info->painstate);
		break;
	case mobjinfo_painchance:
		lua_pushinteger(L, info->painchance);
		break;
	case mobjinfo_painsound:
		lua_pushinteger(L, info->painsound);
		break;
	case mobjinfo_meleestate:
		lua_pushinteger(L, info->meleestate);
		break;
	case mobjinfo_missilestate:
		lua_pushinteger(L, info->missilestate);
		break;
	case mobjinfo_deathstate:
		lua_pushinteger(L, info->deathstate);
		break;
	case mobjinfo_xdeathstate:
		lua_pushinteger(L, info->xdeathstate);
		break;
	case mobjinfo_deathsound:
		lua_pushinteger(L, info->deathsound);
		break;
	case mobjinfo_speed:
		lua_pushinteger(L, info->speed);  
		break;
	case mobjinfo_radius:
		lua_pushfixed(L, info->radius);
		break;
	case mobjinfo_height:
		lua_pushfixed(L, info->height);
		break;
	case mobjinfo_dispoffset:
		lua_pushinteger(L, info->dispoffset);
		break;
	case mobjinfo_mass:
		lua_pushinteger(L, info->mass);
		break;
	case mobjinfo_damage:
		lua_pushinteger(L, info->damage);
		break;
	case mobjinfo_activesound:
		lua_pushinteger(L, info->activesound);
		break;
	case mobjinfo_flags:
		lua_pushinteger(L, info->flags);
		break;
	case mobjinfo_raisestate:
		lua_pushinteger(L, info->raisestate);
		break;
	default:
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, info);
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) {  
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "mobjinfo_t", lua_tostring(L, 2));
			return 0;
		}
		lua_pushvalue(L, 2);  
		lua_gettable(L, -2);
		if (lua_isnil(L, -1))  
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "mobjinfo_t", lua_tostring(L, 2));
		break;
	}
	return 1;
}


static int mobjinfo_set(lua_State *L)
{
	mobjinfo_t *info = *((mobjinfo_t **)luaL_checkudata(L, 1, META_MOBJINFO));
	enum mobjinfo_e field = Lua_optoption(L, 2, -1, mobjinfo_fields_ref);

	if (hud_running)
		return luaL_error(L, "Do not alter mobjinfo in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter mobjinfo in CMD building code!");

	I_Assert(info != NULL);
	I_Assert(info >= mobjinfo);

	switch (field)
	{
	case mobjinfo_doomednum:
		info->doomednum = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_spawnstate:
		info->spawnstate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_spawnhealth:
		info->spawnhealth = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_seestate:
		info->seestate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_seesound:
		info->seesound = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_reactiontime:
		info->reactiontime = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_attacksound:
		info->attacksound = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_painstate:
		info->painstate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_painchance:
		info->painchance = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_painsound:
		info->painsound = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_meleestate:
		info->meleestate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_missilestate:
		info->missilestate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_deathstate:
		info->deathstate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_xdeathstate:
		info->xdeathstate = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_deathsound:
		info->deathsound = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_speed:
		info->speed = luaL_checkfixed(L, 3);
		break;
	case mobjinfo_radius:
		info->radius = luaL_checkfixed(L, 3);
		break;
	case mobjinfo_height:
		info->height = luaL_checkfixed(L, 3);
		break;
	case mobjinfo_dispoffset:
		info->dispoffset = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_mass:
		info->mass = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_damage:
		info->damage = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_activesound:
		info->activesound = luaL_checkinteger(L, 3);
		break;
	case mobjinfo_flags:
		info->flags = (INT32)luaL_checkinteger(L, 3);
		break;
	case mobjinfo_raisestate:
		info->raisestate = luaL_checkinteger(L, 3);
		break;
	default:
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, info);
		lua_rawget(L, -2);
		if (lua_isnil(L, -1)) {

			lua_pop(L, 1);
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; adding it as Lua data.\n"), "mobjinfo_t", lua_tostring(L, 2));
			lua_newtable(L);
			lua_pushlightuserdata(L, info);
			lua_pushvalue(L, -2);  
			lua_rawset(L, -4);  
		}
		lua_pushvalue(L, 2);  
		lua_pushvalue(L, 3);  
		lua_settable(L, -3);
		lua_pop(L, 2);
	}
	return 0;
}


static int mobjinfo_num(lua_State *L)
{
	mobjinfo_t *info = *((mobjinfo_t **)luaL_checkudata(L, 1, META_MOBJINFO));

	I_Assert(info != NULL);
	I_Assert(info >= mobjinfo);

	lua_pushinteger(L, info-mobjinfo);
	return 1;
}






static int lib_getSfxInfo(lua_State *L)
{
	UINT32 i;
	lua_remove(L, 1);

	i = luaL_checkinteger(L, 1);
	if (i == 0 || i >= NUMSFX)
		return luaL_error(L, "sfxinfo[] index %d out of range (1 - %d)", i, NUMSFX-1);
	LUA_PushUserdata(L, &S_sfx[i], META_SFXINFO);
	return 1;
}


static int lib_setSfxInfo(lua_State *L)
{
	sfxinfo_t *info;

	lua_remove(L, 1);
	{
		UINT32 i = luaL_checkinteger(L, 1);
		if (i == 0 || i >= NUMSFX)
			return luaL_error(L, "sfxinfo[] index %d out of range (1 - %d)", i, NUMSFX-1);
		info = &S_sfx[i];  
	}
	luaL_checktype(L, 2, LUA_TTABLE);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	if (hud_running)
		return luaL_error(L, "Do not alter sfxinfo in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter sfxinfo in CMD building code!");

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		enum sfxinfo_write i;

		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2) - 1;  
		else
			i = luaL_checkoption(L, 2, NULL, sfxinfo_wopt);

		switch(i)
		{
		case sfxinfow_singular:
			info->singularity = luaL_checkboolean(L, 3);
			break;
		case sfxinfow_priority:
			info->priority = (INT32)luaL_checkinteger(L, 3);
			break;
		case sfxinfow_flags:
			info->pitch = (INT32)luaL_checkinteger(L, 3);
			break;
		case sfxinfow_caption:
			strlcpy(info->caption, luaL_checkstring(L, 3), sizeof(info->caption));
			break;
		default:
			break;
		}
		lua_pop(L, 1);
	}

	return 0;
}

static int lib_sfxlen(lua_State *L)
{
	lua_pushinteger(L, NUMSFX);
	return 1;
}


static int sfxinfo_get(lua_State *L)
{
	sfxinfo_t *sfx = *((sfxinfo_t **)luaL_checkudata(L, 1, META_SFXINFO));
	enum sfxinfo_read field = luaL_checkoption(L, 2, NULL, sfxinfo_ropt);

	I_Assert(sfx != NULL);

	switch (field)
	{
	case sfxinfor_name:
		lua_pushstring(L, sfx->name);
		return 1;
	case sfxinfor_singular:
		lua_pushboolean(L, sfx->singularity);
		return 1;
	case sfxinfor_priority:
		lua_pushinteger(L, sfx->priority);
		return 1;
	case sfxinfor_flags:
		lua_pushinteger(L, sfx->pitch);
		return 1;
	case sfxinfor_caption:
		lua_pushstring(L, sfx->caption);
		return 1;
	case sfxinfor_skinsound:
		lua_pushinteger(L, sfx->skinsound);
		return 1;
	default:
		return luaL_error(L, "Field does not exist in sfxinfo_t");
	}
	return 0;
}


static int sfxinfo_set(lua_State *L)
{
	sfxinfo_t *sfx = *((sfxinfo_t **)luaL_checkudata(L, 1, META_SFXINFO));
	enum sfxinfo_write field = luaL_checkoption(L, 2, NULL, sfxinfo_wopt);

	if (hud_running)
		return luaL_error(L, "Do not alter S_sfx in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter S_sfx in CMD building code!");

	I_Assert(sfx != NULL);

	lua_remove(L, 1);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	switch (field)
	{
	case sfxinfow_singular:
		sfx->singularity = luaL_checkboolean(L, 1);
		break;
	case sfxinfow_priority:
		sfx->priority = luaL_checkinteger(L, 1);
		break;
	case sfxinfow_flags:
		sfx->pitch = luaL_checkinteger(L, 1);
		break;
	case sfxinfow_caption:
		strlcpy(sfx->caption, luaL_checkstring(L, 1), sizeof(sfx->caption));
		break;
	default:
		return luaL_error(L, "Field does not exist in sfxinfo_t");
	}
	return 0;
}

static int sfxinfo_num(lua_State *L)
{
	sfxinfo_t *sfx = *((sfxinfo_t **)luaL_checkudata(L, 1, META_SFXINFO));

	I_Assert(sfx != NULL);
	I_Assert(sfx >= S_sfx);

	lua_pushinteger(L, (UINT32)(sfx-S_sfx));
	return 1;
}





static int lib_getluabanks(lua_State *L)
{
	UINT8 i;

	lua_remove(L, 1);  

	if (lua_isnumber(L, 1))
		i = lua_tonumber(L, 1);
	else
		return luaL_error(L, "luabanks[] invalid index");

	if (i >= NUM_LUABANKS)
		luaL_error(L, "luabanks[] index %d out of range (%d - %d)", i, 0, NUM_LUABANKS-1);

	lua_pushinteger(L, luabanks[i]);
	return 1;
}

static int lib_setluabanks(lua_State *L)
{
	UINT8 i;
	INT32 j = 0;

	if (hud_running)
		return luaL_error(L, "Do not alter luabanks[] in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter luabanks[] in CMD building code!");

	lua_remove(L, 1);  

	if (lua_isnumber(L, 1))
		i = lua_tonumber(L, 1);
	else
		return luaL_error(L, "luabanks[] invalid index");

	if (i >= NUM_LUABANKS)
		luaL_error(L, "luabanks[] index %d out of range (%d - %d)", i, 0, NUM_LUABANKS-1);

	if (lua_isnumber(L, 2))
		j = lua_tonumber(L, 2);
	else
		return luaL_error(L, "luabanks[] invalid set");

	luabanks[i] = j;
	return 0;
}

static int lib_luabankslen(lua_State *L)
{
	lua_pushinteger(L, NUM_LUABANKS);
	return 1;
}






static int lib_getSkinColor(lua_State *L)
{
	UINT32 i;
	lua_remove(L, 1);

	i = luaL_checkinteger(L, 1);
	if (!i || i >= numskincolors)
		return luaL_error(L, "skincolors[] index %d out of range (1 - %d)", i, numskincolors-1);
	LUA_PushUserdata(L, &skincolors[i], META_SKINCOLOR);
	return 1;
}


static void setRamp(lua_State *L, skincolor_t* c) {
	UINT32 i;
	lua_pushnil(L);
	for (i=0; i<COLORRAMPSIZE; i++) {
		if (lua_objlen(L,-2)!=COLORRAMPSIZE) {
			luaL_error(L, LUA_QL("skincolor_t") " field 'ramp' must be %d entries long; got %d.", COLORRAMPSIZE, luaL_getn(L,-2));
			break;
		}
		if (lua_next(L, -2) != 0) {
			c->ramp[i] = lua_isnumber(L,-1) ? (UINT8)luaL_checkinteger(L,-1) : 120;
			lua_pop(L, 1);
		} else
			c->ramp[i] = 120;
	}
	lua_pop(L,1);
}


static int lib_setSkinColor(lua_State *L)
{
	UINT32 j;
	skincolor_t *info;
	UINT16 cnum;  
	lua_remove(L, 1);  
	{
		cnum = (UINT16)luaL_checkinteger(L, 1);
		if (!cnum || cnum >= numskincolors)
			return luaL_error(L, "skincolors[] index %d out of range (1 - %d)", cnum, numskincolors-1);
		info = &skincolors[cnum];  
	}
	luaL_checktype(L, 2, LUA_TTABLE);  
	lua_remove(L, 1);  
	lua_settop(L, 1);  

	if (hud_running)
		return luaL_error(L, "Do not alter skincolors in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter skincolors in CMD building code!");


	memset(info,0,sizeof(skincolor_t));

	Color_cons_t[cnum].value = cnum;
	lua_pushnil(L);
	while (lua_next(L, 1)) {
		lua_Integer i = 0;
		const char *str = NULL;
		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2);
		else
			str = luaL_checkstring(L, 2);

		if (i == 1 || (str && fastcmp(str,"name"))) {
			const char* n = luaL_checkstring(L, 3);
			strlcpy(info->name, n, MAXCOLORNAME+1);
			if (strlen(n) > MAXCOLORNAME)
				CONS_Alert(CONS_WARNING, "skincolor_t field 'name' ('%s') longer than %d chars; clipped to %s.\n", n, MAXCOLORNAME, info->name);
#if 0
			if (strchr(info->name, ' ') != NULL)
				CONS_Alert(CONS_WARNING, "skincolor_t field 'name' ('%s') contains spaces.\n", info->name);
#endif

			if (info->name[0] != '\0')  
			{
				UINT16 dupecheck = R_GetColorByName(info->name);
				if (!stricmp(info->name, skincolors[SKINCOLOR_NONE].name) || (dupecheck && (dupecheck != info-skincolors)))
					CONS_Alert(CONS_WARNING, "skincolor_t field 'name' ('%s') is a duplicate of another skincolor's name.\n", info->name);
			}
		} else if (i == 2 || (str && fastcmp(str,"ramp"))) {
			if (!lua_istable(L, 3) && luaL_checkudata(L, 3, META_COLORRAMP) == NULL)
				return luaL_error(L, LUA_QL("skincolor_t") " field 'ramp' must be a table or array.");
			else if (lua_istable(L, 3))
				setRamp(L, info);
			else
				for (j=0; j<COLORRAMPSIZE; j++)
					info->ramp[j] = (*((UINT8 **)luaL_checkudata(L, 3, META_COLORRAMP)))[j];
			skincolor_modified[cnum] = true;
		} else if (i == 3 || (str && fastcmp(str,"invcolor"))) {
			UINT16 v = (UINT16)luaL_checkinteger(L, 3);
			if (v >= numskincolors)
				return luaL_error(L, "skincolor_t field 'invcolor' out of range (1 - %d)", numskincolors-1);
			info->invcolor = v;
		} else if (i == 4 || (str && fastcmp(str,"invshade")))
			info->invshade = (UINT8)luaL_checkinteger(L, 3)%COLORRAMPSIZE;
		else if (i == 5 || (str && fastcmp(str,"chatcolor")))
			info->chatcolor = (UINT16)luaL_checkinteger(L, 3);
		else if (i == 6 || (str && fastcmp(str,"accessible"))) {
			boolean v = lua_toboolean(L, 3);
			if (cnum < FIRSTSUPERCOLOR && v != skincolors[cnum].accessible)
				CONS_Alert(CONS_WARNING, "skincolors[] index %d is a standard color; accessibility changes are prohibited.\n", cnum);
			else
				info->accessible = v;
		}
		lua_pop(L, 1);
	}
	return 0;
}


static int lib_skincolorslen(lua_State *L)
{
	lua_pushinteger(L, numskincolors);
	return 1;
}


static int skincolor_get(lua_State *L)
{
	skincolor_t *info = *((skincolor_t **)luaL_checkudata(L, 1, META_SKINCOLOR));
	const char *field = luaL_checkstring(L, 2);

	I_Assert(info != NULL);
	I_Assert(info >= skincolors);

	if (fastcmp(field,"name"))
		lua_pushstring(L, info->name);
	else if (fastcmp(field,"ramp"))
		LUA_PushUserdata(L, info->ramp, META_COLORRAMP);
	else if (fastcmp(field,"invcolor"))
		lua_pushinteger(L, info->invcolor);
	else if (fastcmp(field,"invshade"))
		lua_pushinteger(L, info->invshade);
	else if (fastcmp(field,"chatcolor"))
		lua_pushinteger(L, info->chatcolor);
	else if (fastcmp(field,"accessible"))
		lua_pushboolean(L, info->accessible);
	else {
		CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "skincolor_t", field);
		return 0;
	}
	return 1;
}


static int skincolor_set(lua_State *L)
{
	UINT32 i;
	skincolor_t *info = *((skincolor_t **)luaL_checkudata(L, 1, META_SKINCOLOR));
	const char *field = luaL_checkstring(L, 2);
	UINT16 cnum = (UINT16)(info-skincolors);

	I_Assert(info != NULL);
	I_Assert(info >= skincolors);

	if (!cnum || cnum >= numskincolors)
		return luaL_error(L, "skincolors[] index %d out of range (1 - %d)", cnum, numskincolors-1);

	if (fastcmp(field,"name")) {
		const char* n = luaL_checkstring(L, 3);
		strlcpy(info->name, n, MAXCOLORNAME+1);
		if (strlen(n) > MAXCOLORNAME)
			CONS_Alert(CONS_WARNING, "skincolor_t field 'name' ('%s') longer than %d chars; clipped to %s.\n", n, MAXCOLORNAME, info->name);
#if 0
		if (strchr(info->name, ' ') != NULL)
			CONS_Alert(CONS_WARNING, "skincolor_t field 'name' ('%s') contains spaces.\n", info->name);
#endif

		if (info->name[0] != '\0')  
		{
			UINT16 dupecheck = R_GetColorByName(info->name);
			if (!stricmp(info->name, skincolors[SKINCOLOR_NONE].name) || (dupecheck && (dupecheck != cnum)))
				CONS_Alert(CONS_WARNING, "skincolor_t field 'name' ('%s') is a duplicate of another skincolor's name.\n", info->name);
		}
	} else if (fastcmp(field,"ramp")) {
		if (!lua_istable(L, 3) && luaL_checkudata(L, 3, META_COLORRAMP) == NULL)
			return luaL_error(L, LUA_QL("skincolor_t") " field 'ramp' must be a table or array.");
		else if (lua_istable(L, 3))
			setRamp(L, info);
		else
			for (i=0; i<COLORRAMPSIZE; i++)
				info->ramp[i] = (*((UINT8 **)luaL_checkudata(L, 3, META_COLORRAMP)))[i];
		skincolor_modified[cnum] = true;
	} else if (fastcmp(field,"invcolor")) {
		UINT16 v = (UINT16)luaL_checkinteger(L, 3);
		if (v >= numskincolors)
			return luaL_error(L, "skincolor_t field 'invcolor' out of range (1 - %d)", numskincolors-1);
		info->invcolor = v;
	} else if (fastcmp(field,"invshade"))
		info->invshade = (UINT8)luaL_checkinteger(L, 3)%COLORRAMPSIZE;
	else if (fastcmp(field,"chatcolor"))
		info->chatcolor = (UINT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"accessible")) {
		boolean v = lua_toboolean(L, 3);
		if (cnum < FIRSTSUPERCOLOR && v != skincolors[cnum].accessible)
			CONS_Alert(CONS_WARNING, "skincolors[] index %d is a standard color; accessibility changes are prohibited.\n", cnum);
		else
			info->accessible = v;
	} else
		CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "skincolor_t", field);
	return 1;
}


static int skincolor_num(lua_State *L)
{
	skincolor_t *info = *((skincolor_t **)luaL_checkudata(L, 1, META_SKINCOLOR));

	I_Assert(info != NULL);
	I_Assert(info >= skincolors);

	lua_pushinteger(L, info-skincolors);
	return 1;
}


static int colorramp_get(lua_State *L)
{
	UINT8 *colorramp = *((UINT8 **)luaL_checkudata(L, 1, META_COLORRAMP));
	UINT32 n = luaL_checkinteger(L, 2);
	if (n >= COLORRAMPSIZE)
		return luaL_error(L, LUA_QL("skincolor_t") " field 'ramp' index %d out of range (0 - %d)", n, COLORRAMPSIZE-1);
	lua_pushinteger(L, colorramp[n]);
	return 1;
}


static int colorramp_set(lua_State *L)
{
	UINT8 *colorramp = *((UINT8 **)luaL_checkudata(L, 1, META_COLORRAMP));
	UINT16 cnum = (UINT16)(((UINT8*)colorramp - (UINT8*)(skincolors[0].ramp))/sizeof(skincolor_t));
	UINT32 n = luaL_checkinteger(L, 2);
	UINT8 i = (UINT8)luaL_checkinteger(L, 3);
	if (!cnum || cnum >= numskincolors)
		return luaL_error(L, "skincolors[] index %d out of range (1 - %d)", cnum, numskincolors-1);
	if (n >= COLORRAMPSIZE)
		return luaL_error(L, LUA_QL("skincolor_t") " field 'ramp' index %d out of range (0 - %d)", n, COLORRAMPSIZE-1);
	if (hud_running)
		return luaL_error(L, "Do not alter skincolor_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter skincolor_t in CMD building code!");
	colorramp[n] = i;
	skincolor_modified[cnum] = true;
	return 0;
}


static int colorramp_len(lua_State *L)
{
	lua_pushinteger(L, COLORRAMPSIZE);
	return 1;
}






int LUA_InfoLib(lua_State *L)
{

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_STATEACTION);


	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_ACTIONS);

	LUA_RegisterUserdataMetatable(L, META_STATE, state_get, state_set, state_num);
	LUA_RegisterUserdataMetatable(L, META_MOBJINFO, mobjinfo_get, mobjinfo_set, mobjinfo_num);
	LUA_RegisterUserdataMetatable(L, META_SKINCOLOR, skincolor_get, skincolor_set, skincolor_num);
	LUA_RegisterUserdataMetatable(L, META_COLORRAMP, colorramp_get, colorramp_set, colorramp_len);
	LUA_RegisterUserdataMetatable(L, META_SFXINFO, sfxinfo_get, sfxinfo_set, sfxinfo_num);
	LUA_RegisterUserdataMetatable(L, META_SPRITEINFO, spriteinfo_get, spriteinfo_set, spriteinfo_num);
	LUA_RegisterUserdataMetatable(L, META_PIVOTLIST, pivotlist_get, pivotlist_set, pivotlist_num);
	LUA_RegisterUserdataMetatable(L, META_FRAMEPIVOT, framepivot_get, framepivot_set, framepivot_num);
	LUA_RegisterUserdataMetatable(L, META_LUABANKS, lib_getluabanks, lib_setluabanks, lib_luabankslen);

	mobjinfo_fields_ref = Lua_CreateFieldTable(L, mobjinfo_opt);

	LUA_RegisterGlobalUserdata(L, "sprnames", lib_getSprname, NULL, lib_sprnamelen);
	LUA_RegisterGlobalUserdata(L, "spr2names", lib_getSpr2name, NULL, lib_spr2namelen);
	LUA_RegisterGlobalUserdata(L, "spr2defaults", lib_getSpr2default, lib_setSpr2default, lib_spr2namelen);
	LUA_RegisterGlobalUserdata(L, "states", lib_getState, lib_setState, lib_statelen);
	LUA_RegisterGlobalUserdata(L, "mobjinfo", lib_getMobjInfo, lib_setMobjInfo, lib_mobjinfolen);
	LUA_RegisterGlobalUserdata(L, "skincolors", lib_getSkinColor, lib_setSkinColor, lib_skincolorslen);
	LUA_RegisterGlobalUserdata(L, "spriteinfo", lib_getSpriteInfo, lib_setSpriteInfo, lib_spriteinfolen);
	LUA_RegisterGlobalUserdata(L, "sfxinfo", lib_getSfxInfo, lib_setSfxInfo, lib_sfxlen);

	LUA_RegisterGlobalUserdata(L, "S_sfx", lib_getSfxInfo, lib_setSfxInfo, lib_sfxlen);

	return 0;
}
