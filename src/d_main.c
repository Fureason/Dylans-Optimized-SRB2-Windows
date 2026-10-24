// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2025 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------







#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __GNUC__
#include <unistd.h>  
#endif

#ifdef _WIN32
#include <direct.h>
#include <malloc.h>
#endif

#include <time.h>

#include "doomdef.h"
#include "am_map.h"
#include "console.h"
#include "netcode/d_net.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_time.h"
#include "i_threads.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "r_main.h"
#include "r_local.h"
#include "r_data.h"
#include "r_translation.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_main.h"
#include "netcode/d_netfil.h"
#include "m_cheat.h"
#include "y_inter.h"
#include "p_local.h"  
#include "netcode/mserv.h"  
#include "m_misc.h"  
#include "deh_tables.h"  
#include "m_cond.h"  
#include "fastcmp.h"
#include "r_fps.h"  
#include "keys.h"
#include "filesrch.h"  
#include "g_input.h"  
#include "m_perfstats.h"
#include "m_random.h"
#include "command.h"
#include "discord_presence.h"

#ifdef CMAKECONFIG
#include "config.h"
#else
#include "config.h.in"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"  
#endif

#ifdef _WINDOWS
#include "win32/win_main.h"  
#endif

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

#include "lua_script.h"


int    VERSION;
int SUBVERSION;


UINT8 window_notinfocus = false;

static addfilelist_t startupwadfiles;
static addfilelist_t startuppwads;

boolean devparm = false;  

boolean singletics = false;  
boolean lastdraw = false;

postimg_t postimgtype = postimg_none;
INT32 postimgparam;
postimg_t postimgtype2 = postimg_none;
INT32 postimgparam2;




boolean midi_disabled = false;
boolean sound_disabled = false;
boolean digital_disabled = false;




boolean advancedemo;
#ifdef DEBUGFILE
INT32 debugload = 0;
#endif

UINT16 numskincolors;
menucolor_t *menucolorhead, *menucolortail;

char savegamename[256];
char liveeventbackup[256];

char srb2home[256] = ".";
char srb2path[256] = ".";
boolean usehome = true;
const char *pandf = "%s" PATHSEP "%s";
static char addonsdir[MAX_WADPATH];








event_t events[MAXEVENTS];
INT32 eventhead, eventtail;

boolean dedicated = false;





void D_PostEvent(const event_t *ev)
{
	events[eventhead] = *ev;
	eventhead = (eventhead+1) & (MAXEVENTS-1);
}



UINT8 shiftdown = 0;  
UINT8 ctrldown = 0;  
UINT8 altdown = 0;  
boolean capslock = 0;	 





void D_ProcessEvents(void)
{
	event_t *ev;

	boolean eaten;


	G_SetMouseDeltas(0, 0, 1);
	G_SetMouseDeltas(0, 0, 2);
	mouse.buttons &= ~(MB_SCROLLUP|MB_SCROLLDOWN);
	mouse2.buttons &= ~(MB_SCROLLUP|MB_SCROLLDOWN);

	for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
	{
		boolean hooked = false;

		ev = &events[eventtail];


		if (ev->type == ev_keydown || ev->type == ev_keyup || ev->type == ev_text)
		{

			if ((UINT32)(ev->key - KEY_MOUSE1) < MOUSEBUTTONS)
			{
				if (ev->type == ev_keydown || ev->type == ev_text)
					mouse.buttons |= 1 << (ev->key - KEY_MOUSE1);
				else
					mouse.buttons &= ~(1 << (ev->key - KEY_MOUSE1));
			}
			else if ((UINT32)(ev->key - KEY_2MOUSE1) < MOUSEBUTTONS)
			{
				if (ev->type == ev_keydown || ev->type == ev_text)
					mouse2.buttons |= 1 << (ev->key - KEY_2MOUSE1);
				else
					mouse2.buttons &= ~(1 << (ev->key - KEY_2MOUSE1));
			}

			else switch (ev->key) {
				case KEY_MOUSEWHEELUP:
					mouse.buttons |= MB_SCROLLUP;
					break;
				case KEY_MOUSEWHEELDOWN:
					mouse.buttons |= MB_SCROLLDOWN;
					break;
				case KEY_2MOUSEWHEELUP:
					mouse2.buttons |= MB_SCROLLUP;
					break;
				case KEY_2MOUSEWHEELDOWN:
					mouse2.buttons |= MB_SCROLLDOWN;
					break;
			}
		}


		if (M_ScreenshotResponder(ev))
			continue;  

		if (gameaction == ga_nothing && gamestate == GS_TITLESCREEN)
		{
			if (cht_Responder(ev))
				continue;
		}

		if (!CON_Ready() && !menuactive) {
			if (G_LuaResponder(ev))
				continue;
			hooked = true;
		}


#ifdef HAVE_THREADS
		I_lock_mutex(&m_menu_mutex);
#endif
		{
			eaten = M_Responder(ev);
		}
#ifdef HAVE_THREADS
		I_unlock_mutex(m_menu_mutex);
#endif

		if (eaten)
			continue;  

		if (!hooked && !CON_Ready()) {
			if (G_LuaResponder(ev))
				continue;
			hooked = true;
		}


#ifdef HAVE_THREADS
		I_lock_mutex(&con_mutex);
#endif
		{
			eaten = CON_Responder(ev);
		}
#ifdef HAVE_THREADS
		I_unlock_mutex(con_mutex);
#endif

		if (eaten)
			continue;  

		if (!hooked && !CON_Ready() && G_LuaResponder(ev))
			continue;

		G_Responder(ev);
	}

	if (mouse.rdx || mouse.rdy)
		G_SetMouseDeltas(mouse.rdx, mouse.rdy, 1);
	if (mouse2.rdx || mouse2.rdy)
		G_SetMouseDeltas(mouse2.rdx, mouse2.rdy, 2);
}








gamestate_t wipegamestate = GS_LEVEL;

INT16 wipetypepre = -1;
INT16 wipetypepost = -1;

