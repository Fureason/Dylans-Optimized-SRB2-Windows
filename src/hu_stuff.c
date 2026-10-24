// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"

#include "m_menu.h"  
#include "m_cond.h"  
#include "m_misc.h"  

#include "netcode/d_clisrv.h"
#include "netcode/net_command.h"
#include "netcode/gamestate.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"
#include "i_system.h"

#include "st_stuff.h"
#include "r_local.h"

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#include "p_local.h"  
#include "p_tick.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "lua_hud.h"
#include "lua_hudlib_drawlist.h"
#include "lua_hook.h"

#ifdef _WIN32
#include <shellapi.h>
#endif


#define HU_INPUTX 0
#define HU_INPUTY 0

#define HU_SERVER_SAY 1  
#define HU_CSAY       2  





fontdef_t hu_font;
fontdef_t tny_font;
fontdef_t cred_font;
fontdef_t lt_font;
fontdef_t ntb_font;
fontdef_t nto_font;


patch_t *tallnum[10];  
patch_t *nightsnum[10];  
patch_t *ttlnum[10];  
patch_t *tallminus;
patch_t *tallinfin;

static player_t *plr;
boolean chat_on;  
boolean chat_on_first_event;  
static char w_chat[HU_MAXMSGLEN + 1];
static size_t c_input = 0;  
static boolean headsupactive = false;
boolean hu_showscores;  
static char hu_tick;

patch_t *rflagico;
patch_t *bflagico;
patch_t *rmatcico;
patch_t *bmatcico;
patch_t *tagico;





patch_t *emeraldpics[3][8];  
static patch_t *emblemicon;
patch_t *tokenicon;
static patch_t *exiticon;
static patch_t *nopingicon;






static patch_t *crosshair[HU_CROSSHAIRS];  




static void HU_DrawRankings(void);
static void HU_DrawCoopOverlay(void);
static void HU_DrawNetplayCoopOverlay(void);





char *shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"',  
	'(', ')', '*', '+',
	'<',  
	'_',  
	'>',  
	'?',  
	')',  
	'!',  
	'@',  
	'#',  
	'$',  
	'%',  
	'^',  
	'&',  
	'*',  
	'(',  
	':',
	':',  
	'<',
	'+',  
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{',  
	'|',  
	'}',  
	'"', '_',
	'~',  
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

static char cechotext[1024];
static tic_t cechotimer = 0;
static tic_t cechoduration = 5*TICRATE;
static INT32 cechoflags = 0;

static huddrawlist_h luahuddrawlist_scores;





static tic_t resynch_ticker = 0;


static void Command_Say_f(void);
static void Command_Sayto_f(void);
static void Command_Sayteam_f(void);
static void Command_CSay_f(void);
static void Got_Saycmd(UINT8 **p, INT32 playernum);

void HU_LoadGraphics(void)
{
	char buffer[9];
	INT32 i;

	if (dedicated)
		return;


	HU_LoadFontCharacters(&hu_font,   "STCFN");
	HU_LoadFontCharacters(&tny_font,  "TNYFN");
	HU_LoadFontCharacters(&cred_font, "CRFNT");
	HU_LoadFontCharacters(&lt_font,   "LTFNT");
	HU_LoadFontCharacters(&ntb_font,  "NTFNT");
	HU_LoadFontCharacters(&nto_font,  "NTFNO");


	HU_SetFontProperties(&hu_font,   0,  4,  8, 12);
	HU_SetFontProperties(&tny_font,  0,  2,  4, 12);
	HU_SetFontProperties(&cred_font, 0, 16, 16, 16);
	HU_SetFontProperties(&lt_font,   0, 16, 20, 16);
	HU_SetFontProperties(&ntb_font,  2,  4, 20, 21);
	HU_SetFontProperties(&nto_font,  0,  4, 20, 21);


	for (i = 0; i < 10; i++)
	{
		sprintf(buffer, "STTNUM%d", i);
		tallnum[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
		sprintf(buffer, "NGTNUM%d", i);
		nightsnum[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
		sprintf(buffer, "TTL%.2d", i);
		ttlnum[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}


	tallminus = (patch_t *)W_CachePatchName("STTMINUS", PU_HUDGFX);
	tallinfin = (patch_t *)W_CachePatchName("STTINFIN", PU_HUDGFX);



	for (i = 0; i < HU_CROSSHAIRS; i++)
	{
		sprintf(buffer, "CROSHAI%c", '1'+i);
		crosshair[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	emblemicon = W_CachePatchName("EMBLICON", PU_HUDGFX);
	tokenicon = W_CachePatchName("TOKNICON", PU_HUDGFX);
	exiticon = W_CachePatchName("EXITICON", PU_HUDGFX);
	nopingicon = W_CachePatchName("NOPINGICON", PU_HUDGFX);

	emeraldpics[0][0] = W_CachePatchName("CHAOS1", PU_HUDGFX);
	emeraldpics[0][1] = W_CachePatchName("CHAOS2", PU_HUDGFX);
	emeraldpics[0][2] = W_CachePatchName("CHAOS3", PU_HUDGFX);
	emeraldpics[0][3] = W_CachePatchName("CHAOS4", PU_HUDGFX);
	emeraldpics[0][4] = W_CachePatchName("CHAOS5", PU_HUDGFX);
	emeraldpics[0][5] = W_CachePatchName("CHAOS6", PU_HUDGFX);
	emeraldpics[0][6] = W_CachePatchName("CHAOS7", PU_HUDGFX);
	emeraldpics[0][7] = W_CachePatchName("CHAOS8", PU_HUDGFX);

	emeraldpics[1][0] = W_CachePatchName("TEMER1", PU_HUDGFX);
	emeraldpics[1][1] = W_CachePatchName("TEMER2", PU_HUDGFX);
	emeraldpics[1][2] = W_CachePatchName("TEMER3", PU_HUDGFX);
	emeraldpics[1][3] = W_CachePatchName("TEMER4", PU_HUDGFX);
	emeraldpics[1][4] = W_CachePatchName("TEMER5", PU_HUDGFX);
	emeraldpics[1][5] = W_CachePatchName("TEMER6", PU_HUDGFX);
	emeraldpics[1][6] = W_CachePatchName("TEMER7", PU_HUDGFX);


	emeraldpics[2][0] = W_CachePatchName("EMBOX1", PU_HUDGFX);
	emeraldpics[2][1] = W_CachePatchName("EMBOX2", PU_HUDGFX);
	emeraldpics[2][2] = W_CachePatchName("EMBOX3", PU_HUDGFX);
	emeraldpics[2][3] = W_CachePatchName("EMBOX4", PU_HUDGFX);
	emeraldpics[2][4] = W_CachePatchName("EMBOX5", PU_HUDGFX);
	emeraldpics[2][5] = W_CachePatchName("EMBOX6", PU_HUDGFX);
	emeraldpics[2][6] = W_CachePatchName("EMBOX7", PU_HUDGFX);

}

void HU_LoadFontCharacters(fontdef_t *font, const char *prefix)
{
		char buffer[9];
		INT32 i, j = FONTSTART;

		for (i = 0; i < FONTSIZE; i++, j++)
		{
			sprintf(buffer, "%.5s%.3d", prefix, j);
			if (W_CheckNumForPatchName(buffer) == LUMPERROR)
				font->chars[i] = NULL;
			else
				font->chars[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
		}
}

void HU_SetFontProperties(fontdef_t *font, INT32 kerning, UINT32 spacewidth, UINT32 charwidth, UINT32 linespacing)
{
	font->kerning = kerning;
	font->spacewidth = spacewidth;
	font->charwidth = charwidth;
	font->linespacing = linespacing;
}




void HU_Init(void)
{
	COM_AddCommand("say", Command_Say_f, COM_LUA);
	COM_AddCommand("sayto", Command_Sayto_f, COM_LUA);
	COM_AddCommand("sayteam", Command_Sayteam_f, COM_LUA);
	COM_AddCommand("csay", Command_CSay_f, COM_LUA);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);


	shiftxform = english_shiftxform;

	luahuddrawlist_scores = LUA_HUD_CreateDrawList();
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}




void HU_Start(void)
{
	if (headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];

	headsupactive = true;
}







static UINT32 chat_nummsg_log = 0;
static UINT32 chat_nummsg_min = 0;
static UINT32 chat_scroll = 0;
static tic_t chat_scrolltime = 0;

static UINT32 chat_maxscroll = 0;  






#define HU_CHATDISPLAYLEN (HU_MAXMSGLEN + MAXPLAYERNAME + 64)
static char chat_log[CHAT_BUFSIZE][HU_CHATDISPLAYLEN + 1];
static char chat_mini[8][HU_CHATDISPLAYLEN + 1];
static tic_t chat_timers[8];

#define HU_MAX_CHATLINKS 16
#define HU_MAX_CHATLINK_RECTS 512
typedef struct
{
	char url[255];
} chatlink_t;
typedef struct
{
	INT16 x, y, w, h;
	UINT8 link;
} chatlinkrect_t;
static chatlink_t chat_links[HU_MAX_CHATLINKS];
static chatlinkrect_t chat_linkrects[HU_MAX_CHATLINK_RECTS];
static size_t chat_linkcount;
static size_t chat_linkrectcount;

static boolean HU_OpenChatURLAtCursor(void)
{
	INT32 mousex, mousey, basex, basey;
	size_t i;

	if (!chat_linkrectcount || vid.dup <= 0)
		return false;
	I_GetCursorPosition(&mousex, &mousey);
	basex = mousex / vid.dup;
	basey = (mousey - (vid.height - BASEVIDHEIGHT*vid.dup)) / vid.dup;

	for (i = 0; i < chat_linkrectcount; i++)
	{
		chatlinkrect_t *rect = &chat_linkrects[i];
		if (basex >= rect->x && basex < rect->x + rect->w
		 && basey >= rect->y && basey < rect->y + rect->h)
		{
			const char *url = chat_links[rect->link].url;
#ifdef _WIN32
			HINSTANCE opened = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
			if ((INT_PTR)opened > 32)
				return true;
#endif
			I_ClipboardCopy(url, strlen(url));
			HU_AddChatText("\x82""Could not open the link, so it was copied.", false);
			return true;
		}
	}
	return false;
}

static boolean chat_scrollmedown = false;  



static INT16 addy = 0;  

static void HU_removeChatText_Mini(void)
{
	size_t i;
	for (i = 0; i + 1 < chat_nummsg_min; i++)
	{
		memcpy(chat_mini[i], chat_mini[i+1], sizeof chat_mini[i]);
		chat_timers[i] = chat_timers[i+1];
	}
	chat_mini[chat_nummsg_min-1][0] = '\0';
	chat_nummsg_min--;  


	addy += (vid.width < 640) ? 8 : 6;

}


static void HU_removeChatText_Log(void)
{
	size_t i;
	for (i = 0; i + 1 < chat_nummsg_log; i++)
		memcpy(chat_log[i], chat_log[i+1], sizeof chat_log[i]);
	chat_log[chat_nummsg_log-1][0] = '\0';
	chat_nummsg_log--;  
}

void HU_AddChatText(const char *text, boolean playsound)
{
	if (!text)
		return;

	if (playsound && cv_consolechat.value != 2)  
		S_StartSound(NULL, sfx_radio);


	if (chat_nummsg_log >= CHAT_BUFSIZE)  
		HU_removeChatText_Log();

	strlcpy(chat_log[chat_nummsg_log], text, sizeof chat_log[chat_nummsg_log]);
	chat_nummsg_log++;

	if (chat_nummsg_min >= 8)
		HU_removeChatText_Mini();

	strlcpy(chat_mini[chat_nummsg_min], text, sizeof chat_mini[chat_nummsg_min]);
	chat_timers[chat_nummsg_min] = TICRATE*cv_chattime.value;
	chat_nummsg_min++;

	if (OLDCHAT)  
		CONS_Printf("%s\n", text);
	else			 
		CON_LogMessage(va("%s\n", text));
}

                                                    
                                                                       
                                                              
   
                                                                            
                                                                         
                                                                          
   
                                                                              
   
                                                                      
   
                                               
                                                   
                                                                    
                                                                     
                                       
    


static void DoSayCommand(SINT8 target, size_t usedargs, UINT8 flags)
{
	char buf[2 + HU_MAXMSGLEN + 1];
	size_t numwords, ix;
	char *msg = &buf[2];
	const size_t msgspace = sizeof buf - 2;

	numwords = COM_Argc() - usedargs;
	I_Assert(numwords > 0);

	if (CHAT_MUTE)
	{
		if (cv_mute.value)
			HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"), false);
		else
			HU_AddChatText(va("%s>ERROR: You have been muted. You can't say anything.", "\x85"), false);
		return;
	}


	if(!server && !(IsPlayerAdmin(consoleplayer)))
		flags &= ~HU_CSAY;


	flags &= ~HU_SERVER_SAY;
	if(dedicated && !(flags & HU_CSAY))
		flags |= HU_SERVER_SAY;

	buf[0] = target;
	buf[1] = flags;
	msg[0] = '\0';

	for (ix = 0; ix < numwords; ix++)
	{
		if (ix > 0)
			strlcat(msg, " ", msgspace);
		strlcat(msg, COM_Argv(ix + usedargs), msgspace);
	}

	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0)  
	{


		const char *newmsg;
		char playernum[3+1];
		INT32 spc = 1;  

		strncpy(playernum, msg+3, sizeof(playernum)-1);

		if (((playernum[0] < '0') || (playernum[0] > '9')) || ((playernum[1] < '0') || (playernum[1] > '9')))
		{

			if (playernum[1] == ' ')
				spc = 0;

			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<playernum> \'.", false);
				return;
			}
		}

		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<playernum> \'.", false);
			return;
		}

		target = atoi(playernum);  



		if (target < MAXPLAYERS && playeringame[target])  
			target++;  
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false);  
			return;
		}
		buf[0] = target;
		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}

	SendNetXCmd(XD_SAY, buf, strlen(msg) + 1 + msg-buf);
}

                               
                                                        
                                       
    
static void Command_Say_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("say <message>: send a message\n"));
		return;
	}

	DoSayCommand(0, 1, 0);
}

                                          
                                                      
                                       
    
