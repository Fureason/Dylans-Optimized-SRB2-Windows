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
/// \file  hu_stuff.c
/// \brief Heads up display

#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"

#include "m_menu.h" // gametype_cons_t
#include "m_cond.h" // emblems
#include "m_misc.h" // word jumping

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

#include "p_local.h" // camera, camera2
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

// coords are scaled
#define HU_INPUTX 0
#define HU_INPUTY 0

#define HU_SERVER_SAY 1 // Server message (dedicated).
#define HU_CSAY       2 // Server CECHOes to everyone.

//-------------------------------------------
//              Fonts & stuff
//-------------------------------------------
// Font definitions
fontdef_t hu_font;
fontdef_t tny_font;
fontdef_t cred_font;
fontdef_t lt_font;
fontdef_t ntb_font;
fontdef_t nto_font;

// Numbers
patch_t *tallnum[10]; // 0-9
patch_t *nightsnum[10]; // 0-9
patch_t *ttlnum[10]; // act numbers (0-9)
patch_t *tallminus;
patch_t *tallinfin;

static player_t *plr;
boolean chat_on; // entering a chat message?
static INT32 chat_open_key;
static char w_chat[HU_MAXMSGLEN + 1];
static size_t c_input = 0; // let's try to make the chat input less shitty.
static boolean headsupactive = false;
boolean hu_showscores; // draw rankings
static char hu_tick;

patch_t *rflagico;
patch_t *bflagico;
patch_t *rmatcico;
patch_t *bmatcico;
patch_t *tagico;

//-------------------------------------------
//              coop hud
//-------------------------------------------

patch_t *emeraldpics[3][8]; // 0 = normal, 1 = tiny, 2 = coinbox
static patch_t *emblemicon;
patch_t *tokenicon;
static patch_t *exiticon;
static patch_t *nopingicon;

//-------------------------------------------
//              misc vars
//-------------------------------------------

// crosshair 0 = off, 1 = cross, 2 = angle, 3 = point, see m_menu.c
static patch_t *crosshair[HU_CROSSHAIRS]; // 3 precached crosshair graphics

// -------
// protos.
// -------
static void HU_DrawRankings(void);
static void HU_DrawCoopOverlay(void);
static void HU_DrawNetplayCoopOverlay(void);

//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

char *shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', // shift-[
	'|', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	'}', // shift-]
	'"', '_',
	'~', // shift-`
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

static char cechotext[1024];
static tic_t cechotimer = 0;
static tic_t cechoduration = 5*TICRATE;
static INT32 cechoflags = 0;

static huddrawlist_h luahuddrawlist_scores;

//======================================================================
//                          HEADS UP INIT
//======================================================================

static tic_t resynch_ticker = 0;