static void D_Display(void)
{
	boolean forcerefresh = false;
	static boolean wipe = false;
	INT32 wipedefindex = 0;

	if (dedicated)
		return;

	if (nodrawers)
		return;  

	                                                                         
                                                                           
	PS_UpdateFrameStats();
















	if ((setrenderneeded || setmodeneeded) && !wipe)
		SCR_SetMode();  


	if (vid.recalc)
		SCR_Recalc();  


	if (rendermode == render_soft && !splitscreen)
		R_CheckViewMorph();



	if (setsizeneeded)
	{
		R_ExecuteSetViewSize();
		forcerefresh = true;  
	}



	I_UpdateNoBlit();


	wipe = (gamestate != wipegamestate);
	if (wipe && wipetypepre != INT16_MAX)
	{

		wipedefindex = gamestate;  
		if (gamestate == GS_INTERMISSION)
		{
			if (intertype == int_spec)  
				wipedefindex = wipe_specinter_toblack;
			else if (intertype != int_coop)  
				wipedefindex = wipe_multinter_toblack;
		}

		if (wipetypepre < 0 || !F_WipeExists(wipetypepre))
			wipetypepre = wipedefs[wipedefindex];

		if (rendermode != render_none)
		{

			if ((wipegamestate == (gamestate_t)FORCEWIPE ||
			        (wipegamestate != (gamestate_t)FORCEWIPEOFF
						&& !(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
					)  
			 && wipetypepre != UINT8_MAX)
			{
				F_WipeStartScreen();

				wipestyleflags = WSF_FADEOUT;
				if (wipegamestate == (gamestate_t)FORCEWIPE)
					F_WipeColorFill(31);
				else if (F_TryColormapFade(31))
					wipetypepost = -1;  
				F_WipeEndScreen();
				F_RunWipe(wipetypepre, gamestate != GS_TIMEATTACK && gamestate != GS_TITLESCREEN);
			}

			F_WipeStartScreen();
		}

		wipetypepre = -1;
	}
	else
		wipetypepre = -1;


	switch (gamestate)
	{
		case GS_TITLESCREEN:
			if (!titlemapinaction || !curbghide) {
				F_TitleScreenDrawer();
				break;
			}
			 
		case GS_LEVEL:
			if (!gametic)
				break;
			AM_Drawer();
			break;

		case GS_INTERMISSION:
			Y_IntermissionDrawer();
			HU_Drawer();
			break;

		case GS_TIMEATTACK:
			break;

		case GS_INTRO:
			F_IntroDrawer();
			if (wipegamestate == (gamestate_t)-1)
				wipe = true;
			break;

		case GS_ENDING:
			F_EndingDrawer();
			HU_Drawer();
			break;

		case GS_CUTSCENE:
			F_CutsceneDrawer();
			HU_Drawer();
			break;

		case GS_GAMEEND:
			F_GameEndDrawer();
			break;

		case GS_EVALUATION:
			F_GameEvaluationDrawer();
			HU_Drawer();
			break;

		case GS_CONTINUING:
			F_ContinueDrawer();
			break;

		case GS_CREDITS:
			F_CreditDrawer();
			HU_Drawer();
			break;

		case GS_WAITINGPLAYERS:

			if (netgame)
			{

				F_WaitingPlayersDrawer();
				HU_Drawer();
			}
		case GS_DEDICATEDSERVER:
		case GS_NULL:
			break;
	}


	if (wipegamestate == GS_INTRO && gamestate == GS_TITLESCREEN)
		wipegamestate = FORCEWIPEOFF;
	else
	{
		wipegamestate = gamestate;

		if (gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction && curbghide && (!hidetitlemap)))
		{

			if (cv_debug)
			{
				r_renderwalls = cv_renderwalls.value;
				r_renderfloors = cv_renderfloors.value;
				r_renderthings = cv_renderthings.value;
			}
			else
			{
				r_renderwalls = true;
				r_renderfloors = true;
				r_renderthings = true;
			}

			if (!automapactive && !dedicated && cv_renderview.value)
			{
				R_ApplyLevelInterpolators(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);
				PS_START_TIMING(ps_rendercalltime);
				if (players[displayplayer].mo || players[displayplayer].playerstate == PST_DEAD)
				{
					topleft = screens[0] + viewwindowy*vid.width + viewwindowx;
					objectsdrawn = 0;
	#ifdef HWRENDER
					if (rendermode != render_soft)
						HWR_RenderPlayerView(0, &players[displayplayer]);
					else
	#endif
					if (rendermode != render_none)
						R_RenderPlayerView(&players[displayplayer]);
				}


				if (splitscreen && players[secondarydisplayplayer].mo)
				{
					viewwindowy = vid.height / 2;

#ifdef HWRENDER
					if (rendermode == render_opengl)
						HWR_RenderPlayerView(1, &players[secondarydisplayplayer]);
					else
#endif
					if (rendermode != render_none)
					{
						topleft = screens[0] + viewwindowy*vid.width + viewwindowx;

						R_RenderPlayerView(&players[secondarydisplayplayer]);
					}

					viewwindowy = 0;
				}


				if (rendermode == render_soft)
				{
					if (!splitscreen)
						R_ApplyViewMorph();

					if (postimgtype)
						V_DoPostProcessor(0, postimgtype, postimgparam);
					if (postimgtype2)
						V_DoPostProcessor(1, postimgtype2, postimgparam2);
				}
				PS_STOP_TIMING(ps_rendercalltime);
				R_RestoreLevelInterpolators();
			}

			if (lastdraw)
			{
				if (rendermode == render_soft)
				{
					VID_BlitLinearScreen(screens[0], screens[1], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
					Y_ConsiderScreenBuffer();
					usebuffer = true;
				}
				lastdraw = false;
			}

			PS_START_TIMING(ps_uitime);

			if (gamestate == GS_LEVEL)
			{
				ST_Drawer();
				F_TextPromptDrawer();
				HU_Drawer();
			}
			else
				F_TitleScreenDrawer();
		}
		else
		{
			PS_START_TIMING(ps_uitime);
		}
	}



	if (forcerefresh && !(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
		V_SetPalette(0);


	if (paused && cv_showhud.value && (!menuactive || netgame))
	{
#if 0
		INT32 py;
		patch_t *patch;
		if (automapactive)
			py = 4;
		else
			py = viewwindowy + 4;
		patch = W_CachePatchName("M_PAUSE", PU_PATCH);
		V_DrawScaledPatch(viewwindowx + (BASEVIDWIDTH - patch->width)/2, py, 0, patch);
#else
		INT32 y = ((automapactive) ? (32) : (BASEVIDHEIGHT/2));
		M_DrawTextBox((BASEVIDWIDTH/2) - (60), y - (16), 13, 2);
		V_DrawCenteredString(BASEVIDWIDTH/2, y - (4), V_YELLOWMAP, "Game Paused");
#endif
	}


	vid.recalc = 0;

#ifdef HAVE_THREADS
	I_lock_mutex(&m_menu_mutex);
#endif
	M_Drawer();  
#ifdef HAVE_THREADS
	I_unlock_mutex(m_menu_mutex);
#endif


	CON_Drawer();

	PS_STOP_TIMING(ps_uitime);




	if (wipe && wipetypepost != INT16_MAX)
	{


		wipedefindex += WIPEFINALSHIFT;

		if (wipetypepost < 0 || !F_WipeExists(wipetypepost))
			wipetypepost = wipedefs[wipedefindex];

		if (rendermode != render_none)
		{
			F_WipeEndScreen();


			if (WipeStageTitle && st_overlay)
			{
				lt_ticker--;
				lt_lasttic = lt_ticker;
				ST_preLevelTitleCardDrawer();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, levelfadecol);
				F_WipeStartScreen();
			}


			if (F_ShouldColormapFade())
			{
				wipestyleflags |= WSF_FADEIN;
				wipestyleflags &= ~WSF_FADEOUT;
			}

			F_RunWipe(wipetypepost,
				gamestate != GS_TIMEATTACK && gamestate != GS_TITLESCREEN);
		}


		if (timingdemo)
		{
			framecount = 0;
			demostarttime = I_GetTime();
		}

		wipetypepost = -1;
	}
	else
		wipetypepost = -1;

	NetUpdate();  


	if (G_GetExitGameFlag())
	{
		Command_ExitGame_f();
		G_ClearExitGameFlag();
	}




	if (!wipe)
	{
		if (cv_netstat.value)
		{
			char s[50];
			Net_GetNetStat();

			s[sizeof s - 1] = '\0';

			snprintf(s, sizeof s - 1, "get %d b/s", getbps);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-40, V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "send %d b/s", sendbps);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-30, V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "GameMiss %.2f%%", gamelostpercent);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-20, V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "SysMiss %.2f%%", lostpercent);
			V_DrawRightAlignedString(BASEVIDWIDTH, BASEVIDHEIGHT-10, V_YELLOWMAP, s);
		}

		if (cv_perfstats.value)
		{
			M_DrawPerfStats();
		}

		PS_START_TIMING(ps_swaptime);
		I_FinishUpdate();  
		PS_STOP_TIMING(ps_swaptime);
	}
}





