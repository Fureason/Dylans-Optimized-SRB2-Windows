// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#ifndef __G_INPUT__
#define __G_INPUT__

#include "d_event.h"
#include "keys.h"
#include "command.h"



#define NUMKEYS 256

#define MOUSEBUTTONS 8
#define JOYBUTTONS   32  
#define JOYHATS      4   
#define JOYAXISSET   4   




typedef enum
{
	KEY_MOUSE1 = NUMKEYS,
	KEY_JOY1 = KEY_MOUSE1 + MOUSEBUTTONS,
	KEY_HAT1 = KEY_JOY1 + JOYBUTTONS,

	KEY_DBLMOUSE1 =KEY_HAT1 + JOYHATS*4,  
	KEY_DBLJOY1 = KEY_DBLMOUSE1 + MOUSEBUTTONS,
	KEY_DBLHAT1 = KEY_DBLJOY1 + JOYBUTTONS,

	KEY_2MOUSE1 = KEY_DBLHAT1 + JOYHATS*4,
	KEY_2JOY1 = KEY_2MOUSE1 + MOUSEBUTTONS,
	KEY_2HAT1 = KEY_2JOY1 + JOYBUTTONS,

	KEY_DBL2MOUSE1 = KEY_2HAT1 + JOYHATS*4,
	KEY_DBL2JOY1 = KEY_DBL2MOUSE1 + MOUSEBUTTONS,
	KEY_DBL2HAT1 = KEY_DBL2JOY1 + JOYBUTTONS,

	KEY_MOUSEWHEELUP = KEY_DBL2HAT1 + JOYHATS*4,
	KEY_MOUSEWHEELDOWN = KEY_MOUSEWHEELUP + 1,
	KEY_2MOUSEWHEELUP = KEY_MOUSEWHEELDOWN + 1,
	KEY_2MOUSEWHEELDOWN = KEY_2MOUSEWHEELUP + 1,

	NUMINPUTS = KEY_2MOUSEWHEELDOWN + 1,
} key_input_e;

typedef enum
{
	GC_NULL = 0,  
	GC_FORWARD,
	GC_BACKWARD,
	GC_STRAFELEFT,
	GC_STRAFERIGHT,
	GC_TURNLEFT,
	GC_TURNRIGHT,
	GC_WEAPONNEXT,
	GC_WEAPONPREV,
	GC_WEPSLOT1,
	GC_WEPSLOT2,
	GC_WEPSLOT3,
	GC_WEPSLOT4,
	GC_WEPSLOT5,
	GC_WEPSLOT6,
	GC_WEPSLOT7,
	GC_WEPSLOT8,
	GC_WEPSLOT9,
	GC_WEPSLOT10,
	GC_FIRE,
	GC_FIRENORMAL,
	GC_TOSSFLAG,
	GC_SPIN,
	GC_CAMTOGGLE,
	GC_CAMRESET,
	GC_LOOKUP,
	GC_LOOKDOWN,
	GC_CENTERVIEW,
	GC_MOUSEAIMING,  
	GC_TALKKEY,
	GC_TEAMKEY,
	GC_SCORES,
	GC_JUMP,
	GC_CONSOLE,
	GC_PAUSE,
	GC_SYSTEMMENU,
	GC_SCREENSHOT,
	GC_RECORDGIF,
	GC_VIEWPOINTNEXT,
	GC_VIEWPOINTPREV,
	GC_CUSTOM1,  
	GC_CUSTOM2,  
	GC_CUSTOM3,  
	NUM_GAMECONTROLS
} gamecontrols_e;

typedef enum
{
	gcs_custom,
	gcs_fps,
	gcs_platform,
	num_gamecontrolschemes
} gamecontrolschemes_e;


extern consvar_t cv_mousesens, cv_mouseysens;
extern consvar_t cv_mousesens2, cv_mouseysens2;
extern consvar_t cv_controlperkey;

typedef struct
{
	INT32 dx;  
	INT32 dy;
	INT32 mlookdy;  
	INT32 rdx;  
	INT32 rdy;
	UINT16 buttons;
} mouse_t;

#define MB_BUTTON1    0x0001
#define MB_BUTTON2    0x0002
#define MB_BUTTON3    0x0004
#define MB_BUTTON4    0x0008
#define MB_BUTTON5    0x0010
#define MB_BUTTON6    0x0020
#define MB_BUTTON7    0x0040
#define MB_BUTTON8    0x0080
#define MB_SCROLLUP   0x0100
#define MB_SCROLLDOWN 0x0200

extern mouse_t mouse;
extern mouse_t mouse2;

extern INT32 joyxmove[JOYAXISSET], joyymove[JOYAXISSET], joy2xmove[JOYAXISSET], joy2ymove[JOYAXISSET];


extern UINT8 gamekeydown[NUMINPUTS];


extern INT32 gamecontrol[NUM_GAMECONTROLS][2];
extern INT32 gamecontrolbis[NUM_GAMECONTROLS][2];  
extern INT32 gamecontroldefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2];  
extern INT32 gamecontrolbisdefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2];
#define PLAYER1INPUTDOWN(gc) (gamekeydown[gamecontrol[gc][0]] || gamekeydown[gamecontrol[gc][1]])
#define PLAYER2INPUTDOWN(gc) (gamekeydown[gamecontrolbis[gc][0]] || gamekeydown[gamecontrolbis[gc][1]])
#define PLAYERINPUTDOWN(p, gc) ((p) == 2 ? PLAYER2INPUTDOWN(gc) : PLAYER1INPUTDOWN(gc))

#define num_gcl_tutorial_check 6
#define num_gcl_tutorial_used 8
#define num_gcl_tutorial_full 13
#define num_gcl_movement 4
#define num_gcl_camera 2
#define num_gcl_movement_camera 6
#define num_gcl_jump 1
#define num_gcl_spin 1
#define num_gcl_jump_spin 2

extern const INT32 gcl_tutorial_check[num_gcl_tutorial_check];
extern const INT32 gcl_tutorial_used[num_gcl_tutorial_used];
extern const INT32 gcl_tutorial_full[num_gcl_tutorial_full];
extern const INT32 gcl_movement[num_gcl_movement];
extern const INT32 gcl_camera[num_gcl_camera];
extern const INT32 gcl_movement_camera[num_gcl_movement_camera];
extern const INT32 gcl_jump[num_gcl_jump];
extern const INT32 gcl_spin[num_gcl_spin];
extern const INT32 gcl_jump_spin[num_gcl_jump_spin];





void G_MapEventsToControls(event_t *ev);


const char *G_KeyNumToName(INT32 keynum);
INT32 G_KeyNameToNum(const char *keystr);


void G_ClearControlKeys(INT32 (*setupcontrols)[2], INT32 control);
void G_ClearAllControlKeys(void);
void Command_Setcontrol_f(void);
void Command_Setcontrol2_f(void);
void G_DefineDefaultControls(void);
INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2]);
INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify);


void G_SetMouseDeltas(INT32 dx, INT32 dy, UINT8 ssplayer);

#endif
