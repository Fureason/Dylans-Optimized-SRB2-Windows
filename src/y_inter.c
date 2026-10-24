// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "netcode/i_net.h"
#include "i_video.h"
#include "p_tick.h"
#include "r_defs.h"
#include "r_skins.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"
#include "m_menu.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_setup.h"

#include "r_local.h"
#include "p_local.h"

#include "m_cond.h"  
#include "lua_hook.h"  

#include "lua_hud.h"
#include "lua_hudlib_drawlist.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

typedef struct
{
	char patch[9];
	 INT32 points;
	UINT8 display;
} y_bonus_t;

typedef union
{
	struct
	{
		char passed1[21];  
		char passed2[16];  
		INT32 passedx1;
		INT32 passedx2;

		y_bonus_t bonuses[4];
		patch_t *bonuspatches[4];

		SINT8 gotperfbonus;  

		UINT32 score, total;  
		UINT32 tics;  

		UINT8 actnum;  
		patch_t *ptotal;  
		UINT8 gotlife;  
	} coop;

	struct
	{
		char passed1[29];              
		char passed2[17];              
		char passed3[15];              
		char passed4[SKINNAMESIZE+7];  
		INT32 passedx1;
		INT32 passedx2;
		INT32 passedx3;
		INT32 passedx4;

		INT32 emeraldbounces;
		INT32 emeraldmomy;
		INT32 emeraldy;

		y_bonus_t bonuses[2];
		patch_t *bonuspatches[2];

		patch_t *pscore;  
		UINT32 score;  


		UINT8 continues;
		patch_t *pcontinues;
		UINT8 *playerchar;  
		UINT16 *playercolor;

		UINT8 gotlife;  
	} spec;

	struct
	{
		UINT32 scores[MAXPLAYERS];  
		UINT16 *color[MAXPLAYERS];  
		boolean spectator[MAXPLAYERS];  
		UINT8 *character[MAXPLAYERS];  
		INT32 ctfteam[MAXPLAYERS];  
		INT32 num[MAXPLAYERS];  
		char *name[MAXPLAYERS];  
		patch_t *result;  
		patch_t *blueflag;
		patch_t *redflag;  
		INT32 numplayers;  
		char levelstring[40];  
	} match;

	struct
	{
		UINT16 *color[MAXPLAYERS];  
		UINT8 *character[MAXPLAYERS];  
		INT32 num[MAXPLAYERS];  
		char name[MAXPLAYERS][9];  
		UINT32 times[MAXPLAYERS];
		UINT32 rings[MAXPLAYERS];
		UINT32 maxrings[MAXPLAYERS];
		UINT32 monitors[MAXPLAYERS];
		UINT32 scores[MAXPLAYERS];
		UINT32 points[MAXPLAYERS];
		INT32 numplayers;  
		char levelstring[40];  
	} competition;

} y_data;

static y_data data;


static patch_t *bgpatch = NULL;      
static patch_t *bgtile = NULL;       
static patch_t *interpic = NULL;     
static boolean usetile;
static INT32 timer;

typedef struct
{
	INT32 source_width, source_height;
	INT32 source_bpp, source_rowbytes;
	UINT8 *source_picture;
	INT32 target_width, target_height;
	INT32 target_bpp, target_rowbytes;
	UINT8 *target_picture;
} y_buffer_t;

boolean usebuffer = false;
static boolean useinterpic;
static y_buffer_t *y_buffer;

static INT32 intertic;
static INT32 tallydonetic = -1;
static INT32 endtic = -1;

intertype_t intertype = int_none;
intertype_t intermissiontypes[NUMGAMETYPES];

static huddrawlist_h luahuddrawlist_intermission;

static void Y_RescaleScreenBuffer(void);
static void Y_AwardCoopBonuses(void);
static void Y_AwardSpecialStageBonus(void);
static void Y_CalculateCompetitionWinners(void);
static void Y_CalculateTimeRaceWinners(void);
static void Y_CalculateMatchWinners(void);
static void Y_UnloadData(void);


#define ST_DrawNumFromHud(h,n)        V_DrawTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, n)
#define ST_DrawPadNumFromHud(h,n,q)   V_DrawPaddedTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, n, q)
#define ST_DrawPatchFromHud(h,p)      V_DrawScaledPatch(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, p)

static void Y_IntermissionTokenDrawer(void)
{
	INT32 y, offs, lowy, calc;
	UINT32 tokencount;
	INT16 temp;
	UINT8 em;

	offs = 0;
	lowy = BASEVIDHEIGHT - 32 - 8;
	temp = tokenicon->height / 2;

	em = 0;
	while (emeralds & (1 << em))
		if (++em == 7)
			return;

	if (tallydonetic != -1)
	{
		offs = (intertic - tallydonetic)*2;
		if (offs > 10)
			offs = 8;
	}

	V_DrawSmallScaledPatch(32, lowy-1, 0, emeraldpics[2][em]);  

	y = (lowy + offs + 1) - (temp + (token + 1)*8);

	for (tokencount = token; tokencount; tokencount--)
	{
		if (y >= -temp)
			V_DrawSmallScaledPatch(32, y, 0, tokenicon);
		y += 8;
	}

	y += (offs*(temp - 1)/8);
	calc = (lowy - y)*2;

	if (calc > 0)
		V_DrawCroppedPatch(32<<FRACBITS, y<<FRACBITS, FRACUNIT/2, FRACUNIT/2, 0, tokenicon, NULL, 0, 0, tokenicon->width<<FRACBITS, calc<<FRACBITS);
}







void Y_LoadIntermissionData(void)
{
	INT32 i;

	if (dedicated)
		return;

	switch (intertype)
	{
		case int_coop:
		{
			for (i = 0; i < 4; ++i)
			{
				if (strlen(data.coop.bonuses[i].patch))
					data.coop.bonuspatches[i] = W_CachePatchName(data.coop.bonuses[i].patch, PU_PATCH);
			}
			data.coop.ptotal = W_CachePatchName("YB_TOTAL", PU_PATCH);



			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1]->interscreen, PU_PATCH);
			else  
				bgpatch = W_CachePatchName("INTERSCR", PU_PATCH);
			break;
		}
		case int_spec:
		{
			for (i = 0; i < 2; ++i)
				data.spec.bonuspatches[i] = W_CachePatchName(data.spec.bonuses[i].patch, PU_PATCH);

			data.spec.pscore = W_CachePatchName("YB_SCORE", PU_PATCH);
			data.spec.pcontinues = W_CachePatchName("YB_CONTI", PU_PATCH);


			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1]->interscreen, PU_PATCH);
			else  
				bgtile = W_CachePatchName("SPECTILE", PU_PATCH);
			break;
		}
		case int_ctf:
		case int_teammatch:
		{
			if (!rflagico)  
			{
				rflagico = W_CachePatchName("RFLAGICO", PU_HUDGFX);
				bflagico = W_CachePatchName("BFLAGICO", PU_HUDGFX);
				rmatcico = W_CachePatchName("RMATCICO", PU_HUDGFX);
				bmatcico = W_CachePatchName("BMATCICO", PU_HUDGFX);
			}

			data.match.redflag = (intertype == int_ctf) ? rflagico : rmatcico;
			data.match.blueflag = (intertype == int_ctf) ? bflagico : bmatcico;
		}
		 
		case int_match:
		case int_race:
		case int_comp:
		{
			if (intertype == int_match || intertype == int_race)
			{

				data.match.result = W_CachePatchName("RESULT", PU_PATCH);
			}


			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			break;
		}
		case int_none:
		default:
			break;
	}
}






