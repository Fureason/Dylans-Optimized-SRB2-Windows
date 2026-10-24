// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifdef __GNUC__
#include <unistd.h>
#endif

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "g_input.h"
#include "hu_stuff.h"
#include "keys.h"
#include "r_main.h"
#include "r_defs.h"
#include "sounds.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_threads.h"
#include "d_main.h"
#include "m_menu.h"
#include "filesrch.h"
#include "m_misc.h"

#ifdef _WINDOWS
#include "win32/win_main.h"
#endif

#ifdef _WIN32
#include <shellapi.h>
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#define MAXHUDLINES 20

#ifdef HAVE_THREADS
I_mutex con_mutex;

#  define Lock_state()    I_lock_mutex(&con_mutex)
#  define Unlock_state() I_unlock_mutex(con_mutex)
#else 
#  define Lock_state()
#  define Unlock_state()
#endif 

static boolean con_started = false;  
       boolean con_startup = false;  
       boolean con_refresh = false;  
static boolean con_forcepic = true;  
       boolean con_recalc;           

static tic_t con_tick;  


static boolean consoletoggle;  
static boolean consoleready;   

       INT32 con_destlines;  
static INT32 con_curlines;   

static UINT8  con_hudlines;              
static UINT32 con_hudtime[MAXHUDLINES];  


static char *con_line;           
static size_t con_cx;            
static size_t con_cy;            



static size_t con_totallines;       
static size_t con_width;            
static size_t con_historylines;     

static size_t con_scrollup;         
UINT32 con_scalefactor;             


#define CON_MAXPROMPTCHARS 256
#define CON_PROMPTCHAR '$'

static char inputlines[32][CON_MAXPROMPTCHARS];  

static INT32 inputline;     
static INT32 inputhist;     
static size_t input_cur;  
static size_t input_sel;  
static size_t input_len;  



static void CON_InputInit(void);
static void CON_RecalcSize(void);
static void CON_ChangeHeight(void);
static size_t CON_MaxScroll(void);

static void CON_DrawBackpic(void);
static void CONS_height_Change(void);
static void CONS_hudlines_Change(void);
static void CONS_backcolor_Change(void);




#ifdef macintosh
#define CON_BUFFERSIZE 4096  
#else
                                                                            
                                                                       
#define CON_BUFFERSIZE (8*1024*1024)
#endif

static char con_buffer[CON_BUFFERSIZE];



static CV_PossibleValue_t hudtime_cons_t[] = {{0, "MIN"}, {99999999, "MAX"}, {0, NULL}};
static consvar_t cons_hudtime = CVAR_INIT ("con_hudtime", "5", CV_SAVE, hudtime_cons_t, NULL);


static CV_PossibleValue_t hudlines_cons_t[] = {{0, "MIN"}, {MAXHUDLINES, "MAX"}, {0, NULL}};
static consvar_t cons_hudlines = CVAR_INIT ("con_hudlines", "5", CV_CALL|CV_SAVE, hudlines_cons_t, CONS_hudlines_Change);



static CV_PossibleValue_t speed_cons_t[] = {{0, "MIN"}, {64, "MAX"}, {0, NULL}};
static consvar_t cons_speed = CVAR_INIT ("con_speed", "8", CV_SAVE, speed_cons_t, NULL);


static consvar_t cons_height = CVAR_INIT ("con_height", "50", CV_CALL|CV_SAVE, CV_Unsigned, CONS_height_Change);



static CV_PossibleValue_t lines_cons_t[] = {{-1, "MIN"}, {64, "MAX"}, {0, NULL}};
consvar_t cons_lines = CVAR_INIT ("con_lines", "-1", CV_CALL|CV_SAVE, lines_cons_t, CONS_height_Change);

static CV_PossibleValue_t backpic_cons_t[] = {{0, "Translucent"}, {1, "Picture"}, {2, "None"}, {0, NULL}};

consvar_t cons_backpic = CVAR_INIT ("con_backpic", "Translucent", CV_SAVE, backpic_cons_t, NULL);

static CV_PossibleValue_t backcolor_cons_t[] = {{0, "White"}, 		{1, "Black"},		{2, "Sepia"},
												{3, "Brown"},		{4, "Pink"},		{5, "Raspberry"},
												{6, "Red"},			{7, "Creamsicle"},	{8, "Orange"},
												{9, "Gold"},		{10,"Yellow"},		{11,"Emerald"},
												{12,"Green"},		{13,"Cyan"},		{14,"Steel"},
												{15,"Periwinkle"},	{16,"Blue"},		{17,"Purple"},
												{18,"Lavender"},
												{0, NULL}};


consvar_t cons_backcolor = CVAR_INIT ("con_backcolor", "Green", CV_CALL|CV_SAVE, backcolor_cons_t, CONS_backcolor_Change);

static void CON_Print(char *msg);



static void CONS_height_Change(void)
{
	Lock_state();

	if (con_destlines > 0 && !con_startup)  
		CON_ChangeHeight();  

	Unlock_state();
}



static void CONS_hudlines_Change(void)
{
	INT32 i;

	Lock_state();


	for (i = 0; i < con_hudlines; i++)
		con_hudtime[i] = 0;

	con_hudlines = cons_hudlines.value;

	Unlock_state();

	CONS_Printf(M_GetText("Number of console HUD lines is now %d\n"), con_hudlines);
}



static void CONS_Clear_f(void)
{
	Lock_state();

	memset(con_buffer, 0, CON_BUFFERSIZE);

	con_cx = 0;
	con_cy = con_totallines-1;
	con_line = &con_buffer[con_cy*con_width];
	con_historylines = 1;
	con_scrollup = 0;

	Unlock_state();
}



                                  
 
                                 
                                                              
   

static char *bindtable[NUMINPUTS];

