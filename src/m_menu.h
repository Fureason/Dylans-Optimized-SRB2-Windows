// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 1999-2025 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __X_MENU__
#define __X_MENU__

#include "doomstat.h"  
#include "d_event.h"
#include "command.h"
#include "f_finale.h"  
#include "i_threads.h"
#include "netcode/mserv.h"
#include "r_things.h"  


#define OLDNREPLAYNAME







#define NUMMENULEVELS 3
#define MENUBITS 6


   
                                                                      
   
typedef enum
{
	MN_NONE,

	MN_MAIN,


	MN_SP_MAIN,

	MN_SP_LOAD,
	MN_SP_PLAYER,

	MN_SP_LEVELSELECT,
	MN_SP_LEVELSTATS,

	MN_SP_TIMEATTACK,
	MN_SP_TIMEATTACK_LEVELSELECT,
	MN_SP_GUESTREPLAY,
	MN_SP_REPLAY,
	MN_SP_GHOST,

	MN_SP_NIGHTSATTACK,
	MN_SP_NIGHTS_LEVELSELECT,
	MN_SP_NIGHTS_GUESTREPLAY,
	MN_SP_NIGHTS_REPLAY,
	MN_SP_NIGHTS_GHOST,

	MN_SP_MARATHON,


	MN_MP_MAIN,
	MN_MP_SPLITSCREEN,  
	MN_MP_SERVER,
	MN_MP_CONNECT,
	MN_MP_ROOM,
	MN_MP_PLAYERSETUP,
	MN_MP_SERVER_OPTIONS,


	MN_OP_MAIN,

	MN_OP_P1CONTROLS,
	MN_OP_CHANGECONTROLS,  
	MN_OP_P1MOUSE,
	MN_OP_P1JOYSTICK,
	MN_OP_JOYSTICKSET,  
	MN_OP_P1CAMERA,

	MN_OP_P2CONTROLS,
	MN_OP_P2MOUSE,
	MN_OP_P2JOYSTICK,
	MN_OP_P2CAMERA,

	MN_OP_PLAYSTYLE,

	MN_OP_VIDEO,
	MN_OP_VIDEOMODE,
	MN_OP_COLOR,
	MN_OP_OPENGL,
	MN_OP_OPENGL_LIGHTING,

	MN_OP_SOUND,

	MN_OP_SERVER,
	MN_OP_MONITORTOGGLE,

	MN_OP_DATA,
	MN_OP_ADDONS,
	MN_OP_SCREENSHOTS,
	MN_OP_ERASEDATA,


	MN_SR_MAIN,
	MN_SR_PANDORA,
	MN_SR_LEVELSELECT,
	MN_SR_UNLOCKCHECKLIST,
	MN_SR_EMBLEMHINT,
	MN_SR_PLAYER,
	MN_SR_SOUNDTEST,


	MN_AD_MAIN,













	MN_SPECIAL,
	NUMMENUTYPES,
} menutype_t;  
#define MTREE2(a,b) (a | (b<<MENUBITS))
#define MTREE3(a,b,c) MTREE2(a, MTREE2(b,c))
#define MTREE4(a,b,c,d) MTREE2(a, MTREE3(b,c,d))

typedef struct
{
	char bgname[8+1];  
	SINT8 fadestrength;   
	INT32 bgcolor;  
	INT32 titlescrollxspeed;  
	INT32 titlescrollyspeed;  
	boolean bghide;  

	SINT8 hidetitlepics;  
	ttmode_enum ttmode;  
	UINT8 ttscale;  
	char ttname[8+1];  
	INT16 ttx;  
	INT16 tty;  
	INT16 ttloop;  
	UINT16 tttics;  

	char musname[6+1];  
	UINT16 mustrack;  
	boolean muslooping;  
	boolean musstop;  
	boolean musignore;  

	boolean enterbubble;  
	boolean exitbubble;  
	INT32 entertag;  
	INT32 exittag;  
	INT16 enterwipe;  
	INT16 exitwipe;  
} menupres_t;

extern menupres_t menupres[NUMMENUTYPES];
extern UINT32 prevMenuId;
extern UINT32 activeMenuId;

void M_InitMenuPresTables(void);
UINT8 M_GetYoungestChildMenu(void);
void M_ChangeMenuMusic(const char *defaultmusname, boolean defaultmuslooping);
void M_SetMenuCurBackground(const char *defaultname);
void M_SetMenuCurFadeValue(UINT8 defaultvalue);
void M_SetMenuCurTitlePics(void);






boolean M_Responder(event_t *ev);


void M_Ticker(void);


void M_Drawer(void);


void M_Init(void);
 
void M_RegisterEarlyHostingVariables(void);
 
UINT16 M_CompanionServerPort(void);


void M_InitCharacterTables(INT32 num);