void Y_ConsiderScreenBuffer(void)
{
	if (gameaction != ga_completed)
		return;

	if (y_buffer == NULL)
		y_buffer = Z_Calloc(sizeof(y_buffer_t), PU_STATIC, NULL);
	else
		return;

	y_buffer->source_width = vid.width;
	y_buffer->source_height = vid.height;
	y_buffer->source_bpp = vid.bpp;
	y_buffer->source_rowbytes = vid.rowbytes;
	y_buffer->source_picture = ZZ_Alloc(y_buffer->source_width*vid.bpp * y_buffer->source_height);
	VID_BlitLinearScreen(screens[1], y_buffer->source_picture, vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);


	Y_RescaleScreenBuffer();
}






static void Y_RescaleScreenBuffer(void)
{
	INT32 sx, sy;  
	INT32 dx, dy;  
	fixed_t scalefac, yscalefac;
	fixed_t rowfrac, colfrac;
	UINT8 *dest;


	if (y_buffer == NULL)
		return;

	if (y_buffer->target_picture)
		Z_Free(y_buffer->target_picture);

	y_buffer->target_width = vid.width;
	y_buffer->target_height = vid.height;
	y_buffer->target_rowbytes = vid.rowbytes;
	y_buffer->target_bpp = vid.bpp;
	y_buffer->target_picture = ZZ_Alloc(y_buffer->target_width*vid.bpp * y_buffer->target_height);
	dest = y_buffer->target_picture;

	scalefac = FixedDiv(y_buffer->target_width*FRACUNIT, y_buffer->source_width*FRACUNIT);
	yscalefac = FixedDiv(y_buffer->target_height*FRACUNIT, y_buffer->source_height*FRACUNIT);

	rowfrac = FixedDiv(FRACUNIT, yscalefac);
	colfrac = FixedDiv(FRACUNIT, scalefac);

	for (sy = 0, dy = 0; sy < (y_buffer->source_height << FRACBITS) && dy < y_buffer->target_height; sy += rowfrac, dy++)
		for (sx = 0, dx = 0; sx < (y_buffer->source_width << FRACBITS) && dx < y_buffer->target_width; sx += colfrac, dx += y_buffer->target_bpp)
			dest[(dy * y_buffer->target_rowbytes) + dx] = y_buffer->source_picture[((sy>>FRACBITS) * y_buffer->source_width) + (sx>>FRACBITS)];
}






void Y_CleanupScreenBuffer(void)
{

	if (y_buffer == NULL)
		return;

	if (y_buffer->target_picture)
		Z_Free(y_buffer->target_picture);

	if (y_buffer->source_picture)
		Z_Free(y_buffer->source_picture);

	Z_Free(y_buffer);
	y_buffer = NULL;
}