static void CONS_Bind_f(void)
{
	size_t na;
	char *newcmd = NULL;

	unsigned int i;
	INT32 key;

	na = COM_Argc();

	if (na < 2)
	{
		CONS_Printf(M_GetText("bind <keyname> [<command>] [<arg1>] [...]: create shortcut keys to command(s)\n"));
		CONS_Printf("\x82%s", M_GetText("Bind table :\n"));
		na = 0;
		for (key = 0; key < NUMINPUTS; key++)
			if (bindtable[key])
			{
				CONS_Printf("%s : \"%s\"\n", G_KeyNumToName(key), bindtable[key]);
				na = 1;
			}
		if (!na)
			CONS_Printf(M_GetText("(empty)\n"));
		return;
	}

	key = G_KeyNameToNum(COM_Argv(1));
	if (key <= 0 || key >= NUMINPUTS)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Invalid key name\n"));
		return;
	}

	Z_Free(bindtable[key]);
	bindtable[key] = NULL;

	if (na < 3)
		return;

	for (i = 2; i < na; ++i)
	{
		const char *arg = COM_Argv(i);


		if (i > 2)
		{
			size_t newlen = strlen(bindtable[key]) + strlen(arg) + 1;  
			size_t curpos = newcmd - bindtable[key];  

			newcmd = bindtable[key] = Z_Realloc(bindtable[key], newlen, PU_STATIC, NULL);
			newcmd += curpos;  

			newcmd[0] = ' ';  
			++newcmd;  
		}

		else

			newcmd = bindtable[key] = Z_Calloc(strlen(arg) + 1, PU_STATIC, NULL);


		strcpy(newcmd, arg);


		newcmd += strlen(arg);
	}
}










UINT8 *yellowmap, *magentamap, *lgreenmap, *bluemap, *graymap, *redmap, *orangemap, *skymap, *purplemap, *aquamap, *peridotmap, *azuremap, *brownmap, *rosymap, *invertmap;


UINT8 *consolebgmap = NULL;
UINT8 *promptbgmap = NULL;
static UINT8 promptbgcolor = UINT8_MAX;

void CON_SetupBackColormapEx(INT32 color, boolean prompt)
{
	UINT16 i, palsum;
	UINT8 j, palindex;
	UINT8 *pal = W_CacheLumpName(GetPalette(), PU_CACHE);
	INT32 shift = 6;

	if (color == INT32_MAX)
		color = cons_backcolor.value;

	shift = 6;  

	switch (color)
	{
		case 0:		palindex = 15; 	break; 	 
		case 1:		palindex = 31;	break; 	 
		case 2:		palindex = 251;	break;	 
		case 3:		palindex = 239;	break; 	 
		case 4:		palindex = 215; shift = 7; 	break; 	 
		case 5:		palindex = 37; shift = 7;	break; 	 
		case 6:		palindex = 47; shift = 7;	break; 	 
		case 7:		palindex = 53;	shift = 7;	break;	 
		case 8:		palindex = 63;	break; 	 
		case 9:		palindex = 56; shift = 7;	break; 	 
		case 10:	palindex = 79; shift = 7;	break; 	 
		case 11:	palindex = 119; shift = 7; 	break; 	 
		case 12:	palindex = 111;	break; 	 
		case 13:	palindex = 136;	shift = 7; break; 	 
		case 14:	palindex = 175; shift = 7;	break; 	 
		case 15:	palindex = 166;	shift = 7; 	break; 	 
		case 16:	palindex = 159;	break; 	 
		case 17:	palindex = 187; shift = 7; 	break; 	 
		case 18:	palindex = 199; shift = 7; 	break; 	 

		default:	palindex = 111; break;
	}

	if (prompt)
	{
		if (!promptbgmap)
			promptbgmap = (UINT8 *)Z_Malloc(256, PU_STATIC, NULL);

		if (color == promptbgcolor)
			return;
		else
			promptbgcolor = color;
	}
	else if (!consolebgmap)
		consolebgmap = (UINT8 *)Z_Malloc(256, PU_STATIC, NULL);


	for (i = 0, j = 0; i < 768; i += 3, j++)
	{
		palsum = (pal[i] + pal[i+1] + pal[i+2]) >> shift;
		if (prompt)
			promptbgmap[j] = (UINT8)(palindex - palsum);
		else
			consolebgmap[j] = (UINT8)(palindex - palsum);
	}
}

void CON_SetupBackColormap(void)
{
	CON_SetupBackColormapEx(cons_backcolor.value, false);
	CON_SetupBackColormapEx(1, true);  
}

static void CONS_backcolor_Change(void)
{
	CON_SetupBackColormapEx(cons_backcolor.value, false);
}

static void CON_SetupColormaps(void)
{
	INT32 i;
	UINT8 *memorysrc = (UINT8 *)Z_Malloc((256*15), PU_STATIC, NULL);

	magentamap = memorysrc;
	yellowmap  = (magentamap+256);
	lgreenmap  = (yellowmap+256);
	bluemap    = (lgreenmap+256);
	redmap     = (bluemap+256);
	graymap    = (redmap+256);
	orangemap  = (graymap+256);
	skymap     = (orangemap+256);
	purplemap  = (skymap+256);
	aquamap    = (purplemap+256);
	peridotmap = (aquamap+256);
	azuremap   = (peridotmap+256);
	brownmap   = (azuremap+256);
	rosymap    = (brownmap+256);
	invertmap  = (rosymap+256);






	for (i = 0; i < (256*15); i++, ++memorysrc)
		*memorysrc = (UINT8)(i & 0xFF);  

#define colset(map, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
	map[0x0] = (UINT8)a;\
	map[0x1] = (UINT8)b;\
	map[0x2] = (UINT8)c;\
	map[0x3] = (UINT8)d;\
	map[0x4] = (UINT8)e;\
	map[0x5] = (UINT8)f;\
	map[0x6] = (UINT8)g;\
	map[0x7] = (UINT8)h;\
	map[0x8] = (UINT8)i;\
	map[0x9] = (UINT8)j;\
	map[0xA] = (UINT8)k;\
	map[0xB] = (UINT8)l;\
	map[0xC] = (UINT8)m;\
	map[0xD] = (UINT8)n;\
	map[0xE] = (UINT8)o;\
	map[0xF] = (UINT8)p;




	colset(magentamap, 177, 177, 178, 178, 178, 180, 180, 180, 182, 182, 182, 182, 184, 184, 184, 185);
	colset(yellowmap,   82,  82,  73,  73,  73,  74,  74,  74,  66,  66,  66,  66,  67,  67,  67,  68);
	colset(lgreenmap,   96,  96,  98,  98,  98, 100, 100, 100, 103, 103, 103, 103, 105, 105, 105, 107);
	colset(bluemap,    146, 146, 147, 147, 147, 148, 148, 148, 149, 149, 149, 149, 150, 150, 150, 151);
	colset(redmap,      32,  32,  33,  33,  33,  34,  34,  34,  35,  35,  35,  35,  37,  37,  37,  39);
	colset(graymap,      8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23);
	colset(orangemap,   50,  50,  52,  52,  52,  54,  54,  54,  56,  56,  56,  56,  59,  59,  59,  60);
	colset(skymap,     129, 129, 130, 130, 130, 131, 131, 131, 133, 133, 133, 133, 135, 135, 135, 136);
	colset(purplemap,  160, 160, 161, 161, 161, 162, 162, 162, 163, 163, 163, 163, 164, 164, 164, 165);
	colset(aquamap,    120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 123, 124, 124, 124, 125);
	colset(peridotmap,  73,  73, 188, 188, 188, 189, 189, 189, 190, 190, 190, 190, 191, 191, 191,  94);
	colset(azuremap,   144, 144, 145, 145, 145, 146, 146, 146, 170, 170, 170, 170, 171, 171, 171, 172);
	colset(brownmap,   219, 219, 221, 221, 221, 222, 222, 222, 224, 224, 224, 224, 227, 227, 227, 229);
	colset(rosymap,    200, 200, 201, 201, 201, 202, 202, 202, 203, 203, 203, 203, 204, 204, 204, 205);

#undef colset


	for (i = 0x00; i <= 0x1F; i++)
		invertmap[0x1F - i] = i;


	CON_SetupBackColormap();
}



