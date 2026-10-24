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








#ifndef __DOOMSTAT__
#define __DOOMSTAT__


#include "doomdata.h"


#include "d_player.h"






extern INT16 gamemap;
extern char mapmusname[7];
extern UINT16 mapmusflags;
extern UINT32 mapmusposition;
#define MUSIC_TRACKMASK   0x0FFF  
#define MUSIC_RELOADRESET 0x8000  
#define MUSIC_FORCERESET  0x4000  


extern UINT32 maptol;
extern UINT8 globalweather;
extern INT32 curWeather;
extern INT32 cursaveslot;

extern INT16 lastmaploaded;
extern UINT8 gamecomplete;


typedef enum
{
	MA_RUNNING     = 1,     
	MA_INIT        = 1<<1,  
	MA_NOCUTSCENES = 1<<2,  
	MA_INGAME      = 1<<3   
} marathonmode_t;

extern marathonmode_t marathonmode;
extern tic_t marathontime;

#define maxgameovers 13
extern UINT8 numgameovers;
extern SINT8 startinglivesbalance[maxgameovers+1];

#define PRECIP_NONE  0
#define PRECIP_STORM 1
#define PRECIP_SNOW  2
#define PRECIP_RAIN  3
#define PRECIP_BLANK 4
#define PRECIP_STORM_NORAIN 5
#define PRECIP_STORM_NOSTRIKES 6


extern boolean modifiedgame;
extern UINT16 mainwads;
extern boolean savemoddata;  
extern boolean usedCheats;
extern boolean disableSpeedAdjust;  
extern boolean imcontinuing;  
extern boolean metalrecording;

#define ATTACKING_NONE   0
#define ATTACKING_RECORD 1
#define ATTACKING_NIGHTS 2
extern UINT8 modeattacking;


extern UINT8  numDemos;
extern UINT32 demoDelayTime;
extern UINT32 demoIdleTime;


extern boolean netgame;
extern boolean addedtogame;  

extern boolean multiplayer;

extern INT16 gametype;
extern UINT32 gametyperules;
extern INT16 gametypecount;

extern boolean splitscreen;
extern boolean circuitmap;  
extern boolean fromlevelselect;





extern boolean midi_disabled;
extern boolean sound_disabled;
extern boolean digital_disabled;






extern boolean menuactive;  
extern UINT8 paused;  
extern UINT8 window_notinfocus;  

extern boolean nodrawers;
extern boolean noblit;
extern boolean lastdraw;
extern postimg_t postimgtype;
extern INT32 postimgparam;
extern postimg_t postimgtype2;
extern INT32 postimgparam2;

extern INT32 viewwindowx, viewwindowy;
extern INT32 viewwidth, scaledviewwidth;


extern INT32 consoleplayer;
extern INT32 displayplayer;
extern INT32 secondarydisplayplayer;  


extern INT16 spstage_start, spmarathon_start;
extern INT16 sstage_start, sstage_end, smpstage_start, smpstage_end;

extern INT16 titlemap;
extern boolean hidetitlepics;
extern INT16 bootmap;  

extern INT16 tutorialmap;  
extern boolean tutorialmode;  
extern INT32 tutorialgcs;  
extern INT32 tutorialusemouse;  
extern INT32 tutorialfreelook;  
extern INT32 tutorialmousemove;  
extern INT32 tutorialanalog;  

extern boolean looptitle;


extern UINT16 skincolor_redteam, skincolor_blueteam, skincolor_redring, skincolor_bluering;

extern tic_t countdowntimer;
extern boolean countdowntimeup;
extern boolean exitfadestarted;

typedef struct
{
	UINT8 numpics;
	char picname[8][8+1];
	UINT8 pichires[8];
	char *text;
	UINT16 xcoord[8];
	UINT16 ycoord[8];
	UINT16 picduration[8];
	UINT8 musicloop;
	UINT16 textxpos;
	UINT16 textypos;

	char   musswitch[7];
	UINT16 musswitchflags;
	UINT32 musswitchposition;

	UINT8 fadecolor;  
	UINT8 fadeinid;   
	UINT8 fadeoutid;  
} scene_t;  

typedef struct
{
	scene_t scene[128];  
	INT32 numscenes;  
} cutscene_t;

extern cutscene_t *cutscenes[128];