void Y_IntermissionDrawer(void)
{

	INT32 i;

	if (intertype == int_none || rendermode == render_none)
		return;

	if (useinterpic)
		V_DrawScaledPatch(0, 0, 0, interpic);
	else if (!usetile)
	{
		if (rendermode == render_soft && usebuffer)
		{

			if (y_buffer == NULL)
				VID_BlitLinearScreen(screens[1], screens[0], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			else
			{

				if ((y_buffer->target_width != vid.width) || (y_buffer->target_height != vid.height))
					Y_RescaleScreenBuffer();


				VID_BlitLinearScreen(y_buffer->target_picture, screens[0], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			}
		}
#ifdef HWRENDER
		else if (rendermode != render_soft && usebuffer)
			HWR_DrawIntermissionBG();
#endif
		else if (bgpatch)
		{
			fixed_t hs = vid.width  * FRACUNIT / BASEVIDWIDTH;
			fixed_t vs = vid.height * FRACUNIT / BASEVIDHEIGHT;
			V_DrawStretchyFixedPatch(0, 0, hs, vs, V_NOSCALEPATCH, bgpatch, NULL);
		}
	}
	else if (bgtile)
		V_DrawPatchFill(bgtile);

	if (renderisnewtic)
	{
		LUA_HUD_ClearDrawList(luahuddrawlist_intermission);
		LUA_HUDHOOK(intermission, luahuddrawlist_intermission);
	}
	LUA_HUD_DrawList(luahuddrawlist_intermission);

	if (!LUA_HudEnabled(hud_intermissiontally))
		goto skiptallydrawer;

	if (intertype == int_coop)
	{
		INT32 bonusy;

		if (gottoken)  
			Y_IntermissionTokenDrawer();

		if (!splitscreen)   
		{

			ST_DrawPatchFromHud(HUD_SCORE, sboscore);
			ST_DrawNumFromHud(HUD_SCORENUM, data.coop.score);


			ST_DrawPatchFromHud(HUD_TIME, sbotime);
			if (cv_timetic.value == 3)
				ST_DrawNumFromHud(HUD_SECONDS, data.coop.tics);
			else
			{
				INT32 seconds, minutes, tictrn;

				seconds = G_TicsToSeconds(data.coop.tics);
				minutes = G_TicsToMinutes(data.coop.tics, true);
				tictrn  = G_TicsToCentiseconds(data.coop.tics);

				ST_DrawNumFromHud(HUD_MINUTES, minutes);  
				ST_DrawPatchFromHud(HUD_TIMECOLON, sbocolon);  
				ST_DrawPadNumFromHud(HUD_SECONDS, seconds, 2);  

				if (cv_timetic.value == 1 || cv_timetic.value == 2 || modeattacking || marathonmode)
				{
					ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod);  
					ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2);  
				}
			}
		}

		if (LUA_HudEnabled(hud_intermissiontitletext))
		{

			V_DrawLevelTitle(data.coop.passedx1, 49, 0, data.coop.passed1);
			{
				INT32 h = V_LevelNameHeight(data.coop.passed2);
				V_DrawLevelTitle(data.coop.passedx2, 49+h+2, 0, data.coop.passed2);

				if (data.coop.actnum)
					V_DrawLevelActNum(244, 42+h, 0, data.coop.actnum);
			}
		}

		bonusy = 150;

		V_DrawScaledPatch(152, bonusy, 0, data.coop.ptotal);
		V_DrawTallNum(BASEVIDWIDTH - 68, bonusy + 1, 0, data.coop.total);
		bonusy -= (3*(tallnum[0]->height)/2) + 1;


		for (i = 3; i >= 0; --i)
		{
			if (data.coop.bonuses[i].display)
			{
				V_DrawScaledPatch(152, bonusy, 0, data.coop.bonuspatches[i]);
				V_DrawTallNum(BASEVIDWIDTH - 68, bonusy + 1, 0, data.coop.bonuses[i].points);
			}
			bonusy -= (3*(tallnum[0]->height)/2) + 1;
		}
	}
	else if (intertype == int_spec)
	{
		static tic_t animatetic = 0;
		INT32 ttheight = 16;
		INT32 xoffset1 = 0;  
		INT32 xoffset2 = 0;  
		INT32 xoffset3 = 0;  
		INT32 xoffset4 = 0;  
		INT32 xoffset5 = 0;  
		INT32 xoffset6 = 0;  
		UINT8 drawsection = 0;

		if (gottoken)  
			Y_IntermissionTokenDrawer();


		if (intertic <= 2*TICRATE)
			animatetic = 0;
		else if (!animatetic && data.spec.bonuses[0].points == 0 && data.spec.bonuses[1].points == 0 && data.spec.passed3[0] != '\0')
			animatetic = intertic + TICRATE;

		if (animatetic && (tic_t)intertic >= animatetic)
		{
			const INT32 scradjust = (vid.width/vid.dup)>>3;  
			INT32 animatetimer = (intertic - animatetic);
			if (animatetimer <= 16)
			{
				xoffset1 = -(animatetimer      * scradjust);
				xoffset2 = -((animatetimer- 2) * scradjust);
				xoffset3 = -((animatetimer- 4) * scradjust);
				xoffset4 = -((animatetimer- 6) * scradjust);
				xoffset5 = -((animatetimer- 8) * scradjust);
				xoffset6 = -((animatetimer-10) * scradjust);
				if (xoffset2 > 0) xoffset2 = 0;
				if (xoffset3 > 0) xoffset3 = 0;
				if (xoffset4 > 0) xoffset4 = 0;
				if (xoffset5 > 0) xoffset5 = 0;
				if (xoffset6 > 0) xoffset6 = 0;
			}
			else if (animatetimer < 34)
			{
				drawsection = 1;
				xoffset1 = (24-animatetimer) * scradjust;
				xoffset2 = (26-animatetimer) * scradjust;
				xoffset3 = (28-animatetimer) * scradjust;
				xoffset4 = (30-animatetimer) * scradjust;
				xoffset5 = (32-animatetimer) * scradjust;
				xoffset6 = (34-animatetimer) * scradjust;
				if (xoffset1 < 0) xoffset1 = 0;
				if (xoffset2 < 0) xoffset2 = 0;
				if (xoffset3 < 0) xoffset3 = 0;
				if (xoffset4 < 0) xoffset4 = 0;
				if (xoffset5 < 0) xoffset5 = 0;
			}
			else
			{
				drawsection = 1;
				if (animatetimer == 32)
					S_StartSound(NULL, sfx_s3k68);
			}
		}

		if (drawsection == 1)
		{
			if (LUA_HudEnabled(hud_intermissiontitletext))
			{
				const char *ringtext = "\x82" "50 rings, no shield";
				const char *tut1text = "\x82" "press " "\x80" "spin";
				const char *tut2text = "\x82" "mid-" "\x80" "jump";
				ttheight = 8;
				V_DrawLevelTitle(data.spec.passedx1 + xoffset1, ttheight, 0, data.spec.passed1);
				ttheight += V_LevelNameHeight(data.spec.passed3) + 2;
				V_DrawLevelTitle(data.spec.passedx3 + xoffset2, ttheight, 0, data.spec.passed3);
				ttheight += V_LevelNameHeight(data.spec.passed4) + 2;
				V_DrawLevelTitle(data.spec.passedx4 + xoffset3, ttheight, 0, data.spec.passed4);

				ttheight = 108;
				V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset4 - (V_LevelNameWidth(ringtext)/2), ttheight, 0, ringtext);
				ttheight += V_LevelNameHeight(tut1text) + 2;
				V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset5 - (V_LevelNameWidth(tut1text)/2), ttheight, 0, tut1text);
				ttheight += V_LevelNameHeight(tut2text) + 2;
				V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset6 - (V_LevelNameWidth(tut2text)/2), ttheight, 0, tut2text);
			}
		}
		else
		{
			INT32 yoffset = 0;

			if (LUA_HudEnabled(hud_intermissiontitletext))
			{
				if (data.spec.passed1[0] != '\0')
				{
					ttheight = 24;
					V_DrawLevelTitle(data.spec.passedx1 + xoffset1, ttheight, 0, data.spec.passed1);
					ttheight += V_LevelNameHeight(data.spec.passed2) + 2;
					V_DrawLevelTitle(data.spec.passedx2 + xoffset2, ttheight, 0, data.spec.passed2);
				}
				else
				{
					ttheight = 24 + (V_LevelNameHeight(data.spec.passed2)/2) + 2;
					V_DrawLevelTitle(data.spec.passedx2 + xoffset1, ttheight, 0, data.spec.passed2);
				}
			}

			V_DrawScaledPatch(152 + xoffset3, 108, 0, data.spec.bonuspatches[0]);
			V_DrawTallNum(BASEVIDWIDTH + xoffset3 - 68, 109, 0, data.spec.bonuses[0].points);
			if (data.spec.bonuses[1].display)
			{
				V_DrawScaledPatch(152 + xoffset4, 124, 0, data.spec.bonuspatches[1]);
				V_DrawTallNum(BASEVIDWIDTH + xoffset4 - 68, 125, 0, data.spec.bonuses[1].points);
				yoffset = 16;

				xoffset4 = xoffset5;
				xoffset5 = xoffset6;
			}
			V_DrawScaledPatch(152 + xoffset4, 124+yoffset, 0, data.spec.pscore);
			V_DrawTallNum(BASEVIDWIDTH + xoffset4 - 68, 125+yoffset, 0, data.spec.score);


			if (continuesInSession  )  
			{
				UINT8 continues = data.spec.continues & 0x7F;

				V_DrawScaledPatch(152 + xoffset5, 150+yoffset, 0, data.spec.pcontinues);
				if (continues > 5)
				{
					INT32 leftx = (continues >= 10) ? 216 : 224;
					V_DrawContinueIcon(leftx + xoffset5, 162+yoffset, 0, *data.spec.playerchar, *data.spec.playercolor);
					V_DrawScaledPatch(leftx + xoffset5 + 12, 160+yoffset, 0, stlivex);
					if (!((data.spec.continues & 0x80) && !(endtic < 0 || intertic%20 < 10)))
						V_DrawRightAlignedString(252 + xoffset5, 158+yoffset, 0,
							va("%d",(((data.spec.continues & 0x80) && (endtic < 0)) ? continues-1 : continues)));
				}
				else
				{
					for (i = 0; i < continues; ++i)
					{
						if ((data.spec.continues & 0x80) && i == continues-1 && (endtic < 0 || intertic%20 < 10))
							break;
						V_DrawContinueIcon(246 + xoffset5 - (i*20), 162+yoffset, 0, *data.spec.playerchar, *data.spec.playercolor);
					}
				}
			}
		}


		if (LUA_HudEnabled(hud_intermissionemeralds))
		{
			boolean drawthistic = !(ALL7EMERALDS(emeralds) && (intertic & 1));
			INT32 emeraldx = 152 - 3*28;
			INT32 em = P_GetNextEmerald();

			if (em == 7)
			{
				if (!stagefailed)
				{
					fixed_t adjust = 2*(FINESINE(FixedAngle((intertic + 1)<<(FRACBITS-4)) & FINEMASK));
					V_DrawFixedPatch(152<<FRACBITS, (74<<FRACBITS) - adjust, FRACUNIT, 0, emeraldpics[0][em], NULL);
				}
			}
			else if (em < 7)
			{
				if (drawthistic)
					for (i = 0; i < 7; ++i)
					{
						if ((i != em) && (emeralds & (1 << i)))
							V_DrawScaledPatch(emeraldx, 74, 0, emeraldpics[0][i]);
						emeraldx += 28;
					}

				emeraldx = 152 + (em-3)*28;

				if (intertic > 1)
				{
					if (stagefailed && data.spec.emeraldy < (vid.height/vid.dup)+16)
					{
						emeraldx += intertic - 6;
					}

					if (drawthistic)
						V_DrawScaledPatch(emeraldx, data.spec.emeraldy, 0, emeraldpics[0][em]);
				}
			}
		}
	}
	else if (intertype == int_match || intertype == int_race)
	{
		INT32 j = 0;
		INT32 x = 4;
		INT32 y = 48;
		char name[MAXPLAYERNAME+1];
		char strtime[10];


		V_DrawScaledPatch(112, 2, 0, data.match.result);


		V_DrawCenteredString(BASEVIDWIDTH/2, 20, 0, data.match.levelstring);
		V_DrawFill(4, 42, 312, 1, 0);

		if (data.match.numplayers > 9)
		{
			V_DrawFill(160, 32, 1, 152, 0);

			if (intertype == int_race)
				V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "TIME");
			else
				V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "SCORE");

			V_DrawCenteredString(x+(BASEVIDWIDTH/2)+6, 32, V_YELLOWMAP, "#");
			V_DrawString(x+(BASEVIDWIDTH/2)+36, 32, V_YELLOWMAP, "NAME");
		}

		V_DrawCenteredString(x+6, 32, V_YELLOWMAP, "#");
		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");

		if (intertype == int_race)
			V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "TIME");
		else
			V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "SCORE");

		for (i = 0; i < data.match.numplayers; i++)
		{
			if (data.match.spectator[i])
				continue;  

			V_DrawCenteredString(x+6, y, 0, va("%d", j+1));
			j++;  

			if (playeringame[data.match.num[i]])
			{

				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);

				if (data.match.color[i] == 0)
					V_DrawSmallScaledPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]]);
				else
				{
					UINT8 *colormap = R_GetTranslationColormap(*data.match.character[i], *data.match.color[i], GTC_CACHE);
					V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]], colormap);
				}

				if (data.match.numplayers > 9)
				{
					if (intertype == int_race)
						strlcpy(name, data.match.name[i], 8);
					else
						strlcpy(name, data.match.name[i], 9);
				}
				else
					STRBUFCPY(name, data.match.name[i]);

				V_DrawString(x+36, y, V_ALLOWLOWERCASE, name);

				if (data.match.numplayers > 9)
				{
					if (intertype == int_match)
						V_DrawRightAlignedString(x+152, y, 0, va("%i", data.match.scores[i]));
					else if (intertype == int_race)
					{
						if (players[data.match.num[i]].pflags & PF_GAMETYPEOVER)
							snprintf(strtime, sizeof strtime, "DNF");
						else
							snprintf(strtime, sizeof strtime,
								"%i:%02i.%02i",
								G_TicsToMinutes(data.match.scores[i], true),
								G_TicsToSeconds(data.match.scores[i]), G_TicsToCentiseconds(data.match.scores[i]));

						strtime[sizeof strtime - 1] = '\0';
						V_DrawRightAlignedString(x+152, y, 0, strtime);
					}
				}
				else
				{
					if (intertype == int_match)
						V_DrawRightAlignedString(x+152+BASEVIDWIDTH/2, y, 0, va("%u", data.match.scores[i]));
					else if (intertype == int_race)
					{
						if (players[data.match.num[i]].pflags & PF_GAMETYPEOVER)
							snprintf(strtime, sizeof strtime, "DNF");
						else
							snprintf(strtime, sizeof strtime, "%i:%02i.%02i", G_TicsToMinutes(data.match.scores[i], true),
									G_TicsToSeconds(data.match.scores[i]), G_TicsToCentiseconds(data.match.scores[i]));

						strtime[sizeof strtime - 1] = '\0';

						V_DrawRightAlignedString(x+152+BASEVIDWIDTH/2, y, 0, strtime);
					}
				}
			}

			y += 16;

			if (y > 176)
			{
				y = 48;
				x += BASEVIDWIDTH/2;
			}
		}
	}
	else if (intertype == int_ctf || intertype == int_teammatch)
	{
		INT32 x = 4, y = 0;
		INT32 redplayers = 0, blueplayers = 0;
		char name[MAXPLAYERNAME+1];


		V_DrawSmallScaledPatch(128 - (data.match.blueflag->width / 4), 2, 0, data.match.blueflag);
		V_DrawCenteredString(128, 16, 0, va("%u", bluescore));

		V_DrawSmallScaledPatch(192 - (data.match.redflag->width / 4), 2, 0, data.match.redflag);
		V_DrawCenteredString(192, 16, 0, va("%u", redscore));


		V_DrawCenteredString(BASEVIDWIDTH/2, 24, 0, data.match.levelstring);
		V_DrawFill(4, 42, 312, 1, 0);


		V_DrawFill(160, 32, 1, 152, 0);


		V_DrawCenteredString(x+6, 32, V_YELLOWMAP, "#");
		V_DrawCenteredString(x+(BASEVIDWIDTH/2)+6, 32, V_YELLOWMAP, "#");

		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");
		V_DrawString(x+(BASEVIDWIDTH/2)+36, 32, V_YELLOWMAP, "NAME");

		V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "SCORE");
		V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "SCORE");

		for (i = 0; i < data.match.numplayers; i++)
		{
			if (playeringame[data.match.num[i]] && !(data.match.spectator[i]))
			{
				UINT8 *colormap = R_GetTranslationColormap(*data.match.character[i], *data.match.color[i], GTC_CACHE);

				if (data.match.ctfteam[i] == 1)  
				{
					if (redplayers++ > 9)
						continue;
					x = 4 + (BASEVIDWIDTH/2);
					y = (redplayers * 16) + 32;
					V_DrawCenteredString(x+6, y, 0, va("%d", redplayers));
				}
				else if (data.match.ctfteam[i] == 2)  
				{
					if (blueplayers++ > 9)
						continue;
					x = 4;
					y = (blueplayers * 16) + 32;
					V_DrawCenteredString(x+6, y, 0, va("%d", blueplayers));
				}
				else
					continue;


				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);


				V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]], colormap);

				strlcpy(name, data.match.name[i], 9);

				V_DrawString(x+36, y, V_ALLOWLOWERCASE, name);

				V_DrawRightAlignedString(x+152, y, 0, va("%u", data.match.scores[i]));
			}
		}
	}
	else if (intertype == int_comp)
	{
		INT32 x = 4;
		INT32 y = 48;
		UINT32 ptime, pring, pmaxring, pmonitor, pscore;
		char sstrtime[10];


		V_DrawCenteredString(BASEVIDWIDTH/2, 8, 0, data.competition.levelstring);
		V_DrawFill(4, 42, 312, 1, 0);

		V_DrawCenteredString(x+6, 32, V_YELLOWMAP, "#");
		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");

		V_DrawRightAlignedString(x+160, 32, V_YELLOWMAP, "TIME");


		V_DrawThinString(x+168, 32, V_YELLOWMAP, "RING");


		V_DrawThinString(x+191, 24, V_YELLOWMAP, "TOTAL");
		V_DrawThinString(x+196, 32, V_YELLOWMAP, "RING");


		V_DrawThinString(x+223, 24, V_YELLOWMAP, "ITEM");
		V_DrawThinString(x+229, 32, V_YELLOWMAP, "BOX");


		V_DrawRightAlignedString(x+288, 32, V_YELLOWMAP, "SCORE");


		V_DrawRightAlignedString(x+312, 32, V_YELLOWMAP, "PT");

		for (i = 0; i < data.competition.numplayers; i++)
		{
			ptime = (data.competition.times[i] & ~0x80000000);
			pring = (data.competition.rings[i] & ~0x80000000);
			pmaxring = (data.competition.maxrings[i] & ~0x80000000);
			pmonitor = (data.competition.monitors[i] & ~0x80000000);
			pscore = (data.competition.scores[i] & ~0x80000000);

			V_DrawCenteredString(x+6, y, 0, va("%d", i+1));

			if (playeringame[data.competition.num[i]])
			{

				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);

				if (data.competition.color[i] == 0)
					V_DrawSmallScaledPatch(x+16, y-4, 0,faceprefix[*data.competition.character[i]]);
				else
				{
					UINT8 *colormap = R_GetTranslationColormap(*data.competition.character[i], *data.competition.color[i], GTC_CACHE);
					V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.competition.character[i]], colormap);
				}


				V_DrawString(x+36, y, V_ALLOWLOWERCASE, data.competition.name[i]);

				if (players[data.competition.num[i]].pflags & PF_GAMETYPEOVER)
					snprintf(sstrtime, sizeof sstrtime, "Time Over");
				else if (players[data.competition.num[i]].lives <= 0)
					snprintf(sstrtime, sizeof sstrtime, "Game Over");
				else
					snprintf(sstrtime, sizeof sstrtime, "%i:%02i.%02i", G_TicsToMinutes(ptime, true),
							G_TicsToSeconds(ptime), G_TicsToCentiseconds(ptime));

				sstrtime[sizeof sstrtime - 1] = '\0';

				V_DrawRightAlignedThinString(x+160, y, ((data.competition.times[i] & 0x80000000) ? V_YELLOWMAP : 0), sstrtime);

				V_DrawRightAlignedThinString(x+188, y, V_MONOSPACE|((data.competition.rings[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pring));

				V_DrawRightAlignedThinString(x+216, y, V_MONOSPACE|((data.competition.maxrings[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pmaxring));

				V_DrawRightAlignedThinString(x+244, y, V_MONOSPACE|((data.competition.monitors[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pmonitor));

				V_DrawRightAlignedThinString(x+288, y, V_MONOSPACE|((data.competition.scores[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pscore));

				V_DrawRightAlignedString(x+312, y, V_YELLOWMAP, va("%d", data.competition.points[i]));
			}

			y += 16;

			if (y > 176)
				break;
		}
	}

skiptallydrawer:
	if (!LUA_HudEnabled(hud_intermissionmessages))
		return;

	if (timer)
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, V_YELLOWMAP,
			va("start in %d seconds", timer/TICRATE));


	if (cv_scrambleonchange.value && cv_teamscramble.value && (intertic/TICRATE % 2 == 0))
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, V_YELLOWMAP, M_GetText("Teams will be scrambled next round!"));
}






void Y_Ticker(void)
{
	if (intertype == int_none)
		return;


	if (paused || P_AutoPause())
		return;

	LUA_HookBool(stagefailed, HOOK(IntermissionThinker));

	intertic++;



	if (G_GametypeHasTeams() && cv_teamscramble.value && !cv_scrambleonchange.value && server)
	{


		if ((intertic % (TICRATE/7)) == 0)
			P_DoTeamscrambling();
	}


	if (timer)
	{
		if (!--timer)
		{
			Y_EndIntermission();
			G_AfterIntermission();
			return;
		}
	}

	else if (intertic == endtic)
	{
		Y_EndIntermission();
		G_AfterIntermission();
		return;
	}

	if (endtic != -1)
		return;  

	if (intertype == int_coop)  
	{
		INT32 i;
		UINT32 oldscore = data.coop.score;
		boolean skip = (marathonmode) ? true : false;
		boolean anybonuses = false;

		if (!intertic)  
		{
			if (mapheaderinfo[gamemap-1]->musinterfadeout
#ifdef _WIN32

				&& S_MusicType() != MU_MID
#endif
			)
				S_FadeOutStopMusic(mapheaderinfo[gamemap-1]->musinterfadeout);
			else if (mapheaderinfo[gamemap-1]->musintername[0] && S_MusicExists(mapheaderinfo[gamemap-1]->musintername, !midi_disabled, !digital_disabled))
				S_ChangeMusicInternal(mapheaderinfo[gamemap-1]->musintername, false);  
			else
				S_ChangeMusicInternal("_clear", false);  
			tallydonetic = -1;
		}

		if (intertic < TICRATE)  
			return;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && (players[i].cmd.buttons & BT_SPIN))
				skip = true;


		for (i = 0; i < 4; ++i)
		{
			if (!data.coop.bonuses[i].points)
				continue;

			data.coop.bonuses[i].points -= 222;
			data.coop.total += 222;
			data.coop.score += 222;
			if (data.coop.bonuses[i].points < 0 || skip == true)  
			{
				data.coop.score += data.coop.bonuses[i].points;
				data.coop.total += data.coop.bonuses[i].points;
				data.coop.bonuses[i].points = 0;
			}
			if (data.coop.score > MAXSCORE)
				data.coop.score = MAXSCORE;
			if (data.coop.bonuses[i].points > 0)
				anybonuses = true;
		}

		if (!anybonuses)
		{
			tallydonetic = intertic;
			endtic = intertic + 3*TICRATE;  
			S_StartSound(NULL, (gottoken ? sfx_token : sfx_chchng));  


			if (!demoplayback)
			{
				M_SilentUpdateUnlockablesAndEmblems(serverGamedata);

				if (M_UpdateUnlockablesAndExtraEmblems(clientGamedata))
					S_StartSound(NULL, sfx_s3k68);

				G_SaveGameData(clientGamedata);
			}
		}
		else if (!(intertic & 1))
			S_StartSound(NULL, sfx_ptally);  

		if (data.coop.gotlife > 0 && (skip == true || data.coop.score % 50000 < oldscore % 50000))  
		{

			P_PlayLivesJingle(NULL);
			--data.coop.gotlife;
		}
	}
	else if (intertype == int_spec)  
	{
		INT32 i;
		UINT32 oldscore = data.spec.score;
		boolean skip = (marathonmode) ? true : false, super = false, anybonuses = false;

		if (!intertic)  
		{
			if (mapheaderinfo[gamemap-1]->musinterfadeout
#ifdef _WIN32

				&& S_MusicType() != MU_MID
#endif
			)
				S_FadeOutStopMusic(mapheaderinfo[gamemap-1]->musinterfadeout);
			else if (mapheaderinfo[gamemap-1]->musintername[0] && S_MusicExists(mapheaderinfo[gamemap-1]->musintername, !midi_disabled, !digital_disabled))
				S_ChangeMusicInternal(mapheaderinfo[gamemap-1]->musintername, false);  
			else
				S_ChangeMusicInternal(stagefailed ? "CHFAIL" : "CHPASS", false);  
			tallydonetic = -1;
		}


		if (dedicated || !LUA_HudEnabled(hud_intermissionemeralds))
		{


		}
		else if (intertic <= 1)
		{
			data.spec.emeraldbounces = 0;
			data.spec.emeraldmomy = 20;
			data.spec.emeraldy = -40;
		}
		else if (P_GetNextEmerald() < 7)
		{
			if (!stagefailed)
			{
				if (data.spec.emeraldbounces < 3)
				{
					data.spec.emeraldy += (++data.spec.emeraldmomy);
					if (data.spec.emeraldy > 74)
					{
						S_StartSound(NULL, sfx_tink);  
						data.spec.emeraldbounces++;
						data.spec.emeraldmomy = -(data.spec.emeraldmomy/2);
						data.spec.emeraldy = 74;
					}
				}
			}
			else
			{
				if (data.spec.emeraldy < (vid.height/vid.dup)+16)
				{
					data.spec.emeraldy += (++data.spec.emeraldmomy);
				}
				if (data.spec.emeraldbounces < 1 && data.spec.emeraldy > 74)
				{
					S_StartSound(NULL, sfx_shldls);  
					data.spec.emeraldbounces++;
					data.spec.emeraldmomy = -(data.spec.emeraldmomy/2);
					data.spec.emeraldy = 74;
				}
			}
		}

		if (intertic < 2*TICRATE)  
			return;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				if (players[i].cmd.buttons & BT_SPIN)
					skip = true;
				if (players[i].charflags & SF_SUPER)
					super = true;
			}

		if (tallydonetic != -1 && ((data.spec.continues & 0x80) || (super && ALL7EMERALDS(emeralds))))
		{
			if ((intertic - tallydonetic) > (3*TICRATE)/2)
			{
				endtic = intertic + 4*TICRATE;  
				if (data.spec.continues & 0x80)
					S_StartSound(NULL, sfx_s3kac);  

			}
			return;
		}


		for (i = 0; i < 2; ++i)
		{
			if (!data.spec.bonuses[i].points)
				continue;

			data.spec.bonuses[i].points -= 222;
			data.spec.score += 222;
			if (data.spec.bonuses[i].points < 0 || skip == true)  
			{
				data.spec.score += data.spec.bonuses[i].points;
				data.spec.bonuses[i].points = 0;
			}
			if (data.spec.score > MAXSCORE)
				data.spec.score = MAXSCORE;
			if (data.spec.bonuses[i].points > 0)
				anybonuses = true;
		}

		if (!anybonuses)
		{
			tallydonetic = intertic;
			if (!((data.spec.continues & 0x80) || (super && ALL7EMERALDS(emeralds))))  
				endtic = intertic + 4*TICRATE;  

			S_StartSound(NULL, (gottoken ? sfx_token : sfx_chchng));  


			if (!demoplayback)
			{
				M_SilentUpdateUnlockablesAndEmblems(serverGamedata);

				if (M_UpdateUnlockablesAndExtraEmblems(clientGamedata))
					S_StartSound(NULL, sfx_s3k68);

				G_SaveGameData(clientGamedata);
			}
		}
		else if (!(intertic & 1))
			S_StartSound(NULL, sfx_ptally);  

		if (data.spec.gotlife > 0 && (skip == true || data.spec.score % 50000 < oldscore % 50000))  
		{

			P_PlayLivesJingle(NULL);
			--data.spec.gotlife;
		}
	}
	else if (intertype == int_match || intertype == int_ctf || intertype == int_teammatch)  
	{
		if (!intertic)  
			S_ChangeMusicInternal("_inter", true);  


		if (data.match.numplayers != D_NumPlayers())
			Y_CalculateMatchWinners();
	}
	else if (intertype == int_race || intertype == int_comp)  
	{
		if (!intertic)  
			S_ChangeMusicInternal("_inter", true);  


	}
}






