// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdio.h>
#include "doomdef.h"
#include "p_saveg.h"





 
typedef enum
{
	COM_ADMIN       = 1,
	COM_SPLITSCREEN = 2,
	COM_LOCAL       = 4,





	COM_LUA         = 8,
} com_flags_t;

typedef void (*com_func_t)(void);

void COM_AddCommand(const char *name, com_func_t func, com_flags_t flags);
int COM_AddLuaCommand(const char *name);

size_t COM_Argc(void);
const char *COM_Argv(size_t arg);  
char *COM_Args(void);
size_t COM_CheckParm(const char *check);  
size_t COM_CheckPartialParm(const char *check);
size_t COM_FirstOption(void);


const char *COM_CompleteCommand(const char *partial, INT32 skips);

const char *COM_CompleteAlias(const char *partial, INT32 skips);


#define COM_BufAddText(s) COM_BufAddTextEx(s, 0)
void COM_BufAddTextEx(const char *btext, com_flags_t flags);


#define COM_BufInsertText(s) COM_BufInsertTextEx(s, 0)
void COM_BufInsertTextEx(const char *btext, com_flags_t flags);


void COM_ImmedExecute(const char *ptext);


void COM_BufExecute(void);


boolean COM_ExecFile(const char *scriptname, com_flags_t flags, boolean silent);


void COM_BufTicker(void);


void COM_Init(void);





typedef struct vsbuf_s
{
	boolean allowoverflow;  
	boolean overflowed;  
	UINT8 *data;
	size_t maxsize;
	size_t cursize;
} vsbuf_t;

void VS_Alloc(vsbuf_t *buf, size_t initsize);
void VS_Free(vsbuf_t *buf);
void VS_Clear(vsbuf_t *buf);
void *VS_GetSpace(vsbuf_t *buf, size_t length);
void VS_Write(vsbuf_t *buf, const void *data, size_t length);
void VS_WriteEx(vsbuf_t *buf, const void *data, size_t length, com_flags_t flags);
void VS_Print(vsbuf_t *buf, const char *data);  











typedef enum
{
	CV_SAVE = 1,    
	CV_CALL = 2,    
	CV_NETVAR = 4,  
	CV_NOINIT = 8,  
	CV_FLOAT = 16,  


	CV_NOTINNET = 32,     
	CV_MODIFIED = 64,     
	CV_SHOWMODIF = 128,   
	CV_SHOWMODIFONETIME = 256,  
	CV_NOSHOWHELP = 512,  
	CV_HIDEN = 1024,  


	CV_CHEAT = 2048,  
	CV_ALLOWLUA = 4096, 
} cvflags_t;

typedef struct CV_PossibleValue_s
{
	INT32 value;
	const char *strvalue;
} CV_PossibleValue_t;

typedef struct consvar_s  
{
	const char *name;
	const char *defaultvalue;
	INT32 flags;             
	CV_PossibleValue_t *PossibleValue;  
	void (*func)(void);    
	boolean (*can_change)(const char*);    
	INT32 value;             
	const char *string;    
	char *zstring;         

	struct
	{
		char allocated;  
		union
		{
			char       * string;
			const char * const_munge;
		} v;
	} revert;              

	UINT16 netid;  

	char changed;          
	struct consvar_s *next;
} consvar_t;

 
#define CVAR_INIT( ... ) \
{ __VA_ARGS__, NULL, 0, NULL, NULL, {0, {NULL}}, 0U, (char)0, NULL }

#define CVAR_INIT_WITH_CALLBACKS( ... ) \
{ __VA_ARGS__, 0, NULL, NULL, {0, {NULL}}, 0U, (char)0, NULL }

#ifdef OLD22DEMOCOMPAT
typedef struct old_demo_var old_demo_var_t;

struct old_demo_var
{
	UINT16  checksum;
	boolean collides; 

	consvar_t      *cvar;
	old_demo_var_t *next;
};
#endif 

extern CV_PossibleValue_t CV_OnOff[];
extern CV_PossibleValue_t CV_YesNo[];
extern CV_PossibleValue_t CV_Unsigned[];
extern CV_PossibleValue_t CV_Natural[];
extern CV_PossibleValue_t CV_TrueFalse[];


extern consvar_t cv_execversion;

extern boolean com_sudo;

void CV_InitFilterVar(void);
void CV_ToggleExecVersion(boolean enable);


void CV_RegisterVar(consvar_t *variable);


consvar_t *CV_FindVar(const char *name);


void CV_ClearChangedFlags(void);


const char *CV_CompleteVar(char *partial, INT32 skips);


void CV_Set(consvar_t *var, const char *value);


void CV_SetValue(consvar_t *var, INT32 value);


void CV_StealthSetValue(consvar_t *var, INT32 value);
void CV_StealthSet(consvar_t *var, const char *value);


void CV_AddValue(consvar_t *var, INT32 increment);


void CV_SaveVariables(FILE *f);


void CV_SaveVars(save_t *p, boolean in_demo);

#define CV_SaveNetVars(p) CV_SaveVars(p, false)
void CV_LoadNetVars(save_t *p);


void CV_RevertNetVars(void);

#define CV_SaveDemoVars(p) CV_SaveVars(p, true)
void CV_LoadDemoVars(save_t *p);

#ifdef OLD22DEMOCOMPAT
void CV_LoadOldDemoVars(save_t *p);
#endif


void CV_ResetCheatNetVars(void);

boolean CV_IsSetToDefault(consvar_t *v);
UINT8 CV_CheatsEnabled(void);

#endif  