tic_t rendergametic;

void D_SRB2Loop(void)
{
	tic_t entertic = 0, oldentertics = 0, realtics = 0, rendertimeout = INFTICS;
	double deltatics = 0.0;
	double deltasecs = 0.0;
	static lumpnum_t gstartuplumpnum;
	precise_t frame_deadline = 0;
	UINT32 frame_deadline_cap = 0;

	boolean interp = false;
	boolean doDisplay = false;

	if (dedicated)
		server = true;

	DRPC_Init();



#ifdef _WINDOWS
	CONS_Printf("I_StartupMouse()...\n");
	I_DoStartupMouse();
#endif

	I_UpdateTime(cv_timescale.value);
	oldentertics = I_GetTime();


	con_refresh = false;
	con_startup = false;


	SCR_SetMode();  
	SCR_Recalc();

	chosenrendermode = render_none;



	CONS_Printf(
	"===========================================================================\n"
	"                   We hope you enjoy this game as\n"
	"                     much as we did making it!\n"
	"                            ...wait. =P\n"
	"===========================================================================\n");


	COM_ImmedExecute("cls;version");

	I_FinishUpdate();  
	  
                                            
                                            
   
	 
	if (gamestate != GS_TITLESCREEN)
	{
		gstartuplumpnum = W_CheckNumForPatchName("STARTUP");
		if (gstartuplumpnum == LUMPERROR)
			gstartuplumpnum = W_GetNumForPatchName("MISSING");
		V_DrawScaledPatch(0, 0, 0, W_CachePatchNum(gstartuplumpnum, PU_PATCH));
	}

	for (;;)
	{
		if (I_CompanionShutdownRequested())
		{
			CONS_Printf("Controlling client closed; stopping local dedicated companion.\n");
			I_Quit();
		}


		precise_t capbudget;
		precise_t enterprecise = I_GetPreciseTime();
		precise_t finishprecise = enterprecise;
		const boolean software_vsync_pacing = cv_vidwait.value
			&& !I_VSyncProvidesPacing();
		UINT32 framecap = R_GetFramerateCap();
		if (software_vsync_pacing)
		{
			const UINT32 refresh = I_GetRefreshRate();
			if (refresh && (!framecap || framecap > refresh))
				framecap = refresh;
		}

		if (framecap)
		{

			double budget = round((1.0 / framecap) * I_GetPrecisePrecision());
			capbudget = (precise_t) budget;
		}
		else
			capbudget = 0;

		I_UpdateTime(cv_timescale.value);
		DRPC_Update();

		if (lastwipetic)
		{
			oldentertics = lastwipetic;
			lastwipetic = 0;
		}


		entertic = I_GetTime();
		realtics = entertic - oldentertics;
		oldentertics = entertic;

		if (demoplayback && gamestate == GS_LEVEL)
		{

			realtics = realtics * cv_playbackspeed.value;
		}

#ifdef DEBUGFILE
		if (!realtics)
			if (debugload)
				debugload--;
#endif

		interp = R_UsingFrameInterpolation() && !dedicated;
		doDisplay = false;

#ifdef HW3SOUND
		HW3S_BeginFrameUpdate();
#endif

		refreshdirmenu = 0;  

		if (realtics > 0 || singletics)
		{


			if (realtics > 8)
				realtics = 1;


			TryRunTics(realtics);

			if (lastdraw || singletics || gametic > rendergametic)
			{
				rendergametic = gametic;
				rendertimeout = entertic + TICRATE/17;

				doDisplay = true;
			}
			else if (rendertimeout < entertic)  
			{

				if (gamestate == GS_LEVEL && netgame)
				{


					for (tic_t chasecamtics = 0; chasecamtics < realtics; chasecamtics++)
					{
						                                                               
                                                                      
                                                                   
                                                                     
                                                                         
						if (splitscreen && camera2.chase)
						{
							mobj_t *mo = players[secondarydisplayplayer].mo;
							fixed_t platformmomz = mo ? mo->pmomz : 0;
							if (mo)
								mo->pmomz = 0;
							P_MoveChaseCamera(&players[secondarydisplayplayer], &camera2, false);
							if (mo)
								mo->pmomz = platformmomz;
						}
						if (camera.chase)
						{
							mobj_t *mo = players[displayplayer].mo;
							fixed_t platformmomz = mo ? mo->pmomz : 0;
							if (mo)
								mo->pmomz = 0;
							P_MoveChaseCamera(&players[displayplayer], &camera, false);
							if (mo)
								mo->pmomz = platformmomz;
						}
					}
					R_UpdateViewInterpolation();
				}

				doDisplay = true;
			}

			renderisnewtic = true;
		}
		else
		{
			renderisnewtic = false;
		}

		if (interp)
		{


			ps_interp_frac.value.p = (precise_t)((FIXED_TO_FLOAT(g_time.timefrac)) * 1000.0f);
			ps_interp_lag.value.p = (precise_t)((deltasecs) * 1000.0f);

			renderdeltatics = FLOAT_TO_FIXED(deltatics);

			if (!(paused || P_AutoPause()) && deltatics < 1.0 && !hu_stopped)
			{
				rendertimefrac = g_time.timefrac;
			}
			else
			{
				rendertimefrac = FRACUNIT;
			}
		}
		else
		{
			renderdeltatics = realtics * FRACUNIT;
			rendertimefrac = FRACUNIT;
		}

		if (interp || doDisplay)
		{
			D_Display();
		}


		M_PollAsyncScreenshots();
		if (moviemode)
			M_SaveFrame();
		if (takescreenshot)
			M_DoScreenShot();


		S_UpdateSounds();  
		if (realtics > 0 || singletics)
			S_UpdateClosedCaptions();

#ifdef HW3SOUND
		HW3S_EndFrameUpdate();
#endif

		LUA_Step();

		                                                                       
                                                                          
                                                                        
                        
		if (gamestate == GS_LEVEL)
		{
			precise_t warmup_now = I_GetPreciseTime();
			precise_t warmup_budget = capbudget;
			precise_t warmup_margin = (precise_t)(I_GetPrecisePrecision() / 500);  

			if (!warmup_budget)
			{
				UINT32 refresh = I_GetRefreshRate();
				                                                                 
                                                                        
                                                                      
                                                                       
                                                                         
                                        
				if (!refresh || refresh > 60)
					refresh = 60;
				warmup_budget = (precise_t)(I_GetPrecisePrecision() / refresh);
			}

			if (warmup_now - enterprecise + warmup_margin < warmup_budget)
				R_ContinueLevelSpritePrecache();
		}


		finishprecise = I_GetPreciseTime();
		{
			precise_t adaptive_work = finishprecise - enterprecise;
			                                                                   
                                                                      
                                                 
			if (cv_vidwait.value && ps_swaptime.value.p < adaptive_work)
				adaptive_work -= ps_swaptime.value.p;
			R_UpdateAdaptiveEffects(adaptive_work, capbudget);
		}
#if defined(_WIN32) && !defined(DEDICATED)
		if (!singletics && gamestate == GS_LEVEL)
			I_ReportFrameHitch(enterprecise, finishprecise);
		I_RecordDiagnosticHeartbeat();
#endif
		if (!singletics)
		{
			INT64 elapsed = (INT64)(finishprecise - enterprecise);

			                                            
                                                                            
                                                                          
                                                                           
			const UINT32 refresh = I_GetRefreshRate();
			const boolean vsync_with_match_refresh = cv_vidwait.value
				&& I_VSyncProvidesPacing() && framecap && refresh
				&& framecap >= refresh;

			if (framecap && !vsync_with_match_refresh
			&& (cv_smoothframepacing.value || software_vsync_pacing))
			{
				                                                                     
                                                                      
                                                
				if (!frame_deadline || frame_deadline_cap != framecap)
					frame_deadline = enterprecise + capbudget;

				if ((INT64)(frame_deadline - finishprecise) > 0)
					I_SleepDuration(frame_deadline - finishprecise);

				finishprecise = I_GetPreciseTime();
				if ((INT64)(finishprecise - frame_deadline) > (INT64)capbudget * 2)
					frame_deadline = finishprecise + capbudget;
				else
					frame_deadline += capbudget;
				frame_deadline_cap = framecap;
			}
			else if (framecap && elapsed > 0 && (INT64)capbudget > elapsed && !vsync_with_match_refresh)
			{
				I_SleepDuration(capbudget - (finishprecise - enterprecise));
			}
			else
			{
				frame_deadline = 0;
				frame_deadline_cap = framecap;
			}
		}

		finishprecise = I_GetPreciseTime();
		deltasecs = (double)((INT64)(finishprecise - enterprecise)) / I_GetPrecisePrecision();
		deltatics = deltasecs * NEWTICRATE;
	}
}