void Y_DetermineIntermissionType(void)
{

	intertype = int_none;

	if (intermissiontypes[gametype] != int_none)
		intertype = intermissiontypes[gametype];
	else if (gametype == GT_COOP)
		intertype = (G_IsSpecialStage(gamemap)) ? int_spec : int_coop;
	else if (gametype == GT_TEAMMATCH)
		intertype = int_teammatch;
	else if (gametype == GT_MATCH
	 || gametype == GT_TAG
	 || gametype == GT_HIDEANDSEEK)
		intertype = int_match;
	else if (gametype == GT_RACE)
		intertype = int_race;
	else if (gametype == GT_COMPETITION)
		intertype = int_comp;
	else if (gametype == GT_CTF)
		intertype = int_ctf;
}







void Y_StartIntermission(void)
{
	intertic = -1;

#ifdef PARANOIA
	if (endtic != -1)
		I_Error("endtic is dirty");
#endif

	if (!multiplayer)
	{
		timer = 0;
		intertype = (G_IsSpecialStage(gamemap)) ? int_spec : int_coop;
	}
	else
	{
		if (cv_inttime.value == 0 && ((intertype == int_coop) || (intertype == int_spec)))
			timer = 0;
		else
		{
			timer = cv_inttime.value*TICRATE;

			if (!timer)
				timer = 1;
		}
	}






	if(!mapheaderinfo[prevmap])
		P_AllocMapHeader(prevmap);

	switch (intertype)
	{
		case int_coop:  
		{

			Y_AwardCoopBonuses();


			data.coop.tics = players[consoleplayer].realtime;


			data.coop.actnum = mapheaderinfo[gamemap-1]->actnum;


			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
			{
				useinterpic = true;
				usebuffer = false;
			}
			else
			{
				useinterpic = false;
#ifdef HWRENDER
				if (rendermode == render_opengl)
					usebuffer = true;  
#endif
			}
			usetile = false;


			if (stagefailed)
			{
				strcpy(data.coop.passed1, mapheaderinfo[gamemap-1]->lvlttl);

				if (mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE)
				{
					data.spec.passed2[0] = '\0';
				}
				else
				{
					strcpy(data.coop.passed2, "Zone");
				}
			}
			else
			{

				if (strlen(skins[players[consoleplayer].skin]->realname) > 13)
				{
					strcpy(data.coop.passed1, "you got");
					strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "through act" : "through the act");
				}

				else if (strlen(skins[players[consoleplayer].skin]->realname) > 8)
				{
					strcpy(data.coop.passed1, skins[players[consoleplayer].skin]->realname);
					strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "passed act" : "passed the act");
				}

				else
				{
					snprintf(data.coop.passed1, sizeof data.coop.passed1, "%s got",
						skins[players[consoleplayer].skin]->realname);
					strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "through act" : "through the act");
				}
			}


			if (mapheaderinfo[gamemap-1]->actnum)
			{
				data.coop.passedx1 = 62 + (176 - V_LevelNameWidth(data.coop.passed1))/2;
				data.coop.passedx2 = 62 + (176 - V_LevelNameWidth(data.coop.passed2))/2;
			}
			else
			{
				data.coop.passedx1 = (BASEVIDWIDTH - V_LevelNameWidth(data.coop.passed1))/2;
				data.coop.passedx2 = (BASEVIDWIDTH - V_LevelNameWidth(data.coop.passed2))/2;
			}




			if ((stagefailed) && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
			{

				data.coop.passedx2 = (data.coop.passedx1 + V_LevelNameWidth(data.coop.passed1)) - V_LevelNameWidth(data.coop.passed2);
			}

			break;
		}

		case int_spec:  
		{

			Y_AwardSpecialStageBonus();


			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
				useinterpic = true;
			else
				useinterpic = false;


			usetile = !useinterpic;


                                                
    
                                                                
                         
                                                    
                                                     
    
       
    
                                                                
                         
                              
       


			data.spec.passed3[0] = '\0';
			data.spec.passed4[0] = '\0';


			if (stagefailed)
			{
				strcpy(data.spec.passed2, "Special Stage");
				data.spec.passed1[0] = '\0';
			}
			else if (ALL7EMERALDS(emeralds))
			{
				snprintf(data.spec.passed1,
					sizeof data.spec.passed1, "%s",
					skins[players[consoleplayer].skin]->realname);
				data.spec.passed1[sizeof data.spec.passed1 - 1] = '\0';
				strcpy(data.spec.passed2, "got them all!");

				if (players[consoleplayer].charflags & SF_SUPER)
				{
					strcpy(data.spec.passed3, "can now become");
					if (strlen(skins[players[consoleplayer].skin]->supername) > 20)  
						strcpy(data.spec.passed4, "their super form");
					else
						strcpy(data.spec.passed4, skins[players[consoleplayer].skin]->supername);
				}
			}
			else
			{
				if (strlen(skins[players[consoleplayer].skin]->realname) <= SKINNAMESIZE-5)
				{
					snprintf(data.spec.passed1,
						sizeof data.spec.passed1, "%s got",
						skins[players[consoleplayer].skin]->realname);
					data.spec.passed1[sizeof data.spec.passed1 - 1] = '\0';
				}
				else
					strcpy(data.spec.passed1, "You got");
				strcpy(data.spec.passed2, "a Chaos Emerald");
				if (P_GetNextEmerald() > 6)
				{
					data.spec.passed2[15] = '?';
					data.spec.passed2[16] = '\0';
				}
			}
			data.spec.passedx1 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed1))/2;
			data.spec.passedx2 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed2))/2;
			data.spec.passedx3 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed3))/2;
			data.spec.passedx4 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed4))/2;
			break;
		}

		case int_match:
		{

			Y_CalculateMatchWinners();


			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

			usetile = true;
			useinterpic = false;
			break;
		}

		case int_race:  
		{

			Y_CalculateTimeRaceWinners();


			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

			usetile = true;
			useinterpic = false;
			break;
		}

		case int_teammatch:
		case int_ctf:
		{

			Y_CalculateMatchWinners();


			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

			usetile = true;
			useinterpic = false;
			break;
		}

		case int_comp:  
		{

			Y_CalculateCompetitionWinners();


			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.competition.levelstring,
					sizeof data.competition.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.competition.levelstring,
					sizeof data.competition.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.competition.levelstring[sizeof data.competition.levelstring - 1] = '\0';

			usetile = true;
			useinterpic = false;
			break;
		}

		case int_none:
		default:
			break;
	}

	LUA_HUD_DestroyDrawList(luahuddrawlist_intermission);
	luahuddrawlist_intermission = LUA_HUD_CreateDrawList();
}