void M_StartControlPanel(void);


void M_EndModeAttackRun(void);


void M_SortServerList(void);


void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines);




typedef enum
{
	MM_NOTHING = 0,  
	MM_YESNO,        
	MM_EVENTHANDLER  

} menumessagetype_t;
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype);

typedef enum
{
	M_NOT_WAITING,

	M_WAITING_VERSION,
	M_WAITING_ROOMS,
	M_WAITING_SERVERS,
}
M_waiting_mode_t;

extern M_waiting_mode_t m_waiting_mode;


void M_QuitResponse(INT32 ch);


boolean M_CanShowLevelInList(INT32 mapnum, INT32 gt);



#define IT_TYPE             15      
#define IT_CALL              0      
#define IT_SPACE             1      
#define IT_ARROWS            2      
#define IT_KEYHANDLER        4      
#define IT_SUBMENU           6      
#define IT_CVAR              8      
#define IT_PAIR             11      
#define IT_MSGHANDLER       12      

#define IT_DISPLAY   (48+64+128)     
#define IT_NOTHING            0      
#define IT_PATCH             16      
#define IT_STRING            32      
#define IT_WHITESTRING       48      
#define IT_DYBIGSPACE        64      
#define IT_DYLITLSPACE   (16+64)     
#define IT_STRING2       (32+64)     
#define IT_GRAYPATCH     (16+32+64)  
#define IT_BIGSLIDER        128      
#define IT_TRANSTEXT     (16+128)    
#define IT_TRANSTEXT2    (32+128)    
#define IT_HEADERTEXT    (48+128)    
#define IT_QUESTIONMARKS (64+128)    
#define IT_CENTER           256      


#define IT_CVARTYPE   (512+1024+2048)
#define IT_CV_NORMAL         0
#define IT_CV_SLIDER       512
#define IT_CV_STRING      1024
#define IT_CV_NOPRINT     1536
#define IT_CV_NOMOD       2048
#define IT_CV_INVISSLIDER 2560
#define IT_CV_INTEGERSTEP 4096       
#define IT_CV_FLOATSLIDER 4608       




#define IT_CALLTYPE   (512+1024)
#define IT_CALL_NORMAL          0
#define IT_CALL_NOTMODIFIED   512


#define IT_BIGSPACE    (IT_SPACE  +IT_DYBIGSPACE)
#define IT_LITLSPACE   (IT_SPACE  +IT_DYLITLSPACE)
#define IT_CONTROL     (IT_STRING2+IT_CALL)
#define IT_CVARMAX     (IT_CVAR   +IT_CV_NOMOD)
#define IT_DISABLED    (IT_SPACE  +IT_GRAYPATCH)
#define IT_GRAYEDOUT   (IT_SPACE  +IT_TRANSTEXT)
#define IT_GRAYEDOUT2  (IT_SPACE  +IT_TRANSTEXT2)
#define IT_HEADER      (IT_SPACE  +IT_HEADERTEXT)
#define IT_SECRET      (IT_SPACE  +IT_QUESTIONMARKS)

#define MAXSTRINGLENGTH 32

typedef union
{
	struct menu_s *submenu;       
	consvar_t *cvar;              
	void (*routine)(INT32 choice);  
} itemaction_t;




typedef struct menuitem_s
{

	UINT16 status;

	const char *patch;
	const char *text;  


	void *itemaction;


	UINT16 alphaKey;
} menuitem_t;

extern menuitem_t MP_RoomMenu[];
extern UINT32     roomIds[NUM_LIST_ROOMS];

typedef struct menu_s
{
	UINT32         menuid;              
	const char    *menutitlepic;
	INT16          numitems;            
	struct menu_s *prevMenu;            
	menuitem_t    *menuitems;           
	void         (*drawroutine)(void);  
	INT16          x, y;                
	INT16          lastOn;              
	boolean      (*quitroutine)(void);  
} menu_t;

void M_SetupNextMenu(menu_t *menudef);
void M_ClearMenus(boolean callexitmenufunc);


boolean M_MouseNeeded(void);

#ifdef HAVE_THREADS
extern I_mutex m_menu_mutex;
#endif

extern menu_t *currentMenu;

extern menu_t MainDef;
extern menu_t SP_LoadDef;


void M_SetupJoystickMenu(INT32 choice);
extern menu_t OP_JoystickSetDef;


typedef struct
{
	boolean used;
	char notes[440+1];
	char picname[8+1];
	char skinname[SKINNAMESIZE*2+2];  
	patch_t *charpic;
	UINT8 prev;
	UINT8 next;
	char displayname[SKINNAMESIZE+1];
	INT16 skinnum[2];
	UINT16 oppositecolor;
	char nametag[8+1];
	patch_t *namepic;
	UINT16 tagtextcolor;
	UINT16 tagoutlinecolor;
} description_t;