static void Command_Sayto_f(void)
{
	INT32 target;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("sayto <playername|playernum> <message>: send a message to a player\n"));
		return;
	}

	target = nametonum(COM_Argv(1));
	if (target == -1)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("No player with that name!\n"));
		return;
	}
	target++;  

	DoSayCommand((SINT8)target, 2, 0);
}

                                                   
                                                    
                                       
    
static void Command_Sayteam_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("sayteam <message>: send a message to your team\n"));
		return;
	}

	if (dedicated)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Dedicated servers can't send team messages. Use \"say\".\n"));
		return;
	}

	DoSayCommand(-1, 1, 0);
}

                                                              
                                    
    
static void Command_CSay_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("csay <message>: send a message to be shown in the middle of the screen\n"));
		return;
	}

	if(!server && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Only servers and admins can use csay.\n"));
		return;
	}

	DoSayCommand(0, 1, HU_CSAY);
}

UINT8 spam_tokens[MAXPLAYERS] = { 1 };  
tic_t spam_tics[MAXPLAYERS];

static const char *GetChatColorFromSkinColor(INT32 skincolor)
{
	const char *textcolor = NULL;
	UINT16 chatcolor = skincolors[skincolor].chatcolor;
	if (!chatcolor || chatcolor%0x1000 || chatcolor>V_INVERTMAP)
		textcolor = "\x80";
	else if (chatcolor == V_MAGENTAMAP)
		textcolor = "\x81";
	else if (chatcolor == V_YELLOWMAP)
		textcolor = "\x82";
	else if (chatcolor == V_GREENMAP)
		textcolor = "\x83";
	else if (chatcolor == V_BLUEMAP)
		textcolor = "\x84";
	else if (chatcolor == V_REDMAP)
		textcolor = "\x85";
	else if (chatcolor == V_GRAYMAP)
		textcolor = "\x86";
	else if (chatcolor == V_ORANGEMAP)
		textcolor = "\x87";
	else if (chatcolor == V_SKYMAP)
		textcolor = "\x88";
	else if (chatcolor == V_PURPLEMAP)
		textcolor = "\x89";
	else if (chatcolor == V_AQUAMAP)
		textcolor = "\x8a";
	else if (chatcolor == V_PERIDOTMAP)
		textcolor = "\x8b";
	else if (chatcolor == V_AZUREMAP)
		textcolor = "\x8c";
	else if (chatcolor == V_BROWNMAP)
		textcolor = "\x8d";
	else if (chatcolor == V_ROSYMAP)
		textcolor = "\x8e";
	else if (chatcolor == V_INVERTMAP)
		textcolor = "\x8f";
	return textcolor;
}

                                                       
                    
                                       
    