#define TUTORIAL_PROMPT 201  
#define TUTORIAL_AREAS 6
#define TUTORIAL_AREA_PROMPTS 5
#define MAX_PROMPTS (TUTORIAL_PROMPT+TUTORIAL_AREAS*TUTORIAL_AREA_PROMPTS*3)  
#define MAX_PAGES 128

#define PROMPT_PIC_PERSIST 0
#define PROMPT_PIC_LOOP 1
#define PROMPT_PIC_DESTROY 2
#define MAX_PROMPT_PICS 8
typedef struct
{
	UINT8 numpics;
	UINT8 picmode;  
	UINT8 pictoloop;  
	UINT8 pictostart;  
	char picname[MAX_PROMPT_PICS][8+1];
	UINT8 pichires[MAX_PROMPT_PICS];
	UINT16 xcoord[MAX_PROMPT_PICS];  
	UINT16 ycoord[MAX_PROMPT_PICS];  
	UINT16 picduration[MAX_PROMPT_PICS];

	char   musswitch[6+1];
	UINT16 musswitchflags;
	UINT8 musicloop;

	char tag[32+1];  
	char name[32+2];  
	char iconname[8+1];  
	boolean rightside;  
	boolean iconflip;  
	UINT8 hidehud;  
	UINT8 lines;  
	INT32 backcolor;  
	UINT8 align;  
	UINT8 verticalalign;  
	UINT8 textspeed;  
	sfxenum_t textsfx;  
	UINT8 nextprompt;  
	UINT8 nextpage;  
	char nexttag[32+1];  
	INT32 timetonext;  
	char *text;
} textpage_t;

typedef struct
{
	textpage_t page[MAX_PAGES];
	INT32 numpages;  
} textprompt_t;

extern textprompt_t *textprompts[MAX_PROMPTS];


extern INT16 nextmapoverride;
extern UINT8 skipstats;
extern INT16 nextgametype;

extern UINT32 ssspheres;  


extern INT16 lastmap;  
extern mobj_t *redflag, *blueflag;  
extern mapthing_t *rflagpoint, *bflagpoint;  
#define GF_REDFLAG 1
#define GF_BLUEFLAG 2


typedef struct
{
	fixed_t x, y, z;
} mappoint_t;

extern struct quake
{

	fixed_t x,y,z;
	UINT16 time;


	mappoint_t *epicenter;
	fixed_t radius, intensity;
} quake;


typedef struct
{
	UINT32 grade[6];  
} nightsgrades_t;



typedef struct
{
	char option[31+1];  
	char value[255+1];  
} customoption_t;

                           
    
typedef struct
{

	char lvlttl[21+1];           
	char subttl[32+1];           
	UINT8 actnum;                
	UINT32 typeoflevel;          
	INT16 nextlevel;             
	INT16 marathonnext;          
	char keywords[32+1];         
	char musname[6+1];           
	UINT16 mustrack;             
	UINT32 muspos;               
	char forcecharacter[16+1];   
	UINT8 weather;               
	INT16 skynum;                
	INT16 skybox_scalex;         
	INT16 skybox_scaley;         
	INT16 skybox_scalez;         


	char interscreen[8+1];       
	char runsoc[32+1];           
	char scriptname[32+1];       
	UINT8 precutscenenum;        
	UINT8 cutscenenum;           
	INT16 countdown;             
	UINT16 palette;              
	UINT8 numlaps;               
	SINT8 unlockrequired;        
	UINT8 levelselect;           
	SINT8 bonustype;             
	SINT8 maxbonuslives;         

	UINT16 levelflags;           
	UINT8 menuflags;             

	char selectheading[21+1];    
	UINT16 startrings;           
	INT32 sstimer;               
	UINT32 ssspheres;            
	fixed_t gravity;             
	UINT16 nightstimer[8];       


	char ltzzpatch[8+1];         
	char ltzztext[8+1];          
	char ltactdiamond[8+1];      


	UINT8 numFlickies;           
	mobjtype_t *flickies;        


	UINT8 numGradedMares;        
	nightsgrades_t *grades;      


	UINT32 musinterfadeout;      
	char musintername[6+1];      

	char muspostbossname[6+1];   
	UINT16 muspostbosstrack;     
	UINT32 muspostbosspos;       
	UINT32 muspostbossfadein;    

	SINT8 musforcereset;         



	UINT8 numCustomOptions;      
	customoption_t *customopts;  
} mapheader_t;