static void Y_CalculateMatchWinners(void)
{
	INT32 i, j;
	boolean completed[MAXPLAYERS];


	memset(data.match.ctfteam, 0, sizeof (data.match.ctfteam));
	memset(data.match.scores, 0, sizeof (data.match.scores));
	memset(data.match.color, 0, sizeof (data.match.color));
	memset(data.match.character, 0, sizeof (data.match.character));
	memset(data.match.spectator, 0, sizeof (data.match.spectator));
	memset(completed, 0, sizeof (completed));
	data.match.numplayers = 0;
	i = j = 0;

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (!playeringame[j])
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (players[i].score >= data.match.scores[data.match.numplayers] && completed[i] == false)
			{
				data.match.ctfteam[data.match.numplayers] = players[i].ctfteam;
				data.match.scores[data.match.numplayers] = players[i].score;
				data.match.color[data.match.numplayers] = &players[i].skincolor;
				if (data.match.ctfteam[data.match.numplayers] == 1)  
					data.match.color[data.match.numplayers] = &skincolor_redteam;

				if (data.match.ctfteam[data.match.numplayers] == 2)  
					data.match.color[data.match.numplayers] = &skincolor_blueteam;

				data.match.character[data.match.numplayers] = &players[i].skin;
				data.match.name[data.match.numplayers] = player_names[i];
				data.match.spectator[data.match.numplayers] = players[i].spectator;
				data.match.num[data.match.numplayers] = i;
			}
		}
		completed[data.match.num[data.match.numplayers]] = true;
		data.match.numplayers++;
	}
}