void D_AdvanceDemo(void)
{
	advancedemo = true;
}








void D_StartTitle(void)
{
	INT32 i;

	S_StopMusic();

	if (netgame)
	{
		if (gametyperules & GTR_CAMPAIGN)
		{
			G_SetGamestate(GS_WAITINGPLAYERS);  

			if (server)
			{
				char mapname[6];

				strlcpy(mapname, G_BuildMapName(spstage_start), sizeof (mapname));
				strlwr(mapname);
				mapname[5] = '\0';

				COM_BufAddText(va("map %s\n", mapname));
			}
		}

		return;
	}



	SV_StopServer();
	SV_ResetServer();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	players[consoleplayer].availabilities = players[1].availabilities = R_GetSkinAvailabilities();  

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	cv_debug = 0;
	emeralds = 0;
	memset(&luabanks, 0, sizeof(luabanks));
	lastmaploaded = 0;
	pickedchar = R_SkinAvailable(cv_skin.string);



	modeattacking = ATTACKING_NONE;
	marathonmode = 0;


	maptol = 0;

	gameaction = ga_nothing;
	displayplayer = consoleplayer = 0;
	G_SetGametype(GT_COOP);
	paused = false;
	advancedemo = false;
	F_InitMenuPresValues();
	F_StartTitleScreen();

	currentMenu = &MainDef;  


	if (rendermode != render_none)
		V_SetPaletteLump("PLAYPAL");


	if (tutorialmode && tutorialgcs)
	{
		G_CopyControls(gamecontrol, gamecontroldefault[gcs_custom], gcl_tutorial_full, num_gcl_tutorial_full);  
		CV_SetValue(&cv_usemouse, tutorialusemouse);
		CV_SetValue(&cv_alwaysfreelook, tutorialfreelook);
		CV_SetValue(&cv_mousemove, tutorialmousemove);
		CV_SetValue(&cv_analog[0], tutorialanalog);
		M_StartMessage("Do you want to \x82save the recommended \x82movement controls?\x80\n\nPress 'Y' or 'Enter' to confirm\nPress 'N' or any key to keep \nyour current controls",
			M_TutorialSaveControlResponse, MM_YESNO);
	}
	tutorialmode = false;
}