void CON_Init(void)
{
	INT32 i;

	for (i = 0; i < NUMINPUTS; i++)
		bindtable[i] = NULL;

	Lock_state();


	memset(con_buffer, 0, CON_BUFFERSIZE);


	con_width = 0;

	Unlock_state();

	CON_RecalcSize();

	CON_SetupColormaps();

	Lock_state();

	con_hudlines = atoi(cons_hudlines.defaultvalue);

	Unlock_state();


	CON_InputInit();



	COM_AddCommand("cls", CONS_Clear_f, 0);


	Lock_state();

	con_destlines = vid.height;
	con_curlines = vid.height;

	Unlock_state();

	if (!dedicated)
	{
		Lock_state();

		con_started = true;
		con_startup = true;
		con_refresh = true;  
		consoletoggle = false;

		Unlock_state();

		CV_RegisterVar(&cons_hudtime);
		CV_RegisterVar(&cons_hudlines);
		CV_RegisterVar(&cons_speed);
		CV_RegisterVar(&cons_height);
		CV_RegisterVar(&cons_lines);
		CV_RegisterVar(&cons_backpic);
		CV_RegisterVar(&cons_backcolor);
		COM_AddCommand("bind", CONS_Bind_f, 0);
	}
	else
	{
		Lock_state();

		con_started = true;
		con_startup = false;
		con_refresh = false;  
		consoletoggle = true;

		Unlock_state();
	}

	COM_AddCommand("these_arent_the_mods_youre_looking_for", G_SetGameNotModified, 0);
}

void CON_StartRefresh(void)
{
	if (con_startup)
		con_refresh = true;
}

void CON_StopRefresh(void)
{
	if (con_startup)
		con_refresh = false;
}



static void CON_InputInit(void)
{
	Lock_state();


	memset(inputlines, 0, sizeof (inputlines));
	inputline = 0;
	input_cur = input_sel = input_len = 0;

	Unlock_state();
}








static void CON_RecalcSize(void)
{
	size_t conw, oldcon_width, oldnumlines, oldhistorylines, i, oldcon_cy;
	char *tmp_buffer;
	char *string;

	Lock_state();

	switch (cv_constextsize.value)
	{
	case V_NOSCALEPATCH:
		con_scalefactor = 1;
		break;
	case V_SMALLSCALEPATCH:
		con_scalefactor = vid.smalldup;
		break;
	case V_MEDSCALEPATCH:
		con_scalefactor = vid.meddup;
		break;
	default:	 
		con_scalefactor = vid.dup;
		break;
	}

	con_recalc = false;

	if (dedicated)
		conw = 1;
	else
		conw = (vid.width>>3) / con_scalefactor - 2;

	if (con_curlines == vid.height)  
	{
		con_curlines = vid.height;
		con_destlines = vid.height;
	}

	if (con_destlines > 0)  
	{
		CON_ChangeHeight();
		con_curlines = con_destlines;
	}


	if (conw == con_width)
	{
		Unlock_state();
		return;  
	}

	Unlock_state();

	tmp_buffer = Z_Malloc(CON_BUFFERSIZE, PU_STATIC, NULL);
	string = Z_Malloc(CON_BUFFERSIZE, PU_STATIC, NULL);  

	Lock_state();

	oldcon_width = con_width;
	oldnumlines = con_totallines;
	oldhistorylines = con_historylines;
	oldcon_cy = con_cy;
	M_Memcpy(tmp_buffer, con_buffer, CON_BUFFERSIZE);

	if (conw < 1)
		con_width = (BASEVIDWIDTH>>3) - 2;
	else
		con_width = conw;

	con_width += 11;  

	con_totallines = CON_BUFFERSIZE / con_width;
	memset(con_buffer, ' ', CON_BUFFERSIZE);

	con_cx = 0;
	con_cy = con_totallines-1;
	con_line = &con_buffer[con_cy*con_width];
	con_historylines = 1;
	con_scrollup = 0;

	Unlock_state();


	if (oldcon_width && oldhistorylines > 1)  
	{
		                                                                       
                                                                        
                                                           
		for (i = oldcon_cy + 1 - oldhistorylines; i < oldcon_cy; i++)
		{
			if (tmp_buffer[(i%oldnumlines)*oldcon_width])
			{
				M_Memcpy(string, &tmp_buffer[(i%oldnumlines)*oldcon_width], oldcon_width);
				conw = oldcon_width - 1;
				while (string[conw] == ' ' && conw)
					conw--;
				string[conw+1] = '\n';
				string[conw+2] = '\0';
				CON_Print(string);
			}
		}
	}

	Z_Free(string);
	Z_Free(tmp_buffer);
}

                                                                            
                                               