// just after
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

	// Cache fonts
	HU_LoadFontCharacters(&hu_font,   "STCFN");
	HU_LoadFontCharacters(&tny_font,  "TNYFN");
	HU_LoadFontCharacters(&cred_font, "CRFNT");
	HU_LoadFontCharacters(&lt_font,   "LTFNT");
	HU_LoadFontCharacters(&ntb_font,  "NTFNT");
	HU_LoadFontCharacters(&nto_font,  "NTFNO");

	// For each font, set kerning, space width, character width and line spacing
	HU_SetFontProperties(&hu_font,   0,  4,  8, 12);
	HU_SetFontProperties(&tny_font,  0,  2,  4, 12);
	HU_SetFontProperties(&cred_font, 0, 16, 16, 16);
	HU_SetFontProperties(&lt_font,   0, 16, 20, 16);
	HU_SetFontProperties(&ntb_font,  2,  4, 20, 21);
	HU_SetFontProperties(&nto_font,  0,  4, 20, 21);

	//cache numbers too!
	for (i = 0; i < 10; i++)
	{
		sprintf(buffer, "STTNUM%d", i);
		tallnum[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
		sprintf(buffer, "NGTNUM%d", i);
		nightsnum[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
		sprintf(buffer, "TTL%.2d", i);
		ttlnum[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	// minus for negative tallnums
	tallminus = (patch_t *)W_CachePatchName("STTMINUS", PU_HUDGFX);
	tallinfin = (patch_t *)W_CachePatchName("STTINFIN", PU_HUDGFX);

	// cache the crosshairs, don't bother to know which one is being used,
	// just cache all 3, they're so small anyway.
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
	//emeraldpics[1][7] = W_CachePatchName("TEMER8", PU_HUDGFX); -- unused

	emeraldpics[2][0] = W_CachePatchName("EMBOX1", PU_HUDGFX);
	emeraldpics[2][1] = W_CachePatchName("EMBOX2", PU_HUDGFX);
	emeraldpics[2][2] = W_CachePatchName("EMBOX3", PU_HUDGFX);
	emeraldpics[2][3] = W_CachePatchName("EMBOX4", PU_HUDGFX);
	emeraldpics[2][4] = W_CachePatchName("EMBOX5", PU_HUDGFX);
	emeraldpics[2][5] = W_CachePatchName("EMBOX6", PU_HUDGFX);
	emeraldpics[2][6] = W_CachePatchName("EMBOX7", PU_HUDGFX);
	//emeraldpics[2][7] = W_CachePatchName("EMBOX8", PU_HUDGFX); -- unused
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

// Initialise Heads up
// once at game startup.
//
void HU_Init(void)
{
	COM_AddCommand("say", Command_Say_f, COM_LUA);
	COM_AddCommand("sayto", Command_Sayto_f, COM_LUA);
	COM_AddCommand("sayteam", Command_Sayteam_f, COM_LUA);
	COM_AddCommand("csay", Command_CSay_f, COM_LUA);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);

	// set shift translation table
	shiftxform = english_shiftxform;

	luahuddrawlist_scores = LUA_HUD_CreateDrawList();
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}

//
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
	if (headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];

	headsupactive = true;
}

//======================================================================
//                            EXECUTION
//======================================================================

// EVERY CHANGE IN THIS SCRIPT IS LOL XD! BY VINCYTM

#define HU_CHATDISPLAYLEN (HU_MAXMSGLEN + MAXPLAYERNAME + 64)
#define HU_CHATCOLWIDTH 4
#define HU_CHATROWHEIGHT 6
#define HU_CHATBARWIDTH 4
#define HU_CHATPAD 4
#define HU_CHATDRAWFLAGS (V_SNAPTOBOTTOM | V_SNAPTOLEFT)

typedef struct
{
	char text[HU_CHATDISPLAYLEN + 1];
	UINT8 link[HU_CHATDISPLAYLEN + 1];
	UINT16 name_start, name_end;
	SINT8 player;
} chatmessage_t;

typedef struct
{
	UINT32 message;
	UINT16 offset;
} chatposition_t;

typedef struct
{
	chatposition_t start;
	UINT16 end;
	UINT8 columns, color;
} chatrow_t;

static INT16 chatx = 13, chaty = 169;
static chatmessage_t chat_log[CHAT_BUFSIZE];
static chatrow_t chat_rows[CHAT_BUFSIZE * (HU_CHATDISPLAYLEN + 1)];
static UINT32 chat_first_message = 1, chat_nummsg_log;
static INT32 chat_row_count, chat_layout_width;
static boolean chat_layout_dirty = true;
static INT32 chat_scroll, chat_maxscroll;
static boolean chat_follow = true;
static INT32 chat_view_top, chat_view_width, chat_view_lines;
static INT32 chat_mouse_x, chat_mouse_y, chat_scroll_grab;
static boolean chat_scroll_drag, chat_select_drag, chat_select_moved;
static chatposition_t chat_selection_start, chat_selection_end, chat_click;
static INT32 chat_recipient = -1;
static boolean chat_recipient_left;

static UINT32 chat_nummsg_min;
static char chat_mini[8][HU_CHATDISPLAYLEN + 1];
static tic_t chat_timers[8];

static size_t HU_ChatLinkLength(const char *text)
{
	size_t length;
	if (strnicmp(text, "https://", 8) && strnicmp(text, "http://", 7))
		return 0;
	for (length = 0; (UINT8)text[length] > ' ' && (UINT8)text[length] < 0x80; length++)
		;
	while (length && strchr(".,;:!?)]}>\"'", text[length - 1]))
		length--;
	return length;
}

static chatmessage_t *HU_ChatMessage(UINT32 message)
{
	const UINT32 index = message - chat_first_message;
	return index < chat_nummsg_log ? &chat_log[index] : NULL;
}

static void HU_OpenChatURL(const chatmessage_t *message, UINT16 offset)
{
	size_t start = offset, end = offset + 1;
	char url[HU_CHATDISPLAYLEN + 1];
	while (start && message->link[start - 1])
		start--;
	while (message->link[end])
		end++;
	memcpy(url, message->text + start, end - start);
	url[end - start] = '\0';
#ifdef _WIN32
	HINSTANCE opened = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
	if ((INT_PTR)opened > 32)
		return;
#endif
	I_ClipboardCopy(url, end - start);
	HU_AddChatText("\x82""Could not open the link, so it was copied.", false);
}

void HU_ForgetChatPlayer(INT32 player)
{
	for (UINT32 i = 0; i < chat_nummsg_log; i++)
	{
		if (chat_log[i].player == player)
			chat_log[i].player = -1;
	}
	if (chat_recipient == player)
		chat_recipient_left = true;
}

static boolean HU_ChatRecipientAvailable(INT32 player)
{
	return player >= 0 && player < MAXPLAYERS && player != consoleplayer
		&& netgame && playeringame[player] && !players[player].quittime;
}
static INT16 addy = 0; // use this to make the messages scroll smoothly when one fades away

static void HU_removeChatText_Mini(void)
{
	size_t i;
	for (i = 0; i + 1 < chat_nummsg_min; i++)
	{
		memcpy(chat_mini[i], chat_mini[i+1], sizeof chat_mini[i]);
		chat_timers[i] = chat_timers[i+1];
	}
	chat_mini[chat_nummsg_min-1][0] = '\0';
	chat_nummsg_min--; // lost 1 msg.

	// use addy and make shit slide smoothly af.
	addy += (vid.width < 640) ? 8 : 6;

}

static void HU_AddChatMessage(const char *text, boolean playsound, INT32 player, size_t name_start, size_t name_end)
{
	if (!text)
		return;

	if (playsound && cv_consolechat.value != 2)
		S_StartSound(NULL, sfx_radio);

	if (chat_nummsg_log == CHAT_BUFSIZE)
	{
		memmove(chat_log, chat_log + 1, (CHAT_BUFSIZE - 1) * sizeof chat_log[0]);
		chat_nummsg_log--;
		chat_first_message++;
	}

	chatmessage_t *message = &chat_log[chat_nummsg_log++];
	strlcpy(message->text, text, sizeof message->text);
	message->player = (SINT8)player;
	const size_t length = strlen(message->text);
	message->name_start = (UINT16)min(name_start, length);
	message->name_end = (UINT16)min(name_end, length);
	memset(message->link, 0, sizeof message->link);
	for (size_t i = 0; i < length;)
	{
		const size_t linklen = HU_ChatLinkLength(&message->text[i]);
		if (linklen)
			memset(&message->link[i], 1, linklen);
		i += max(1, linklen);
	}
	chat_layout_dirty = true;

	if (chat_nummsg_min >= 8)
		HU_removeChatText_Mini();

	strlcpy(chat_mini[chat_nummsg_min], message->text, sizeof chat_mini[chat_nummsg_min]);
	chat_timers[chat_nummsg_min++] = TICRATE * cv_chattime.value;

	if (OLDCHAT)
		CONS_Printf("%s\n", text);
	else
		CON_LogMessage(va("%s\n", text));
}

void HU_AddChatText(const char *text, boolean playsound)
{
	HU_AddChatMessage(text, playsound, -1, 0, 0);
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

	// Only servers/admins can CSAY.
	if(!server && !(IsPlayerAdmin(consoleplayer)))
		flags &= ~HU_CSAY;

	// We handle HU_SERVER_SAY, not the caller.
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

	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0) // used /pm
	{
		// what we're gonna do now is check if the player exists
		// with that logic, characters 4 and 5 are our numbers:
		const char *newmsg;
		char playernum[3+1];
		INT32 spc = 1; // used if playernum[1] is a space.

		strncpy(playernum, msg+3, sizeof(playernum)-1);
		// check for undesirable characters in our "number"
		if (((playernum[0] < '0') || (playernum[0] > '9')) || ((playernum[1] < '0') || (playernum[1] > '9')))
		{
			// check if playernum[1] is a space
			if (playernum[1] == ' ')
				spc = 0;
			// let it slide
			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<playernum> \'.", false);
				return;
			}
		}
		// I'm very bad at C, I swear I am, additional checks eww!
		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<playernum> \'.", false);
			return;
		}

		target = atoi(playernum); // turn that into a number
		//CONS_Printf("%d\n", target);

		// check for target player, if it doesn't exist then we can't send the message!
		if (target < MAXPLAYERS && playeringame[target]) // player exists
			target++; // even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false); // same
			return;
		}
		buf[0] = target;
		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}

	SendNetXCmd(XD_SAY, buf, strlen(msg) + 1 + msg-buf);
}

/** Send a message to everyone.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Say_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("say <message>: send a message\n"));
		return;
	}

	DoSayCommand(0, 1, 0);
}

/** Send a message to a particular person.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Say_f
  * \author Graue <graue@oceanbase.org>
  */
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
	target++; // Internally we use 0 to 31, but say command uses 1 to 32.

	DoSayCommand((SINT8)target, 2, 0);
}

/** Send a message to members of the player's team.
  * \sa DoSayCommand, Command_Say_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
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

/** Send a message to everyone, to be displayed by CECHO. Only
  * permitted to servers and admins.
  */
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

UINT8 spam_tokens[MAXPLAYERS] = { 1 }; // fill the buffer with 1 so the motd can be sent.
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

