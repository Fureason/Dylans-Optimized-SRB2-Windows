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




#ifndef __DOOMDEF__
#define __DOOMDEF__




#define SOUND_DUMMY   0
#define SOUND_SDL     1
#define SOUND_MIXER   2
#define SOUND_FMOD    3

#ifndef SOUND
#ifdef HAVE_SDL


#ifdef HAVE_MIXER
    #define SOUND SOUND_MIXER
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
#endif


#ifndef SOUND
#define SOUND SOUND_SDL
#endif

#else  


#ifdef HAVE_FMOD
    #define SOUND SOUND_FMOD
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
#else

    #define SOUND SOUND_DUMMY
#endif

#endif
#endif

#ifdef _WINDOWS
#if !defined (HWRENDER) && !defined (NOHW)
#define HWRENDER
#endif
#endif

#ifdef _WIN32
#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4127 4152 4213 4514)
#ifdef _WIN64
#pragma warning(disable : 4306)
#endif
#endif






#include "version.h"
#include "doomtype.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES  
#include <math.h>

#ifdef GETTEXT
#include <libintl.h>
#endif
#include <locale.h>  

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#endif

FILE *fopenfile(const char*, const char*);




#if !defined (NDEBUG)
#ifndef PACKETDROP
#define PACKETDROP
#endif
#ifndef PARANOIA
#define PARANOIA
#endif
#ifndef RANGECHECK
#define RANGECHECK
#endif
#ifndef ZDEBUG
#define ZDEBUG
#endif
#endif







#if defined (_WIN32) || defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (macintosh)
#define LOGMESSAGES  
#endif

#ifdef LOGMESSAGES
extern FILE *logstream;
extern char logfilename[1024];
#endif

 
#define SRB2APPLICATION "SRB2"


#ifdef DEVELOP
#define VERSIONSTRING "Development EXE"
#define VERSIONSTRING_RC "Development EXE" "\0"



#else
#ifdef BETAVERSION
#define VERSIONSTRING "v"SRB2VERSION" "BETAVERSION
#define VERSIONSTRING_RC SRB2VERSION " " BETAVERSION "\0"
#else
#define VERSIONSTRING "v"SRB2VERSION
#define VERSIONSTRING_RC SRB2VERSION "\0"
#endif


#endif

#define VERSIONSTRINGW WSTRING (VERSIONSTRING)

 
#define SERVER_URL_PROTOCOL "srb2://"

















#ifndef BETAVERSION
#define UPDATE_ALERT
#endif



#define UPDATE_ALERT_STRING \
"A new update is available for SRB2.\n"\
"Please visit SRB2.org to download it.\n"\
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"\
"\n"\
"This update is required for online\n"\
"play using the Master Server.\n"\
"You will not be able to connect to\n"\
"the Master Server until you update to\n"\
"the newest version of the game.\n"\
"\n"\
"(Press a key)\n"



#define UPDATE_ALERT_STRING_CONSOLE \
"A new update is available for SRB2.\n"\
"Please visit SRB2.org to download it.\n"\
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"\
"\n"\
"This update is required for online play using the Master Server.\n"\
"You will not be able to connect to the Master Server\n"\
"until you update to the newest version of the game.\n"





#define CODEBASE 220





#define MAJOREXECVERSION MODVERSION
#define MINOREXECVERSION 0



#define GETMAJOREXECVERSION(v) (v & 0xFFFF)
#define GETMINOREXECVERSION(v) (v >> 16)
#define GETEXECVERSION(major,minor) (major + (minor << 16))
#define EXECVERSION GETEXECVERSION(MAJOREXECVERSION, MINOREXECVERSION)






#define MAXPLAYERS 32
#define MAXPLAYERNAME 21
#define PLAYERSMASK (MAXPLAYERS-1)






#define MAXSKINS 255
#define MAXCHARACTERSLOTS (MAXSKINS * 3)  

#define COLORRAMPSIZE 16
#define MAXCOLORNAME 32
#define NUMCOLORFREESLOTS 1024

typedef struct skincolor_s
{
	char name[MAXCOLORNAME+1];   
	UINT8 ramp[COLORRAMPSIZE];   
	UINT16 invcolor;             
	UINT8 invshade;              
	UINT16 chatcolor;            
	boolean accessible;          
} skincolor_t;