static size_t CON_MaxScroll(void)
{
	size_t visiblelines;
	INT32 minheight = 20 * (INT32)con_scalefactor;
	INT32 charheight = 8 * (INT32)con_scalefactor;

	if (con_curlines < minheight || charheight <= 0)
		return 0;

	visiblelines = (size_t)((con_curlines - minheight) / charheight) + 1;
	if (con_historylines <= visiblelines + 1)
		return 0;

	return con_historylines - visiblelines - 1;
}

static void CON_ChangeHeight(void)
{
	INT32 minheight;
	INT32 charheight;

	Lock_state();

	minheight = 20 * con_scalefactor;	 
	charheight = 8 * con_scalefactor;


	if (cons_lines.value == 0)
		con_destlines = 0;
	else if (cons_lines.value > 0)
		con_destlines = minheight + (cons_lines.value - 1) * charheight;
	else
		con_destlines = (cons_height.value*vid.height)/100;

	if (!con_destlines)
	{
		con_curlines = 0;
		consoleready = false;
		CON_ClearHUD();
		I_UpdateMouseGrab();
		Unlock_state();
		return;
	}

	if (con_destlines < minheight)
		con_destlines = minheight;
	else if (con_destlines > vid.height)
		con_destlines = vid.height;

	con_destlines &= ~0x3;  

	Unlock_state();
}



static void CON_MoveConsole(void)
{
	static fixed_t fracmovement = 0;

	Lock_state();


	if (!cons_speed.value)
	{
		con_curlines = con_destlines;
		Unlock_state();
		return;
	}


	fracmovement += FixedMul(cons_speed.value*vid.fdup, renderdeltatics);

	if (con_curlines < con_destlines)  
	{
		con_curlines += FixedInt(fracmovement);  
		if (con_curlines > con_destlines)  
			con_curlines = con_destlines;  
	}
	else  
	{
		con_curlines -= FixedInt(fracmovement);
		if (con_curlines < con_destlines)
			con_curlines = con_destlines;

		if (con_destlines == 0)  
			con_tick = 0;  
	}

	fracmovement %= FRACUNIT;  

	Unlock_state();
}



void CON_ClearHUD(void)
{
	INT32 i;

	Lock_state();

	for (i = 0; i < con_hudlines; i++)
		con_hudtime[i] = 0;

	Unlock_state();
}



void CON_ToggleOff(void)
{
	Lock_state();

	if (!con_destlines)
	{
		Unlock_state();
		return;
	}

	con_destlines = 0;
	con_curlines = 0;
	CON_ClearHUD();
	con_forcepic = 0;

	I_UpdateMouseGrab();

	Unlock_state();
}

boolean CON_Ready(void)
{
	boolean ready;
	Lock_state();
	{
		ready = consoleready;
	}
	Unlock_state();
	return ready;
}



void CON_Ticker(void)
{
	INT32 i;
	INT32 minheight;

	Lock_state();

	minheight = 20 * con_scalefactor;	 


	con_tick++;
	con_tick &= 7;


	if (consoletoggle)
	{
		consoletoggle = false;


		if (con_destlines > 0)
		{
			con_destlines = 0;
			CON_ClearHUD();
			I_UpdateMouseGrab();
		}
		else
		{
			 
			con_scrollup = 0;
			CON_ChangeHeight();
			I_SetMouseGrab(false);
		}
	}


	if (con_destlines >= minheight)
		consoleready = true;
	else
		consoleready = false;


	for (i = 0; i < con_hudlines; i++)
	{
		if (con_hudtime[i])
			con_hudtime[i]--;
	}

	Unlock_state();
}








static void CON_InputClear(void)
{
	Lock_state();

	memset(inputlines[inputline], 0, CON_MAXPROMPTCHARS);
	input_cur = input_sel = input_len = 0;

	Unlock_state();
}

static void CON_InputSetString(const char *c)
{
	size_t newlen;

	Lock_state();

	                                                                          
                                                                            
                                                  
	newlen = strlen(c);
	if (newlen >= CON_MAXPROMPTCHARS)
		newlen = CON_MAXPROMPTCHARS - 1;
	memmove(inputlines[inputline], c, newlen);
	memset(inputlines[inputline] + newlen, 0, CON_MAXPROMPTCHARS - newlen);
	input_cur = input_sel = input_len = newlen;

	Unlock_state();
}

static void CON_InputAddString(const char *c)
{
	size_t csize = strlen(c);

	Lock_state();

	if (input_len + csize > CON_MAXPROMPTCHARS-1)
	{
		Unlock_state();
		return;
	}
	if (input_cur != input_len)
		memmove(&inputlines[inputline][input_cur+csize], &inputlines[inputline][input_cur], input_len-input_cur);
	memcpy(&inputlines[inputline][input_cur], c, csize);
	input_len += csize;
	input_sel = (input_cur += csize);

	Unlock_state();
}

static void CON_InputDelSelection(void)
{
	size_t start, end, len;

	Lock_state();

	if (input_cur > input_sel)
	{
		start = input_sel;
		end = input_cur;
	}
	else
	{
		start = input_cur;
		end = input_sel;
	}
	len = (end - start);

	if (end != input_len)
		memmove(&inputlines[inputline][start], &inputlines[inputline][end], input_len-end);
	memset(&inputlines[inputline][input_len - len], 0, len);

	input_len -= len;
	input_sel = input_cur = start;

	Unlock_state();
}

static void CON_InputAddChar(char c)
{
	if (input_len >= CON_MAXPROMPTCHARS-1)
		return;

	Lock_state();

	if (input_cur != input_len)
		memmove(&inputlines[inputline][input_cur+1], &inputlines[inputline][input_cur], input_len-input_cur);
	inputlines[inputline][input_cur++] = c;
	inputlines[inputline][++input_len] = 0;
	input_sel = input_cur;

	Unlock_state();
}