/** Receives a message, processing an ::XD_SAY command.
  * \sa DoSayCommand
  * \author Graue <graue@oceanbase.org>
  */
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

	//check for invalid characters (0x80 or above)
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

	// before we do anything, let's verify the guy isn't spamming, get this easier on us.

	//if (stop_spamming[playernum] != 0 && cv_chatspamprotection.value && !(flags & HU_CSAY))
	if (spam_tokens[playernum] <= 0 && cv_chatspamprotection.value && !(flags & HU_CSAY))
	{
		CONS_Debug(DBG_NETPLAY,"Received SAY cmd too quickly from Player %d (%s), assuming as spam and blocking message.\n", playernum+1, player_names[playernum]);
		spam_tics[playernum] = 0;
		spam_eatmsg = 1;
	}
	else
		spam_tokens[playernum] -= 1;

	if (spam_eatmsg)
		return; // don't proceed if we were supposed to eat the message.

	if (LUA_HookPlayerMsg(playernum, target, flags, msg))
		return;

	// If it's a CSAY, just CECHO and be done with it.
	if (flags & HU_CSAY)
	{
		HU_SetCEchoDuration(5);
		I_OutputMsg("Server message: ");
		HU_DoCEcho(msg);
		return;
	}

	// Handle "/me" actions, but only in messages to everyone.
	if (target == 0 && strlen(msg) > 4 && strnicmp(msg, "/me ", 4) == 0)
	{
		msg += 4;
		action = true;
	}

	if (flags & HU_SERVER_SAY)
		dispname = "SERVER";
	else
		dispname = player_names[playernum];

	// Clean up message a bit
	// If you use a \r character, you can remove your name
	// from before the text and then pretend to be someone else!
	// If you use a \n character, you can create a new line in
	// the log and then pretend to be someone else as well!
	ptr = msg;
	while (*ptr != '\0')
	{
		if (*ptr == '\r' || *ptr == '\n')
			*ptr = ' ';

		ptr++;
	}

	// Show messages sent by you, to you, to your team, or to everyone:
	if (playernum == consoleplayer // By you
	|| (target == -1 && ST_SameTeam(&players[consoleplayer], &players[playernum])) // To your team
	|| target == 0 // To everyone
	|| consoleplayer == target-1) // To you
	{
		const char *prefix = "", *cstart = "", *cend = "", *adminchar = "\x82~\x83", *remotechar = "\x82@\x83", *fmt2, *textcolor = "\x80";
		char *tempchar = NULL;

		// player is a spectator?
        if (players[playernum].spectator)
		{
			cstart = "\x86";    // grey name
			textcolor = "\x86";
		}
		else if (target == -1) // say team
		{
			if (players[playernum].ctfteam == 1) // red
			{
				cstart = textcolor = GetChatColorFromSkinColor(skincolor_redteam);
			}
			else // blue
			{
				cstart = textcolor = GetChatColorFromSkinColor(skincolor_blueteam);
			}
		}
		else
        {
			cstart = GetChatColorFromSkinColor(players[playernum].skincolor);
			if (G_GametypeHasTeams())
			{
				if (players[playernum].ctfteam == 1) // red
				{
					cstart = GetChatColorFromSkinColor(skincolor_redteam);
				}
				else // blue
				{
					cstart = GetChatColorFromSkinColor(skincolor_blueteam);
				}
			}
        }
		prefix = cstart;

		// Give admins and remote admins their symbols.
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

		// Choose the proper format string for display.
		// Each format includes four strings: color start, display
		// name, color end, and the message itself.
		// '\4' makes the message yellow and beeps; '\3' just beeps.
		if (action)
			fmt2 = "* %s%s%s%s \x82%s%s";
		else if (target-1 == consoleplayer) // To you
		{
			prefix = "\x82[PM]";
			cstart = "\x82";
			textcolor = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";
		}
		else if (target > 0) // By you, to another player
		{
			// Use target's name.
			dispname = player_names[target-1];
			prefix = "\x82[TO]";
			cstart = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";

		}
		else if (target == 0) // To everyone
			fmt2 = "%s<%s%s%s>\x80 %s%s";
		else // To your team
		{
			if (players[playernum].ctfteam == 1) // red
				prefix = "\x85[TEAM]";
			else if (players[playernum].ctfteam == 2) // blue
				prefix = "\x84[TEAM]";
			else
				prefix = "\x83"; // makes sure this doesn't implode if you sayteam on non-team gamemodes

			fmt2 = "%s<%s%s>\x80%s %s%s";
		}

		const INT32 chat_player = (flags & HU_SERVER_SAY) ? -1
			: target > 0 && target - 1 != consoleplayer ? target - 1 : playernum;
		const size_t name_start = strlen(prefix) + strlen(cstart) + (action ? 2 : 1);
		HU_AddChatMessage(va(fmt2, prefix, cstart, dispname, cend, textcolor, msg),
			cv_chatnotifications.value, chat_player, name_start, name_start + strlen(dispname));

		if (tempchar)
			Z_Free(tempchar);
	}
#ifdef _DEBUG
	// I just want to point out while I'm here that because the data is still
	// sent to all players, techincally anyone can see your chat if they really
	// wanted to, even if you used sayto or sayteam.
	// You should never send any sensitive info through sayto for that reason.
	else
		CONS_Printf("Dropped chat: %d %d %s\n", playernum, target, msg);
#endif
}

//
//
void HU_Ticker(void)
{
	// do this server-side, too
	if (netgame)
	{
		size_t i = 0;

		// handle spam while we're at it:
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
	hu_tick &= 7; // currently only to blink chat input cursor

	if (PLAYER1INPUTDOWN(GC_SCORES))
		hu_showscores = !chat_on;
	else
		hu_showscores = false;


	if (netgame)
	{
		size_t i = 0;

		// handle chat timers
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


static INT16 typelines = 1; // number of drawfill lines we need when drawing the chat. it's some weird hack and might be one frame off but I'm lazy to make another loop.
// It's up here since it has to be reset when we open the chat.

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

	// if our message was nothing but spaces, don't send it.
	if (HU_chatboxContainsOnlySpaces())
		return;

	ci = 0;
	for (const char *p = w_chat; *p; p++)
	{
		if ((UINT8)*p >= ' ' && (UINT8)*p < 0x7f)
			msg[ci++] = *p;
	}
	msg[ci] = '\0';

	memset(w_chat, '\0', sizeof(w_chat));
	c_input = 0;

	// last minute mute check
	if (CHAT_MUTE)
	{
		if (cv_mute.value)
			HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"), false);
		else
			HU_AddChatText(va("%s>ERROR: You have been muted. You can't say anything.", "\x85"), false);
		return;
	}

	if (chat_recipient >= 0)
	{
		if (chat_recipient_left || !HU_ChatRecipientAvailable(chat_recipient))
		{
			HU_AddChatText("\x82""That player is no longer in the game.", false);
			return;
		}
		target = chat_recipient + 1;
	}
	else if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0)
	{
		INT32 spc = 1; // used if playernum[1] is a space.
		char playernum[3+1];
		const char *newmsg;

		// what we're gonna do now is check if the player exists
		// with that logic, characters 4 and 5 are our numbers:

		// teamtalk can't send PMs, just don't send it, else everyone would be able to see it, and no one wants to see your sex RP sicko.
		if (teamtalk)
		{
			HU_AddChatText(va("%sCannot send sayto in Say-Team.", "\x85"), false);
			return;
		}

		strncpy(playernum, msg+3, sizeof(playernum)-1);
		// check for undesirable characters in our "number"
		if (!(isdigit(playernum[0]) && isdigit(playernum[1])))
		{
			// check if playernum[1] is a space
			if (playernum[1] == ' ')
				spc = 0;
				// let it slide
			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
				return;
			}
		}
		// I'm very bad at C, I swear I am, additional checks eww!
		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
			return;
		}

		target = atoi(playernum); // turn that into a number

		// check for target player, if it doesn't exist then we can't send the message!
		if (target < MAXPLAYERS && playeringame[target]) // player exists
			target++; // even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false); // same
			return;
		}

		// we need to get rid of the /pm<player num>
		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}
	if (msg[0])
	{
		buf[0] = teamtalk ? -1 : target; // target
		buf[1] = 0; // flags
		SendNetXCmd(XD_SAY, buf, 2 + strlen(&buf[2]) + 1);
	}
}

