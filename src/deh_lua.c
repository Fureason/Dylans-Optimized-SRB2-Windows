// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "deh_lua.h"







static inline int lib_freeslot(lua_State *L)
{
	int n = lua_gettop(L);
	int r = 0;  
	char *s, *type,*word;

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	if (!deh_loaded)
	{
		initfreeslots();
		deh_loaded = true;
	}

	while (n-- > 0)
	{
		s = Z_StrDup(luaL_checkstring(L,1));
		type = strtok(s, "_");
		if (type)
			strupr(type);
		else {
			Z_Free(s);
			return luaL_error(L, "Unknown enum type in '%s'\n", luaL_checkstring(L, 1));
		}

		word = strtok(NULL, "\n");
		if (word)
			strupr(word);
		else {
			Z_Free(s);
			return luaL_error(L, "Missing enum name in '%s'\n", luaL_checkstring(L, 1));
		}
		if (fastcmp(type, "SFX")) {
			sfxenum_t sfx;
			strlwr(word);
			CONS_Printf("Sound sfx_%s allocated.\n",word);
			sfx = S_AddSoundFx(word, false, 0, false);
			if (sfx != sfx_None) {
				lua_pushinteger(L, sfx);
				r++;
			} else
				CONS_Alert(CONS_WARNING, "Ran out of free SFX slots!\n");
		}
		else if (fastcmp(type, "SPR"))
		{
			spritenum_t j;

			if (strlen(word) > MAXSPRITENAME)
				return luaL_error(L, "Sprite name is longer than %d characters\n", MAXSPRITENAME);

			for (j = SPR_FIRSTFREESLOT; j <= SPR_LASTFREESLOT; j++)
			{
				if (in_bit_array(used_spr, j - SPR_FIRSTFREESLOT))
					continue;  

				CONS_Printf("Sprite SPR_%s allocated.\n",word);
				strcpy(sprnames[j], word);
				set_bit_array(used_spr, j - SPR_FIRSTFREESLOT);  

				LUA_UpdateSprName(word, j);
				lua_pushinteger(L, j);
				r++;
				break;
			}
			if (j > SPR_LASTFREESLOT)
				CONS_Alert(CONS_WARNING, "Ran out of free sprite slots!\n");
		}
		else if (fastcmp(type, "S"))
		{
			statenum_t i;
			for (i = 0; i < NUMSTATEFREESLOTS; i++)
				if (!FREE_STATES[i]) {
					CONS_Printf("State S_%s allocated.\n",word);
					FREE_STATES[i] = Z_Malloc(strlen(word)+1, PU_STATIC, NULL);
					strcpy(FREE_STATES[i],word);
					lua_pushinteger(L, S_FIRSTFREESLOT + i);
					r++;
					break;
				}
			if (i == NUMSTATEFREESLOTS)
				CONS_Alert(CONS_WARNING, "Ran out of free State slots!\n");
		}
		else if (fastcmp(type, "MT"))
		{
			mobjtype_t i;
			for (i = 0; i < NUMMOBJFREESLOTS; i++)
				if (!FREE_MOBJS[i]) {
					CONS_Printf("MobjType MT_%s allocated.\n",word);
					FREE_MOBJS[i] = Z_Malloc(strlen(word)+1, PU_STATIC, NULL);
					strcpy(FREE_MOBJS[i],word);
					lua_pushinteger(L, MT_FIRSTFREESLOT + i);
					r++;
					break;
				}
			if (i == NUMMOBJFREESLOTS)
				CONS_Alert(CONS_WARNING, "Ran out of free MobjType slots!\n");
		}
		else if (fastcmp(type, "SKINCOLOR"))
		{
			skincolornum_t i;
			for (i = 0; i < NUMCOLORFREESLOTS; i++)
				if (!FREE_SKINCOLORS[i]) {
					CONS_Printf("Skincolor SKINCOLOR_%s allocated.\n",word);
					FREE_SKINCOLORS[i] = Z_Malloc(strlen(word)+1, PU_STATIC, NULL);
					strcpy(FREE_SKINCOLORS[i],word);
					M_AddMenuColor(numskincolors++);
					lua_pushinteger(L, SKINCOLOR_FIRSTFREESLOT + i);
					r++;
					break;
				}
			if (i == NUMCOLORFREESLOTS)
				CONS_Alert(CONS_WARNING, "Ran out of free skincolor slots!\n");
		}
		else if (fastcmp(type, "SPR2"))
		{

			playersprite_t i;
			for (i = SPR2_FIRSTFREESLOT; i < free_spr2; i++)
				if (memcmp(spr2names[i],word,4) == 0)
					break;

			if (i >= free_spr2) {
				if (free_spr2 < NUMPLAYERSPRITES)
				{
					CONS_Printf("Sprite SPR2_%s allocated.\n",word);
					strncpy(spr2names[free_spr2],word,4);
					spr2defaults[free_spr2] = 0;
					lua_pushinteger(L, free_spr2);
					r++;
					spr2names[free_spr2++][4] = 0;
				} else
					CONS_Alert(CONS_WARNING, "Ran out of free SPR2 slots!\n");
			}
		}
		else if (fastcmp(type, "TOL"))
		{

			int i;
			for (i = 0; TYPEOFLEVEL[i].name; i++)
				if (fastcmp(word, TYPEOFLEVEL[i].name))
					break;


			if (TYPEOFLEVEL[i].name == NULL) {
				if (lastcustomtol == (UINT32)MAXTOL)  
					CONS_Alert(CONS_WARNING, "Ran out of free typeoflevel slots!\n");
				else {
					CONS_Printf("TypeOfLevel TOL_%s allocated.\n",word);
					G_AddTOL(lastcustomtol, word);
					lua_pushinteger(L, lastcustomtol);
					lastcustomtol <<= 1;
					r++;
				}
			}
		}
		Z_Free(s);
		lua_remove(L, 1);
		continue;
	}

	R_RefreshSprite2();

	return r;
}