static void CON_InputDelChar(void)
{
	if (!input_cur)
		return;

	Lock_state();

	if (input_cur != input_len)
		memmove(&inputlines[inputline][input_cur-1], &inputlines[inputline][input_cur], input_len-input_cur);
	inputlines[inputline][--input_len] = 0;
	input_sel = --input_cur;

	Unlock_state();
}

                                                                              
                                                                        
                                                                  
static boolean CON_OpenURLAtCursor(void)
{
	                                                                          
                                                                      
                                                                  
	char line[1024];
	char url[1024];
	UINT8 *source;
	size_t raw, plain = 0, start, end;
	INT64 bufferline;
	INT32 mousex, mousey, row, column;
	INT32 charwidth = (INT32)con_scalefactor << 3;
	INT32 charheight = charwidth;
	INT32 minheight = 20 * con_scalefactor;
	INT32 firsty = (con_curlines-minheight) % charheight;
#ifdef _WIN32
	HINSTANCE opened;
#endif

	if (con_curlines < minheight)
		return false;
	I_GetCursorPosition(&mousex, &mousey);
	if (mousex < charwidth || mousey < firsty || mousey > con_curlines-minheight)
		return false;
	column = (mousex-charwidth) / charwidth;
	row = (mousey-firsty) / charheight;
	if (column < 0 || (size_t)column >= con_width)
		return false;

	Lock_state();
	bufferline = (INT64)con_cy - (INT64)con_scrollup - 1;
	bufferline -= (con_curlines-minheight) / charheight;
	bufferline += row;
	if (bufferline < 0)
		bufferline = 0;
	source = (UINT8 *)&con_buffer[((size_t)bufferline%con_totallines)*con_width];
	for (raw = 0; raw < con_width && plain+1 < sizeof line; raw++)
	{
		if (!(source[raw] & 0x80))
			line[plain++] = (char)source[raw];
	}
	line[plain] = '\0';
	Unlock_state();

	for (start = 0; start < plain; start++)
	{
		if (strnicmp(&line[start], "https://", 8)
		 && strnicmp(&line[start], "http://", 7))
			continue;
		for (end = start; end < plain && line[end] > ' '; end++)
			;
		while (end > start && strchr(".,;:!?)]}>\"'", line[end-1]))
			end--;
		if ((size_t)column >= start && (size_t)column < end)
		{
			size_t length = min(end-start, sizeof url-1);
			memcpy(url, &line[start], length);
			url[length] = '\0';
#ifdef _WIN32
			opened = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
			if ((INT_PTR)opened > 32)
			{
				CONS_Printf("Opened link: %s\n", url);
				return true;
			}
#endif
			I_ClipboardCopy(url, strlen(url));
			CONS_Printf("Could not open the link, so it was copied: %s\n", url);
			return true;
		}
		start = end;
	}
	return false;
}