static void Y_CalculateTimeRaceWinners(void)
{
	INT32 i, j;
	boolean completed[MAXPLAYERS];



	for (i = 0; i < MAXPLAYERS; i++)
		data.match.scores[i] = INT32_MAX;

	memset(data.match.color, 0, sizeof (data.match.color));
	memset(data.match.character, 0, sizeof (data.match.character));
	memset(data.match.spectator, 0, sizeof (data.match.spectator));
	memset(completed, 0, sizeof (completed));
	data.match.numplayers = 0;
	i = j = 0;

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (!playeringame[j])
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (players[i].realtime <= data.match.scores[data.match.numplayers] && completed[i] == false)
			{
				data.match.scores[data.match.numplayers] = players[i].realtime;
				data.match.color[data.match.numplayers] = &players[i].skincolor;
				data.match.character[data.match.numplayers] = &players[i].skin;
				data.match.name[data.match.numplayers] = player_names[i];
				data.match.num[data.match.numplayers] = i;
			}
		}
		completed[data.match.num[data.match.numplayers]] = true;
		data.match.numplayers++;
	}
}




static void Y_CalculateCompetitionWinners(void)
{
	INT32 i, j;
	boolean bestat[5];
	boolean completed[MAXPLAYERS];
	INT32 winner;  

	UINT32 points[MAXPLAYERS];
	UINT32 times[MAXPLAYERS];
	UINT32 rings[MAXPLAYERS];
	UINT32 maxrings[MAXPLAYERS];
	UINT32 monitors[MAXPLAYERS];
	UINT32 scores[MAXPLAYERS];
	char tempname[9];

	memset(data.competition.points, 0, sizeof (data.competition.points));
	memset(points, 0, sizeof (points));
	memset(completed, 0, sizeof (completed));


	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		for (j = 0; j < 5; j++)
			bestat[j] = true;

		if ((players[i].pflags & PF_GAMETYPEOVER) || players[i].lives <= 0)
			players[i].rings = 0;

		times[i]    = players[i].realtime;
		rings[i]    = (UINT32)max(players[i].rings, 0);
		maxrings[i] = (UINT32)players[i].totalring;
		monitors[i] = (UINT32)players[i].numboxes;
		scores[i]   = (UINT32)min(players[i].score, MAXSCORE);

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j] || j == i)
				continue;

			if (players[i].realtime <= players[j].realtime)
				points[i]++;
			else
				bestat[0] = false;

			if (max(players[i].rings, 0) >= max(players[j].rings, 0))
				points[i]++;
			else
				bestat[1] = false;

			if (players[i].totalring >= players[j].totalring)
				points[i]++;
			else
				bestat[2] = false;

			if (players[i].numboxes >= players[j].numboxes)
				points[i]++;
			else
				bestat[3] = false;

			if (players[i].score >= players[j].score)
				points[i]++;
			else
				bestat[4] = false;
		}


		if (bestat[0])
			times[i] |= 0x80000000;
		if (bestat[1])
			rings[i] |= 0x80000000;
		if (bestat[2])
			maxrings[i] |= 0x80000000;
		if (bestat[3])
			monitors[i] |= 0x80000000;
		if (bestat[4])
			scores[i] |= 0x80000000;
	}


	data.competition.numplayers = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		winner = 0;

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j])
				continue;

			if (points[j] >= data.competition.points[data.competition.numplayers] && completed[j] == false)
			{
				data.competition.points[data.competition.numplayers] = points[j];
				data.competition.num[data.competition.numplayers] = winner = j;
			}
		}

		data.competition.times[data.competition.numplayers] = times[winner];
		data.competition.rings[data.competition.numplayers] = rings[winner];
		data.competition.maxrings[data.competition.numplayers] = maxrings[winner];
		data.competition.monitors[data.competition.numplayers] = monitors[winner];
		data.competition.scores[data.competition.numplayers] = scores[winner];

		strncpy(tempname, player_names[winner], 8);
		tempname[8] = '\0';
		strncpy(data.competition.name[data.competition.numplayers], tempname, 9);

		data.competition.color[data.competition.numplayers] = &players[winner].skincolor;
		data.competition.character[data.competition.numplayers] = &players[winner].skin;

		completed[winner] = true;
		data.competition.numplayers++;
	}
}