static void Got_Saycmd(UINT8 **p, INT32 playernum)
{
	SINT8 target;
	UINT8 flags;
	const char *dispname;
	char buf[HU_MAXMSGLEN + 1];
	char *msg;
	boolean action = false;
	char *ptr;
	INT32 spam_eatmsg = 0;

	CONS_Debug(DBG_NETPLAY,"Received SAY cmd from Player %d (%s)\n", playernum+1, player_names[playernum]);

	target = READSINT8(*p);
	flags = READUINT8(*p);
	msg = buf;
	READSTRINGL(*p, msg, HU_MAXMSGLEN + 1);

	if ((cv_mute.value || players[playernum].muted || flags & (HU_CSAY|HU_SERVER_SAY)) && playernum != serverplayer && !(IsPlayerAdmin(playernum)))
	{
		CONS_Alert(CONS_WARNING, (cv_mute.value || players[playernum].muted) ?
			M_GetText("Illegal say command received from %s while muted\n") : M_GetText("Illegal csay command received from non-admin %s\n"),
			player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}


	{
		size_t i;
		const size_t j = strlen(msg);
		for (i = 0; i < j; i++)
		{
			if (msg[i] & 0x80)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Illegal say command received from %s containing invalid characters\n"), player_names[playernum]);
				if (server)
					SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
				return;
			}
		}
	}




	if (spam_tokens[playernum] <= 0 && cv_chatspamprotection.value && !(flags & HU_CSAY))
	{
		CONS_Debug(DBG_NETPLAY,"Received SAY cmd too quickly from Player %d (%s), assuming as spam and blocking message.\n", playernum+1, player_names[playernum]);
		spam_tics[playernum] = 0;
		spam_eatmsg = 1;
	}
	else
		spam_tokens[playernum] -= 1;

	if (spam_eatmsg)
		return;  

	if (LUA_HookPlayerMsg(playernum, target, flags, msg))
		return;


	if (flags & HU_CSAY)
	{
		HU_SetCEchoDuration(5);
		I_OutputMsg("Server message: ");
		HU_DoCEcho(msg);
		return;
	}


	if (target == 0 && strlen(msg) > 4 && strnicmp(msg, "/me ", 4) == 0)
	{
		msg += 4;
		action = true;
	}

	if (flags & HU_SERVER_SAY)
		dispname = "SERVER";
	else
		dispname = player_names[playernum];






	ptr = msg;
	while (*ptr != '\0')
	{
		if (*ptr == '\r' || *ptr == '\n')
			*ptr = ' ';

		ptr++;
	}


	if (playernum == consoleplayer  
	|| (target == -1 && ST_SameTeam(&players[consoleplayer], &players[playernum]))  
	|| target == 0  
	|| consoleplayer == target-1)  
	{
		const char *prefix = "", *cstart = "", *cend = "", *adminchar = "\x82~\x83", *remotechar = "\x82@\x83", *fmt2, *textcolor = "\x80";
		char *tempchar = NULL;


        if (players[playernum].spectator)
		{
			cstart = "\x86";     
			textcolor = "\x86";
		}
		else if (target == -1)  
		{
			if (players[playernum].ctfteam == 1)  
			{
				cstart = textcolor = GetChatColorFromSkinColor(skincolor_redteam);
			}
			else  
			{
				cstart = textcolor = GetChatColorFromSkinColor(skincolor_blueteam);
			}
		}
		else
        {
			cstart = GetChatColorFromSkinColor(players[playernum].skincolor);
			if (G_GametypeHasTeams())
			{
				if (players[playernum].ctfteam == 1)  
				{
					cstart = GetChatColorFromSkinColor(skincolor_redteam);
				}
				else  
				{
					cstart = GetChatColorFromSkinColor(skincolor_blueteam);
				}
			}
        }
		prefix = cstart;


		if (playernum == serverplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(adminchar) + 1, PU_STATIC, NULL);
		else if (IsPlayerAdmin(playernum))
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(remotechar) + 1, PU_STATIC, NULL);
		if (tempchar)
		{
			if (playernum == serverplayer)
				strcat(tempchar, adminchar);
			else
				strcat(tempchar, remotechar);
			strcat(tempchar, cstart);
			cstart = tempchar;
		}





		if (action)
			fmt2 = "* %s%s%s%s \x82%s%s";
		else if (target-1 == consoleplayer)  
		{
			prefix = "\x82[PM]";
			cstart = "\x82";
			textcolor = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";
		}
		else if (target > 0)  
		{

			dispname = player_names[target-1];
			prefix = "\x82[TO]";
			cstart = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";

		}
		else if (target == 0)  
			fmt2 = "%s<%s%s%s>\x80 %s%s";
		else  
		{
			if (players[playernum].ctfteam == 1)  
				prefix = "\x85[TEAM]";
			else if (players[playernum].ctfteam == 2)  
				prefix = "\x84[TEAM]";
			else
				prefix = "\x83";  

			fmt2 = "%s<%s%s>\x80%s %s%s";
		}

		HU_AddChatText(va(fmt2, prefix, cstart, dispname, cend, textcolor, msg), cv_chatnotifications.value);  

		if (tempchar)
			Z_Free(tempchar);
	}
#ifdef _DEBUG




	else
		CONS_Printf("Dropped chat: %d %d %s\n", playernum, target, msg);
#endif
}



void HU_Ticker(void)
{

	if (netgame)
	{
		size_t i = 0;


		for(; (i<MAXPLAYERS); i++)
		{
			if (spam_tokens[i] < (tic_t)cv_chatspamburst.value)
			{
				if (++spam_tics[i] >= (tic_t)cv_chatspamspeed.value)
				{
					spam_tokens[i]++;
					spam_tics[i] = 0;
				}
			}
		}
	}

	if (dedicated)
		return;

	hu_tick++;
	hu_tick &= 7;  

	if (PLAYER1INPUTDOWN(GC_SCORES))
		hu_showscores = !chat_on;
	else
		hu_showscores = false;

	if (chat_on)
	{

		if (chat_scrolltime > 0)
			chat_scrolltime--;
	}

	if (netgame)
	{
		size_t i = 0;


		for (i=0; (i<chat_nummsg_min); i++)
		{
			if (chat_timers[i] > 0)
				chat_timers[i]--;
			else
				HU_removeChatText_Mini();
		}
	}

	if (cechotimer > 0) --cechotimer;

	if (hu_redownloadinggamestate)
		resynch_ticker++;
}

static boolean teamtalk = false;
static boolean justscrolleddown;
static boolean justscrolledup;
static INT16 typelines = 1;  


static boolean HU_chatboxContainsOnlySpaces(void)
{
	size_t i;

	for (i = 0; w_chat[i]; i++)
		if (w_chat[i] != ' ')
			return false;

	return true;
}

static void HU_sendChatMessage(void)
{
	char buf[2 + HU_MAXMSGLEN + 1];
	char *msg = &buf[2];
	size_t ci;
	INT32 target = 0;


	if (HU_chatboxContainsOnlySpaces())
		return;


	for (ci = 2; w_chat[ci-2]; ci++)
	{
		char c = w_chat[ci-2];
		if (c >= ' ' && !(c & 0x80))
			buf[ci] = c;
	};
	buf[ci] = '\0';

	memset(w_chat, '\0', sizeof(w_chat));
	c_input = 0;


	if (CHAT_MUTE)
	{
		if (cv_mute.value)
			HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"), false);
		else
			HU_AddChatText(va("%s>ERROR: You have been muted. You can't say anything.", "\x85"), false);
		return;
	}

	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0)  
	{
		INT32 spc = 1;  
		char playernum[3+1];
		const char *newmsg;





		if (teamtalk)
		{
			HU_AddChatText(va("%sCannot send sayto in Say-Team.", "\x85"), false);
			return;
		}

		strncpy(playernum, msg+3, sizeof(playernum)-1);

		if (!(isdigit(playernum[0]) && isdigit(playernum[1])))
		{

			if (playernum[1] == ' ')
				spc = 0;

			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
				return;
			}
		}

		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
			return;
		}

		target = atoi(playernum);  


		if (target < MAXPLAYERS && playeringame[target])  
			target++;  
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false);  
			return;
		}


		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}
	if (ci > 2)  
	{
		buf[0] = teamtalk ? -1 : target;  
		buf[1] = 0;  
		SendNetXCmd(XD_SAY, buf, 2 + strlen(&buf[2]) + 1);
	}
}

void HU_clearChatChars(void)
{
	memset(w_chat, '\0', sizeof(w_chat));
	I_SetTextInputMode(false);
	chat_on = false;
	c_input = 0;

	I_UpdateMouseGrab();
}




