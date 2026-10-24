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



#ifndef __I_VIDEO__
#define __I_VIDEO__

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

typedef enum
{

	render_soft = 1,


	render_opengl = 2,


	render_none = 3   
} rendermode_t;

                              
  
extern rendermode_t rendermode;

                                                    
  
extern rendermode_t chosenrendermode;

                           
  
void I_StartupGraphics(void);

                              
  
void I_ShutdownGraphics(void);

                                    

                                       

             
  
void I_SetPalette(RGBA_t *palette);

                                           
  
INT32 VID_NumModes(void);

                                          

               
                

                                 
  
INT32 VID_GetModeForSize(INT32 w, INT32 h);


                                   

                              
                                                                     
                                           
                                                       


                                    

                           
  
INT32 VID_SetMode(INT32 modenum);

                                  
                                     
  
boolean VID_CheckRenderer(void);

                           
  
void VID_StartupOpenGL(void);

                                  
  
void VID_CheckGLLoaded(rendermode_t oldrender);

                                       

                                 

                           
  
const char *VID_GetModeName(INT32 modenum);

void VID_PrepareModeList(void);

                                         
  
extern boolean allow_fullscreen;

                                                     
  
void I_UpdateNoBlit(void);

                                                  
  
void I_FinishUpdate(void);

                                               
  
void I_UpdateNoVsync(void);

                                                    

                      

             
  
void I_WaitVBL(INT32 count);

                                    

                                    

             
  
void I_ReadScreen(UINT8 *scr);

                          
  
void I_BeginRead(void);

                         
  
void I_EndRead(void);

UINT32 I_GetRefreshRate(void);
boolean I_VSyncProvidesPacing(void);
boolean I_CompositorProvidesPacing(void);

                                                                              
                                                                                  
boolean I_WindowManagerBlockedDuring(precise_t started, precise_t finished);

#endif