void HU_clearChatChars(void)
{
	memset(w_chat, '\0', sizeof(w_chat));
	I_SetTextInputMode(false);
	chat_on = false;
	chat_open_key = 0;
	c_input = 0;
	chat_recipient = -1;
	chat_recipient_left = false;
	chat_view_lines = 0;
	chat_scroll_drag = chat_select_drag = false;
	chat_selection_start.message = chat_selection_end.message = 0;

	I_UpdateMouseGrab();
}

//
// Returns true if key eaten
//
static boolean HU_ChatPositionBefore(chatposition_t a, chatposition_t b)
{
	return a.message < b.message || (a.message == b.message && a.offset < b.offset);
}

static boolean HU_ChatSelection(chatposition_t *first, chatposition_t *last)
{
	*first = chat_selection_start;
	*last = chat_selection_end;
	if (HU_ChatPositionBefore(*last, *first))
	{
		*first = chat_selection_end;
		*last = chat_selection_start;
	}
	return HU_ChatPositionBefore(*first, *last)
		&& HU_ChatMessage(first->message) && HU_ChatMessage(last->message);
}

static void HU_CopyChatSelection(void)
{
	chatposition_t first, last;
	if (!HU_ChatSelection(&first, &last))
		return;

	char *text = Z_Malloc((last.message - first.message + 1) * (HU_CHATDISPLAYLEN + 1), PU_STATIC, NULL);
	char *out = text;
	for (UINT32 id = first.message; id <= last.message; id++)
	{
		const char *message = HU_ChatMessage(id)->text;
		const size_t start = id == first.message ? first.offset : 0;
		const size_t end = id == last.message ? last.offset : strlen(message);
		for (size_t i = start; i < end; i++)
		{
			const UINT8 c = message[i];
			if ((c >= ' ' && c < 0x7f) || c == '\n')
				*out++ = (char)c;
		}
		if (id != last.message)
			*out++ = '\n';
	}
	if (out != text)
		I_ClipboardCopy(text, (size_t)(out - text));
	Z_Free(text);
}

static boolean HU_ChatPrintable(UINT8 c)
{
	return c >= FONTSTART && !(c & 0x80);
}

static boolean HU_ChatPoint(chatposition_t *point, boolean dragging, boolean exact)
{
	const INT32 y = chat_mouse_y - chat_view_top - 2;
	const INT32 x = chat_mouse_x - chatx - HU_CHATPAD;
	const INT32 visible = min(chat_view_lines, chat_row_count - chat_scroll);
	if (visible <= 0 || (!dragging && (y < 0 || y >= visible * HU_CHATROWHEIGHT)))
		return false;

	const chatrow_t *row = &chat_rows[chat_scroll + max(0, min(y / HU_CHATROWHEIGHT, visible - 1))];
	const chatmessage_t *message = HU_ChatMessage(row->start.message);
	if (!message || (exact && (x < 0 || x >= row->columns * HU_CHATCOLWIDTH)))
		return false;

	INT32 column = max(0, (x + (exact ? 0 : HU_CHATCOLWIDTH / 2)) / HU_CHATCOLWIDTH);
	if (dragging && y < 0)
		column = 0;
	else if (dragging && y >= visible * HU_CHATROWHEIGHT)
		column = row->columns;

	*point = row->start;
	while (point->offset < row->end)
	{
		if (HU_ChatPrintable(message->text[point->offset]) && column-- == 0)
			break;
		point->offset++;
	}
	return true;
}

static void HU_UpdateChatSelection(void)
{
	chatposition_t point;
	if (HU_ChatPoint(&point, true, false))
	{
		chat_select_moved |= point.message != chat_selection_start.message
			|| point.offset != chat_selection_start.offset;
		chat_selection_end = point;
	}
}

static void HU_SetChatScroll(INT32 row)
{
	chat_scroll = max(0, min(row, chat_maxscroll));
	chat_follow = chat_scroll == chat_maxscroll && !chat_select_drag;
}

static INT32 HU_ChatScrollbarX(void)
{
	return chatx + chat_view_width - HU_CHATBARWIDTH - 1;
}

static INT32 HU_ChatScrollbar(INT32 *top)
{
	return V_ScrollbarThumb(chat_view_lines * HU_CHATROWHEIGHT,
		chat_maxscroll, chat_view_lines, chat_scroll, top);
}

static void HU_DragChatScrollbar(void)
{
	INT32 top;
	const INT32 thumb = HU_ChatScrollbar(&top);
	const INT32 travel = chat_view_lines * HU_CHATROWHEIGHT - thumb;
	HU_SetChatScroll(V_ScrollbarValue(chat_mouse_y - chat_view_top - 2 - chat_scroll_grab,
		travel, chat_maxscroll));
}

boolean HU_ChatHistoryResponder(const event_t *ev)
{
	if (!chat_on || OLDCHAT)
		return false;

	if (ev->type == ev_keydown)
	{
		if (ctrldown && ev->key == 'c')
		{
			HU_CopyChatSelection();
			return true;
		}
		if (ev->key == KEY_MOUSEWHEELUP || ev->key == KEY_MOUSEWHEELDOWN)
		{
			HU_SetChatScroll(chat_scroll + (ev->key == KEY_MOUSEWHEELUP ? -3 : 3));
			if (chat_select_drag)
				HU_UpdateChatSelection();
			return true;
		}
	}

	if (ev->type == ev_mouseabsolute)
	{
		if (vid.dup > 0)
		{
			chat_mouse_x = ev->x < 0 ? -1 : ev->x / vid.dup;
			chat_mouse_y = ev->y < 0 ? -1 : (ev->y - (vid.height - BASEVIDHEIGHT * vid.dup)) / vid.dup;
		}
		if (!(mouse.buttons & MB_BUTTON1))
			chat_scroll_drag = chat_select_drag = false;
		if (chat_scroll_drag)
			HU_DragChatScrollbar();
		else if (chat_select_drag)
			HU_UpdateChatSelection();
		return true;
	}

	if (ev->key != KEY_MOUSE1)
		return false;

	if (ev->type == ev_keydown)
	{
		chat_scroll_drag = chat_select_drag = chat_select_moved = false;
		chat_selection_start.message = chat_selection_end.message = chat_click.message = 0;
		if (!chat_view_lines)
			return true;

		const INT32 y = chat_mouse_y - chat_view_top - 2;
		const INT32 bar_x = HU_ChatScrollbarX();
		if (y < 0 || y >= chat_view_lines * HU_CHATROWHEIGHT
			|| chat_mouse_x < chatx || chat_mouse_x >= chatx + chat_view_width)
			return true;
		if (chat_maxscroll > 0 && chat_mouse_x >= bar_x && chat_mouse_x < bar_x + HU_CHATBARWIDTH)
		{
			INT32 top;
			const INT32 thumb = HU_ChatScrollbar(&top);
			chat_scroll_drag = true;
			if (y >= top && y < top + thumb)
				chat_scroll_grab = y - top;
			else
			{
				chat_scroll_grab = thumb / 2;
				HU_DragChatScrollbar();
			}
			return true;
		}
		if (HU_ChatPoint(&chat_selection_start, false, false))
		{
			chat_selection_end = chat_selection_start;
			HU_ChatPoint(&chat_click, false, true);
			chat_select_drag = true;
			chat_follow = false;
		}
		return true;
	}

	if (ev->type == ev_keyup)
	{
		chat_scroll_drag = false;
		if (chat_select_drag)
		{
			HU_UpdateChatSelection();
			chat_select_drag = false;
			if (!chat_select_moved)
			{
				chatposition_t point;
				const chatmessage_t *message = HU_ChatMessage(chat_click.message);
				if (message && HU_ChatPoint(&point, false, true)
					&& point.message == chat_click.message && point.offset == chat_click.offset)
				{
					if (point.offset >= message->name_start && point.offset < message->name_end
						&& !CHAT_MUTE && HU_ChatRecipientAvailable(message->player))
					{
						chat_recipient = message->player;
						chat_recipient_left = false;
						teamtalk = false;
					}
					else if (message->link[point.offset])
						HU_OpenChatURL(message, point.offset);
				}
				chat_selection_start.message = chat_selection_end.message = 0;
			}
		}
		return true;
	}

	return false;
}