boolean HU_Responder(event_t *ev)
{
	INT32 c=0;

	if (ev->type != ev_keydown && ev->type != ev_text)
		return false;



	                                                                                        
                                                                      
                                                                                        
                                  
                                                                                                                           
                                                                                            

                           
  
          
                                        
   
                                                                    
          
   

                            
                
    	 

	c = (INT32)ev->key;

	if (!chat_on)
	{
		if (ev->type == ev_text)
			return false;


		if ((ev->key == gamecontrol[GC_TALKKEY][0] || ev->key == gamecontrol[GC_TALKKEY][1])
			&& netgame && !OLD_MUTE)  
		{
			I_SetTextInputMode(true);
			chat_on = true;
			chat_on_first_event = false;
			w_chat[0] = 0;
			teamtalk = false;
			chat_scrollmedown = true;
			typelines = 1;
			I_SetMouseGrab(false);
			return true;
		}
		if ((ev->key == gamecontrol[GC_TEAMKEY][0] || ev->key == gamecontrol[GC_TEAMKEY][1])
			&& netgame && !OLD_MUTE)
		{
			I_SetTextInputMode(true);
			chat_on = true;
			chat_on_first_event = false;
			w_chat[0] = 0;
			teamtalk = G_GametypeHasTeams();  
			chat_scrollmedown = true;
			typelines = 1;
			I_SetMouseGrab(false);
			return true;
		}
	}
	else  
	{
		if (!chat_on_first_event)
		{



			chat_on_first_event = true;
			return true;
		}

		if (ev->type == ev_text)
		{
			if ((c < FONTSTART || c > FONTEND || !hu_font.chars[c-FONTSTART])
				&& c != ' ')  
			{
				return false;
			}

			if (CHAT_MUTE || strlen(w_chat) >= HU_MAXMSGLEN)
				return true;

			memmove(&w_chat[c_input + 1], &w_chat[c_input], strlen(w_chat) - c_input + 1);
			w_chat[c_input] = c;
			c_input++;
			return true;
		}




		if (ev->key == KEY_LSHIFT || ev->key == KEY_RSHIFT
		 || ev->key == KEY_LCTRL || ev->key == KEY_RCTRL
		 || ev->key == KEY_LALT || ev->key == KEY_RALT)
			return true;

		if (c == KEY_MOUSE1)
		{
			HU_OpenChatURLAtCursor();
			return true;
		}


		if (c == 'v' && ctrldown)
		{
			const char *paste;
			size_t chatlen;
			size_t pastelen;

			if (CHAT_MUTE)
				return true;

			paste = I_ClipboardPaste();
			if (paste == NULL)
				return true;

			chatlen = strlen(w_chat);
			pastelen = strlen(paste);
			if (chatlen+pastelen > HU_MAXMSGLEN)
				return true;  

			memmove(&w_chat[c_input + pastelen], &w_chat[c_input], (chatlen - c_input) + 1);  
			memcpy(&w_chat[c_input], paste, pastelen);  
			c_input += pastelen;
			return true;
		}
		else if (c == KEY_ENTER)
		{
			if (!CHAT_MUTE)
				HU_sendChatMessage();

			I_SetTextInputMode(false);
			chat_on = false;
			c_input = 0;  
			chat_scrollmedown = true;  
			I_UpdateMouseGrab();
		}
		else if (c == KEY_ESCAPE
			|| ((c == gamecontrol[GC_TALKKEY][0] || c == gamecontrol[GC_TALKKEY][1]
			|| c == gamecontrol[GC_TEAMKEY][0] || c == gamecontrol[GC_TEAMKEY][1])
			&& c >= KEY_MOUSE1))  
		{
			I_SetTextInputMode(false);
			chat_on = false;
			c_input = 0;  
			I_UpdateMouseGrab();
		}
		else if ((c == KEY_UPARROW || c == KEY_MOUSEWHEELUP) && chat_scroll > 0 && !OLDCHAT)  
		{
			chat_scroll--;
			justscrolledup = true;
			chat_scrolltime = 4;
		}
		else if ((c == KEY_DOWNARROW || c == KEY_MOUSEWHEELDOWN) && chat_scroll < chat_maxscroll && chat_maxscroll > 0 && !OLDCHAT)
		{
			chat_scroll++;
			justscrolleddown = true;
			chat_scrolltime = 4;
		}
		else if (c == KEY_LEFTARROW && c_input != 0 && !OLDCHAT)  
		{
			if (ctrldown)
				c_input = M_JumpWordReverse(w_chat, c_input);
			else
				c_input--;
		}
		else if (c == KEY_RIGHTARROW && c_input < strlen(w_chat) && !OLDCHAT)  
		{
			if (ctrldown)
				c_input += M_JumpWord(&w_chat[c_input]);
			else
				c_input++;
		}
		else if (c == KEY_BACKSPACE)
		{
			if (CHAT_MUTE || c_input <= 0)
				return true;

			memmove(&w_chat[c_input - 1], &w_chat[c_input], strlen(w_chat) - c_input + 1);
			c_input--;
		}
		else if (c == KEY_DEL)
		{
			if (CHAT_MUTE || c_input >= strlen(w_chat))
				return true;

			memmove(&w_chat[c_input], &w_chat[c_input + 1], strlen(w_chat) - c_input);
		}

		return true;
	}

	return false;
}








INT16 chatx = 13, chaty = 169;  