static void Y_SetNullBonus(player_t *player, y_bonus_t *bstruct)
{
	(void)player;
	memset(bstruct, 0, sizeof(y_bonus_t));
}




static void Y_SetTimeBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 secs, bonus;

	strncpy(bstruct->patch, "YB_TIME", sizeof(bstruct->patch));
	bstruct->display = true;

	if (stagefailed == true)
	{

		bonus = 0;
	}
	else
	{

		secs = player->realtime / TICRATE;
		if      (secs <  30)   bonus = 50000;
		else if (secs <  60)   bonus = 10000;
		else if (secs <  90)   bonus = 5000;
		else if (secs < 120)   bonus = 4000;
		else if (secs < 180)   bonus = 3000;
		else if (secs < 240)   bonus = 2000;
		else if (secs < 300)   bonus = 1000;
		else if (secs < 360)   bonus = 500;
		else if (secs < 420)   bonus = 400;
		else if (secs < 480)   bonus = 300;
		else if (secs < 540)   bonus = 200;
		else if (secs < 600)   bonus = 100;
		else    bonus = 0;
	}

	bstruct->points = bonus;
}




static void Y_SetRingBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_RING", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, (player->rings) * 100);
}




static void Y_SetNightsBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_NIGHT", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = player->totalmarescore;
}




static void Y_SetLapBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_LAP", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, player->totalmarebonuslap * 1000);
}




static void Y_SetLinkBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_LINK", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, (player->maxlink - 1) * 100);
}




