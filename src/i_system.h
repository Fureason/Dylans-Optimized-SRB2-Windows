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



#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "d_ticcmd.h"
#include "d_event.h"

#ifdef __GNUG__
#pragma interface
#endif

                             
  
#define MAX_QUIT_FUNCS     16


                                        
  
extern UINT8 graphics_started;

                                        
  
extern UINT8 keyboard_started;

                                    

                                        

                                  
  
size_t I_GetFreeMem(size_t *total);

 
void I_ReportHitch(const char *operation, precise_t started, UINT32 threshold_ms);

#if defined(_WIN32) && !defined(DEDICATED)
 
void I_SetCrashContext(const char *context);
 
void I_SetCrashAddonContext(const char *filename);
 
void I_RecordDiagnosticHeartbeat(void);
 
void I_MarkLevelTransitionStart(precise_t started);
 
void I_MarkLevelTransitionEnd(void);
 
void I_ReportFrameHitch(precise_t started, precise_t finished);
 
void I_ReportRemoteExit(const char *playername, INT32 node, boolean timedout,
	UINT32 ping_tics);
#endif

 
boolean I_StartCompanionServer(const char *responsefile, const char *workingdir);
void I_StopCompanionServer(void);
boolean I_CompanionServerRunning(void);
 
boolean I_CompanionShutdownRequested(void);

                                                                              
                                                                             
                                                                 
                                                                       
    
precise_t I_GetPreciseTime(void);

                                                                            
    
size_t I_GetRandomBytes(char *destination, size_t count);

                                                                              
                                                                                
    
UINT64 I_GetPrecisePrecision(void);

                                                                        
  
double I_GetFrameTime(void);

                                                                          
                                                                            
                                                                     
                                                                     
                                                                    
                                                                 

             
  
void I_Sleep(UINT32 ms);

                     

                      
                                              
                                                
                      
  
void I_GetEvent(void);

                                                                          
                                           
                             
  
void I_OsPolling(void);






                                     
  
ticcmd_t *I_BaseTiccmd(void);

                                       
  
ticcmd_t *I_BaseTiccmd2(void);

                                                                          
  
void I_Quit(void) FUNCNORETURN;

typedef enum
{
	EvilForce = -1,

	ConstantForce = 0,

	RampForce,

	SquareForce,
	SineForce,
	TriangleForce,
	SawtoothUpForce,
	SawtoothDownForce,

	NumberofForces,
} FFType;

typedef struct JoyFF_s
{
	INT32 ForceX;  
	INT32 ForceY;  

	UINT32 Duration;  
	INT32 Gain;  

	INT32 Magnitude;  

	INT32 Start;  
	INT32 End;  

	INT32 Offset;  
	UINT32 Phase;  
	UINT32 Period;  
} JoyFF_t;

                                               

                                  
                          

             
  

void I_Tactile(FFType Type, const JoyFF_t *Effect);

                                                

                                  
                          

             
  
void I_Tactile2(FFType Type, const JoyFF_t *Effect);

                                             
  
void I_JoyScale(void);

                                              
  
void I_JoyScale2(void);



                                        
  
void I_InitJoystick(void);

                                         
  
void I_InitJoystick2(void);

                                                      
  
INT32 I_NumJoys(void);

                                     

                               

                      
  
const char *I_GetJoyName(INT32 joyindex);

#ifndef NOMUMBLE
#include "p_mobj.h"  
#include "s_sound.h"  
                                             
  
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener);
#endif

                                  
  
void I_StartupMouse(void);

                                   
  
void I_StartupMouse2(void);

                                                   
  
void I_StartupTimer(void);

                               
  
typedef void (*quitfuncptr)();

                                                             

                                                      

             
  
void I_AddExitFunc(void (*func)());

                                        

                                                  

             
  
void I_RemoveExitFunc(void (*func)());

                                                                                 
  
INT32 I_StartupSystem(void);

                           
  
void I_ShutdownSystem(void);

                                          

                                                               

             
  
void I_GetDiskFreeSpace(INT64 *freespace);

                                   
  
char *I_GetUserName(void);

                               

                               
                            

                             
  
INT32 I_mkdir(const char *dirname, INT32 unixright);

typedef struct {
	int FPU        : 1;  
	int CPUID      : 1;  
	int RDTSC      : 1;  
	int MMX        : 1;  
	int MMXExt     : 1;  
	int CMOV       : 1;  
	int AMD3DNow   : 1;  
	int AMD3DNowExt: 1;  
	int SSE        : 1;  
	int SSE2       : 1;  
	int SSE3       : 1;  
	int IA64       : 1;  
	int AMD64      : 1;  
	int AltiVec    : 1;  
	int FPPE       : 1;  
	int PFC        : 1;  
	int cmpxchg    : 1;  
	int cmpxchg16b : 1;  
	int cmp8xchg16 : 1;  
	int FPE        : 1;  
	int DEP        : 1;  
	int PPCMM64    : 1;  
	int ALPHAbyte  : 1;  
	int PAE        : 1;  
	int CPUs       : 8;
} CPUInfoFlags;


                         
                         
  
const CPUInfoFlags *I_CPUInfo(void);

                        
                          
  
const char *I_LocateWad(void);

                                  
  
void I_GetJoystickEvents(void);

                                   
  
void I_GetJoystick2Events(void);

                        
  
void I_GetMouseEvents(void);

                                                  
  
void I_UpdateMouseGrab(void);

char *I_GetEnv(const char *name);

INT32 I_PutEnv(char *variable);

                                       
  
INT32 I_ClipboardCopy(const char *data, size_t size);

                                              
  
const char *I_ClipboardPaste(void);

void I_RegisterSysCommands(void);

                                                                                    
  
void I_GetCursorPosition(INT32 *x, INT32 *y);

                                            
  
void I_SetMouseGrab(boolean grab);

                                   
  
const char *I_GetSysName(void);

                                                                                      
   
void I_SetTextInputMode(boolean active);

                                             
   
boolean I_GetTextInputMode(void);

#endif