static void HU_drawMiniChat(void)
{
	INT32 x = chatx+2, y;
	INT32 chatheight = 0;
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 dx = 0, dy = 0;
	boolean prev_linereturn = false;

	if (!chat_nummsg_min)
		return;  

	for (size_t i = chat_nummsg_min; i > 0; i--)
	{
		char *msg = V_ChatWordWrap(0, boxw-charwidth-2, V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE|V_MONOSPACE, chat_mini[i-1]);
		for(size_t j = 0; msg[j]; j++)  
		{
			if (msg[j] == '\n')  
			{
				if (!prev_linereturn)
				{
					chatheight += charheight;
					dx = 0;
				}
				prev_linereturn = true;
			}
			else if (msg[j] >= FONTSTART)
			{
				prev_linereturn = false;

				dx += charwidth;

				if (dx >= boxw-charwidth-2)
				{
					dx = 0;
					chatheight += charheight;
					prev_linereturn = true;
				}
			}
		}
		dx = 0;
		chatheight += charheight;

		if (msg)
			Z_Free(msg);
	}

	y = chaty - (chatheight + charheight);
	prev_linereturn = false;

	for (size_t i = 0; i < chat_nummsg_min; i++)  
	{
		INT32 timer = ((cv_chattime.value*TICRATE)-chat_timers[i]) - cv_chattime.value*TICRATE+9;  
		INT32 transflag = (timer >= 0 && timer <= 9) ? (timer*V_10TRANS) : 0;  
		char *msg = V_ChatWordWrap(0, boxw-charwidth-2, V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE|V_MONOSPACE, chat_mini[i]);  
		UINT8 *colormap = NULL;

		for(size_t j = 0; msg[j]; j++)  
		{
			if (msg[j] == '\n')  
			{
				if (!prev_linereturn)
				{
					dy += charheight;
					dx = 0;
				}
				prev_linereturn = true;
			}
			else if (msg[j] & 0x80)  
				colormap = V_GetStringColormap(((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK);
			else if (msg[j] >= FONTSTART)
			{
				prev_linereturn = false;

				if (cv_chatbacktint.value)  
					V_DrawFillConsoleMap(x + dx + 2, y+dy, charwidth, charheight, 239|V_SNAPTOBOTTOM|V_SNAPTOLEFT);

				V_DrawChatCharacter(x + dx + 2, y+dy, msg[j] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_MONOSPACE|transflag, true, colormap);
				dx += charwidth;

				if (dx >= boxw-charwidth-2)
				{
					dx = 0;
					dy += charheight;
					prev_linereturn = true;
				}
			}
		}
		dy += charheight;
		dx = 0;

		if (msg)
			Z_Free(msg);
	}


	addy /= 2;
}



static void HU_drawChatLog(INT32 offset)
{
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value, boxh = cv_chatheight.value;
	INT32 x = chatx+2, y, dx = 0, dy = 0;
	UINT32 i = 0;
	INT32 chat_topy, chat_bottomy;
	boolean atbottom = false;
	boolean prev_linereturn = false;


	if (chat_scroll > chat_maxscroll)
		chat_scroll = chat_maxscroll;

#ifdef NETSPLITSCREEN
	if (splitscreen)
	{
		boxh = max(6, boxh/2);
		if (splitscreen > 1)
			boxw = max(64, boxw/2);
	}
#endif

	y = chaty - offset*charheight - (chat_scroll*charheight) - boxh*charheight - 12;

#ifdef NETSPLITSCREEN
	if (splitscreen)
	{
		y -= BASEVIDHEIGHT/2;


	}
#endif

	chat_topy = y + chat_scroll*charheight;
	chat_bottomy = chat_topy + boxh*charheight;

	V_DrawFillConsoleMap(chatx, chat_topy, boxw, boxh*charheight +2, 239|V_SNAPTOBOTTOM|V_SNAPTOLEFT);  

	for (i=0; i<chat_nummsg_log; i++)  
	{
		char *msg = V_ChatWordWrap(0, boxw-charwidth-2, V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE|V_MONOSPACE, chat_log[i]);  
		UINT8 *colormap = NULL;
		INT32 activelink = -1;
		size_t linkend = 0;
		for(size_t j = 0; msg[j]; j++)  
		{
			if (j >= linkend)
				activelink = -1;
			if (activelink < 0 && chat_linkcount < HU_MAX_CHATLINKS
			 && (!strnicmp(&msg[j], "https://", 8) || !strnicmp(&msg[j], "http://", 7)))
			{
				size_t k, length = 0;
				chatlink_t *link = &chat_links[chat_linkcount];
				for (k = j; msg[k]; k++)
				{
					if (msg[k] & 0x80)
						break;
					if (msg[k] == '\n')
						continue;
					if (msg[k] <= ' ')
						break;
					if (length+1 < sizeof link->url)
						link->url[length++] = msg[k];
				}
				while (length && strchr(".,;:!?)]}>\"'", link->url[length-1]))
					length--;
				link->url[length] = '\0';
				if (length)
				{
					activelink = (INT32)chat_linkcount++;
					linkend = k;
				}
			}
			if (msg[j] == '\n')  
			{
				if (!prev_linereturn)
				{
					dy += charheight;
					dx = 0;
				}
				prev_linereturn = true;
			}
			else if (msg[j] & 0x80)  
				colormap = V_GetStringColormap(((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK);
			else
			{
				prev_linereturn = false;

				if (msg[j] >= FONTSTART)
				{
					if ((y+dy+2 >= chat_topy) && (y+dy < (chat_bottomy)))
					{
						UINT8 *drawmap = activelink >= 0 ? V_GetStringColormap(V_AQUAMAP) : colormap;
						V_DrawChatCharacter(x + dx + 2, y+dy+2, msg[j] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_MONOSPACE, true, drawmap);
						if (activelink >= 0 && chat_linkrectcount < HU_MAX_CHATLINK_RECTS)
						{
							chatlinkrect_t *rect = &chat_linkrects[chat_linkrectcount++];
							rect->x = (INT16)(x + dx + 2);
							rect->y = (INT16)(y + dy + 2);
							rect->w = (INT16)charwidth;
							rect->h = (INT16)charheight;
							rect->link = (UINT8)activelink;
						}
					}

					dx += charwidth;
				}

				if (dx >= boxw-charwidth-2 && i < chat_nummsg_log)  
				{
					dx = 0;
					dy += charheight;
					prev_linereturn = true;
				}
			}
		}
		dy += charheight;
		dx = 0;

		if (msg)
			Z_Free(msg);
	}

	if (((chat_scroll >= chat_maxscroll) || (chat_scrollmedown)) && !(justscrolleddown || justscrolledup || chat_scrolltime))  
		atbottom = true;  

	chat_scrollmedown = false;


	chat_maxscroll = max(dy / charheight - cv_chatheight.value, 0);


	if (atbottom)
		chat_scroll = chat_maxscroll;


	if (chat_scroll > 0)
		V_DrawThinString(chatx-8, ((justscrolledup) ? (chat_topy-1) : (chat_topy)) - 1, V_SNAPTOBOTTOM | V_SNAPTOLEFT | V_YELLOWMAP, "\x1A");  
	if (chat_scroll < chat_maxscroll)
		V_DrawThinString(chatx-8, chat_bottomy-((justscrolleddown) ? 5 : 6) - 1, V_SNAPTOBOTTOM | V_SNAPTOLEFT | V_YELLOWMAP, "\x1B");  

	justscrolleddown = justscrolledup = false;
}







static void HU_DrawChat(void)
{
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 t = 0, c = 0, y = chaty - (typelines*charheight);
	UINT32 i = 0, saylen = strlen(w_chat);  
	INT32 cflag = 0;
	const char *ntalk = "Say: ", *ttalk = "Team: ";
	const char *talk = ntalk;

	chat_linkcount = 0;
	chat_linkrectcount = 0;

#ifdef NETSPLITSCREEN
	if (splitscreen)
	{
		y -= BASEVIDHEIGHT/2;
		if (splitscreen > 1)
		{
			y += 16;
			boxw = max(64, boxw/2);
		}
	}
#endif

	if (teamtalk)
		talk = ttalk;

	if (CHAT_MUTE)
	{
		if (cv_mute.value)
			talk = "Chat has been muted.";
		else
			talk = "You have been muted.";
		typelines = 1;
		cflag = V_GRAYMAP;  
	}

	V_DrawFillConsoleMap(chatx, y-1, boxw, (typelines*charheight), 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

	for (i = 0; talk[i]; i++)
	{
		if (talk[i] >= FONTSTART)
			V_DrawChatCharacter(chatx + c + 2, y, talk[i] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|cflag, true, V_GetStringColormap(talk[i]|cflag));
		c += charwidth;
	}


	if (CHAT_MUTE)
	{
		HU_drawChatLog(0);
		return;
	}

	typelines = 1;

	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawChatCharacter(chatx + 2 + c, y+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, true, NULL);

	for (i = 0; w_chat[i]; i++)
	{
		boolean skippedline = false;
		if (c_input == (i+1))
		{
			INT32 cursorx = (c+charwidth < boxw-charwidth) ? (chatx + 2 + c+charwidth) : (chatx+1);  
			INT32 cursory = (cursorx != chatx+1) ? (y) : (y+charheight);
			if (hu_tick < 4)
				V_DrawChatCharacter(cursorx, cursory+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, true, NULL);

			if (cursorx == chatx+1 && saylen == i)  
			{
				typelines += 1;
				skippedline = true;
			}
		}

		if (w_chat[i] >= FONTSTART)
			V_DrawChatCharacter(chatx + c + 2, y, w_chat[i] | V_SNAPTOBOTTOM|V_SNAPTOLEFT | t, true, NULL);

		c += charwidth;
		if (c > boxw-charwidth && !skippedline)
		{
			c = 0;
			y += charheight;
			typelines += 1;
		}
	}


	if (strnicmp(w_chat, "/pm", 3) == 0 && vid.width >= 400 && !teamtalk)  
	{
		INT32 count = 0;
		INT32 p_dispy = chaty - charheight -1;
#ifdef NETSPLITSCREEN
		if (splitscreen)
		{
			p_dispy -= BASEVIDHEIGHT/2;
			if (splitscreen > 1)
				p_dispy += 16;
		}
#endif

		for(i=0; i<MAXPLAYERS; i++)
		{

			if (w_chat[3])
			{
				char playernum[3+1];
				UINT32 n;

				if ( ( ((w_chat[3] != 0) && ((w_chat[3] < '0') || (w_chat[3] > '9'))) || ((w_chat[4] != 0) && (((w_chat[4] < '0') || (w_chat[4] > '9'))))) && (w_chat[4] != ' '))
					break;

				strncpy(playernum, w_chat+3, sizeof(playernum)-1);
				playernum[3] = 0;
				n = atoi(playernum);  

				if ((n == 0) && !(w_chat[4] == '0') && (!(i<10)))
					continue;
				else if ((n == 1) && !(w_chat[3] == '0') && (!((i == 1) || ((i >= 10) && (i <= 19)))))
					continue;
				else if ((n == 2) && !(w_chat[3] == '0') && (!((i == 2) || ((i >= 20) && (i <= 29)))))
					continue;
				else if ((n == 3) && !(w_chat[3] == '0') && (!((i == 3) || ((i >= 30) && (i <= 31)))))
					continue;
				else	 
					if (i != n) continue;
			}

			if (playeringame[i])
			{
				char name[MAXPLAYERNAME+1];
				strlcpy(name, player_names[i], 7);  
				V_DrawFillConsoleMap(chatx+ boxw + 2, p_dispy- (6*count), 48, 6, 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);  
				V_DrawSmallString(chatx+ boxw + 4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, va("\x82%d\x80 - %s", i, name));
				count++;
			}
		}
		if (count == 0)  
		{
			V_DrawFillConsoleMap(chatx+boxw+2, p_dispy- (6*count), 48, 6, 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);  
			V_DrawSmallString(chatx+boxw+4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, "NO RESULT.");
		}
	}

	HU_drawChatLog(typelines-1);  
}




static void HU_DrawChat_Old(void)
{
	INT32 t = 0, c = 0, y = HU_INPUTY;
	size_t i = 0;
	const char *ntalk = "Say: ", *ttalk = "Say-Team: ";
	const char *talk = ntalk;
	INT32 charwidth = 8 * con_scalefactor, charheight = 8 * con_scalefactor;
	if (teamtalk)
		talk = ttalk;

	for (i = 0; talk[i]; i++)
	{
		if (talk[i] >= FONTSTART)
			V_DrawCharacter(HU_INPUTX + c, y, talk[i] | cv_constextsize.value | V_NOSCALESTART, true);
		c += charwidth;
	}

	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawCharacter(HU_INPUTX+c, y+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, true);

	for (i = 0; w_chat[i]; i++)
	{
		if (c_input == (i+1) && hu_tick < 4)
		{
			INT32 cursorx = (HU_INPUTX+c+charwidth < vid.width) ? (HU_INPUTX + c + charwidth) : (HU_INPUTX);  
			INT32 cursory = (cursorx != HU_INPUTX) ? (y) : (y+charheight);
			V_DrawCharacter(cursorx, cursory+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, true);
		}

		if (w_chat[i] >= FONTSTART)
			V_DrawCharacter(HU_INPUTX + c, y, w_chat[i] | cv_constextsize.value | V_NOSCALESTART | t, true);

		c += charwidth;
		if (c >= vid.width)
		{
			c = 0;
			y += charheight;
		}
	}
}





static inline void HU_DrawCrosshairs(void)
{
	INT32 cross1 = cv_crosshair.value & 3;
	INT32 cross2 = cv_crosshair2.value & 3;

	if (automapactive || demoplayback)
		return;

	stplyr = ((stplyr == &players[displayplayer]) ? &players[secondarydisplayplayer] : &players[displayplayer]);
	if (!players[displayplayer].spectator && (!camera.chase || ticcmd_ztargetfocus[0]) && cross1)
		V_DrawStretchyFixedPatch((BASEVIDWIDTH/2)<<FRACBITS, (BASEVIDHEIGHT/2)<<FRACBITS, FRACUNIT, splitscreen ? 2*FRACUNIT : FRACUNIT, V_TRANSLUCENT|V_PERPLAYER, crosshair[cross1 - 1], NULL);

	stplyr = ((stplyr == &players[displayplayer]) ? &players[secondarydisplayplayer] : &players[displayplayer]);
	if (!players[secondarydisplayplayer].spectator && (!camera2.chase || ticcmd_ztargetfocus[1]) && cross2 && splitscreen)
		V_DrawStretchyFixedPatch((BASEVIDWIDTH/2)<<FRACBITS, (BASEVIDHEIGHT/2)<<FRACBITS, FRACUNIT, 2*FRACUNIT, V_TRANSLUCENT|V_PERPLAYER, crosshair[cross2 - 1], NULL);
}

static void HU_DrawCEcho(void)
{
	INT32 i = 0;
	INT32 y = (BASEVIDHEIGHT/2)-4;
	INT32 pnumlines = 0;

	UINT32 realflags = cechoflags|V_PERPLAYER;  
	INT32 realalpha = (INT32)((cechoflags & V_ALPHAMASK) >> V_ALPHASHIFT);

	char *line;
	char *echoptr;
	char temp[1024];

	for (i = 0; cechotext[i] != '\0'; ++i)
		if (cechotext[i] == '\\')
			pnumlines++;

	y -= (pnumlines-1)*((realflags & V_RETURN8) ? 4 : 6);


	if (y < 0)
	{
		CONS_Alert(CONS_WARNING, "CEcho contained too many lines, not displaying\n");
		cechotimer = 0;
		return;
	}


	if (realflags & V_AUTOFADEOUT)
	{
		UINT32 tempalpha = (UINT32)max((INT32)(10 - cechotimer), realalpha);

		realflags &= ~V_ALPHASHIFT;
		realflags |= (tempalpha << V_ALPHASHIFT);
	}

	strcpy(temp, cechotext);
	echoptr = &temp[0];

	while (*echoptr != '\0')
	{
		line = strchr(echoptr, '\\');

		if (line == NULL)
			break;

		*line = '\0';

		V_DrawCenteredString(BASEVIDWIDTH/2, y, realflags, echoptr);
		if (splitscreen)
		{
			stplyr = ((stplyr == &players[displayplayer]) ? &players[secondarydisplayplayer] : &players[displayplayer]);
			V_DrawCenteredString(BASEVIDWIDTH/2, y, realflags, echoptr);
			stplyr = ((stplyr == &players[displayplayer]) ? &players[secondarydisplayplayer] : &players[displayplayer]);
		}
		y += ((realflags & V_RETURN8) ? 8 : 12);

		echoptr = line;
		echoptr++;
	}
}




UINT32 hu_demoscore;
UINT32 hu_demotime;
UINT16 hu_demorings;

static void HU_DrawDemoInfo(void)
{
	INT32 h = 188;
	if (modeattacking == ATTACKING_NIGHTS)
		h -= 12;

	V_DrawString(4, h-24, V_YELLOWMAP|V_ALLOWLOWERCASE, va(M_GetText("%s's replay"), player_names[0]));
	if (modeattacking)
	{
		V_DrawString(4, h-16, V_YELLOWMAP|V_MONOSPACE, "SCORE:");
		V_DrawRightAlignedString(120, h-16, V_MONOSPACE, va("%d", hu_demoscore));

		V_DrawString(4, h-8, V_YELLOWMAP|V_MONOSPACE, "TIME:");
		if (hu_demotime != UINT32_MAX)
			V_DrawRightAlignedString(120, h-8, V_MONOSPACE, va("%i:%02i.%02i",
				G_TicsToMinutes(hu_demotime,true),
				G_TicsToSeconds(hu_demotime),
				G_TicsToCentiseconds(hu_demotime)));
		else
			V_DrawRightAlignedString(120, h-8, V_MONOSPACE, "--:--.--");

		if (modeattacking == ATTACKING_RECORD)
		{
			V_DrawString(4, h, V_YELLOWMAP|V_MONOSPACE, "RINGS:");
			V_DrawRightAlignedString(120, h, V_MONOSPACE, va("%d", hu_demorings));
		}
	}
}



void HU_Drawer(void)
{

	if (chat_on)
	{
		if (!OLDCHAT)
			HU_DrawChat();
		else
			HU_DrawChat_Old();
	}
	else
	{
		typelines = 1;
		chat_scrolltime = 0;

		if (!OLDCHAT && cv_consolechat.value < 2 && netgame)  
			HU_drawMiniChat();  
	}

	if (cechotimer)
		HU_DrawCEcho();

	if (demoplayback && hu_showscores)
		HU_DrawDemoInfo();

	if (!Playing()
	 || gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE
	 || gamestate == GS_CREDITS      || gamestate == GS_EVALUATION
	 || gamestate == GS_ENDING       || gamestate == GS_GAMEEND)
		return;


	if (hu_showscores)
	{
		if (netgame || multiplayer)
		{
			if (LUA_HudEnabled(hud_rankings))
				HU_DrawRankings();
			if (gametyperules & GTR_CAMPAIGN)
				HU_DrawNetplayCoopOverlay();
		}
		else
			HU_DrawCoopOverlay();

		if (renderisnewtic)
		{
			LUA_HUD_ClearDrawList(luahuddrawlist_scores);
			LUA_HUDHOOK(scores, luahuddrawlist_scores);
		}
		LUA_HUD_DrawList(luahuddrawlist_scores);
	}

	if (gamestate != GS_LEVEL)
		return;


	if (LUA_HudEnabled(hud_crosshair))
		HU_DrawCrosshairs();


	if (hu_redownloadinggamestate)
	{
		char resynch_text[14];
		UINT32 i;

		strcpy(resynch_text, "Resynching");
		for (i = 0; i < (resynch_ticker / 16) % 4; i++)
			strcat(resynch_text, ".");

		V_DrawCenteredString(BASEVIDWIDTH/2, 180, V_YELLOWMAP | V_ALLOWLOWERCASE, resynch_text);
	}

	if (modeattacking && pausedelay > 0 && !(pausebreakkey || cv_instantretry.value))
	{
		INT32 strength = ((pausedelay - 1 - NEWTICRATE/2)*10)/(NEWTICRATE/3);
		INT32 y = hudinfo[HUD_LIVES].y - 13;

		if (players[consoleplayer].powers[pw_carry] == CR_NIGHTSMODE)
			y -= 16;
		else
		{
			if (players[consoleplayer].pflags & PF_AUTOBRAKE)
				y -= 8;
			if (players[consoleplayer].pflags & PF_ANALOGMODE)
				y -= 8;
		}

		V_DrawThinString(hudinfo[HUD_LIVES].x-2, y,
			hudinfo[HUD_LIVES].f|((leveltime & 4) ? V_SKYMAP : V_BLUEMAP),
			"HOLD TO RETRY...");

		if (strength > 9)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);
		else if (strength > 0)
			V_DrawFadeScreen(0, strength);
	}
}





#define supercheckdef (!(players[tab[i].num].charflags & SF_NOSUPERSPRITES) && ((players[tab[i].num].powers[pw_super] && players[tab[i].num].mo && (players[tab[i].num].mo->state < &states[S_PLAY_SUPER_TRANS1] || players[tab[i].num].mo->state >= &states[S_PLAY_SUPER_TRANS6])) || (players[tab[i].num].powers[pw_carry] == CR_NIGHTSMODE && skins[players[tab[i].num].skin]->flags & SF_SUPER)))
#define greycheckdef (players[tab[i].num].spectator || players[tab[i].num].playerstate == PST_DEAD || (G_IsSpecialStage(gamemap) && players[tab[i].num].exiting))




void HU_drawPing(INT32 x, INT32 y, UINT32 ping, boolean notext, INT32 flags)
{
	UINT8 numbars = 0;  
	UINT8 barcolor = 31;  
	SINT8 i = 0;
	SINT8 yoffset = 6;
	INT32 dx = x+1 - (V_SmallStringWidth(va("%dms", ping),
				V_ALLOWLOWERCASE|flags)/2);

	if (ping < 128)
	{
		numbars = 3;
		barcolor = 112;
	}
	else if (ping < 256)
	{
		numbars = 2;
		barcolor = 73;
	}
	else if (ping < UINT32_MAX)
	{
		numbars = 1;
		barcolor = 35;
	}

	if (ping < UINT32_MAX && (!notext || vid.width >= 640))  
		V_DrawSmallString(dx, y+4, V_ALLOWLOWERCASE|flags, va("%dms", ping));

	for (i=0; (i<3); i++)  
	{
		V_DrawFill(x+2 *(i-1), y+yoffset-4, 2, 8-yoffset, 31|flags);
		if (i < numbars)
			V_DrawFill(x+2 *(i-1), y+yoffset-3, 1, 8-yoffset-1, barcolor|flags);

		yoffset -= 2;
	}

	if (ping == UINT32_MAX)
		V_DrawSmallScaledPatch(x + 4 - nopingicon->width/2, y + 9 - nopingicon->height/2, 0, nopingicon);
}




void HU_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	boolean greycheck, supercheck;


	I_Assert(scorelines <= 9);

	V_DrawFill(1, 26, 318, 1, 0);  

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametyperankings[gametype] != GT_COOP)
			continue;  

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		if (!splitscreen)  
		{
			if (tab[i].num != serverplayer)
				HU_drawPing(x + 253, y, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], false, 0);


		}

		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 20, y,
		                 ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		                 | (greycheck ? V_60TRANS : 0)
		                 | V_ALLOWLOWERCASE, tab[i].name);


		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
			HU_DrawEmeralds(x-12,y+2,255);
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		if (greycheck)
			V_DrawSmallTranslucentPatch (x, y-4, V_80TRANS, livesback);
		else
			V_DrawSmallScaledPatch (x, y-4, 0, livesback);

		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (supercheck)
				V_DrawSmallScaledPatch(x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				if (greycheck)
					V_DrawSmallTranslucentPatch(x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin]);
				else
					V_DrawSmallScaledPatch(x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
		}
		else
		{
			if (supercheck)
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo->color, GTC_CACHE);
				V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (greycheck)
					V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}

		if (G_GametypeUsesLives() && !(G_GametypeUsesCoopLives() && (cv_cooplives.value == 0 || cv_cooplives.value == 3)) && (players[tab[i].num].lives != INFLIVES))  
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE|(greycheck ? V_60TRANS : 0), va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
		{
			if (greycheck)
				V_DrawSmallTranslucentPatch(x-32, y-4, V_60TRANS, tagico);
			else
				V_DrawSmallScaledPatch(x-32, y-4, 0, tagico);
		}

		if (players[tab[i].num].exiting || (players[tab[i].num].pflags & PF_FINISHED))
			V_DrawSmallScaledPatch(x - exiticon->width/2 - 1, y-3, 0, exiticon);

		if (gametyperankings[gametype] == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedString(x+240, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedString(x+240, y, (greycheck ? V_60TRANS : 0), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedString(x+240, y, (greycheck ? V_60TRANS : 0), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedString(x+240, y, (greycheck ? V_60TRANS : 0), va("%u", tab[i].count));

		y += 16;
	}
}




static void HU_Draw32Emeralds(INT32 x, INT32 y, INT32 pemeralds)
{

	if (pemeralds & EMERALD1)
		V_DrawSmallScaledPatch(x  , y, 0, emeraldpics[1][0]);

	if (pemeralds & EMERALD2)
		V_DrawSmallScaledPatch(x+4, y, 0, emeraldpics[1][1]);

	if (pemeralds & EMERALD3)
		V_DrawSmallScaledPatch(x+8, y, 0, emeraldpics[1][2]);

	if (pemeralds & EMERALD4)
		V_DrawSmallScaledPatch(x+12  , y, 0, emeraldpics[1][3]);

	if (pemeralds & EMERALD5)
		V_DrawSmallScaledPatch(x+16, y, 0, emeraldpics[1][4]);

	if (pemeralds & EMERALD6)
		V_DrawSmallScaledPatch(x+20, y, 0, emeraldpics[1][5]);

	if (pemeralds & EMERALD7)
		V_DrawSmallScaledPatch(x+24,   y,   0, emeraldpics[1][6]);
}




static void HU_Draw32TeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0);  
	V_DrawFill(1, 26, 318, 1, 0);  
	V_DrawFill(1, 180, 318, 1, 0);  

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue;  

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		if (players[tab[i].num].ctfteam == 1)  
		{
			redplayers++;
			x = 14 + (BASEVIDWIDTH/2);
			y = (redplayers * 9) + 20;
		}
		else if (players[tab[i].num].ctfteam == 2)  
		{
			blueplayers++;
			x = 14;
			y = (blueplayers * 9) + 20;
		}
		else  
			continue;

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 8);
		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 10, y,
			             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
			             | (greycheck ? V_TRANSLUCENT : 0)
			             | V_ALLOWLOWERCASE, name);

		if (gametyperules & GTR_TEAMFLAGS)
		{
			if (players[tab[i].num].gotflag & GF_REDFLAG)  
				V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, rflagico, 0);
			else if (players[tab[i].num].gotflag & GF_BLUEFLAG)  
				V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, bflagico, 0);
		}


		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, 255);

		}
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, tab[i].emeralds);

		}

		if (supercheck)
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], colormap);
		}
		else
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			if (players[tab[i].num].spectator || players[tab[i].num].playerstate == PST_DEAD)
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], colormap);
			else
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], colormap);
		}
		V_DrawRightAlignedThinString(x+128, y, ((players[tab[i].num].spectator || players[tab[i].num].playerstate == PST_DEAD) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));
		if (!splitscreen)
		{
			if (tab[i].num != serverplayer)
				HU_drawPing(x + 135, y+1, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], true, 0);


		}
	}
}