typedef enum
{
	SKINCOLOR_NONE = 0,


	SKINCOLOR_WHITE,
	SKINCOLOR_BONE,
	SKINCOLOR_CLOUDY,
	SKINCOLOR_GREY,
	SKINCOLOR_SILVER,
	SKINCOLOR_CARBON,
	SKINCOLOR_JET,
	SKINCOLOR_BLACK,


	SKINCOLOR_AETHER,
	SKINCOLOR_SLATE,
	SKINCOLOR_MOONSTONE,
	SKINCOLOR_BLUEBELL,
	SKINCOLOR_PINK,
	SKINCOLOR_ROSEWOOD,
	SKINCOLOR_YOGURT,
	SKINCOLOR_LATTE,
	SKINCOLOR_BROWN,
	SKINCOLOR_BOULDER,
	SKINCOLOR_BRONZE,
	SKINCOLOR_SEPIA,
	SKINCOLOR_ECRU,
	SKINCOLOR_TAN,
	SKINCOLOR_BEIGE,
	SKINCOLOR_ROSEBUSH,
	SKINCOLOR_MOSS,
	SKINCOLOR_AZURE,
	SKINCOLOR_EGGPLANT,
	SKINCOLOR_LAVENDER,



	SKINCOLOR_RUBY,
	SKINCOLOR_CHERRY,
	SKINCOLOR_SALMON,
	SKINCOLOR_PEPPER,
	SKINCOLOR_RED,
	SKINCOLOR_CRIMSON,
	SKINCOLOR_FLAME,
	SKINCOLOR_GARNET,
	SKINCOLOR_KETCHUP,
	SKINCOLOR_PEACHY,
	SKINCOLOR_QUAIL,
	SKINCOLOR_FOUNDATION,
	SKINCOLOR_SUNSET,
	SKINCOLOR_COPPER,
	SKINCOLOR_APRICOT,
	SKINCOLOR_ORANGE,
	SKINCOLOR_RUST,
	SKINCOLOR_TANGERINE,
	SKINCOLOR_TOPAZ,
	SKINCOLOR_GOLD,
	SKINCOLOR_SANDY,
	SKINCOLOR_GOLDENROD,
	SKINCOLOR_YELLOW,
	SKINCOLOR_OLIVE,
	SKINCOLOR_PEAR,
	SKINCOLOR_LEMON,
	SKINCOLOR_LIME,
	SKINCOLOR_PERIDOT,
	SKINCOLOR_APPLE,
	SKINCOLOR_HEADLIGHT,
	SKINCOLOR_CHARTREUSE,
	SKINCOLOR_GREEN,
	SKINCOLOR_FOREST,
	SKINCOLOR_SHAMROCK,
	SKINCOLOR_JADE,
	SKINCOLOR_MINT,
	SKINCOLOR_MASTER,
	SKINCOLOR_EMERALD,
	SKINCOLOR_SEAFOAM,
	SKINCOLOR_ISLAND,
	SKINCOLOR_BOTTLE,
	SKINCOLOR_AQUA,
	SKINCOLOR_TEAL,
	SKINCOLOR_OCEAN,
	SKINCOLOR_WAVE,
	SKINCOLOR_CYAN,
	SKINCOLOR_TURQUOISE,
	SKINCOLOR_AQUAMARINE,
	SKINCOLOR_SKY,
	SKINCOLOR_MARINE,
	SKINCOLOR_CERULEAN,
	SKINCOLOR_DREAM,
	SKINCOLOR_ICY,
	SKINCOLOR_DAYBREAK,
	SKINCOLOR_SAPPHIRE,  
	SKINCOLOR_ARCTIC,
	SKINCOLOR_CORNFLOWER,
	SKINCOLOR_BLUE,
	SKINCOLOR_COBALT,
	SKINCOLOR_MIDNIGHT,
	SKINCOLOR_GALAXY,
	SKINCOLOR_VAPOR,
	SKINCOLOR_DUSK,
	SKINCOLOR_MAJESTY,
	SKINCOLOR_PASTEL,
	SKINCOLOR_PURPLE,
	SKINCOLOR_NOBLE,
	SKINCOLOR_FUCHSIA,
	SKINCOLOR_BUBBLEGUM,
	SKINCOLOR_SIBERITE,
	SKINCOLOR_MAGENTA,
	SKINCOLOR_NEON,
	SKINCOLOR_VIOLET,
	SKINCOLOR_ROYAL,
	SKINCOLOR_LILAC,
	SKINCOLOR_MAUVE,
	SKINCOLOR_EVENTIDE,
	SKINCOLOR_PLUM,
	SKINCOLOR_RASPBERRY,
	SKINCOLOR_TAFFY,
	SKINCOLOR_ROSY,
	SKINCOLOR_FANCY,
	SKINCOLOR_SANGRIA,
	SKINCOLOR_VOLCANIC,

	FIRSTSUPERCOLOR,


	SKINCOLOR_SUPERSILVER1 = FIRSTSUPERCOLOR,
	SKINCOLOR_SUPERSILVER2,
	SKINCOLOR_SUPERSILVER3,
	SKINCOLOR_SUPERSILVER4,
	SKINCOLOR_SUPERSILVER5,

	SKINCOLOR_SUPERRED1,
	SKINCOLOR_SUPERRED2,
	SKINCOLOR_SUPERRED3,
	SKINCOLOR_SUPERRED4,
	SKINCOLOR_SUPERRED5,

	SKINCOLOR_SUPERORANGE1,
	SKINCOLOR_SUPERORANGE2,
	SKINCOLOR_SUPERORANGE3,
	SKINCOLOR_SUPERORANGE4,
	SKINCOLOR_SUPERORANGE5,

	SKINCOLOR_SUPERGOLD1,
	SKINCOLOR_SUPERGOLD2,
	SKINCOLOR_SUPERGOLD3,
	SKINCOLOR_SUPERGOLD4,
	SKINCOLOR_SUPERGOLD5,

	SKINCOLOR_SUPERPERIDOT1,
	SKINCOLOR_SUPERPERIDOT2,
	SKINCOLOR_SUPERPERIDOT3,
	SKINCOLOR_SUPERPERIDOT4,
	SKINCOLOR_SUPERPERIDOT5,

	SKINCOLOR_SUPERSKY1,
	SKINCOLOR_SUPERSKY2,
	SKINCOLOR_SUPERSKY3,
	SKINCOLOR_SUPERSKY4,
	SKINCOLOR_SUPERSKY5,

	SKINCOLOR_SUPERPURPLE1,
	SKINCOLOR_SUPERPURPLE2,
	SKINCOLOR_SUPERPURPLE3,
	SKINCOLOR_SUPERPURPLE4,
	SKINCOLOR_SUPERPURPLE5,

	SKINCOLOR_SUPERRUST1,
	SKINCOLOR_SUPERRUST2,
	SKINCOLOR_SUPERRUST3,
	SKINCOLOR_SUPERRUST4,
	SKINCOLOR_SUPERRUST5,

	SKINCOLOR_SUPERTAN1,
	SKINCOLOR_SUPERTAN2,
	SKINCOLOR_SUPERTAN3,
	SKINCOLOR_SUPERTAN4,
	SKINCOLOR_SUPERTAN5,

	SKINCOLOR_FIRSTFREESLOT,
	SKINCOLOR_LASTFREESLOT = SKINCOLOR_FIRSTFREESLOT + NUMCOLORFREESLOTS - 1,

	MAXSKINCOLORS,

	NUMSUPERCOLORS = ((SKINCOLOR_FIRSTFREESLOT - FIRSTSUPERCOLOR)/5)
} skincolornum_t;