#define LF_SCRIPTISFILE       (1<<0)  
#define LF_SPEEDMUSIC         (1<<1)  
#define LF_NOSSMUSIC          (1<<2)  
#define LF_NORELOAD           (1<<3)  
#define LF_NOZONE             (1<<4)  
#define LF_SAVEGAME           (1<<5)  
#define LF_MIXNIGHTSCOUNTDOWN (1<<6)  
#define LF_WARNINGTITLE       (1<<7)  

#define LF_NOTITLECARDFIRST        (1<<8)
#define LF_NOTITLECARDRESPAWN      (1<<9)
#define LF_NOTITLECARDRECORDATTACK (1<<10)
#define LF_NOTITLECARD  (LF_NOTITLECARDFIRST|LF_NOTITLECARDRESPAWN|LF_NOTITLECARDRECORDATTACK)  

#define LF2_HIDEINMENU     1  
#define LF2_HIDEINSTATS    2  
#define LF2_RECORDATTACK   4  
#define LF2_NIGHTSATTACK   8  
#define LF2_NOVISITNEEDED 16  
#define LF2_WIDEICON      32  

extern mapheader_t* mapheaderinfo[NUMMAPS];


#define NUMGAMETYPEFREESLOTS 128
enum GameType
{
	GT_COOP = 0,  
	GT_COMPETITION,  
	GT_RACE,

	GT_MATCH,
	GT_TEAMMATCH,

	GT_TAG,
	GT_HIDEANDSEEK,

	GT_CTF,  

	GT_FIRSTFREESLOT,
	GT_LASTFREESLOT = GT_FIRSTFREESLOT + NUMGAMETYPEFREESLOTS - 1,
	NUMGAMETYPES
};



enum GameTypeRules
{
	GTR_CAMPAIGN         = 1,      
	GTR_RINGSLINGER      = 1<<1,   
	GTR_SPECTATORS       = 1<<2,   
	GTR_LIVES            = 1<<3,   
	GTR_TEAMS            = 1<<4,   
	GTR_FIRSTPERSON      = 1<<5,   
	GTR_POWERSTONES      = 1<<6,   
	GTR_TEAMFLAGS        = 1<<7,   
	GTR_FRIENDLY         = 1<<8,   
	GTR_SPECIALSTAGES    = 1<<9,   
	GTR_EMERALDTOKENS    = 1<<10,  
	GTR_EMERALDHUNT      = 1<<11,  
	GTR_RACE             = 1<<12,  
	GTR_TAG              = 1<<13,  
	GTR_POINTLIMIT       = 1<<14,  
	GTR_TIMELIMIT        = 1<<15,  
	GTR_OVERTIME         = 1<<16,  
	GTR_HURTMESSAGES     = 1<<17,  
	GTR_FRIENDLYFIRE     = 1<<18,  
	GTR_STARTCOUNTDOWN   = 1<<19,  
	GTR_HIDEFROZEN       = 1<<20,  
	GTR_BLINDFOLDED      = 1<<21,  
	GTR_RESPAWNDELAY     = 1<<22,  
	GTR_PITYSHIELD       = 1<<23,  
	GTR_DEATHPENALTY     = 1<<24,  
	GTR_NOSPECTATORSPAWN = 1<<25,  
	GTR_DEATHMATCHSTARTS = 1<<26,  
	GTR_SPAWNINVUL       = 1<<27,  
	GTR_SPAWNENEMIES     = 1<<28,  
	GTR_ALLOWEXIT        = 1<<29,  
	GTR_NOTITLECARD      = 1<<30,  
	GTR_CUTSCENES        = 1<<31,  
};


extern const char *Gametype_Names[NUMGAMETYPES];
extern const char *Gametype_ConstantNames[NUMGAMETYPES];


extern INT32 pointlimits[NUMGAMETYPES];
extern INT32 timelimits[NUMGAMETYPES];


enum TypeOfLevel
{
	TOL_SP          = 0x01,  
	TOL_COOP        = 0x02,  
	TOL_COMPETITION = 0x04,  
	TOL_RACE        = 0x08,  


	TOL_MATCH       = 0x10,  
	TOL_TAG         = 0x20,  


	TOL_CTF         = 0x40,  




	TOL_2D     = 0x0100,  
	TOL_MARIO  = 0x0200,  
	TOL_NIGHTS = 0x0400,  
	TOL_ERZ3   = 0x0800,  
	TOL_XMAS   = 0x1000,  
};