#define REALLOC_FILE_LIST \
	if (list->files == NULL) \
	{ \
		list->files = calloc(2, sizeof(list->files)); \
		list->numfiles = 1; \
	} \
	else \
	{ \
		index = list->numfiles; \
		list->files = realloc(list->files, sizeof(list->files) * ((++list->numfiles) + 1)); \
		if (list->files == NULL) \
			I_Error("%s: No more free memory to add file %s", __FUNCTION__, file); \
	}

static void D_AddFile(addfilelist_t *list, const char *file)
{
	char *newfile;
	size_t index = 0;

	REALLOC_FILE_LIST

	newfile = malloc(strlen(file) + 1);
	if (!newfile)
		I_Error("D_AddFile: No more free memory to add file %s", file);

	strcpy(newfile, file);
	list->files[index] = newfile;
}

static void D_AddFolder(addfilelist_t *list, const char *file)
{
	char *newfile;
	size_t index = 0;

	REALLOC_FILE_LIST

	newfile = malloc(strlen(file) + 2);  
	if (!newfile)
		I_Error("D_AddFolder: No more free memory to add folder %s", file);

	strcpy(newfile, file);
	strcat(newfile, PATHSEP);

	list->files[index] = newfile;
}

#undef REALLOC_FILE_LIST

static inline void D_CleanFile(addfilelist_t *list)
{
	if (list->files)
	{
		size_t pnumwadfiles = 0;

		for (; pnumwadfiles < list->numfiles; pnumwadfiles++)
			free(list->files[pnumwadfiles]);

		free(list->files);
		list->files = NULL;
	}

	list->numfiles = 0;
}



static void ChangeDirForUrlHandler(void)
{


	if (M_GetUrlProtocolArg() != NULL && !M_CheckParm("-nochdir"))
	{
		size_t i;

		CONS_Printf("%s connect links load game files from the SRB2 application's stored directory. Switching to ", SERVER_URL_PROTOCOL);
		strlcpy(srb2path, myargv[0], sizeof(srb2path));


		for (i = strlen(srb2path)-1; i > 0; i--)
		{
			if (srb2path[i] == '/' || srb2path[i] == '\\')
			{
				srb2path[i] = '\0';
				break;
			}
		}

		CONS_Printf("%s\n", srb2path);

#if defined (_WIN32)
		SetCurrentDirectoryA(srb2path);
#else
		if (chdir(srb2path) == -1)
			I_OutputMsg("Couldn't change working directory\n");
#endif
	}
}





static void IdentifyVersion(void)
{
	char *srb2wad;
	const char *srb2waddir = NULL;

#if defined (__unix__) || defined (UNIXCOMMON) || defined (HAVE_SDL)

	srb2waddir = I_LocateWad();
#endif


	if (srb2waddir)
	{
		strlcpy(srb2path,srb2waddir,sizeof (srb2path));
	}
	else
	{
		if (getcwd(srb2path, 256) != NULL)
			srb2waddir = srb2path;
		else
		{
			srb2waddir = ".";
		}
	}

#if defined (macintosh) && !defined (HAVE_SDL)

	if (!stricmp(srb2waddir, "/"))
		srb2waddir = I_GetWadDir();
#endif

	srb2wad = malloc(strlen(srb2waddir)+1+8+1);
	if (srb2wad == NULL)
		I_Error("No more free memory to look in %s", srb2waddir);
	else
		sprintf(srb2wad, pandf, srb2waddir, "srb2.pk3");


	snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, srb2waddir);
	configfile[sizeof configfile - 1] = '\0';


	if (srb2wad != NULL && FIL_ReadFileOK(srb2wad))
		D_AddFile(&startupwadfiles, srb2wad);
	else
		I_Error("srb2.pk3 not found! Expected in %s, ss file: %s\n", srb2waddir, srb2wad);

	if (srb2wad)
		free(srb2wad);





	D_AddFile(&startupwadfiles, va(pandf,srb2waddir, "zones.pk3"));


	D_AddFile(&startupwadfiles, va(pandf,srb2waddir, "characters.pk3"));