static void Y_SetGuardBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 bonus;
	strncpy(bstruct->patch, "YB_GUARD", sizeof(bstruct->patch));
	bstruct->display = true;

	if (stagefailed == true)
	{

		bonus = 0;
	}
	else
	{
		if      (player->timeshit == 0) bonus = 10000;
		else if (player->timeshit == 1) bonus = 5000;
		else if (player->timeshit == 2) bonus = 1000;
		else if (player->timeshit == 3) bonus = 500;
		else if (player->timeshit == 4) bonus = 100;
		else                            bonus = 0;
	}

	bstruct->points = bonus;
}




static void Y_SetPerfectBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 i;

	(void)player;
	memset(bstruct, 0, sizeof(y_bonus_t));
	strncpy(bstruct->patch, "YB_PERFE", sizeof(bstruct->patch));

	if (intertype != int_coop || data.coop.gotperfbonus == -1)
	{
		INT32 sharedringtotal = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i]) continue;
			sharedringtotal += players[i].rings;
		}
		if (!sharedringtotal || nummaprings == -1 || sharedringtotal < nummaprings)
			bstruct->display = false;
		else
		{
			bstruct->display = true;
			bstruct->points = 50000;
		}
	}
	if (intertype != int_coop)
		return;

	data.coop.gotperfbonus = (bstruct->display ? 1 : 0);
}

static void Y_SetSpecialRingBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 i, sharedringtotal = 0;

	(void)player;
	strncpy(bstruct->patch, "YB_RING", sizeof(bstruct->patch));
	bstruct->display = true;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i]) continue;
		sharedringtotal += players[i].rings;
	}
	bstruct->points = max(0, (sharedringtotal) * 100);
}


typedef void (*bonus_f)(player_t *, y_bonus_t *);
bonus_f bonuses_list[6][4] = {
	{
		Y_SetNullBonus,
		Y_SetNullBonus,
		Y_SetNullBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetTimeBonus,
		Y_SetRingBonus,
		Y_SetPerfectBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetGuardBonus,
		Y_SetRingBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetGuardBonus,
		Y_SetRingBonus,
		Y_SetPerfectBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetNightsBonus,
		Y_SetLapBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetLinkBonus,
		Y_SetLapBonus,
		Y_SetNullBonus,
	},
};








static void Y_AwardCoopBonuses(void)
{
	INT32 i, j, bonusnum, oldscore, ptlives;
	y_bonus_t localbonuses[4];


	data.coop.total = (players[consoleplayer].pflags & PF_FINISHED) ? players[consoleplayer].recordscore : 0;
	data.coop.score = players[consoleplayer].score;
	data.coop.gotperfbonus = -1;
	memset(data.coop.bonuses, 0, sizeof(data.coop.bonuses));
	memset(data.coop.bonuspatches, 0, sizeof(data.coop.bonuspatches));

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i] || players[i].lives < 1 || players[i].bot == BOT_2PAI || players[i].bot == BOT_2PHUMAN)  
			bonusnum = 0;  
		else
			bonusnum = mapheaderinfo[prevmap]->bonustype + 1;  

		oldscore = players[i].score;

		for (j = 0; j < 4; ++j)  
		{

			if (players[i].pflags & PF_FINISHED)
				(bonuses_list[bonusnum][j])(&players[i], &localbonuses[j]);
			else
				Y_SetNullBonus(&players[i], &localbonuses[j]);

			players[i].score += localbonuses[j].points;
			if (players[i].score > MAXSCORE)
				players[i].score = MAXSCORE;
			players[i].recordscore += localbonuses[j].points;
			if (players[i].recordscore > MAXSCORE)
				players[i].recordscore = MAXSCORE;
		}

		ptlives = min(
			(INT32)((!ultimatemode && !modeattacking && players[i].lives != INFLIVES) ? max((INT32)((players[i].score/50000) - (oldscore/50000)), (INT32)0) : 0),
			(INT32)(mapheaderinfo[prevmap]->maxbonuslives < 0 ? INT32_MAX : mapheaderinfo[prevmap]->maxbonuslives));
		if (ptlives)
			P_GivePlayerLives(&players[i], ptlives);

		if (i == consoleplayer)
		{
			data.coop.gotlife = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? 0 : ptlives);
			M_Memcpy(&data.coop.bonuses, &localbonuses, sizeof(data.coop.bonuses));
		}
	}


	if (data.coop.gotperfbonus < 0)
		data.coop.gotperfbonus = 0;
}





static void Y_AwardSpecialStageBonus(void)
{
	INT32 i, oldscore, ptlives;
	y_bonus_t localbonuses[2];

	data.spec.score = players[consoleplayer].score;
	memset(data.spec.bonuses, 0, sizeof(data.spec.bonuses));
	memset(data.spec.bonuspatches, 0, sizeof(data.spec.bonuspatches));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		oldscore = players[i].score;

		if (!playeringame[i] || players[i].lives < 1 || players[i].bot == BOT_2PAI || players[i].bot == BOT_2PHUMAN)  
		{
			Y_SetNullBonus(&players[i], &localbonuses[0]);
			Y_SetNullBonus(&players[i], &localbonuses[1]);
		}
		else if (maptol & TOL_NIGHTS)  
		{
			Y_SetNightsBonus(&players[i], &localbonuses[0]);
			Y_SetNullBonus(&players[i], &localbonuses[1]);
		}
		else
		{
			Y_SetSpecialRingBonus(&players[i], &localbonuses[0]);
			Y_SetPerfectBonus(&players[i], &localbonuses[1]);
		}
		players[i].score += localbonuses[0].points;
		players[i].score += localbonuses[1].points;
		if (players[i].score > MAXSCORE)
			players[i].score = MAXSCORE;
		players[i].recordscore += localbonuses[0].points;
		players[i].recordscore += localbonuses[1].points;
		if (players[i].recordscore > MAXSCORE)
			players[i].recordscore = MAXSCORE;


		ptlives = min(
			(INT32)((!ultimatemode && !modeattacking && players[i].lives != INFLIVES) ? max((INT32)((players[i].score/50000) - (oldscore/50000)), (INT32)0) : 0),
			(INT32)(mapheaderinfo[prevmap]->maxbonuslives < 0 ? INT32_MAX : mapheaderinfo[prevmap]->maxbonuslives));
		P_GivePlayerLives(&players[i], ptlives);

		if (i == consoleplayer)
		{
			data.spec.gotlife = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? 0 : ptlives);
			M_Memcpy(&data.spec.bonuses, &localbonuses, sizeof(data.spec.bonuses));


			data.spec.continues = min(players[i].continues, 8);
			if (players[i].gotcontinue)
				data.spec.continues |= 0x80;
			data.spec.playercolor = &players[i].skincolor;
			data.spec.playerchar = &players[i].skin;
		}
	}
}






void Y_EndIntermission(void)
{
	if (!dedicated)
		Y_UnloadData();

	endtic = -1;
	intertype = int_none;
	usebuffer = false;
}

#define UNLOAD(x) if (x) {Patch_Free(x);} x = NULL;




static void Y_UnloadData(void)
{

	UNLOAD(bgpatch);
	UNLOAD(bgtile);
	UNLOAD(interpic);

	switch (intertype)
	{
		case int_coop:

			UNLOAD(data.coop.bonuspatches[3]);
			UNLOAD(data.coop.bonuspatches[2]);
			UNLOAD(data.coop.bonuspatches[1]);
			UNLOAD(data.coop.bonuspatches[0]);
			UNLOAD(data.coop.ptotal);
			break;
		case int_spec:



			UNLOAD(data.spec.bonuspatches[1]);
			UNLOAD(data.spec.bonuspatches[0]);
			UNLOAD(data.spec.pscore);
			UNLOAD(data.spec.pcontinues);
			break;
		case int_match:
		case int_race:
			UNLOAD(data.match.result);
			break;
		case int_ctf:
			UNLOAD(data.match.blueflag);
			UNLOAD(data.match.redflag);
			break;
		default:



			break;
	}
}