extern UINT16 numskincolors;

extern skincolor_t skincolors[MAXSKINCOLORS];



#define TICRATE 35
#define NEWTICRATERATIO 1  
#define NEWTICRATE (TICRATE*NEWTICRATERATIO)

#define MUSICRATE 1000  

#define RING_DIST 512*FRACUNIT  

#define PUSHACCEL (2*FRACUNIT)  


enum {
	LE_PINCHPHASE      =    -2,  
	LE_ALLBOSSESDEAD   =    -3,  
	LE_BOSSDEAD        =    -4,  
	LE_BOSS4DROP       =    -5,  
	LE_BRAKVILEATACK   =    -6,  
	LE_TURRET          = 32000,  
	LE_BRAKPLATFORM    =  4200,  
	LE_CAPSULE2        =   682,  
	LE_CAPSULE1        =   681,  
	LE_CAPSULE0        =   680,  
	LE_KOOPA           =   650,  
	LE_AXE             =   649,  
	LE_PARAMWIDTH      =  -100   
};


#if (defined (__unix__) || defined (UNIXCOMMON)) && !defined (__CYGWIN__) && !defined (__APPLE__)
#define DEFAULTDIR ".srb2"
#else
#define DEFAULTDIR "srb2"
#endif

#include "g_state.h"



                               

                               

             


  
void I_Error(const char *error, ...) FUNCIERROR;

                                                                                                       
                               

                            

             
  
void I_OutputMsg(const char *error, ...) FUNCPRINTF;