#ifdef USE_PATCH_DTA

	D_AddFile(&startupwadfiles, va(pandf,srb2waddir, "patch.pk3"));
#endif

#if !defined (HAVE_SDL) || defined (HAVE_MIXER)
	{
#define MUSICTEST(str) \
		{\
			const char *musicpath = va(pandf,srb2waddir,str);\
			int ms = W_VerifyNMUSlumps(musicpath, false); \
			if (ms == 1) \
				D_AddFile(&startupwadfiles, musicpath); \
			else if (ms == 0) \
				I_Error("File "str" has been modified with non-music/sound lumps"); \
		}

		MUSICTEST("music.pk3")

	}
#endif
}

static void
D_ConvertVersionNumbers (void)
{
	 
#ifndef DEVELOP
	int major;
	int minor;

	sscanf(SRB2VERSION, "%d.%d.%d", &major, &minor, &SUBVERSION);

	 
	VERSION = ( major * 100 ) + minor;
#endif
}

static void Command_assert(void)
{
#if !defined(NDEBUG) || defined(PARANOIA)
	CONS_Printf("Yes, assertions are enabled.\n");
#else
	CONS_Printf("No, assertions are NOT enabled.\n");
#endif
}




void D_SRB2Main(void)
{
	INT32 p;

	INT32 pstartmap = 1;
	boolean autostart = false;

	 
	D_ConvertVersionNumbers();

	if (!strcmp(compbranch, ""))
	{
		compbranch = "detached HEAD";
	}

	// Print GPL notice for our console users (Linux)
	CONS_Printf(
	"\n\nSonic Robo Blast 2\n"
	"Copyright (C) 1998-2025 by Sonic Team Junior\n\n"
	"This program comes with ABSOLUTELY NO WARRANTY.\n\n"
	"This is free software, and you are welcome to redistribute it\n"
	"and/or modify it under the terms of the GNU General Public License\n"
	"as published by the Free Software Foundation; either version 2 of\n"
	"the License, or (at your option) any later version.\n"
	"See the 'LICENSE.txt' file for details.\n\n"
	"Sonic the Hedgehog and related characters are trademarks of SEGA.\n"
	"We do not claim ownership of SEGA's intellectual property used\n"
	"in this program.\n\n");


#if !defined(NOTERMIOS)
	if (setvbuf(stderr, NULL, _IOFBF, 1000))
		I_OutputMsg("setvbuf didnt work\n");
#endif


	M_StartupLocale();


	M_FindResponseFile();


	G_LoadGameSettings();


	DEH_TableCheck();


	ChangeDirForUrlHandler();


	IdentifyVersion();

#if !defined(NOTERMIOS)
	setbuf(stdout, NULL);  
#endif

#if 0  
	devparm = M_CheckParm("-nodebug") == 0;
#else
	devparm = M_CheckParm("-debug") != 0;
#endif


#if !defined (_WINDOWS) && !defined (DEDICATED)  
	dedicated = M_CheckParm("-dedicated") != 0;
#endif

	if (devparm)
		CONS_Printf(M_GetText("Development mode ON.\n"));


	strcpy(savegamename, SAVEGAMENAME"%u.ssg");
	strcpy(liveeventbackup, "live"SAVEGAMENAME".bkp");  

	{
		const char *userhome = D_Home();  

		if (!userhome)
		{
#if (defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)) && !defined (__CYGWIN__)
			I_Error("Please set $HOME to your home directory\n");
#else
			if (dedicated)
				snprintf(configfile, sizeof configfile, "d"CONFIGFILENAME);
			else
				snprintf(configfile, sizeof configfile, CONFIGFILENAME);
#endif
		}
		else
		{

#ifdef DEFAULTDIR
			snprintf(srb2home, sizeof srb2home, "%s" PATHSEP DEFAULTDIR, userhome);
			snprintf(downloaddir, sizeof downloaddir, "%s" PATHSEP "DOWNLOAD", srb2home);
			if (dedicated)
				snprintf(configfile, sizeof configfile, "%s" PATHSEP "d"CONFIGFILENAME, srb2home);
			else
				snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, srb2home);


			strcatbf(savegamename, srb2home, PATHSEP);
			strcatbf(liveeventbackup, srb2home, PATHSEP);

			snprintf(luafiledir, sizeof luafiledir, "%s" PATHSEP "luafiles", srb2home);
#else  
			snprintf(srb2home, sizeof srb2home, "%s", userhome);
			snprintf(downloaddir, sizeof downloaddir, "%s", userhome);
			if (dedicated)
				snprintf(configfile, sizeof configfile, "%s" PATHSEP "d"CONFIGFILENAME, userhome);
			else
				snprintf(configfile, sizeof configfile, "%s" PATHSEP CONFIGFILENAME, userhome);


			strcatbf(savegamename, userhome, PATHSEP);
			strcatbf(liveeventbackup, userhome, PATHSEP);

			snprintf(luafiledir, sizeof luafiledir, "%s" PATHSEP "luafiles", userhome);
#endif  
		}

		configfile[sizeof configfile - 1] = '\0';
	}


	snprintf(addonsdir, sizeof addonsdir, "%s%s%s", srb2home, PATHSEP, "addons");
	I_mkdir(addonsdir, 0755);



	if (!M_RandomSeedFromOS())
		M_RandomSeed((UINT32)time(NULL));  

	P_SetRandSeed(M_RandomizedSeed());



	M_InitPlayerSetupColors();

	CONS_Printf("Z_Init(): Init zone memory allocation daemon. \n");
	Z_Init();

	                                                                           
                             
	if (M_CheckParm("-password") && M_IsNextParm())
		D_SetPassword(M_GetNextParm());

	clientGamedata = M_NewGameDataStruct();
	serverGamedata = M_NewGameDataStruct();


	COM_Init();
	DRPC_RegisterVariables();

	COM_AddCommand("assert", Command_assert, COM_LUA);



	if (!((M_GetUrlProtocolArg() || M_CheckParm("-connect")) && !M_CheckParm("-server")))
	{
		INT32 addontype = 0;
		INT32 i;

		for (i = 1; i < myargc; i++)
		{
			if (!strcasecmp(myargv[i], "-file"))
				addontype = 1;
			else if (!strcasecmp(myargv[i], "-folder"))
				addontype = 2;
			else if (myargv[i][0] == '-' || myargv[i][0] == '+')
				addontype = 0;
			else if (addontype == 1)
				D_AddFile(&startuppwads, myargv[i]);
			else if (addontype == 2)
				D_AddFolder(&startuppwads, myargv[i]);
		}
	}



	if (M_CheckParm("-server") || dedicated)
		netgame = server = true;


	P_PatchInfoTables();


	M_InitMenuPresTables();


	if (M_GetUrlProtocolArg() || M_CheckParm("-connect"))
		F_InitMenuPresValues();




	CONS_Printf("I_InitializeTime()...\n");
	I_InitializeTime();


	P_BackupTables();

	mainwads = 3;  