void HU_DrawTeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	boolean smol = false;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;


	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue;  

		if (players[tab[i].num].ctfteam == 1)  
		{
			if (redplayers++ > 8)
			{
				smol = true;
				break;  
			}
		}
		else if (players[tab[i].num].ctfteam == 2)  
		{
			if (blueplayers++ > 8)
			{
				smol = true;
				break;
			}
		}
		else  
			continue;

	}


	if (smol == true || cv_compactscoreboard.value)
	{
		HU_Draw32TeamTabRankings(tab, whiteplayer);
		return;
	}

	V_DrawFill(160, 26, 1, 154, 0);  
	V_DrawFill(1, 26, 318, 1, 0);  
	V_DrawFill(1, 180, 318, 1, 0);  

	i=0, redplayers=0, blueplayers=0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue;  

		if (players[tab[i].num].ctfteam == 1)  
		{
			if (redplayers++ > 8)
				continue;
			x = 32 + (BASEVIDWIDTH/2);
			y = (redplayers * 16) + 16;
		}
		else if (players[tab[i].num].ctfteam == 2)  
		{
			if (blueplayers++ > 8)
				continue;
			x = 32;
			y = (blueplayers * 16) + 16;
		}
		else  
			continue;

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 7);
		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 20, y,
			             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
			             | (greycheck ? V_TRANSLUCENT : 0)
			             | V_ALLOWLOWERCASE, name);

		if (gametyperules & GTR_TEAMFLAGS)
		{
			if (players[tab[i].num].gotflag & GF_REDFLAG)  
				V_DrawSmallScaledPatch(x-28, y-4, 0, rflagico);
			else if (players[tab[i].num].gotflag & GF_BLUEFLAG)  
				V_DrawSmallScaledPatch(x-28, y-4, 0, bflagico);
		}


		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
			HU_DrawEmeralds(x-12,y+2,255);
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		if (supercheck)
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
		}
		else
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			if (greycheck)
				V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
			else
				V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
		}
		V_DrawRightAlignedThinString(x+100, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));
		if (!splitscreen)
		{
			if (tab[i].num != serverplayer)
				HU_drawPing(x+ 113, y, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], false, 0);


		}
	}
}