boolean HU_Responder(event_t *ev)
{
	if (ev->type != ev_keydown && ev->type != ev_text)
		return false;

	const INT32 c = ev->key;
	if (!chat_on)
	{
		if (ev->type == ev_text || !netgame || OLD_MUTE)
			return false;

		const boolean talk = c == gamecontrol[GC_TALKKEY][0] || c == gamecontrol[GC_TALKKEY][1];
		const boolean team = c == gamecontrol[GC_TEAMKEY][0] || c == gamecontrol[GC_TEAMKEY][1];
		if (!talk && !team)
			return false;

		I_SetTextInputMode(true);
		chat_on = true;
		chat_open_key = c;
		w_chat[0] = 0;
		c_input = 0;
		teamtalk = !talk && G_GametypeHasTeams();
		chat_follow = true;
		typelines = 1;
		I_SetMouseGrab(false);
		return true;
	}
	else
	{
		const boolean opening_text = ev->type == ev_text && chat_open_key >= ' ' && chat_open_key <= '~'
			&& (UINT32)c < 128 && tolower(c) == tolower(chat_open_key);
		chat_open_key = 0;
		if (opening_text)
			return true;
		if (ev->type == ev_text)
		{
			if ((c < FONTSTART || c > FONTEND || !hu_font.chars[c-FONTSTART])
				&& c != ' ') // Allow spaces, of course
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

		// Ignore modifier keys
		// Note that we do this here so users can still set
		// their chat keys to one of these, if they so desire.
		if (ev->key == KEY_LSHIFT || ev->key == KEY_RSHIFT
		 || ev->key == KEY_LCTRL || ev->key == KEY_RCTRL
		 || ev->key == KEY_LALT || ev->key == KEY_RALT)
			return true;


		// pasting. pasting is cool. chat is a bit limited, though :(
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
				return true; // we can't paste this!!

			memmove(&w_chat[c_input + pastelen], &w_chat[c_input], (chatlen - c_input) + 1); // +1 for '\0'
			memcpy(&w_chat[c_input], paste, pastelen); // copy all of that.
			c_input += pastelen;
			return true;
		}
		else if (c == KEY_ENTER)
		{
			if (!CHAT_MUTE)
				HU_sendChatMessage();
			HU_clearChatChars();
		}
		else if (c == KEY_ESCAPE
			|| ((c == gamecontrol[GC_TALKKEY][0] || c == gamecontrol[GC_TALKKEY][1]
			|| c == gamecontrol[GC_TEAMKEY][0] || c == gamecontrol[GC_TEAMKEY][1])
			&& c >= KEY_MOUSE1))
			HU_clearChatChars();
		else if (!OLDCHAT && (c == KEY_UPARROW || c == KEY_DOWNARROW))
			HU_SetChatScroll(chat_scroll + (c == KEY_UPARROW ? -1 : 1));
		else if (c == KEY_LEFTARROW && c_input != 0 && !OLDCHAT) // i said go back
		{
			if (ctrldown)
				c_input = M_JumpWordReverse(w_chat, c_input);
			else
				c_input--;
		}
		else if (c == KEY_RIGHTARROW && c_input < strlen(w_chat) && !OLDCHAT) // don't need to check for admin or w/e here since the chat won't ever contain anything if it's muted.
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


//======================================================================
//                         HEADS UP DRAWING
//======================================================================


// HU_DrawMiniChat

static void HU_drawMiniChat(void)
{
	INT32 x = chatx+2, y;
	INT32 chatheight = 0;
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 dx = 0, dy = 0;
	boolean prev_linereturn = false;

	if (!chat_nummsg_min)
		return; // needless to say it's useless to do anything if we don't have anything to draw.

	for (size_t i = chat_nummsg_min; i > 0; i--)
	{
		char *msg = V_ChatWordWrap(0, boxw-charwidth-2, V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE|V_MONOSPACE, chat_mini[i-1]);
		for(size_t j = 0; msg[j]; j++) // iterate through msg
		{
			if (msg[j] == '\n') // get back down.
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

	for (size_t i = 0; i < chat_nummsg_min; i++) // iterate through our hot messages
	{
		INT32 timer = ((cv_chattime.value*TICRATE)-chat_timers[i]) - cv_chattime.value*TICRATE+9; // see below...
		INT32 transflag = (timer >= 0 && timer <= 9) ? (timer*V_10TRANS) : 0; // you can make bad jokes out of this one.
		char *msg = V_ChatWordWrap(0, boxw-charwidth-2, V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE|V_MONOSPACE, chat_mini[i]); // get the current message, and word wrap it.
		UINT8 *colormap = NULL;

		for(size_t j = 0; msg[j]; j++) // iterate through msg
		{
			if (msg[j] == '\n') // get back down.
			{
				if (!prev_linereturn)
				{
					dy += charheight;
					dx = 0;
				}
				prev_linereturn = true;
			}
			else if (msg[j] & 0x80) // get colormap
				colormap = V_GetStringColormap(((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK);
			else if (msg[j] >= FONTSTART)
			{
				prev_linereturn = false;

				if (cv_chatbacktint.value) // on request of wolfy
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

	// decrement addy and make that shit smooth:
	addy /= 2;
}

// HU_DrawChatLog

static void HU_LayoutChat(INT32 width)
{
	if (!chat_layout_dirty && chat_layout_width == width)
		return;

	const chatposition_t anchor = chat_row_count ? chat_rows[chat_scroll].start : (chatposition_t){0, 0};
	const INT32 columns = max(1, (width - HU_CHATPAD - HU_CHATBARWIDTH - 2) / HU_CHATCOLWIDTH);
	chat_layout_width = width;
	chat_layout_dirty = false;
	chat_row_count = 0;

	for (UINT32 i = 0; i < chat_nummsg_log; i++)
	{
		char *wrapped = V_ChatWordWrap(0, columns * HU_CHATCOLWIDTH, V_ALLOWLOWERCASE | V_MONOSPACE, chat_log[i].text);
		chatrow_t row = {{chat_first_message + i, 0}, 0, 0, 0x80};
		UINT8 color = row.color;
		UINT16 offset;
		for (offset = 0; wrapped[offset]; offset++)
		{
			const UINT8 c = wrapped[offset];
			const boolean printable = HU_ChatPrintable(c);
			if (c == '\n' || (printable && row.columns == columns))
			{
				row.end = offset;
				chat_rows[chat_row_count++] = row;
				row.start.offset = offset + (c == '\n');
				row.columns = 0;
				row.color = color;
			}
			if (c & 0x80)
				color = c;
			else if (printable)
				row.columns++;
		}
		row.end = offset;
		chat_rows[chat_row_count++] = row;
		Z_Free(wrapped);
	}

	if (!chat_follow)
	{
		chat_scroll = 0;
		for (INT32 i = 0; i < chat_row_count && !HU_ChatPositionBefore(anchor, chat_rows[i].start); i++)
			chat_scroll = i;
	}
}

static INT32 HU_ChatColumn(const chatrow_t *row, const char *text, UINT16 offset)
{
	INT32 column = 0;
	for (UINT16 i = row->start.offset; i < min(row->end, offset); i++)
		column += HU_ChatPrintable(text[i]);
	return column;
}

static void HU_drawChatLog(INT32 offset)
{
	INT32 width = cv_chatwidth.value, visible_lines = cv_chatheight.value;
	INT32 bottom = chaty - offset * HU_CHATROWHEIGHT - 12;
#ifdef NETSPLITSCREEN
	if (splitscreen)
	{
		visible_lines = max(6, visible_lines / 2);
		bottom -= BASEVIDHEIGHT / 2;
		if (splitscreen > 1)
			width = max(64, width / 2);
	}
#endif

	chat_view_width = width;
	chat_view_lines = max(0, min(visible_lines, bottom / HU_CHATROWHEIGHT));
	if (!chat_view_lines)
		return;

	HU_LayoutChat(width);
	chat_maxscroll = max(0, chat_row_count - chat_view_lines);
	chat_scroll = chat_follow ? chat_maxscroll : min(chat_scroll, chat_maxscroll);

	const INT32 height = chat_view_lines * HU_CHATROWHEIGHT;
	chat_view_top = bottom - height;
	V_DrawFillConsoleMap(chatx, chat_view_top, width, height + 2, 239 | HU_CHATDRAWFLAGS);
	if (chat_maxscroll > 0)
	{
		INT32 top;
		const INT32 thumb = HU_ChatScrollbar(&top);
		const INT32 x = HU_ChatScrollbarX();
		V_DrawFill(x, chat_view_top + 2, HU_CHATBARWIDTH, height, HU_CHATDRAWFLAGS | 159);
		V_DrawFill(x, chat_view_top + 2 + top, HU_CHATBARWIDTH, thumb,
			HU_CHATDRAWFLAGS | (chat_scroll_drag ? 120 : 148));
	}

	chatposition_t first, last;
	const boolean selected = HU_ChatSelection(&first, &last);
	UINT8 *linkmap = V_GetStringColormap(V_AQUAMAP);
	const INT32 end = min(chat_scroll + chat_view_lines, chat_row_count);
	for (INT32 i = chat_scroll; i < end; i++)
	{
		const chatrow_t *row = &chat_rows[i];
		const UINT32 id = row->start.message;
		const chatmessage_t *message = HU_ChatMessage(id);
		const INT32 y = chat_view_top + 2 + (i - chat_scroll) * HU_CHATROWHEIGHT;
		const INT32 left = chatx + HU_CHATPAD;
		const char *text = message->text;
		if (selected && id >= first.message && id <= last.message)
		{
			const INT32 from = id == first.message ? HU_ChatColumn(row, text, first.offset) : 0;
			const INT32 to = id == last.message ? HU_ChatColumn(row, text, last.offset) : row->columns;
			if (to > from)
				V_DrawFill(left + from * HU_CHATCOLWIDTH, y, (to - from) * HU_CHATCOLWIDTH,
					HU_CHATROWHEIGHT, HU_CHATDRAWFLAGS | 156);
		}

		INT32 x = left;
		UINT8 *colormap = V_GetStringColormap((row->color & 0x7f) << V_CHARCOLORSHIFT);
		for (UINT16 j = row->start.offset; j < row->end; j++)
		{
			const UINT8 c = text[j];
			if (c & 0x80)
				colormap = V_GetStringColormap((c & 0x7f) << V_CHARCOLORSHIFT);
			else if (HU_ChatPrintable(c))
			{
				V_DrawChatCharacter(x, y, c | HU_CHATDRAWFLAGS | V_MONOSPACE, true,
					message->link[j] ? linkmap : colormap);
				x += HU_CHATCOLWIDTH;
			}
		}
	}
}
//
// HU_DrawChat
//
// Draw chat input
//

static void HU_DrawChat(void)
{
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 t = 0, c = 0, y = chaty - (typelines*charheight);
	UINT32 i = 0, saylen = strlen(w_chat); // You learn new things everyday!
	INT32 cflag = 0;
	const char *ntalk = "Say: ", *ttalk = "Team: ";
	const char *talk = ntalk;
	char private_prompt[MAXPLAYERNAME + 8];

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

	if (chat_recipient >= 0)
	{
		if (chat_recipient_left || !HU_ChatRecipientAvailable(chat_recipient))
			talk = "PM (left): ";
		else
		{
			const INT32 maxname = max(1, boxw / charwidth - 7);
			snprintf(private_prompt, sizeof private_prompt, "To %.*s: ", maxname, player_names[chat_recipient]);
			talk = private_prompt;
		}
	}
	else if (teamtalk)
		talk = ttalk;

	if (CHAT_MUTE)
	{
		if (cv_mute.value)
			talk = "Chat has been muted.";
		else
			talk = "You have been muted.";
		typelines = 1;
		cflag = V_GRAYMAP; // set text in gray if chat is muted.
	}

	V_DrawFillConsoleMap(chatx, y-1, boxw, (typelines*charheight), 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

	for (i = 0; talk[i]; i++)
	{
		if (talk[i] >= FONTSTART)
			V_DrawChatCharacter(chatx + c + 2, y, talk[i] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|cflag, true, V_GetStringColormap(talk[i]|cflag));
		c += charwidth;
	}

	// if chat is muted, just draw the log and get it over with, no need to draw anything else.
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
			INT32 cursorx = (c+charwidth < boxw-charwidth) ? (chatx + 2 + c+charwidth) : (chatx+1); // we may have to go down.
			INT32 cursory = (cursorx != chatx+1) ? (y) : (y+charheight);
			if (hu_tick < 4)
				V_DrawChatCharacter(cursorx, cursory+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, true, NULL);

			if (cursorx == chatx+1 && saylen == i) // a weirdo hack
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

	// handle /pm list. It's messy, horrible and I don't care.
	if (chat_recipient < 0 && strnicmp(w_chat, "/pm", 3) == 0 && vid.width >= 400 && !teamtalk) // 320x200 unsupported kthxbai
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
			// filter: (code needs optimization pls help I'm bad with C)
			if (w_chat[3])
			{
				char playernum[3+1];
				UINT32 n;
				// right, that's half important: (w_chat[4] may be a space since /pm0 msg is perfectly acceptable!)
				if ( ( ((w_chat[3] != 0) && ((w_chat[3] < '0') || (w_chat[3] > '9'))) || ((w_chat[4] != 0) && (((w_chat[4] < '0') || (w_chat[4] > '9'))))) && (w_chat[4] != ' '))
					break;

				strncpy(playernum, w_chat+3, sizeof(playernum)-1);
				playernum[3] = 0;
				n = atoi(playernum); // turn that into a number
				// special cases:
				if ((n == 0) && !(w_chat[4] == '0') && (!(i<10)))
					continue;
				else if ((n == 1) && !(w_chat[3] == '0') && (!((i == 1) || ((i >= 10) && (i <= 19)))))
					continue;
				else if ((n == 2) && !(w_chat[3] == '0') && (!((i == 2) || ((i >= 20) && (i <= 29)))))
					continue;
				else if ((n == 3) && !(w_chat[3] == '0') && (!((i == 3) || ((i >= 30) && (i <= 31)))))
					continue;
				else	// general case.
					if (i != n) continue;
			}

			if (playeringame[i])
			{
				char name[MAXPLAYERNAME+1];
				strlcpy(name, player_names[i], 7); // shorten name to 7 characters.
				V_DrawFillConsoleMap(chatx+ boxw + 2, p_dispy- (6*count), 48, 6, 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT); // fill it like the chat so the text doesn't become hard to read because of the hud.
				V_DrawSmallString(chatx+ boxw + 4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, va("\x82%d\x80 - %s", i, name));
				count++;
			}
		}
		if (count == 0) // no results.
		{
			V_DrawFillConsoleMap(chatx+boxw+2, p_dispy- (6*count), 48, 6, 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT); // fill it like the chat so the text doesn't become hard to read because of the hud.
			V_DrawSmallString(chatx+boxw+4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, "NO RESULT.");
		}
	}

	HU_drawChatLog(typelines-1); // typelines is the # of lines we're typing. If there's more than 1 then the log should scroll up to give us more space.
}


// For anyone who, for some godforsaken reason, likes oldchat.

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
			INT32 cursorx = (HU_INPUTX+c+charwidth < vid.width) ? (HU_INPUTX + c + charwidth) : (HU_INPUTX); // we may have to go down.
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

// Draw crosshairs at the exact center of the view.
// In splitscreen, crosshairs are stretched vertically to compensate for V_PERPLAYER squishing them.
// Crosshairs are pre-cached at HU_Init

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

	UINT32 realflags = cechoflags|V_PERPLAYER; // requested as part of splitscreen's stuff
	INT32 realalpha = (INT32)((cechoflags & V_ALPHAMASK) >> V_ALPHASHIFT);

	char *line;
	char *echoptr;
	char temp[1024];

	for (i = 0; cechotext[i] != '\0'; ++i)
		if (cechotext[i] == '\\')
			pnumlines++;

	y -= (pnumlines-1)*((realflags & V_RETURN8) ? 4 : 6);

	// Prevent crashing because I'm sick of this
	if (y < 0)
	{
		CONS_Alert(CONS_WARNING, "CEcho contained too many lines, not displaying\n");
		cechotimer = 0;
		return;
	}

	// Automatic fadeout
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

//
// demo info stuff
//
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

// Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
	// draw chat string plus cursor
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


		if (!OLDCHAT && cv_consolechat.value < 2 && netgame) // Don't display minimized chat if you set the mode to Window (Hidden)
			HU_drawMiniChat(); // draw messages in a cool fashion.
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

	// draw multiplayer rankings
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

	// draw the crosshair
	if (cv_translucenthud.value && LUA_HudEnabled(hud_crosshair))
		HU_DrawCrosshairs();

	// draw desynch text
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

//======================================================================
//                   IN-LEVEL MULTIPLAYER RANKINGS
//======================================================================

#define supercheckdef (!(players[tab[i].num].charflags & SF_NOSUPERSPRITES) && ((players[tab[i].num].powers[pw_super] && players[tab[i].num].mo && (players[tab[i].num].mo->state < &states[S_PLAY_SUPER_TRANS1] || players[tab[i].num].mo->state >= &states[S_PLAY_SUPER_TRANS6])) || (players[tab[i].num].powers[pw_carry] == CR_NIGHTSMODE && skins[players[tab[i].num].skin]->flags & SF_SUPER)))
#define greycheckdef (players[tab[i].num].spectator || players[tab[i].num].playerstate == PST_DEAD || (G_IsSpecialStage(gamemap) && players[tab[i].num].exiting))

//
// HU_drawPing
//
void HU_drawPing(INT32 x, INT32 y, UINT32 ping, boolean notext, INT32 flags)
{
	UINT8 numbars = 0; // how many ping bars do we draw?
	UINT8 barcolor = 31; // color we use for the bars (green, yellow, red or black)
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

	if (ping < UINT32_MAX && (!notext || vid.width >= 640)) // how sad, we're using a shit resolution.
		V_DrawSmallString(dx, y+4, V_ALLOWLOWERCASE|flags, va("%dms", ping));

	for (i=0; (i<3); i++) // Draw the ping bar
	{
		V_DrawFill(x+2 *(i-1), y+yoffset-4, 2, 8-yoffset, 31|flags);
		if (i < numbars)
			V_DrawFill(x+2 *(i-1), y+yoffset-3, 1, 8-yoffset-1, barcolor|flags);

		yoffset -= 2;
	}

	if (ping == UINT32_MAX)
		V_DrawSmallScaledPatch(x + 4 - nopingicon->width/2, y + 9 - nopingicon->height/2, 0, nopingicon);
}

static const UINT8 *HU_PendingFaceColormap(const player_t *player)
{
	if (player->mo && player->mo->color != SKINCOLOR_NONE)
		return R_GetTranslationColormap(player->skin, player->mo->color, GTC_CACHE);


	return R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_BLUE, GTC_CACHE);
}

//
// HU_DrawTabRankings
//
void HU_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	boolean greycheck, supercheck;

	//this function is designed for 9 or less score lines only
	I_Assert(scorelines <= 9);

	V_DrawFill(1, 26, 318, 1, 0); //Draw a horizontal line because it looks nice!

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametyperankings[gametype] != GT_COOP)
			continue; //ignore them.

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		if (!splitscreen) // don't draw it on splitscreen,
		{
			if (tab[i].num != serverplayer)
				HU_drawPing(x + 253, y, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], false, 0);
			//else
			//	V_DrawSmallString(x+ 246, y+4, V_YELLOWMAP, "SERVER");
		}

		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 20, y,
		                 ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		                 | (greycheck ? V_60TRANS : 0)
		                 | V_ALLOWLOWERCASE, tab[i].name);

		// Draw emeralds
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
			if (supercheck)
				V_DrawSmallScaledPatch(x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				colormap = HU_PendingFaceColormap(&players[tab[i].num]);
				if (greycheck)
					V_DrawSmallTranslucentMappedPatch(x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch(x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
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

		if (G_GametypeUsesLives() && !(G_GametypeUsesCoopLives() && (cv_cooplives.value == 0 || cv_cooplives.value == 3)) && (players[tab[i].num].lives != INFLIVES)) //show lives
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

//
// HU_Draw32Emeralds
//
static void HU_Draw32Emeralds(INT32 x, INT32 y, INT32 pemeralds)
{
	//Draw the emeralds, in the CORRECT order, using tiny emerald sprites.
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

//
// HU_Draw32TeamTabRankings
//
static void HU_Draw32TeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two teams.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		if (players[tab[i].num].ctfteam == 1) //red
		{
			redplayers++;
			x = 14 + (BASEVIDWIDTH/2);
			y = (redplayers * 9) + 20;
		}
		else if (players[tab[i].num].ctfteam == 2) //blue
		{
			blueplayers++;
			x = 14;
			y = (blueplayers * 9) + 20;
		}
		else //er?  not on red or blue, so ignore them
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
			if (players[tab[i].num].gotflag & GF_REDFLAG) // Red
				V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, rflagico, 0);
			else if (players[tab[i].num].gotflag & GF_BLUEFLAG) // Blue
				V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, bflagico, 0);
		}

		// Draw emeralds
		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, 255);
			//HU_DrawEmeralds(x-12,y+2,255);
		}
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, tab[i].emeralds);
			//HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
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
			//else
				//V_DrawSmallString(x+ 129, y+4, V_YELLOWMAP, "HOST");
		}
	}
}