#ifdef USE_PATCH_DTA
	mainwads++;
#endif


	CONS_Printf("W_InitMultipleFiles(): Adding IWAD and main PWADs.\n");
	W_InitMultipleFiles(&startupwadfiles);
	D_CleanFile(&startupwadfiles);

#ifndef DEVELOP  


	W_VerifyFileMD5(0, ASSET_HASH_SRB2_PK3);  
	W_VerifyFileMD5(1, ASSET_HASH_ZONES_PK3);  
	W_VerifyFileMD5(2, ASSET_HASH_CHARACTERS_PK3);  
#ifdef USE_PATCH_DTA
	W_VerifyFileMD5(3, ASSET_HASH_PATCH_PK3);  
#endif


#endif  

	cht_Init();




	CONS_Printf("I_StartupGraphics()...\n");
	I_StartupGraphics();

#ifdef HWRENDER



	HWR_AddCommands();
#endif



	SCR_Startup();

	PaletteRemap_Init();

	HU_Init();

	CON_Init();

	D_RegisterServerCommands();
	D_RegisterClientCommands();  
	M_RegisterEarlyHostingVariables();  
	R_RegisterEngineStuff();
	S_RegisterSoundStuff();

	I_RegisterSysCommands();

	CON_StopRefresh();  

	if (startuppwads.numfiles)
	{
		CONS_Printf("W_InitMultipleFiles(): Adding extra PWADs.\n");
		W_InitMultipleFiles(&startuppwads);
		D_CleanFile(&startuppwads);
	}

	CON_StartRefresh();  

	CONS_Printf("HU_LoadGraphics()...\n");
	HU_LoadGraphics();


	M_FirstLoadConfig();  
	Net_ApplyIPv6Default();  

	if (M_CheckParm("-gamedata") && M_IsNextParm())
	{


		strlcpy(gamedatafilename, M_GetNextParm(), sizeof gamedatafilename);
	}

	G_LoadGameData(clientGamedata);
	M_CopyGameData(serverGamedata, clientGamedata);

	VID_PrepareModeList();  


	SCR_CheckDefaultMode();

	wipegamestate = gamestate;

	savedata.lives = 0;  






	if (M_CheckParm("-warp") && M_IsNextParm())
	{
		const char *word = M_GetNextParm();
		pstartmap = G_FindMapByNameOrCode(word, 0);
		if (! pstartmap)
			I_Error("Cannot find a map remotely named '%s'\n", word);
		else
		{
			if (!(M_CheckParm("-server") || dedicated))
				G_SetUsedCheats(true);
			autostart = true;
		}
	}

	if (M_CheckParm("-noupload"))
		COM_BufAddText("downloading 0\n");

	CONS_Printf("M_Init(): Init miscellaneous info.\n");
	M_Init();

	CONS_Printf("R_Init(): Init SRB2 refresh daemon.\n");
	R_Init();


	if (dedicated)
	{
		sound_disabled = true;
		midi_disabled = digital_disabled = true;
	}
	if (M_CheckParm("-noaudio"))  
	{
		sound_disabled = true;
		digital_disabled = true;
		midi_disabled = true;
	}
	else
	{
		if (M_CheckParm("-nosound"))
			sound_disabled = true;
		if (M_CheckParm("-nomusic"))  
		{
			digital_disabled = true;
			midi_disabled = true;
		}
		else
		{
			if (M_CheckParm("-nomidimusic"))
				midi_disabled = true;  
			if (M_CheckParm("-nodigmusic"))
				digital_disabled = true;  
		}
	}
	if (!( sound_disabled && digital_disabled
#ifndef NO_MIDI
				&& midi_disabled
#endif
	 ))
	{
		CONS_Printf("S_InitSfxChannels(): Setting up sound channels.\n");
		I_StartupSound();
		I_InitMusic();
		S_InitSfxChannels(cv_soundvolume.value);
	}

	S_InitMusicDefs();

	CONS_Printf("ST_Init(): Init status bar.\n");
	ST_Init();

	if (M_CheckParm("-room"))
	{
		if (!M_IsNextParm())
			I_Error("usage: -room <room_id>\nCheck the Master Server's webpage for room ID numbers.\n");
		CV_SetValue(&cv_masterserver_room_id, atoi(M_GetNextParm()));

#ifdef UPDATE_ALERT
		GetMODVersion_Console();
#endif
	}


	CONS_Printf("D_CheckNetGame(): Checking network game status.\n");
	if (D_CheckNetGame())
		autostart = true;

	if (!dedicated)
		pickedchar = R_SkinAvailable(cv_defaultskin.string);



	if (M_CheckParm("-metal"))
	{
		G_RecordMetal();
		autostart = true;
	}
	else if (M_CheckParm("-record") && M_IsNextParm())
	{
		G_RecordDemo(M_GetNextParm());
		autostart = true;
	}


	if (dedicated)
		COM_ImmedExecute(va("exec \"%s"PATHSEP"adedserv.cfg\"\n", srb2home));
	else
		COM_ImmedExecute(va("exec \"%s"PATHSEP"autoexec.cfg\" -noerror\n", srb2home));

	if (!autostart)
		M_PushSpecialParameters();  


	p = M_CheckParm("-playdemo");
	if (!p)
		p = M_CheckParm("-timedemo");
	if (p && M_IsNextParm())
	{
		char tmp[MAX_WADPATH];




		strcpy(tmp, M_GetNextParm());

		while (M_IsNextParm())
		{
			strcat(tmp, " ");
			strcat(tmp, M_GetNextParm());
		}

		FIL_DefaultExtension(tmp, ".lmp");

		CONS_Printf(M_GetText("Playing demo %s.\n"), tmp);

		if (M_CheckParm("-playdemo"))
		{
			singledemo = true;  
			G_DeferedPlayDemo(tmp);
		}
		else
			G_TimeDemo(tmp);

		G_SetGamestate(GS_NULL);
		wipegamestate = GS_NULL;
		return;
	}

	if (M_CheckParm("-ultimatemode"))
	{
		autostart = true;
		ultimatemode = true;
	}

	if (M_CheckParm("-splitscreen"))
	{
		autostart = true;
		splitscreen = true;
	}


	if (bootmap && !(M_CheckParm("-warp") && M_IsNextParm()))
	{
		pstartmap = bootmap;

		if (pstartmap < 1 || pstartmap > NUMMAPS)
			I_Error("Cannot warp to map %d (out of range)\n", pstartmap);
		else
		{
			autostart = true;
		}
	}

	if (autostart || netgame)
	{
		gameaction = ga_nothing;

		CV_ClearChangedFlags();



		M_PushSpecialParameters();  

		COM_BufExecute();  

		if (M_CheckParm("-gametype") && M_IsNextParm())
		{

			INT32 j;
			INT16 newgametype = -1;
			const char *sgametype = M_GetNextParm();

			newgametype = G_GetGametypeByName(sgametype);

			if (newgametype == -1)  
			{
				j = atoi(sgametype);  
				if (j >= 0 && j < gametypecount)
					newgametype = (INT16)j;
			}

			if (newgametype != -1)
			{
				j = gametype;
				G_SetGametype(newgametype);
				D_GameTypeChanged(j);
			}
		}

		if (server && !M_CheckParm("+map"))
		{

			if (W_CheckNumForName(G_BuildMapName(pstartmap)) == LUMPERROR)
				I_Error("Could not warp to %s (map not found)\n", G_BuildMapName(pstartmap));
			else
			{
				if (M_CampaignWarpIsCheat(gametype, pstartmap, serverGamedata))
				{


					G_SetUsedCheats(false);
				}

				D_MapChange(pstartmap, gametype, ultimatemode, true, 0, false, false);
			}
		}
	}
	else if (M_CheckParm("-skipintro"))
	{
		F_InitMenuPresValues();
		F_StartTitleScreen();
	}
	else if (cv_startupsequence.value >= 1)
	{
		F_InitMenuPresValues();
		F_StartTitleScreen();
		if (cv_startupsequence.value >= 2)
			M_StartControlPanel();
	}
	else
		F_StartIntro();  

	CON_ToggleOff();

	if (dedicated && server)
	{
		levelstarttic = gametic;
		G_SetGamestate(GS_LEVEL);
		if (!P_LoadLevel(false, false))
			I_Quit();  
	}
}