void HU_DrawDualTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0);  
	V_DrawFill(1, 26, 318, 1, 0);  
	V_DrawFill(1, 180, 318, 1, 0);  

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametyperankings[gametype] != GT_COOP)
			continue;  

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 7);
		if (tab[i].num != serverplayer)
			HU_drawPing(x+ 113, y, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], false, 0);



		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 20, y,
			             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
			             | (greycheck ? V_TRANSLUCENT : 0)
			             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives() && !(G_GametypeUsesCoopLives() && (cv_cooplives.value == 0 || cv_cooplives.value == 3)) && (players[tab[i].num].lives != INFLIVES))  
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE, va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawSmallScaledPatch(x-28, y-4, 0, tagico);

		if (players[tab[i].num].exiting || (players[tab[i].num].pflags & PF_FINISHED))
			V_DrawSmallScaledPatch(x - exiticon->width/2 - 1, y-3, 0, exiticon);


		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
			HU_DrawEmeralds(x-12,y+2,255);
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}


		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (supercheck)
				V_DrawSmallScaledPatch (x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				if (greycheck)
					V_DrawSmallTranslucentPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin]);
				else
					V_DrawSmallScaledPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
		}
		else
		{
			if (supercheck)
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (greycheck)
					V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}


		if (gametyperankings[gametype] == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedThinString(x+100, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedThinString(x+100, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedThinString(x+100, y, (greycheck ? V_TRANSLUCENT : 0), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedThinString(x+100, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));

		y += 16;
		if (y > 160)
		{
			y = 32;
			x += BASEVIDWIDTH/2;
		}
	}
}




static void HU_Draw32TabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0);  
	V_DrawFill(1, 26, 318, 1, 0);  
	V_DrawFill(1, 180, 318, 1, 0);  

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametyperankings[gametype] != GT_COOP)
			continue;  

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 7);
		if (!splitscreen)  
		{
			if (tab[i].num != serverplayer)
				HU_drawPing(x+ 135, y+1, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], true, 0);


		}

		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 10, y,
			             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
			             | (greycheck ? V_TRANSLUCENT : 0)
			             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives())  
			V_DrawRightAlignedThinString(x-1, y, V_ALLOWLOWERCASE, va("%d", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, tagico, 0);


		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, 255);

		}
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, tab[i].emeralds);

		}


		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (players[tab[i].num].powers[pw_super] && !(players[tab[i].num].charflags & SF_NOSUPERSPRITES))
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], 0);
			else
			{
				if (greycheck)
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], 0);
				else
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], 0);
			}
		}
		else
		{
			if (supercheck)
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (greycheck)
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}


		if (gametyperankings[gametype] == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedThinString(x+128, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedThinString(x+128, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedThinString(x+128, y, (greycheck ? V_TRANSLUCENT : 0), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedThinString(x+128, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));

		y += 9;
		if (i == 16)
		{
			y = 32;
			x += BASEVIDWIDTH/2;
		}
	}
}




void HU_DrawEmeralds(INT32 x, INT32 y, INT32 pemeralds)
{

	if (pemeralds & EMERALD1)
		V_DrawSmallScaledPatch(x  , y-6, 0, emeraldpics[1][0]);

	if (pemeralds & EMERALD2)
		V_DrawSmallScaledPatch(x+4, y-3, 0, emeraldpics[1][1]);

	if (pemeralds & EMERALD3)
		V_DrawSmallScaledPatch(x+4, y+3, 0, emeraldpics[1][2]);

	if (pemeralds & EMERALD4)
		V_DrawSmallScaledPatch(x  , y+6, 0, emeraldpics[1][3]);

	if (pemeralds & EMERALD5)
		V_DrawSmallScaledPatch(x-4, y+3, 0, emeraldpics[1][4]);

	if (pemeralds & EMERALD6)
		V_DrawSmallScaledPatch(x-4, y-3, 0, emeraldpics[1][5]);

	if (pemeralds & EMERALD7)
		V_DrawSmallScaledPatch(x,   y,   0, emeraldpics[1][6]);
}




static inline void HU_DrawSpectatorTicker(void)
{
	int i;
	int length = 0, height = 174;
	int totallength = 0;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].spectator)
			totallength += (signed)strlen(player_names[i]) * 8 + 16;

	length -= (leveltime % (totallength + (vid.width / vid.dup)));
	length += (vid.width / vid.dup);

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].spectator)
		{
			if (length >= -((signed)strlen(player_names[i]) * 8 + 16) && length <= (vid.width / vid.dup))
				V_DrawString(length, height + 8, V_TRANSLUCENT|V_ALLOWLOWERCASE|V_SNAPTOLEFT, player_names[i]);

			length += (signed)strlen(player_names[i]) * 8 + 16;
		}
}