//
// HU_DrawTeamTabRankings
//
void HU_DrawTeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	boolean smol = false;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	// before we draw, we must count how many players are in each team. It makes an additional loop, but we need to know if we have to draw a big or a small ranking.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		if (players[tab[i].num].ctfteam == 1) //red
		{
			if (redplayers++ > 8)
			{
				smol = true;
				break; // don't make more loops than we need to.
			}
		}
		else if (players[tab[i].num].ctfteam == 2) //blue
		{
			if (blueplayers++ > 8)
			{
				smol = true;
				break;
			}
		}
		else //er?  not on red or blue, so ignore them
			continue;

	}

	// I'll be blunt with you, this may add more lines, but I'm not adding weird cases for this, so we're executing a separate function.
	if (smol == true || cv_compactscoreboard.value)
	{
		HU_Draw32TeamTabRankings(tab, whiteplayer);
		return;
	}

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two teams.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	i=0, redplayers=0, blueplayers=0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		if (players[tab[i].num].ctfteam == 1) //red
		{
			if (redplayers++ > 8)
				continue;
			x = 32 + (BASEVIDWIDTH/2);
			y = (redplayers * 16) + 16;
		}
		else if (players[tab[i].num].ctfteam == 2) //blue
		{
			if (blueplayers++ > 8)
				continue;
			x = 32;
			y = (blueplayers * 16) + 16;
		}
		else //er?  not on red or blue, so ignore them
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
			if (players[tab[i].num].gotflag & GF_REDFLAG) // Red
				V_DrawSmallScaledPatch(x-28, y-4, 0, rflagico);
			else if (players[tab[i].num].gotflag & GF_BLUEFLAG) // Blue
				V_DrawSmallScaledPatch(x-28, y-4, 0, bflagico);
		}

		// Draw emeralds
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
			//else
			//	V_DrawSmallString(x+ 94, y+4, V_YELLOWMAP, "SERVER");
		}
	}
}