static int action_call(lua_State *L)
{
	actionf_t *action = *((actionf_t **)luaL_checkudata(L, 1, META_ACTION));
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));

	var1 = (INT32)luaL_optinteger(L, 3, 0);
	var2 = (INT32)luaL_optinteger(L, 4, 0);

	if (!actor)
	{
		return LUA_ErrInvalid(L, "mobj_t");
	}

	action->acp1(actor);
	return 0;
}



const char *luaactions[MAX_ACTION_RECURSION];
UINT8 luaactionstack = 0;

static int lib_dummysuper(lua_State *L)
{


	return luaL_error(L, "Can't call super() outside of hardcode-replacing A_Action functions!");
}

static void CacheAndPushConstant(lua_State *L, const char *name, lua_Integer value)
{

	lua_pushstring(L, name);
	lua_pushinteger(L, value);
	lua_rawset(L, LUA_GLOBALSINDEX);

	lua_pushinteger(L, value);
}



static int ScanConstants(lua_State *L, boolean mathlib, const char *word)
{
	const char *p;
	fixed_t i;

	if (strlen(word) == 1) {  
		if (*word >= 'A' && *word <= '~')
		{
			CacheAndPushConstant(L, word, *word-'A');
			return 1;
		}
		if (mathlib) return luaL_error(L, "constant '%s' could not be parsed.\n", word);
		return 0;
	}
	else if (fastncmp("MF_", word, 3)) {
		p = word+3;
		for (i = 0; MOBJFLAG_LIST[i]; i++)
			if (fastcmp(p, MOBJFLAG_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}
		if (mathlib) return luaL_error(L, "mobjflag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("MF2_", word, 4)) {
		p = word+4;
		for (i = 0; MOBJFLAG2_LIST[i]; i++)
			if (fastcmp(p, MOBJFLAG2_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}
		if (mathlib) return luaL_error(L, "mobjflag2 '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("MFE_", word, 4)) {
		p = word+4;
		for (i = 0; MOBJEFLAG_LIST[i]; i++)
			if (fastcmp(p, MOBJEFLAG_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}
		if (fastcmp(p, "REVERSESUPER"))
		{
			CacheAndPushConstant(L, word, (lua_Integer)MFE_REVERSESUPER);
			return 1;
		}
		if (mathlib) return luaL_error(L, "mobjeflag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("MTF_", word, 4)) {
		p = word+4;
		for (i = 0; MAPTHINGFLAG_LIST[i]; i++)
			if (fastcmp(p, MAPTHINGFLAG_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}
		if (mathlib) return luaL_error(L, "mapthingflag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("PF_", word, 3)) {
		p = word+3;
		for (i = 0; PLAYERFLAG_LIST[i]; i++)
			if (fastcmp(p, PLAYERFLAG_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}
		if (fastcmp(p, "FULLSTASIS"))
		{
			CacheAndPushConstant(L, word, (lua_Integer)PF_FULLSTASIS);
			return 1;
		}

		else if (fastcmp(p, "USEDOWN"))
		{
			CacheAndPushConstant(L, word, (lua_Integer)PF_SPINDOWN);
			return 1;
		}
		if (mathlib) return luaL_error(L, "playerflag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("GT_", word, 3)) {
		p = word;
		for (i = 0; Gametype_ConstantNames[i]; i++)
			if (fastcmp(p, Gametype_ConstantNames[i])) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "gametype '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("GTR_", word, 4)) {
		p = word+4;
		for (i = 0; GAMETYPERULE_LIST[i]; i++)
			if (fastcmp(p, GAMETYPERULE_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}
		if (mathlib) return luaL_error(L, "game type rule '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("TOL_", word, 4)) {
		p = word+4;
		for (i = 0; TYPEOFLEVEL[i].name; i++)
			if (fastcmp(p, TYPEOFLEVEL[i].name)) {
				CacheAndPushConstant(L, word, TYPEOFLEVEL[i].flag);
				return 1;
			}
		if (mathlib) return luaL_error(L, "typeoflevel '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("ML_", word, 3)) {
		p = word+3;

		for (i = 0; ML_LIST[i]; i++)
			if (fastcmp(p, ML_LIST[i])) {
				CacheAndPushConstant(L, word, ((lua_Integer)1<<i));
				return 1;
			}

		if (fastcmp(p, "EFFECT1"))
		{
			lua_pushinteger(L, (lua_Integer)ML_SKEWTD);
			return 1;
		}
		if (fastcmp(p, "EFFECT2"))
		{
			lua_pushinteger(L, (lua_Integer)ML_NOSKEW);
			return 1;
		}
		if (fastcmp(p, "EFFECT3"))
		{
			lua_pushinteger(L, (lua_Integer)ML_MIDPEG);
			return 1;
		}
		if (fastcmp(p, "EFFECT4"))
		{
			lua_pushinteger(L, (lua_Integer)ML_MIDSOLID);
			return 1;
		}
		if (fastcmp(p, "EFFECT5"))
		{
			lua_pushinteger(L, (lua_Integer)ML_WRAPMIDTEX);
			return 1;
		}
		if (mathlib) return luaL_error(L, "linedef flag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("MSF_", word, 4)) {
		p = word + 4;
		for (i = 0; MSF_LIST[i]; i++)
			if (fastcmp(p, MSF_LIST[i])) {
				lua_pushinteger(L, ((lua_Integer)1 << i));
				return 1;
			}
		if (fastcmp(p, "FLIPSPECIAL_BOTH"))
		{
			lua_pushinteger(L, (lua_Integer)MSF_FLIPSPECIAL_BOTH);
			return 1;
		}
		if (mathlib) return luaL_error(L, "sector flag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("SSF_", word, 4)) {
		p = word + 4;
		for (i = 0; SSF_LIST[i]; i++)
			if (fastcmp(p, SSF_LIST[i])) {
				lua_pushinteger(L, ((lua_Integer)1 << i));
				return 1;
			}
		if (mathlib) return luaL_error(L, "sector special flag '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("SD_", word, 3)) {
		p = word + 3;
		for (i = 0; SD_LIST[i]; i++)
			if (fastcmp(p, SD_LIST[i])) {
				lua_pushinteger(L, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "sector damagetype '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("TO_", word, 3)) {
		p = word + 3;
		for (i = 0; TO_LIST[i]; i++)
			if (fastcmp(p, TO_LIST[i])) {
				lua_pushinteger(L, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "sector triggerer '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("S_",word,2)) {
		p = word+2;
		for (i = 0; i < NUMSTATEFREESLOTS; i++) {
			if (!FREE_STATES[i])
				break;
			if (fastcmp(p, FREE_STATES[i])) {
				CacheAndPushConstant(L, word, S_FIRSTFREESLOT+i);
				return 1;
			}
		}
		for (i = 0; i < S_FIRSTFREESLOT; i++)
			if (fastcmp(p, STATE_LIST[i]+2)) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return luaL_error(L, "state '%s' does not exist.\n", word);
	}
	else if (fastncmp("MT_",word,3)) {
		p = word+3;
		for (i = 0; i < NUMMOBJFREESLOTS; i++) {
			if (!FREE_MOBJS[i])
				break;
			if (fastcmp(p, FREE_MOBJS[i])) {
				CacheAndPushConstant(L, word, MT_FIRSTFREESLOT+i);
				return 1;
			}
		}
		for (i = 0; i < MT_FIRSTFREESLOT; i++)
			if (fastcmp(p, MOBJTYPE_LIST[i]+3)) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return luaL_error(L, "mobjtype '%s' does not exist.\n", word);
	}
	else if (fastncmp("SPR_",word,4)) {
		p = word+4;
		i = R_GetSpriteNumByName(p);
		if (i != NUMSPRITES)
		{


			if (mathlib)
				lua_pushinteger(L, i);
			else
				CacheAndPushConstant(L, word, i);
			return 1;
		}
		else if (mathlib)
			return luaL_error(L, "sprite '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("SPR2_",word,5)) {
		p = word+5;
		for (i = 0; i < (fixed_t)free_spr2; i++)
			if (!spr2names[i][4])
			{


				if (spr2names[i][3] == '_' && !p[3]) {
					if (fastncmp(p,spr2names[i],3)) {
						CacheAndPushConstant(L, word, i);
						return 1;
					}
				}
				else if (fastncmp(p,spr2names[i],4)) {
					CacheAndPushConstant(L, word, i);
					return 1;
				}
			}
		if (mathlib) return luaL_error(L, "player sprite '%s' could not be found.\n", word);
		return 0;
	}
	else if (!mathlib && fastncmp("sfx_",word,4)) {
		p = word+4;
		for (i = 0; i < NUMSFX; i++)
			if (S_sfx[i].name && fastcmp(p, S_sfx[i].name)) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return 0;
	}
	else if (mathlib && fastncmp("SFX_",word,4)) {  
		p = word+4;
		for (i = 0; i < NUMSFX; i++)
			if (S_sfx[i].name && fasticmp(p, S_sfx[i].name)) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return luaL_error(L, "sfx '%s' could not be found.\n", word);
	}
	else if (mathlib && fastncmp("DS",word,2)) {
		p = word+2;
		for (i = 0; i < NUMSFX; i++)
			if (S_sfx[i].name && fasticmp(p, S_sfx[i].name)) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "sfx '%s' could not be found.\n", word);
		return 0;
	}
	else if (!mathlib && fastncmp("pw_",word,3)) {
		p = word+3;
		for (i = 0; i < NUMPOWERS; i++)
			if (fasticmp(p, POWERS_LIST[i])) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return 0;
	}
	else if (mathlib && fastncmp("PW_",word,3)) {  
		p = word+3;
		for (i = 0; i < NUMPOWERS; i++)
			if (fastcmp(p, POWERS_LIST[i])) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return luaL_error(L, "power '%s' could not be found.\n", word);
	}
	else if (fastncmp("HUD_",word,4)) {
		p = word+4;
		for (i = 0; i < NUMHUDITEMS; i++)
			if (fastcmp(p, HUDITEMS_LIST[i])) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "huditem '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("SKINCOLOR_",word,10)) {
		p = word+10;
		for (i = 0; i < NUMCOLORFREESLOTS; i++) {
			if (!FREE_SKINCOLORS[i])
				break;
			if (fastcmp(p, FREE_SKINCOLORS[i])) {
				CacheAndPushConstant(L, word, SKINCOLOR_FIRSTFREESLOT+i);
				return 1;
			}
		}
		for (i = 0; i < SKINCOLOR_FIRSTFREESLOT; i++)
			if (fastcmp(p, COLOR_ENUMS[i])) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		return luaL_error(L, "skincolor '%s' could not be found.\n", word);
	}
	else if (fastncmp("GRADE_",word,6))
	{
		p = word+6;
		for (i = 0; NIGHTSGRADE_LIST[i]; i++)
			if (*p == NIGHTSGRADE_LIST[i])
			{
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "NiGHTS grade '%s' could not be found.\n", word);
		return 0;
	}
	else if (fastncmp("MN_",word,3))
	{
		p = word+3;
		for (i = 0; i < NUMMENUTYPES; i++)
			if (fastcmp(p, MENUTYPES_LIST[i])) {
				CacheAndPushConstant(L, word, i);
				return 1;
			}
		if (mathlib) return luaL_error(L, "menutype '%s' could not be found.\n", word);
		return 0;
	}
	else if (mathlib && fastncmp("TRANSLATION_",word,12))
	{
		p = word+12;
		int id = R_FindCustomTranslation_CaseInsensitive(p);
		if (id != -1)
		{
			lua_pushinteger(L, id);
			return 1;
		}
		return luaL_error(L, "translation '%s' could not be found.\n", word);
	}


	if (fastcmp(word, "BT_USE"))
	{
		CacheAndPushConstant(L, word, (lua_Integer)BT_SPIN);
		return 1;
	}

	for (i = 0; INT_CONST[i].n; i++)
		if (fastcmp(word,INT_CONST[i].n)) {
			CacheAndPushConstant(L, word, INT_CONST[i].v);
			return 1;
		}

	return 0;
}

FUNCINLINE static ATTRINLINE int getEnum(lua_State *L, boolean mathlib, const char *word)
{
	fixed_t i;




	if (!mathlib && fastncmp("A_",word,2)) {
		char *caps;




		for (i = 0; actionpointers[i].name; i++)
		{
			if (fasticmp(word, actionpointers[i].name))
			{

				LUA_PushUserdata(L, &actionpointers[i].action, META_ACTION);
				return 1;
			}
		}





		lua_getfield(L, LUA_REGISTRYINDEX, LREG_ACTIONS);


		caps = Z_StrDup(word);
		strupr(caps);
		lua_getfield(L, -1, caps);
		Z_Free(caps);

		if (!lua_isnil(L, -1))
		{
			return 1;  
		}


		lua_pop(L, 2);  
		return 0;
	}
	else if (!mathlib && fastcmp("super",word))
	{
		if (!luaactionstack)
		{

			lua_pushcfunction(L, lib_dummysuper);
			return 1;
		}

		for (i = 0; actionpointers[i].name; i++)
		{
			if (fasticmp(luaactions[luaactionstack-1], actionpointers[i].name))
			{
				LUA_PushUserdata(L, &actionpointers[i].action, META_ACTION);
				return 1;
			}
		}


		lua_pushcfunction(L, lib_dummysuper);
		return 1;
	}
	else if ((!mathlib && LUA_PushGlobals(L, word)) || ScanConstants(L, mathlib, word))
		return 1;

	return -1;
}

static int constants_get(lua_State *L)
{
	const char *key;
	int ret;

	if (!lua_isstring(L, 2))
		return 0;

	key = luaL_checkstring(L, 2);


	ret = getEnum(L, false, key);

	if (ret != -1)



		if (!lua_isuserdata(L, -1) && !lua_isfunction(L, -1))
			return ret;

	return 0;
}

static inline int lib_getenum(lua_State *L)
{
	const char *word;
	int ret;
	boolean mathlib = lua_toboolean(L, lua_upvalueindex(1));
	if (lua_type(L,2) != LUA_TSTRING)
		return 0;
	word = lua_tostring(L,2);

	ret = getEnum(L, mathlib, word);

	if (ret != -1)
		return ret;

	if (mathlib) return luaL_error(L, "constant '%s' could not be parsed.\n", word);

	return 0;
}


void LUA_UpdateSprName(const char *name, lua_Integer value)
{
	char fullname[4 + MAXSPRITENAME + 1] = "SPR_";

	if (!gL)
		return;

	strcpy(&fullname[4], name);
	lua_pushstring(gL, fullname);
	lua_rawget(gL, LUA_GLOBALSINDEX);

	if (!lua_isnil(gL, -1))
	{
		lua_pushstring(gL, fullname);
		lua_pushinteger(gL, value);
		lua_rawset(gL, LUA_GLOBALSINDEX);
	}

	lua_pop(gL, 1);  
}

int LUA_EnumLib(lua_State *L)
{
	if (lua_gettop(L) == 0)
		lua_pushboolean(L, 0);


	lua_createtable(L, 0, 1);
	lua_pushvalue(L, 1);  
	lua_pushcclosure(L, lib_getenum, 1);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, LUA_GLOBALSINDEX);
	return 0;
}


static int lib_getActionName(lua_State *L)
{
	if (lua_isuserdata(L, 1))  
	{
		actionf_t *action = *((actionf_t **)luaL_checkudata(L, 1, META_ACTION));
		const char *name = NULL;
		if (!action)
			return luaL_error(L, "not a valid action?");
		name = LUA_GetActionName(action);
		if (!name)  
			return luaL_error(L, "no name string could be found for this action");
		lua_pushstring(L, name);
		return 1;
	}
	else if (lua_isfunction(L, 1))  
	{
		lua_settop(L, 1);  

		lua_getfield(L, LUA_REGISTRYINDEX, LREG_ACTIONS);
		lua_pushnil(L);



		while (lua_next(L, -2))
		{



			if (lua_rawequal(L, -1, 1))  
			{


				if (lua_type(L, -2) == LUA_TSTRING)
				{
					lua_pushvalue(L, -2);  
					return 1;
				}
				lua_pop(L, 2);  
				break;  
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);  
		return 0;  
	}

	return luaL_typerror(L, 1, "action userdata or Lua function");
}



int LUA_SOCLib(lua_State *L)
{
	lua_register(L,"freeslot",lib_freeslot);
	lua_register(L,"getActionName",lib_getActionName);

	luaL_newmetatable(L, META_ACTION);
		LUA_SetCFunctionField(L, "__call", action_call);
	lua_pop(L, 1);



	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, constants_get);
			lua_setfield(L, -2, "__index");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "constants");

	return 0;
}

const char *LUA_GetActionName(void *action)
{
	actionf_t *act = (actionf_t *)action;
	size_t z;
	for (z = 0; actionpointers[z].name; z++)
	{
		if (actionpointers[z].action.acv == act->acv)
			return actionpointers[z].name;
	}
	return NULL;
}

void LUA_SetActionByName(void *state, const char *actiontocompare)
{
	state_t *st = (state_t *)state;
	size_t z;
	for (z = 0; actionpointers[z].name; z++)
	{
		if (fasticmp(actiontocompare, actionpointers[z].name))
		{
			st->action = actionpointers[z].action;
			st->action.acv = actionpointers[z].action.acv;  
			st->action.acp1 = actionpointers[z].action.acp1;
			return;
		}
	}
}

enum actionnum LUA_GetActionNumByName(const char *actiontocompare)
{
	size_t z;
	for (z = 0; actionpointers[z].name; z++)
		if (fasticmp(actiontocompare, actionpointers[z].name))
			return z;
	return z;
}