boolean CON_Responder(event_t *ev)
{

	static char completioncmd[80 + sizeof("find ")] = "find ";
	static char *completion = &completioncmd[sizeof("find ")-1];

	static INT32 skips;

	static INT32   com_skips;
	static INT32   var_skips;
	static INT32 alias_skips;

	const char *cmd = NULL;
	INT32 key;

	if (chat_on)
		return false;


	if (ev->type != ev_keydown && ev->type != ev_text && ev->type != ev_console)
	{
		return false;
	}

	key = ev->key;


	if (ev->type == ev_keydown)
	{
		if (modeattacking || metalrecording || marathonmode)
			return false;

		if ((key == gamecontrol[GC_CONSOLE][0] || key == gamecontrol[GC_CONSOLE][1]) && !shiftdown)
		{
			I_SetTextInputMode(con_destlines == 0);  
			consoletoggle = true;
			return true;
		}


		if (!consoleready && key < NUMINPUTS)  
		{
			if (!menuactive && bindtable[key])
			{
				COM_BufAddText(bindtable[key]);
				COM_BufAddText("\n");
				return true;
			}
			return false;
		}


		if (key == KEY_ESCAPE)
		{
			I_SetTextInputMode(false);
			consoletoggle = true;
			return true;
		}
	}
	else if (ev->type == ev_text)
	{
		if (!consoletoggle && consoleready)
			CON_InputAddChar(key);
		return true;
	}


	if (key == KEY_LSHIFT || key == KEY_RSHIFT
	 || key == KEY_LCTRL || key == KEY_RCTRL
	 || key == KEY_LALT || key == KEY_RALT)
		return true;

	if (key == KEY_MOUSE1)
	{
		CON_OpenURLAtCursor();
		return true;
	}

	if (key == KEY_LEFTARROW)
	{
		if (input_cur != 0)
		{
			if (ctrldown)
				input_cur = M_JumpWordReverse(inputlines[inputline], input_cur);
			else
				--input_cur;
		}
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}
	else if (key == KEY_RIGHTARROW)
	{
		if (input_cur < input_len)
		{
			if (ctrldown)
				input_cur += M_JumpWord(&inputlines[inputline][input_cur]);
			else
				++input_cur;
		}
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}


	if (input_sel != input_cur)
	{
		if (key == KEY_BACKSPACE || key == KEY_DEL)
		{
			CON_InputDelSelection();
			return true;
		}
	}
	else if (key == KEY_BACKSPACE)
	{
		if (ctrldown && input_cur != 0)
		{
			input_sel = M_JumpWordReverse(inputlines[inputline], input_cur);
			CON_InputDelSelection();
		}
		else
			CON_InputDelChar();
		return true;
	}
	else if (key == KEY_DEL)
	{
		if (input_cur == input_len)
			return true;

		if (ctrldown)
		{
			input_sel = input_cur + M_JumpWord(&inputlines[inputline][input_cur]);
			CON_InputDelSelection();
		}
		else
		{
			++input_cur;
			CON_InputDelChar();
		}
		return true;
	}


	if (ctrldown)
	{

		if (key == KEY_TAB)
		{
			if (!completion[0])
			{
				if (!input_len || input_len >= 40 || strchr(inputlines[inputline], ' '))
					return true;
				strcpy(completion, inputlines[inputline]);
			}
			COM_BufInsertText(completioncmd);
			completion[0] = 0;
			return true;
		}


		if (key == KEY_HOME)  
		{
			con_scrollup = CON_MaxScroll();
			return true;
		}
		else if (key == KEY_END)  
		{
			con_scrollup = 0;
			return true;
		}

		if (key == 'x' || key == 'X')
		{
			if (input_sel == input_cur)  
				return true;
			else if (input_sel > input_cur)
				I_ClipboardCopy(&inputlines[inputline][input_cur], input_sel-input_cur);
			else
				I_ClipboardCopy(&inputlines[inputline][input_sel], input_cur-input_sel);
			CON_InputDelSelection();
			completion[0] = 0;
			return true;
		}
		else if (key == 'c' || key == 'C')
		{
			if (input_sel == input_cur)  
				return true;
			else if (input_sel > input_cur)
				I_ClipboardCopy(&inputlines[inputline][input_cur], input_sel-input_cur);
			else
				I_ClipboardCopy(&inputlines[inputline][input_sel], input_cur-input_sel);
			return true;
		}
		else if (key == 'v' || key == 'V')
		{
			const char *paste = I_ClipboardPaste();
			if (input_sel != input_cur)
				CON_InputDelSelection();
			if (paste != NULL)
				CON_InputAddString(paste);
			completion[0] = 0;
			return true;
		}


		if (key == 'a' || key == 'A')
		{
			input_sel = 0;
			input_cur = input_len;
			return true;
		}



		return true; 
	}


	if (key == KEY_TAB)
	{



		if (!completion[0])
		{
			if (!input_len || input_len >= 40 || strchr(inputlines[inputline], ' '))
				return true;
			strcpy(completion, inputlines[inputline]);
			skips       = 0;
			com_skips   = 0;
			var_skips   = 0;
			alias_skips = 0;
		}
		else
		{
			if (shiftdown)
			{
				if (skips > 0)
					skips--;
			}
			else
			{
				skips++;
			}
		}

		if (skips <= com_skips)
		{
			cmd = COM_CompleteCommand(completion, skips);

			if (cmd && skips == com_skips)
			{
				com_skips  ++;
				var_skips  ++;
				alias_skips++;
			}
		}

		if (!cmd && skips <= var_skips)
		{
			cmd = CV_CompleteVar(completion, skips - com_skips);

			if (cmd && skips == var_skips)
			{
				var_skips  ++;
				alias_skips++;
			}
		}

		if (!cmd && skips <= alias_skips)
		{
			cmd = COM_CompleteAlias(completion, skips - var_skips);

			if (cmd && skips == alias_skips)
			{
				alias_skips++;
			}
		}

		if (cmd)
		{
			CON_InputSetString(va("%s ", cmd));
		}
		else
		{
			skips--;
		}

		return true;
	}


	if (key == KEY_PGUP)
	{
		if (con_scrollup < CON_MaxScroll())
			con_scrollup++;
		return true;
	}
	else if (key == KEY_PGDN)
	{
		if (con_scrollup > 0)
			con_scrollup--;
		return true;
	}
	else if (key == KEY_HOME)
	{
		input_cur = 0;
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}
	else if (key == KEY_END)
	{
		input_cur = input_len;
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}



	completion[0] = 0;


	if (key == KEY_ENTER)
	{
		if (!input_len)
			return true;


		COM_BufAddText(inputlines[inputline]);
		COM_BufAddText("\n");

		CONS_Printf("\x86""%c""\x80""%s\n", CON_PROMPTCHAR, inputlines[inputline]);

		inputline = (inputline+1) & 31;
		inputhist = inputline;
		CON_InputClear();

		return true;
	}


	if (key == KEY_UPARROW)
	{

		do
			inputhist = (inputhist - 1) & 31;  
		while (inputhist != inputline && !inputlines[inputhist][0]);



		if (inputhist == inputline)
			inputhist = (inputline + 1) & 31;

		CON_InputSetString(inputlines[inputhist]);
		return true;
	}


	if (key == KEY_DOWNARROW)
	{
		if (inputhist == inputline)
			return true;
		do
			inputhist = (inputhist + 1) & 31;
		while (inputhist != inputline && !inputlines[inputhist][0]);


		if (inputhist == inputline)
			CON_InputClear();
		else
			CON_InputSetString(inputlines[inputhist]);
		return true;
	}


	if (key >= KEY_KEYPAD7 && key <= KEY_KPADDEL)
	{
		char keypad_translation[] = {'7','8','9','-',
		                             '4','5','6','+',
		                             '1','2','3',
		                             '0','.'};

		key = keypad_translation[key - KEY_KEYPAD7];
	}
	else if (key == KEY_KPADSLASH)
		key = '/';


	if (key < 32 || key > 127)
		return true;

	if (input_sel != input_cur)
		CON_InputDelSelection();
	if (ev->type == ev_console)
		CON_InputAddChar(key);

	return true;
}



static void CON_Linefeed(void)
{

	if (con_hudlines)
		con_hudtime[con_cy%con_hudlines] = cons_hudtime.value*TICRATE;

	con_cy++;
	con_cx = 0;
	if (con_historylines < con_totallines)
		con_historylines++;

	con_line = &con_buffer[(con_cy%con_totallines)*con_width];
	memset(con_line, ' ', con_width);
}


