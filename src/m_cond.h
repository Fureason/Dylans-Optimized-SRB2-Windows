// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2012-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __M_COND__
#define __M_COND__

#include "doomdef.h"
#include "doomdata.h"







typedef enum
{
	UC_PLAYTIME,         
	UC_GAMECLEAR,        
	UC_ALLEMERALDS,      
	UC_ULTIMATECLEAR,    
	UC_OVERALLSCORE,     
	UC_OVERALLTIME,      
	UC_OVERALLRINGS,     
	UC_MAPVISITED,       
	UC_MAPBEATEN,        
	UC_MAPALLEMERALDS,   
	UC_MAPULTIMATE,      
	UC_MAPPERFECT,       
	UC_MAPSCORE,         
	UC_MAPTIME,          
	UC_MAPRINGS,         
	UC_NIGHTSSCORE,      
	UC_NIGHTSTIME,       
	UC_NIGHTSGRADE,      
	UC_TRIGGER,          
	UC_TOTALEMBLEMS,     
	UC_EMBLEM,           
	UC_EXTRAEMBLEM,      
	UC_CONDITIONSET,     
} conditiontype_t;


typedef struct
{
	UINT32 id;            



	conditiontype_t type; 
	INT32 requirement;    
	INT16 extrainfo1;     
	INT16 extrainfo2;     
} condition_t;
typedef struct
{
	UINT32 numconditions;    
	condition_t *condition;  
} conditionset_t;


#define ET_GLOBAL 0  
#define ET_SKIN   1  
#define ET_MAP    2  
#define ET_SCORE  3  
#define ET_TIME   4  
#define ET_RINGS  5  
#define ET_NGRADE 6  
#define ET_NTIME  7  


#define GE_NIGHTSPULL 1  
#define GE_NIGHTSITEM 2  


#define ME_ALLEMERALDS 1
#define ME_ULTIMATE    2
#define ME_PERFECT     4

typedef struct
{
	UINT8 type;       
	INT16 tag;        
	INT16 level;      
	UINT8 sprite;     
	UINT16 color;     
	INT32 var;        
	char *stringVar;  
	char hint[110];   
} emblem_t;
typedef struct
{
	char name[20];           
	char description[40];    
	UINT8 conditionset;      
	UINT8 showconditionset;  
	UINT8 sprite;            
	UINT16 color;            
} extraemblem_t;


typedef struct
{
	char name[64];
	char objective[64];
	UINT16 height;  
	UINT8 conditionset;
	UINT8 showconditionset;
	INT16 type;
	INT16 variable;
	char *stringVar;
	UINT8 nocecho;
	UINT8 nochecklist;
} unlockable_t;

#define SECRET_NONE         -6  
#define SECRET_ITEMFINDER	-5  
#define SECRET_EMBLEMHINTS	-4  
#define SECRET_PANDORA		-3  
#define SECRET_RECORDATTACK	-2  
#define SECRET_NIGHTSMODE	-1  
#define SECRET_HEADER		 0  
#define SECRET_LEVELSELECT	 1  
#define SECRET_WARP			 2  
#define SECRET_SOUNDTEST	 3  
#define SECRET_CREDITS		 4  
#define SECRET_SKIN			 5  



#define MAXCONDITIONSETS 128
#define MAXEMBLEMS       512
#define MAXEXTRAEMBLEMS   48
#define MAXUNLOCKABLES    80

                                                              
    
typedef struct
{
	tic_t time;    
	UINT32 score;  
	UINT16 rings;  
} recorddata_t;

                             
                                                       
    
#define GRADE_F 0
#define GRADE_E 1
#define GRADE_D 2
#define GRADE_C 3
#define GRADE_B 4
#define GRADE_A 5
#define GRADE_S 6

typedef struct
{

	UINT8	nummares;
	UINT32	score[9];
	UINT8	grade[9];
	tic_t	time[9];
} nightsdata_t;


#define MV_VISITED      1
#define MV_BEATEN       2
#define MV_ALLEMERALDS  4
#define MV_ULTIMATE     8
#define MV_PERFECT     16
#define MV_PERFECTRA   32
#define MV_MAX         63  


extern nightsdata_t ntemprecords[MAXPLAYERS];



typedef struct
{

	boolean loaded;


	boolean achieved[MAXCONDITIONSETS];


	boolean collected[MAXEMBLEMS];


	boolean extraCollected[MAXEXTRAEMBLEMS];


	boolean unlocked[MAXUNLOCKABLES];


	recorddata_t *mainrecords[NUMMAPS];
	nightsdata_t *nightsrecords[NUMMAPS];
	UINT8 mapvisited[NUMMAPS];


	UINT32 timesBeaten;
	UINT32 timesBeatenWithEmeralds;
	UINT32 timesBeatenUltimate;


	UINT32 totalplaytime;
} gamedata_t;

extern gamedata_t *clientGamedata;
extern gamedata_t *serverGamedata;

extern conditionset_t conditionSets[MAXCONDITIONSETS];
extern emblem_t emblemlocations[MAXEMBLEMS];
extern extraemblem_t extraemblems[MAXEXTRAEMBLEMS];
extern unlockable_t unlockables[MAXUNLOCKABLES];

extern INT32 numemblems;
extern INT32 numextraemblems;

extern UINT32 unlocktriggers;

gamedata_t *M_NewGameDataStruct(void);
void M_CopyGameData(gamedata_t *dest, gamedata_t *src);


void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2);


void M_ClearConditionSet(UINT8 set);
void M_ClearSecrets(gamedata_t *data);


void M_CheckUnlockConditions(gamedata_t *data);
UINT8 M_UpdateUnlockablesAndExtraEmblems(gamedata_t *data);
void M_SilentUpdateUnlockablesAndEmblems(gamedata_t *data);
UINT8 M_CheckLevelEmblems(gamedata_t *data);
UINT8 M_CompletionEmblems(gamedata_t *data);

void M_SilentUpdateSkinAvailabilites(void);


UINT8 M_AnySecretUnlocked(gamedata_t *data);
UINT8 M_SecretUnlocked(INT32 type, gamedata_t *data);
UINT8 M_MapLocked(INT32 mapnum, gamedata_t *data);
UINT8 M_CampaignWarpIsCheat(INT32 gt, INT32 mapnum, gamedata_t *data);
INT32 M_CountEmblems(gamedata_t *data);


emblem_t *M_GetLevelEmblems(INT32 mapnum);
skincolornum_t M_GetEmblemColor(emblem_t *em);
const char *M_GetEmblemPatch(emblem_t *em, boolean big);
skincolornum_t M_GetExtraEmblemColor(extraemblem_t *em);
const char *M_GetExtraEmblemPatch(extraemblem_t *em, boolean big);




UINT8 M_GotEnoughEmblems(INT32 number, gamedata_t *data);
UINT8 M_GotHighEnoughScore(INT32 tscore, gamedata_t *data);
UINT8 M_GotLowEnoughTime(INT32 tictime, gamedata_t *data);
UINT8 M_GotHighEnoughRings(INT32 trings, gamedata_t *data);

INT32 M_UnlockableSkinNum(unlockable_t *unlock);
INT32 M_EmblemSkinNum(emblem_t *emblem);

#define M_Achieved(a, data) ((a) >= MAXCONDITIONSETS || data->achieved[a])

#endif
