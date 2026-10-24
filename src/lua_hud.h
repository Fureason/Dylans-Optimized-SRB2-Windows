// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __LUA_HUD_H__
#define __LUA_HUD_H__

#include "lua_hudlib_drawlist.h"

enum hud {
	hud_stagetitle = 0,
	hud_textspectator,
	hud_crosshair,
	hud_powerups,

	hud_score,
	hud_time,
	hud_rings,
	hud_lives,
	hud_input,

	hud_weaponrings,
	hud_powerstones,
	hud_teamscores,

	hud_nightslink,
	hud_nightsdrill,
	hud_nightsspheres,
	hud_nightsscore,
	hud_nightstime,
	hud_nightsrecords,

	hud_rankings,
	hud_coopemeralds,
	hud_tokens,
	hud_tabemblems,

	hud_intermissiontally,
	hud_intermissiontitletext,
	hud_intermissionmessages,
	hud_intermissionemeralds,
	hud_MAX
};

extern boolean hud_running;

boolean LUA_HudEnabled(enum hud option);

void LUA_SetHudHook(int hook, huddrawlist_h list);

#endif  