static void CON_Print(char *msg)
{
	size_t l;
	size_t oldscroll;
	size_t maxscroll;
	size_t oldcy;
	INT32 controlchars = 0;  
	char color = '\x80';   

	if (msg == NULL)
		return;

	if (*msg == '\3')  
		S_StartSound(NULL, sfx_radio);
	else if (*msg == '\4')  
	{
		*msg = '\x82';  
		S_StartSound(NULL, sfx_radio);
	}

	Lock_state();
	oldscroll = con_scrollup;
	oldcy = con_cy;

	if (!(*msg & 0x80))
	{
		con_line[con_cx++] = '\x80';
		controlchars = 1;
	}

	while (*msg)
	{

		while (*msg && *msg <= ' ')
		{
			if (*msg & 0x80)
			{
				color = con_line[con_cx++] = *(msg++);
				controlchars++;
				continue;
			}
			else if (*msg == '\r')  
			{
				if (con_historylines > 1)
					con_historylines--;
				con_cy--;
				CON_Linefeed();
				color = '\x80';
				controlchars = 0;
			}
			else if (*msg == '\n')  
			{
				CON_Linefeed();
				con_line[con_cx++] = color;
				controlchars = 1;
			}
			else if (*msg == ' ')  
			{
				con_line[con_cx++] = ' ';
				if (con_cx - controlchars >= con_width-11)
				{
					CON_Linefeed();
					con_line[con_cx++] = color;
					controlchars = 1;
				}
			}
			else if (*msg == '\t')
			{


				do
				{
					con_line[con_cx++] = ' ';
				} while ((con_cx - controlchars) % 4 != 0);

				if (con_cx - controlchars >= con_width-11)
				{
					CON_Linefeed();
					con_line[con_cx++] = color;
					controlchars = 1;
				}
			}
			msg++;
		}

		if (*msg == '\0')
			break;


		for (l = 0; l < (con_width-11) && msg[l] > ' '; l++)
			;


		if ((con_cx - controlchars) + l > con_width-11)
		{
			CON_Linefeed();
			con_line[con_cx++] = color;
			controlchars = 1;
		}


		for (; l > 0; l--)
			con_line[con_cx++] = *(msg++);
	}

	                                                                          
                                                                          
	if (oldscroll && con_cy > oldcy)
	{
		maxscroll = CON_MaxScroll();
		con_scrollup = min(oldscroll + (con_cy - oldcy), maxscroll);
	}

	Unlock_state();
}

void CON_LogMessage(const char *msg)
{
	char txt[8192], *t;
	const char *p = msg, *e = txt+sizeof (txt)-2;

	for (t = txt; *p != '\0'; p++)
	{
		if (*p == '\n' || *p >= ' ')  
			*t++ = *p;

		if (t >= e)
		{
			*t = '\0';  
			I_OutputMsg("%s", txt);  
			t = txt;  
			memset(txt,'\0', sizeof (txt));  
		}
	}
	*t = '\0';  
	I_OutputMsg("%s", txt);
}




void CONS_Printf(const char *fmt, ...)
{
	va_list argptr;
	static char *txt = NULL;
	boolean refresh;

	if (txt == NULL)
		txt = malloc(8192);
	if (txt == NULL)
		return;

	va_start(argptr, fmt);
	vsnprintf(txt, 8192, fmt, argptr);
	va_end(argptr);


	DEBFILE(txt);


	if (con_started)
		CON_Print(txt);

	CON_LogMessage(txt);

	Lock_state();

	refresh = con_refresh;

	Unlock_state();


	if (refresh)
	{
		CON_Drawer();  
		I_FinishUpdate();  
	}
}

void CONS_Alert(alerttype_t level, const char *fmt, ...)
{
	va_list argptr;
	static char *txt = NULL;

	if (txt == NULL)
		txt = malloc(8192);
	if (txt == NULL)
		return;

	va_start(argptr, fmt);
	vsnprintf(txt, 8192, fmt, argptr);
	va_end(argptr);

	switch (level)
	{
		case CONS_NOTICE:

			CONS_Printf("\x83" "%s" "\x80 ", M_GetText("NOTICE:"));
			break;
		case CONS_WARNING:
			refreshdirmenu |= REFRESHDIR_WARNING;
			CONS_Printf("\x82" "%s" "\x80 ", M_GetText("WARNING:"));
			break;
		case CONS_ERROR:
			refreshdirmenu |= REFRESHDIR_ERROR;
			CONS_Printf("\x85" "%s" "\x80 ", M_GetText("ERROR:"));
			break;
	}



	CONS_Printf("%s", txt);
}

void CONS_Debug(INT32 debugflags, const char *fmt, ...)
{
	va_list argptr;
	static char *txt = NULL;

	if ((cv_debug & debugflags) != debugflags)
		return;

	if (txt == NULL)
		txt = malloc(8192);
	if (txt == NULL)
		return;

	va_start(argptr, fmt);
	vsnprintf(txt, 8192, fmt, argptr);
	va_end(argptr);


	CONS_Printf("%s", txt);
}





void CONS_Error(const char *msg)
{
#if defined(RPC_NO_WINDOWS_H) && defined(_WINDOWS)
	if (!graphics_started)
	{
		MessageBoxA(vid.WndParent, msg, "SRB2 Warning", MB_OK);
		return;
	}
#endif
	CONS_Printf("\x82%s", msg);  
	CONS_Printf(M_GetText("Press ENTER to continue\n"));


	while (I_GetKey() != KEY_ENTER)
		I_OsPolling();
}







static void CON_DrawInput(void)
{
	INT32 charwidth = (INT32)con_scalefactor << 3;
	const char *p = inputlines[inputline];
	size_t c, clen, cend;
	UINT8 lellip = 0, rellip = 0;
	INT32 x, y, i;

	y = con_curlines - 12 * con_scalefactor;
	x = charwidth*2;

	clen = con_width-13;

	if (input_len <= clen)
	{
		c = 0;
		clen = input_len;
	}
	else  
	{
		clen -= 2;  

		if (input_cur <= clen/2)
		{

			c = 0;

			rellip = 1;
		}
		else
		{


			c = input_cur - (clen/2) + 2;
			x += charwidth*2;
			lellip = 1;

			if (c + clen >= input_len)
			{


				c = input_len - clen;
			}
			else
			{

				clen -= 2;
				rellip = 1;
			}
		}
	}

	if (lellip)
	{
		x -= charwidth*3;
		if (input_sel < c)
			V_DrawFill(x, y, charwidth*3, (10 * con_scalefactor), 77 | V_NOSCALESTART);
		for (i = 0; i < 3; ++i, x += charwidth)
			V_DrawCharacter(x, y, '.' | cv_constextsize.value | V_GRAYMAP | V_NOSCALESTART, true);
	}
	else
		V_DrawCharacter(x-charwidth, y, CON_PROMPTCHAR | cv_constextsize.value | V_GRAYMAP | V_NOSCALESTART, true);

	for (cend = c + clen; c < cend; ++c, x += charwidth)
	{
		if ((input_sel > c && input_cur <= c) || (input_sel <= c && input_cur > c))
		{
			V_DrawFill(x, y, charwidth, (10 * con_scalefactor), 77 | V_NOSCALESTART);
			V_DrawCharacter(x, y, p[c] | cv_constextsize.value | V_YELLOWMAP | V_NOSCALESTART, true);
		}
		else
			V_DrawCharacter(x, y, p[c] | cv_constextsize.value | V_NOSCALESTART, true);

		if (c == input_cur && con_tick >= 4)
			V_DrawCharacter(x, y + (con_scalefactor*2), '_' | cv_constextsize.value | V_NOSCALESTART, true);
	}
	if (cend == input_cur && con_tick >= 4)
		V_DrawCharacter(x, y + (con_scalefactor*2), '_' | cv_constextsize.value | V_NOSCALESTART, true);
	if (rellip)
	{
		if (input_sel > cend)
			V_DrawFill(x, y, charwidth*3, (10 * con_scalefactor), 77 | V_NOSCALESTART);
		for (i = 0; i < 3; ++i, x += charwidth)
			V_DrawCharacter(x, y, '.' | cv_constextsize.value | V_GRAYMAP | V_NOSCALESTART, true);
	}
}