//
// HU_DrawDualTabRankings
//
void HU_DrawDualTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two sides.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametyperankings[gametype] != GT_COOP)
			continue; //ignore them.

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 7);
		if (tab[i].num != serverplayer)
			HU_drawPing(x+ 113, y, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], false, 0);
		//else
		//	V_DrawSmallString(x+ 94, y+4, V_YELLOWMAP, "SERVER");

		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 20, y,
			             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
			             | (greycheck ? V_TRANSLUCENT : 0)
			             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives() && !(G_GametypeUsesCoopLives() && (cv_cooplives.value == 0 || cv_cooplives.value == 3)) && (players[tab[i].num].lives != INFLIVES)) //show lives
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE, va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawSmallScaledPatch(x-28, y-4, 0, tagico);

		if (players[tab[i].num].exiting || (players[tab[i].num].pflags & PF_FINISHED))
			V_DrawSmallScaledPatch(x - exiticon->width/2 - 1, y-3, 0, exiticon);

		// Draw emeralds
		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
			HU_DrawEmeralds(x-12,y+2,255);
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		//V_DrawSmallScaledPatch (x, y-4, 0, livesback);
		if (tab[i].color == 0)
		{
			if (supercheck)
				V_DrawSmallScaledPatch (x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				colormap = HU_PendingFaceColormap(&players[tab[i].num]);
				if (greycheck)
					V_DrawSmallTranslucentMappedPatch(x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch(x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
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

		// All data drawn with thin string for space.
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

//
// HU_Draw32TabRankings
//
static void HU_Draw32TabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two sides.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametyperankings[gametype] != GT_COOP)
			continue; //ignore them.

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 7);
		if (!splitscreen) // don't draw it on splitscreen,
		{
			if (tab[i].num != serverplayer)
				HU_drawPing(x+ 135, y+1, players[tab[i].num].quittime ? UINT32_MAX : playerpingtable[tab[i].num], true, 0);
			//else
			//	V_DrawSmallString(x+ 129, y+4, V_YELLOWMAP, "HOST");
		}

		if (!players[tab[i].num].quittime || (leveltime / (TICRATE/2) & 1))
			V_DrawString(x + 10, y,
			             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
			             | (greycheck ? V_TRANSLUCENT : 0)
			             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives()) //show lives
			V_DrawRightAlignedThinString(x-1, y, V_ALLOWLOWERCASE, va("%d", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, tagico, 0);

		// Draw emeralds
		if (players[tab[i].num].powers[pw_invulnerability] && (players[tab[i].num].powers[pw_invulnerability] == players[tab[i].num].powers[pw_sneakers]) && ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, 255);
			//HU_DrawEmeralds(x-12,y+2,255);
		}
		else if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, tab[i].emeralds);
			//HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		//V_DrawSmallScaledPatch (x, y-4, 0, livesback);
		if (tab[i].color == 0)
		{
			if (players[tab[i].num].powers[pw_super] && !(players[tab[i].num].charflags & SF_NOSUPERSPRITES))
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], 0);
			else
			{
				colormap = HU_PendingFaceColormap(&players[tab[i].num]);
				if (greycheck)
					V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], colormap);
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

		// All data drawn with thin string for space.
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