#define MAXTOL             (1<<31)
#define NUMBASETOLNAMES    (19)
#define NUMTOLNAMES        (NUMBASETOLNAMES + NUMGAMETYPEFREESLOTS)

typedef struct
{
	const char *name;
	UINT32 flag;
} tolinfo_t;
extern tolinfo_t TYPEOFLEVEL[NUMTOLNAMES];
extern UINT32 lastcustomtol;

extern boolean stagefailed;


extern UINT16 emeralds;
#define EMERALD1 1
#define EMERALD2 2
#define EMERALD3 4
#define EMERALD4 8
#define EMERALD5 16
#define EMERALD6 32
#define EMERALD7 64
#define ALL7EMERALDS(v) ((v & (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7)) == (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7))

#define NUM_LUABANKS 16  
extern INT32 luabanks[NUM_LUABANKS];

extern INT32 nummaprings;  

extern UINT32 token;  
extern UINT32 tokenlist;  
extern boolean gottoken;  
extern INT32 tokenbits;  
extern INT32 sstimer;  
extern UINT32 bluescore;  
extern UINT32 redscore;   


extern boolean CheckForBustableBlocks;
extern boolean CheckForBouncySector;
extern boolean CheckForQuicksand;
extern boolean CheckForMarioBlocks;
extern boolean CheckForFloatBob;
extern boolean CheckForReverseGravity;


extern UINT16 invulntics;
extern UINT16 sneakertics;
extern UINT16 flashingtics;
extern UINT16 tailsflytics;
extern UINT16 underwatertics;
extern UINT16 spacetimetics;
extern UINT16 extralifetics;
extern UINT16 nightslinktics;

extern UINT8 introtoplay;
extern UINT8 creditscutscene;
extern UINT8 useBlackRock;

extern UINT8 use1upSound;
extern UINT8 maxXtraLife;  

extern UINT8 useContinues;
#define continuesInSession (!multiplayer && (ultimatemode || (useContinues && !marathonmode) || (!modeattacking && !(cursaveslot > 0))))

extern UINT8 shareEmblems;

extern mobj_t *hunt1, *hunt2, *hunt3;  


extern UINT32 countdown;
extern UINT32 countdown2;

extern fixed_t gravity;


extern INT16 autobalance;
extern INT16 teamscramble;
extern INT16 scrambleplayers[MAXPLAYERS];  
extern INT16 scrambleteams[MAXPLAYERS];  
extern INT16 scrambletotal;  
extern INT16 scramblecount;  

extern INT32 cheats;

extern tic_t hidetime;








extern tic_t gametic;
#define localgametic leveltime


extern mapthing_t *playerstarts[MAXPLAYERS];  
extern mapthing_t *bluectfstarts[MAXPLAYERS];  
extern mapthing_t *redctfstarts[MAXPLAYERS];  

#define WAYPOINTSEQUENCESIZE 256
#define NUMWAYPOINTSEQUENCES 256
extern mobj_t *waypoints[NUMWAYPOINTSEQUENCES][WAYPOINTSEQUENCESIZE];
extern UINT16 numwaypoints[NUMWAYPOINTSEQUENCES];

void P_AddWaypoint(UINT8 sequence, UINT8 id, mobj_t *waypoint);
mobj_t *P_GetFirstWaypoint(UINT8 sequence);
mobj_t *P_GetLastWaypoint(UINT8 sequence);
mobj_t *P_GetPreviousWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetNextWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetClosestWaypoint(UINT8 sequence, mobj_t *mo);
boolean P_IsDegeneratedWaypointSequence(UINT8 sequence);





#if defined (macintosh)
#define DEBFILE(msg) I_OutputMsg(msg)
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if (debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
#else
#define DEBFILE(msg) {}
#endif
#endif

#ifdef DEBUGFILE
extern FILE *debugfile;
extern INT32 debugload;
#endif


extern boolean precache;



extern gamestate_t wipegamestate;
extern INT16 wipetypepre;
extern INT16 wipetypepost;


extern boolean singletics;





#include "netcode/d_clisrv.h"

extern consvar_t cv_timetic;  
extern consvar_t cv_powerupdisplay;  
extern consvar_t cv_showinput;  
extern consvar_t cv_showinputjoy;  
extern consvar_t cv_forceskin;  
extern consvar_t cv_downloading;  
extern consvar_t cv_autodownloadaddons;  
extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
extern INT32 serverplayer;
extern INT32 adminplayers[MAXPLAYERS];



#endif  
