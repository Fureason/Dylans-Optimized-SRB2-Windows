// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomdef.h"
#include "sounds.h"
#include "command.h"


typedef enum {
	MU_NONE,
	MU_WAV,
	MU_MOD,
	MU_MID,
	MU_OGG,
	MU_MP3,
	MU_FLAC,
	MU_GME,
	MU_MOD_EX,  
	MU_MID_EX  
} musictype_t;

                                             
  
extern UINT8 sound_started;

                             
  
extern consvar_t cv_samplerate;


                                

                        

                     
  
void *I_GetSfx(sfxinfo_t *sfx);

                                 

                              

             
  
void I_FreeSfx(sfxinfo_t *sfx);

                                   
  
void I_StartupSound(void);

                                                           
  
void I_ShutdownSound(void);

                                                                         
                                                                              
boolean I_StartMovieAudioCapture(const char *filename);
void I_UpdateMovieAudioCapture(void);
void I_StopMovieAudioCapture(void);





                                                        
                
                            
                               
                      
                         

                   
  
INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority, INT32 channel);

                                 

                              

             
  
void I_StopSound(INT32 handle);

                                                
  
void I_UpdateSound(void);

                                                                          

                         

                                              
  
boolean I_SoundIsPlaying(INT32 handle);

                                 

                         
                  
                      
                   

             
  
void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch);

                                      

                               

             
  
void I_SetSfxVolume(UINT8 volume);





                                 
  
void I_InitMusic(void);

                                     
  
void I_ShutdownMusic(void);





musictype_t I_SongType(void);
boolean I_SongPlaying(void);
boolean I_SongPaused(void);





boolean I_SetSongSpeed(float speed);





UINT32 I_GetSongLength(void);

boolean I_SetSongLoopPoint(UINT32 looppoint);
UINT32 I_GetSongLoopPoint(void);

boolean I_SetSongPosition(UINT32 position);
UINT32 I_GetSongPosition(void);





                                                

                                 
                       

                    

                  
  
boolean I_LoadSong(char *data, size_t len);

                                                 

                          

               
                         
  
void I_UnloadSong(void);

                                                        

                          
                                  

                                       

                                  
  
boolean I_PlaySong(boolean looping);

                                      

                          
             

                  
  
void I_StopSong(void);

                               

                          

             
  
void I_PauseSong(void);

                               

                          

             
  
void I_ResumeSong(void);

                                        

                               

             
  
void I_SetMusicVolume(UINT8 volume);

boolean I_SetSongTrack(INT32 track);





void I_SetInternalMusicVolume(UINT8 volume);
void I_StopFadingSong(void);
boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void));
boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void));
boolean I_FadeOutStopSong(UINT32 ms);
boolean I_FadeInPlaySong(UINT32 ms, boolean looping);

#endif