static void CON_DrawHudlines(void)
{
	UINT8 *p;
	size_t i;
	INT32 y;
	INT32 charflags = 0;
	INT32 charwidth = 8 * con_scalefactor;
	INT32 charheight = 8 * con_scalefactor;

	if (!con_hudlines)
		return;

	if (chat_on && OLDCHAT)
		y = charheight;  
	else
		y = 0;

	for (i = con_cy - con_hudlines; i <= con_cy; i++)
	{
		size_t c;
		INT32 x;

		if ((signed)i < 0)
			continue;
		if (con_hudtime[i%con_hudlines] == 0)
			continue;

		p = (UINT8 *)&con_buffer[(i%con_totallines)*con_width];

		for (c = 0, x = 0; c < con_width; c++, x += charwidth, p++)
		{
			while (*p & 0x80)  
			{
				charflags = (*p & 0x7f) << V_CHARCOLORSHIFT;
				p++;
				c++;
			}
			if (c >= con_width)
				break;
			if (*p < FONTSTART)
				; 
			else
			{

				V_DrawCharacter(x, y, (INT32)(*p) | charflags | cv_constextsize.value | V_NOSCALESTART, true);
			}
		}


		y += charheight;
	}
}


static void CON_DrawBackpic(void)
{
	patch_t *con_backpic;
	lumpnum_t piclump;
	int x, w, h;


	if (con_startup)
		piclump = W_CheckNumForPatchName("STARTUP");
	else
		piclump = W_CheckNumForPatchName("CONSBACK");

	if (piclump == LUMPERROR)
		piclump = W_GetNumForPatchName("MISSING");


	con_backpic = W_CachePatchNum(piclump, PU_PATCH);
	if (con_backpic == NULL)
		return;


	w = con_backpic->width * vid.dup;
	x = (vid.width / 2) - (w / 2);
	h = con_curlines/vid.dup;



	if (x > 0)
	{
		column_t *column = &con_backpic->columns[0];
		if (column->num_posts && !column->posts[0].topdelta)
		{
			UINT8 *source = column->pixels;
			INT32 color = (source[0] | V_NOSCALESTART);

			V_DrawFill(0, 0, x, con_curlines, color);

			V_DrawFill((x + w), 0, (vid.width - w), con_curlines, color);
		}
	}


	V_DrawCroppedPatch(x << FRACBITS, 0, FRACUNIT, FRACUNIT, V_NOSCALESTART, con_backpic, NULL,
			0, (BASEVIDHEIGHT - h) << FRACBITS, BASEVIDWIDTH << FRACBITS, h << FRACBITS);


	W_UnlockCachedPatch(con_backpic);
}



static void CON_DrawConsole(void)
{
	UINT8 *p;
	size_t i;
	INT32 y;
	INT32 charflags = 0;
	INT32 charwidth = (INT32)con_scalefactor << 3;
	INT32 charheight = charwidth;
	INT32 minheight = 20 * con_scalefactor;	 

	if (con_curlines <= 0)
		return;


	if (cons_backpic.value == 1 || con_forcepic)
		CON_DrawBackpic();
	else if (cons_backpic.value == 0)
	{

		if (rendermode != render_none)
			V_DrawFadeConsBack(con_curlines);  
	}


	if (con_curlines >= minheight)
	{
		i = con_cy - con_scrollup;


		i--;

		i -= (con_curlines - minheight) / charheight;

		if (rendermode == render_none) return;

		for (y = (con_curlines-minheight) % charheight; y <= con_curlines-minheight; y += charheight, i++)
		{
			INT32 x;
			size_t c, drawchars;

			p = (UINT8 *)&con_buffer[((i > 0 ? i : 0)%con_totallines)*con_width];
			drawchars = con_width;
			while (drawchars && (p[drawchars-1] == ' ' || p[drawchars-1] == '\0'))
				drawchars--;

			for (c = 0, x = charwidth; c < drawchars; c++, x += charwidth, p++)
			{
				while (*p & 0x80)
				{
					charflags = (*p & 0x7f) << V_CHARCOLORSHIFT;
					p++;
					c++;
				}
				if (c >= drawchars)
					break;
				if (*p != ' ' && *p != '\0')
					V_DrawCharacter(x, y, (INT32)(*p) | charflags | cv_constextsize.value | V_NOSCALESTART, true);
			}
		}
	}


	if ((con_curlines >= (minheight-charheight)) && !con_startup)
		CON_DrawInput();
}



void CON_Drawer(void)
{
	Lock_state();

	if (!con_started || !graphics_started)
	{
		Unlock_state();
		return;
	}

	if (con_recalc)
	{
		CON_RecalcSize();
		if (con_curlines <= 0)
			CON_ClearHUD();
	}


	if (con_curlines != con_destlines)
		CON_MoveConsole();

	if (con_curlines > 0)
		CON_DrawConsole();
	else if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_CREDITS
	|| gamestate == GS_EVALUATION || gamestate == GS_ENDING || gamestate == GS_CUTSCENE
	|| gamestate == GS_WAITINGPLAYERS || cv_debug)
		CON_DrawHudlines();

	Unlock_state();
}