typedef struct
{
	char header[22+5];  
	INT32 maplist[3];
	char mapnames[3][22];  
	boolean mapavailable[4];  
} levelselectrow_t;

typedef struct
{
	UINT8 numrows;
	levelselectrow_t *rows;
} levelselect_t;



typedef struct
{
	UINT8 col[2];
	char notes[441];
} gtdesc_t;
extern gtdesc_t gametypedesc[NUMGAMETYPES];


typedef struct
{
	INT32 modenum;  
	const char *desc;   
	UINT8 goodratio;  
} modedesc_t;


typedef struct
{
	char levelname[32];
	UINT8 skinnum;
	char skinname [SKINNAMESIZE+1];
	UINT8 botskin;
	UINT8 numemeralds;
	UINT8 numgameovers;
	INT32 lives;
	INT32 continuescore;
	INT32 gamemap;
} saveinfo_t;

extern description_t *description;
extern INT32 numdescriptions;

extern consvar_t cv_showfocuslost;
extern consvar_t cv_startupsequence;
extern consvar_t cv_newgametype, cv_nextmap, cv_chooseskin, cv_serversort;
extern CV_PossibleValue_t gametype_cons_t[];

extern INT16 startmap;
extern INT32 ultimate_selectable;
extern INT16 char_on, startchar;

#define MAXSAVEGAMES 31
#define NOSAVESLOT 0  
#define MARATHONSLOT 420  

#define BwehHehHe() S_StartSound(NULL, sfx_bewar1+M_RandomKey(4))  

void M_TutorialSaveControlResponse(INT32 ch);

void M_ForceSaveSlotSelected(INT32 sslot);

void M_CheatActivationResponder(INT32 ch);

void M_ModeAttackRetry(INT32 choice);


void Nextmap_OnChange(void);


void Moviemode_mode_Onchange(void);
void Screenshot_option_Onchange(void);


void Addons_option_Onchange(void);


void Moviemode_option_Onchange(void);


typedef struct menucolor_s {
	struct menucolor_s *next;
	struct menucolor_s *prev;
	UINT16 color;
} menucolor_t;

extern menucolor_t *menucolorhead, *menucolortail;

void M_AddMenuColor(UINT16 color);
void M_MoveColorBefore(UINT16 color, UINT16 targ);
void M_MoveColorAfter(UINT16 color, UINT16 targ);
UINT16 M_GetColorBefore(UINT16 color);
UINT16 M_GetColorAfter(UINT16 color);
UINT16 M_GetColorIndex(UINT16 color);
menucolor_t* M_GetColorFromIndex(UINT16 index);
void M_InitPlayerSetupColors(void);
void M_FreePlayerSetupColors(void);


#define DEFAULTMENUSTYLE(id, header, source, prev, x, y)\
{\
	id,\
	header,\
	sizeof(source)/sizeof(menuitem_t),\
	prev,\
	source,\
	M_DrawGenericMenu,\
	x, y,\
	0,\
	NULL\
}

#define DEFAULTSCROLLMENUSTYLE(id, header, source, prev, x, y)\
{\
	id,\
	header,\
	sizeof(source)/sizeof(menuitem_t),\
	prev,\
	source,\
	M_DrawGenericScrollMenu,\
	x, y,\
	0,\
	NULL\
}

#define PAUSEMENUSTYLE(source, x, y)\
{\
	MN_SPECIAL,\
	NULL,\
	sizeof(source)/sizeof(menuitem_t),\
	NULL,\
	source,\
	M_DrawPauseMenu,\
	x, y,\
	0,\
	NULL\
}

#define CENTERMENUSTYLE(id, header, source, prev, y)\
{\
	id,\
	header,\
	sizeof(source)/sizeof(menuitem_t),\
	prev,\
	source,\
	M_DrawCenteredMenu,\
	BASEVIDWIDTH/2, y,\
	0,\
	NULL\
}

#define MAPPLATTERMENUSTYLE(id, header, source)\
{\
	id,\
	header,\
	sizeof (source)/sizeof (menuitem_t),\
	&MainDef,\
	source,\
	M_DrawLevelPlatterMenu,\
	0,0,\
	0,\
	NULL\
}

#define CONTROLMENUSTYLE(id, source, prev)\
{\
	id,\
	"M_CONTRO",\
	sizeof (source)/sizeof (menuitem_t),\
	prev,\
	source,\
	M_DrawControl,\
	24, 40,\
	0,\
	NULL\
}

#define IMAGEDEF(source)\
{\
	MN_SPECIAL,\
	NULL,\
	sizeof (source)/sizeof (menuitem_t),\
	NULL,\
	source,\
	M_DrawImageDef,\
	0, 0,\
	0,\
	NULL\
}

#endif  