//
// HU_DrawEmeralds
//
void HU_DrawEmeralds(INT32 x, INT32 y, INT32 pemeralds)
{
	//Draw the emeralds, in the CORRECT order, using tiny emerald sprites.
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

//
// HU_DrawSpectatorTicker
//
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

//
// HU_DrawRankings
//
static void HU_DrawRankings(void)
{
	playersort_t tab[MAXPLAYERS];
	INT32 i, j, scorelines;
	boolean completed[MAXPLAYERS];
	UINT32 whiteplayer;

	// draw the current gametype in the lower right
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

	// When you play, you quickly see your score because your name is displayed in white.
	// When playing back a demo, you quickly see who's the view.
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
				// todo put something more fitting for the gametype here, such as current
				// number of categories led
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

	//if (scorelines > 20)
	//	scorelines = 20; //dont draw past bottom of screen, show the best only
	// shush, we'll do it anyway.

	if (G_GametypeHasTeams())
		HU_DrawTeamTabRankings(tab, whiteplayer);
	else if (scorelines <= 9 && !cv_compactscoreboard.value)
		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
	else if (scorelines <= 18 && !cv_compactscoreboard.value)
		HU_DrawDualTabRankings(32, 32, tab, scorelines, whiteplayer);
	else
		HU_Draw32TabRankings(14, 28, tab, scorelines, whiteplayer);

	// draw spectators in a ticker across the bottom
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


// Interface to CECHO settings for the outside world, avoiding the
// expense (and security problems) of going via the console buffer.
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
	// Don't allow cechoflags to contain any bits in V_PARAMMASK
	cechoflags = (flags & ~V_PARAMMASK);
}

void HU_DoCEcho(const char *msg)
{
	I_OutputMsg("%s\n", msg); // print to log

	strncpy(cechotext, msg, sizeof(cechotext)-1);
	strncat(cechotext, "\\", sizeof(cechotext) - strlen(cechotext) - 1);
	cechotext[sizeof(cechotext) - 1] = '\0';
	cechotimer = cechoduration;
}