typedef enum
{
	CONS_NOTICE,
	CONS_WARNING,
	CONS_ERROR
} alerttype_t;

void CONS_Printf(const char *fmt, ...) FUNCPRINTF;
void CONS_Alert(alerttype_t level, const char *fmt, ...) FUNCDEBUG;
void CONS_Debug(INT32 debugflags, const char *fmt, ...) FUNCDEBUG;


#define POTENTIALLYUNUSED CONS_Alert(CONS_WARNING, "(%s:%d) Unused code appears to be used.\n", __FILE__, __LINE__)

#include "m_swap.h"


#define SAVEGAMENAME "srb2sav"
extern char savegamename[256];
extern char liveeventbackup[256];


#ifdef GETTEXT
#define M_GetText(String) gettext(String)
#else


#define M_GetText(x) (x)
#endif
void M_StartupLocale(void);
void *M_Memcpy(void* dest, const void* src, size_t n);
char *va(const char *format, ...) FUNCPRINTF;
char *M_GetToken(const char *inputString);
void M_UnGetToken(void);
void M_TokenizerOpen(const char *inputString, size_t len);
void M_TokenizerClose(void);
const char *M_TokenizerRead(UINT32 i);
const char *M_TokenizerReadZDoom(UINT32 i);
UINT32 M_TokenizerGetEndPos(void);
void M_TokenizerSetEndPos(UINT32 newPos);
char *sizeu1(size_t num);
char *sizeu2(size_t num);
char *sizeu3(size_t num);
char *sizeu4(size_t num);
char *sizeu5(size_t num);


extern int    VERSION;
extern int SUBVERSION;
extern boolean devparm;  

extern INT32 cv_debug;

#define DBG_BASIC       0x0001
#define DBG_DETAILED    0x0002
#define DBG_PLAYER      0x0004
#define DBG_RENDER      0x0008
#define DBG_NIGHTSBASIC 0x0010
#define DBG_NIGHTS      0x0020
#define DBG_POLYOBJ     0x0040
#define DBG_GAMELOGIC   0x0080
#define DBG_NETPLAY     0x0100
#define DBG_MEMORY      0x0200
#define DBG_SETUP       0x0400
#define DBG_LUA         0x0800
#define DBG_RANDOMIZER  0x1000
#define DBG_VIEWMORPH   0x2000





#define ANG2RAD(angle) ((float)((angle)*M_PI)/ANGLE_180)


extern UINT8 shiftdown, ctrldown, altdown;
extern boolean capslock;


#define ZZ_Alloc(x) Z_Malloc(x, PU_STATIC, NULL)
#define ZZ_Calloc(x) Z_Calloc(x, PU_STATIC, NULL)


INT32 I_GetKey(void);

 
static inline
UINT32 quickncasehash (const char *p, size_t n)
{
	size_t i = 0;
	UINT32 x = 5381;

	while (i < n && p[i])
	{
		x = (x * 33) ^ tolower(p[i]);
		i++;
	}

	return x;
}

#ifndef min  
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max  
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif


#define MAX_JOYSTICKS 4

#ifndef M_PIl
#define M_PIl 3.1415926535897932384626433832795029L
#endif


#ifndef FLT_EPSILON
#define FLT_EPSILON 1.1920928955078125e-7f
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16l
#endif


#ifdef PARANOIA
#define I_Assert(e) ((e) ? (void)0 : I_Error("assert failed: %s, file %s, line %d", #e, __FILE__, __LINE__))
#else
#define I_Assert(e) ((void)0)
#endif
#define I_StaticAssert(e) static_assert(e, "Static assertion failed: " #e)



#if defined (_WIN32)
	#define PATHSEP "\\"
#else
	#define PATHSEP "/"
#endif

#define PUNCTUATION "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"


extern const char
	*compdate,
	*comptime,
	*comprevision,
	*compbranch,
	*compnote,
	*comptype;
extern int
	compuncommitted,
	compoptimized;
























#define SHUFFLE  






#define ALLOW_RESETDATA








#define GL_SHADERS



#define SECTORSPECIALSAFTERTHINK


#define ROTSPRITE
#define ROTANGLES 72  
#define ROTANGDIFF (360 / ROTANGLES)


#ifndef HAVE_PNG
#define NO_PNG_LUMPS
#endif


#define OLD22DEMOCOMPAT

#ifdef HAVE_CURL
#define MASTERSERVER
#else
#undef UPDATE_ALERT
#endif

#endif  