const char *D_Home(void)
{
	const char *userhome = NULL;

#ifdef ANDROID
	return "/data/data/org.srb2/";
#endif

	if (M_CheckParm("-home") && M_IsNextParm())
		userhome = M_GetNextParm();
	else
	{
#if !(defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON))
		if (FIL_FileOK(CONFIGFILENAME))
			usehome = false;  
		else
#endif
			userhome = I_GetEnv("HOME");  
	}
#ifdef _WIN32  
	if (!userhome && usehome)  
	{
		char *testhome = NULL;
		testhome = I_GetEnv("APPDATA");
		if (testhome != NULL
			&& (FIL_FileOK(va("%s" PATHSEP "%s" PATHSEP CONFIGFILENAME, testhome, DEFAULTDIR))))
		{
			userhome = testhome;
		}
	}
#ifndef __CYGWIN__
	if (!userhome && usehome)  
	{
		char *testhome = NULL;
		testhome = I_GetEnv("USERPROFILE");
		if (testhome != NULL
			&& (FIL_FileOK(va("%s" PATHSEP "%s" PATHSEP CONFIGFILENAME, testhome, DEFAULTDIR))))
		{
			userhome = testhome;
		}
	}
#endif 
#endif 
	if (usehome) return userhome;
	else return NULL;
}

static boolean check_top_dir(const char **path, const char *top)
{

	if (!strcmp(top, ""))
		return false;

	if (!startswith(*path, top))
		return false;

	*path += strlen(top);



	if (!endswith(top, PATHSEP))
	{
		if (startswith(*path, PATHSEP))
			*path += strlen(PATHSEP);
		else
			return false;
	}

	return true;
}

static int cmp_strlen_desc(const void *A, const void *B)
{
	const char *pA = A;
	const char *pB = B;
	size_t As = strlen(pA);
	size_t Bs = strlen(pB);
	return ((int)Bs - (int)As);
}

boolean D_IsPathAllowed(const char *path)
{
	char *paths[] = {
		srb2home,
		srb2path,
		cv_addons_folder.zstring
	};

	const size_t n_paths = sizeof paths / sizeof *paths;

	size_t i;





	qsort(paths, n_paths, sizeof *paths, cmp_strlen_desc);




	for (i = 0; i < n_paths; ++i)
	{
		if (check_top_dir(&path, paths[i]))
			break;
	}


	if (i == n_paths)
	{

		if (M_IsPathAbsolute(path))
			return false;
	}


	if (strstr(path, ".."))
		return false;

	return true;
}

boolean D_CheckPathAllowed(const char *path, const char *why)
{
	if (!D_IsPathAllowed(path))
	{
		CONS_Alert(CONS_WARNING, "%s: %s, location is not allowed\n", why, path);
		return false;
	}

	return true;
}