static void HU_DrawRankings(void)
{
	playersort_t tab[MAXPLAYERS];
	INT32 i, j, scorelines;
	boolean completed[MAXPLAYERS];
	UINT32 whiteplayer;


	if (gametype >= 0 && gametype < gametypecount)
		V_DrawString(4, splitscreen ? 184 : 192, 0, Gametype_Names[gametype]);

	if (gametyperules & (GTR_TIMELIMIT|GTR_POINTLIMIT))
	{
		if ((gametyperules & GTR_TIMELIMIT) && cv_timelimit.value && timelimitintics > 0)
		{
			V_DrawCenteredString(64, 8, 0, "TIME");
			V_DrawCenteredString(64, 16, 0, va("%i:%02i", G_TicsToMinutes(stplyr->realtime, true), G_TicsToSeconds(stplyr->realtime)));
		}

		if ((gametyperules & GTR_POINTLIMIT) && cv_pointlimit.value > 0)
		{
			V_DrawCenteredString(256, 8, 0, "POINT LIMIT");
			V_DrawCenteredString(256, 16, 0, va("%d", cv_pointlimit.value));
		}
	}
	else
	{
		if (circuitmap)
		{
			V_DrawCenteredString(64, 8, 0, "NUMBER OF LAPS");
			V_DrawCenteredString(64, 16, 0, va("%d", cv_numlaps.value));
		}
	}



	whiteplayer = demoplayback ? displayplayer : consoleplayer;

	scorelines = 0;
	memset(completed, 0, sizeof (completed));
	memset(tab, 0, sizeof (playersort_t)*MAXPLAYERS);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		tab[i].num = -1;
		tab[i].name = 0;

		if (gametyperankings[gametype] == GT_RACE && !circuitmap)
			tab[i].count = INT32_MAX;
	}

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (!playeringame[j])
			continue;

		if (!G_PlatformGametype() && players[j].spectator)
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (!G_PlatformGametype() && players[i].spectator)
				continue;

			if (gametyperankings[gametype] == GT_RACE)
			{
				if (circuitmap)
				{
					if ((unsigned)players[i].laps+1 >= tab[scorelines].count && completed[i] == false)
					{
						tab[scorelines].count = players[i].laps+1;
						tab[scorelines].num = i;
						tab[scorelines].color = players[i].skincolor;
						tab[scorelines].name = player_names[i];
					}
				}
				else
				{
					if (players[i].realtime <= tab[scorelines].count && completed[i] == false)
					{
						tab[scorelines].count = players[i].realtime;
						tab[scorelines].num = i;
						tab[scorelines].color = players[i].skincolor;
						tab[scorelines].name = player_names[i];
					}
				}
			}
			else if (gametyperankings[gametype] == GT_COMPETITION)
			{


				if (players[i].score >= tab[scorelines].count && completed[i] == false)
				{
					tab[scorelines].count = players[i].score;
					tab[scorelines].num = i;
					tab[scorelines].color = players[i].skincolor;
					tab[scorelines].name = player_names[i];
					tab[scorelines].emeralds = players[i].powers[pw_emeralds];
				}
			}
			else
			{
				if (players[i].score >= tab[scorelines].count && completed[i] == false)
				{
					tab[scorelines].count = players[i].score;
					tab[scorelines].num = i;
					tab[scorelines].color = players[i].skincolor;
					tab[scorelines].name = player_names[i];
					tab[scorelines].emeralds = players[i].powers[pw_emeralds];
				}
			}
		}
		completed[tab[scorelines].num] = true;
		scorelines++;
	}





	if (G_GametypeHasTeams())
		HU_DrawTeamTabRankings(tab, whiteplayer);
	else if (scorelines <= 9 && !cv_compactscoreboard.value)
		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
	else if (scorelines <= 18 && !cv_compactscoreboard.value)
		HU_DrawDualTabRankings(32, 32, tab, scorelines, whiteplayer);
	else
		HU_Draw32TabRankings(14, 28, tab, scorelines, whiteplayer);


	if (!splitscreen && G_GametypeHasSpectators())
		HU_DrawSpectatorTicker();
}

static void HU_DrawCoopOverlay(void)
{
	if (token && LUA_HudEnabled(hud_tokens))
	{
		V_DrawString(168, 176, 0, va("- %d", token));
		V_DrawSmallScaledPatch(148, 172, 0, tokenicon);
	}

	if (LUA_HudEnabled(hud_tabemblems))
	{
		V_DrawString(160, 144, 0, va("- %d/%d", M_CountEmblems(clientGamedata), numemblems+numextraemblems));
		V_DrawScaledPatch(128, 144 - emblemicon->height/4, 0, emblemicon);
	}

	if (!LUA_HudEnabled(hud_coopemeralds))
		return;

	if (emeralds & EMERALD1)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8   , (BASEVIDHEIGHT/3)-32, 0, emeraldpics[0][0]);
	if (emeralds & EMERALD2)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8+24, (BASEVIDHEIGHT/3)-16, 0, emeraldpics[0][1]);
	if (emeralds & EMERALD3)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8+24, (BASEVIDHEIGHT/3)+16, 0, emeraldpics[0][2]);
	if (emeralds & EMERALD4)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8   , (BASEVIDHEIGHT/3)+32, 0, emeraldpics[0][3]);
	if (emeralds & EMERALD5)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8-24, (BASEVIDHEIGHT/3)+16, 0, emeraldpics[0][4]);
	if (emeralds & EMERALD6)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8-24, (BASEVIDHEIGHT/3)-16, 0, emeraldpics[0][5]);
	if (emeralds & EMERALD7)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8   , (BASEVIDHEIGHT/3)   , 0, emeraldpics[0][6]);
}

static void HU_DrawNetplayCoopOverlay(void)
{
	int i;

	if (token && LUA_HudEnabled(hud_tokens))
	{
		V_DrawString(168, 10, 0, va("- %d", token));
		V_DrawSmallScaledPatch(148, 6, 0, tokenicon);
	}

	if (G_CoopGametype() && LUA_HudEnabled(hud_tabemblems))
	{
		V_DrawCenteredString(256, 14, 0, "/");
		V_DrawString(256 + 4, 14, 0, va("%d", numemblems + numextraemblems));
		V_DrawRightAlignedString(256 - 4, 14, 0, va("%d", M_CountEmblems(clientGamedata)));

		V_DrawSmallScaledPatch(256 - (emblemicon->width / 4), 6, 0, emblemicon);
	}

	if (!LUA_HudEnabled(hud_coopemeralds))
		return;

	for (i = 0; i < 7; ++i)
	{
		if (emeralds & (1 << i))
			V_DrawScaledPatch(20 + (i * 10), 9, 0, emeraldpics[1][i]);
	}
}




void HU_ClearCEcho(void)
{
	cechotimer = 0;
}

void HU_SetCEchoDuration(INT32 seconds)
{
	cechoduration = seconds * TICRATE;
}

void HU_SetCEchoFlags(INT32 flags)
{

	cechoflags = (flags & ~V_PARAMMASK);
}

void HU_DoCEcho(const char *msg)
{
	I_OutputMsg("%s\n", msg);  

	strncpy(cechotext, msg, sizeof(cechotext)-1);
	strncat(cechotext, "\\", sizeof(cechotext) - strlen(cechotext) - 1);
	cechotext[sizeof(cechotext) - 1] = '\0';
	cechotimer = cechoduration;
}
