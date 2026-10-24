/*
  SDL Mixer X:  An extended audio mixer library, forked from SDL_mixer
  Copyright (C) 2014-2018 Vitaly Novichkov <admin@wohlnet.ru>

  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SDL_MIXER_H_
#define SDL_MIXER_H_

#include "SDL_stdinc.h"
#include "SDL_rwops.h"
#include "SDL_audio.h"
#include "SDL_endian.h"
#include "SDL_version.h"
#include "begin_code.h"

 
#define SDL_MIXER_X 1

#define MIXSDLCALL

#if defined(FORCE_STDCALLS) && defined(_WIN32)
#ifdef SDLCALL
#undef SDLCALL
#endif
#define SDLCALL   __stdcall
#define SDLCALLCC __stdcall
#else
#define SDLCALLCC
#endif

#ifndef DEPRECATED
#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif
#endif

 
#ifdef __cplusplus
extern "C" {
#endif

                                                         
  
#define SDL_MIXER_MAJOR_VERSION 2
#define SDL_MIXER_MINOR_VERSION 2
#define SDL_MIXER_PATCHLEVEL    0

                                                                           
                                    
   
#define SDL_MIXER_VERSION(X)                        \
{                                                   \
    (X)->major = SDL_MIXER_MAJOR_VERSION;           \
    (X)->minor = SDL_MIXER_MINOR_VERSION;           \
    (X)->patch = SDL_MIXER_PATCHLEVEL;              \
}

 
#define MIX_MAJOR_VERSION   SDL_MIXER_MAJOR_VERSION
#define MIX_MINOR_VERSION   SDL_MIXER_MINOR_VERSION
#define MIX_PATCHLEVEL      SDL_MIXER_PATCHLEVEL
#define MIX_VERSION(X)      SDL_MIXER_VERSION(X)

   
                                                                       
   
#define SDL_MIXER_COMPILEDVERSION \
    SDL_VERSIONNUM(SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL)

   
                                                                               
   
#define SDL_MIXER_VERSION_ATLEAST(X, Y, Z) \
    (SDL_MIXER_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))

                                                                              
                                                                        
                                     
   
extern DECLSPEC const SDL_version * SDLCALL Mix_Linked_Version(void);

typedef enum
{
    MIX_INIT_FLAC   = 0x00000001,
    MIX_INIT_MOD    = 0x00000002,
    MIX_INIT_MP3    = 0x00000008,
    MIX_INIT_OGG    = 0x00000010,
    MIX_INIT_MID    = 0x00000020,
    MIX_INIT_OPUS   = 0x00000040
} MIX_InitFlags;

                                                                      
                                                      
                                                                  
   
extern DECLSPEC int SDLCALL Mix_Init(int flags);

 
extern DECLSPEC void SDLCALL Mix_Quit(void);


 
#ifndef MIX_CHANNELS
#define MIX_CHANNELS    8
#endif

 
#define MIX_DEFAULT_FREQUENCY   44100
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define MIX_DEFAULT_FORMAT  AUDIO_S16LSB
#else
#define MIX_DEFAULT_FORMAT  AUDIO_S16MSB
#endif
#define MIX_DEFAULT_CHANNELS    2
#define MIX_MAX_VOLUME          SDL_MIX_MAXVOLUME  

 
typedef struct Mix_Chunk {
    int allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8 volume;        
} Mix_Chunk;

 
typedef enum {
    MIX_NO_FADING,
    MIX_FADING_OUT,
    MIX_FADING_IN
} Mix_Fading;

 
typedef enum {
    MUS_NONE,
    MUS_CMD,
    MUS_WAV,
    MUS_MOD,
    MUS_MID,
    MUS_OGG,
    MUS_MP3,
    MUS_MP3_MAD_UNUSED,
    MUS_FLAC,
    MUS_MODPLUG_UNUSED,
    MUS_OPUS,
    MUS_GME,
    MUS_ADLMIDI 
} Mix_MusicType;

typedef enum {
    MIDI_ADLMIDI,
    MIDI_Native,
    MIDI_Timidity,
    MIDI_OPNMIDI,
    MIDI_Fluidsynth,
    MIDI_ANY,
    MIDI_KnuwnDevices    
} Mix_MIDI_Device;

 
typedef enum {
    ADLMIDI_VM_AUTO,
    ADLMIDI_VM_GENERIC,
    ADLMIDI_VM_CMF,
    ADLMIDI_VM_DMX,
    ADLMIDI_VM_APOGEE,
    ADLMIDI_VM_9X
} Mix_ADLMIDI_VolumeModel;

 
typedef enum {
    ADLMIDI_OPL3_EMU_DEFAULT = -1,
    ADLMIDI_OPL3_EMU_NUKED = 0,
    ADLMIDI_OPL3_EMU_NUKED_1_7_4,
    ADLMIDI_OPL3_EMU_DOSBOX,
} Mix_ADLMIDI_Emulator;

 
typedef enum {
    OPNMIDI_OPN2_EMU_DEFAULT = -1,
    OPNMIDI_OPN2_EMU_MIME = 0,
    OPNMIDI_OPN2_EMU_NUKED,
    OPNMIDI_OPN2_EMU_GENS,
} Mix_OPNMIDI_Emulator;

 
typedef struct _Mix_Music Mix_Music;

 
extern DECLSPEC int SDLCALL Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);

 
extern DECLSPEC int SDLCALL Mix_OpenAudioDevice(int frequency, Uint16 format, int channels, int chunksize, const char* device, int allowed_changes);

                                                                  
                                                               
           
                                                              
   
extern DECLSPEC int SDLCALL Mix_AllocateChannels(int numchans);

                                                        
                                                                     
   
extern DECLSPEC int SDLCALL Mix_QuerySpec(int *frequency,Uint16 *format,int *channels);

                                                       
                                                                   
                                                             
                                    
                                                                       
                                                    
  
extern DECLSPEC Mix_Chunk * SDLCALL Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);
#define Mix_LoadWAV(file)   Mix_LoadWAV_RW(SDL_RWFromFile(file, "rb"), 1)
extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUS(const char *file);

                                            
                                                          
extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUS_RW(SDL_RWops *src, int freesrc);

                                                                                                                   
                                                                                          
    
extern DECLSPEC Mix_Music *SDLCALL Mix_LoadMUS_RW_ARG(SDL_RWops *src, int freesrc, const char *args);

                                                                         
                                                                          
                            
   
extern DECLSPEC Mix_Music *SDLCALL Mix_LoadMUS_RW_GME(SDL_RWops *src, int freesrc, int trackID);

 
extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUSType_RW(SDL_RWops *src, Mix_MusicType type, int freesrc);

                                                                       
                                                                            
extern DECLSPEC Mix_Music * SDLCALL Mix_LoadMUSType_RW_ARG(SDL_RWops *src, Mix_MusicType type, int freesrc, const char *args);

 
extern DECLSPEC Mix_Chunk * SDLCALL Mix_QuickLoad_WAV(Uint8 *mem);

 
extern DECLSPEC Mix_Chunk * SDLCALL Mix_QuickLoad_RAW(Uint8 *mem, Uint32 len);

 
extern DECLSPEC void SDLCALL Mix_FreeChunk(Mix_Chunk *chunk);
extern DECLSPEC void SDLCALL Mix_FreeMusic(Mix_Music *music);

                                                                            
                                                                           
                                                     
                                                                             
                                                           

              
         
                                               
                              
                                                                         

                                                                       
                                                                           
           

                                                                            
                                                             
  
extern DECLSPEC int SDLCALL Mix_GetNumChunkDecoders(void);
extern DECLSPEC const char * SDLCALL Mix_GetChunkDecoder(int index);
extern DECLSPEC SDL_bool SDLCALL Mix_HasChunkDecoder(const char *name);
extern DECLSPEC int SDLCALL Mix_GetNumMusicDecoders(void);
extern DECLSPEC const char * SDLCALL Mix_GetMusicDecoder(int index);
extern DECLSPEC SDL_bool SDLCALL Mix_HasMusicDecoder(const char *name);

                                                                       
                             
  
extern DECLSPEC Mix_MusicType SDLCALL Mix_GetMusicType(const Mix_Music *music);

 
extern DECLSPEC const char *SDLCALL Mix_GetMusicTitle(const Mix_Music *music);
 
extern DECLSPEC const char *SDLCALL Mix_GetMusicTitleTag(const Mix_Music *music);
 
extern DECLSPEC const char *SDLCALL Mix_GetMusicArtistTag(const Mix_Music *music);
 
extern DECLSPEC const char *SDLCALL Mix_GetMusicAlbumTag(const Mix_Music *music);
/* Get music copyright from meta-tag if possible */
extern DECLSPEC const char *SDLCALL Mix_GetMusicCopyrightTag(const Mix_Music *music);

                                                               
                                                                           
                                                    
  
extern DECLSPEC void SDLCALL Mix_SetPostMix(void (SDLCALL *mix_func)(void *udata, Uint8 *stream, int len), void *arg);

                                                          
                                                                 
   
extern DECLSPEC void SDLCALL Mix_HookMusic(void (SDLCALL *mix_func)(void *udata, Uint8 *stream, int len), void *arg);

                                                                              
                                        
   
extern DECLSPEC void SDLCALL Mix_HookMusicFinished(void (SDLCALL *music_finished)(void));

 
extern DECLSPEC void * SDLCALL Mix_GetMusicHookData(void);

  
                                                                  
                                                                          
                                                                         
                                                                      
                                                                          
                                 
   
extern DECLSPEC void SDLCALL Mix_ChannelFinished(void (SDLCALL *channel_finished)(int channel));


 

#define MIX_CHANNEL_POST  -2

                                                   
  
                                                            
  
                                                                          
                                                                       
                                                                            
                                                                         
                                                                             
                                                                             
                                                                             
                                                                              
                                                                            
           
  
                                                                
   
typedef void (SDLCALL *Mix_EffectFunc_t)(int chan, void *stream, int len, void *udata);

  
                                                                        
                                                                    
                                                                         
                                                                        
                       
  
                                                                
   
typedef void (SDLCALL *Mix_EffectDone_t)(int chan, void *udata);


                                                                          
                                                                            
                                                                         
                                                                         
                                                                           
                                                                        
                                                                       
                                                                         
                                                                       
                                                                               
                                                                        
                                                                        
                                                                         
            
                                                                         
                                                                            
                                                                        
                                                                 
                                                                       
                                                                           
                                                                            
                                                                          
                                                                         
                                                                             
                                                  
                                                                          
                                                                             
                                                                          
                                                                        
                                                                        
                                                                         
                                                                            
                                                                           
                                                                           
                                                                          
                                          
  
                                                                            
                                                                        
           
  
                                                                
  
                                                             
                                                        
   
extern DECLSPEC int SDLCALL Mix_RegisterEffect(int chan, Mix_EffectFunc_t f, Mix_EffectDone_t d, void *arg);


                                                                       
                                                               
                                                                          
                                                                    
                                              
                                                                         
                                                        
   
extern DECLSPEC int SDLCALL Mix_UnregisterEffect(int channel, Mix_EffectFunc_t f);


                                                                        
                                                                          
                                                                  
                                                                        
                                                        
                                                                          
                                                                    
                                              
                                                                           
                                                        
   
extern DECLSPEC int SDLCALL Mix_UnregisterAllEffects(int channel);


#define MIX_EFFECTSMAXSPEED  "MIX_EFFECTSMAXSPEED"

  
                                                                              
                                                                      
                                                                            
                                                                         
                                                              
                    
   


                                                                          
                                                                     
  
                                                                        
                                                                        
                                                
  
                                               
  
                          
  
                                                                            
                                                                           
                        
  
                                                                         
                                                                         
                                                                         
                                                                     
  
                                                                         
                                                                        
                                                                       
                                                        
   
extern DECLSPEC int SDLCALL Mix_SetPanning(int channel, Uint8 left, Uint8 right);


                                                                           
                                                                            
                                                                             
                                                                       
                                                                         
                                                                              
                                                                       
                                                                          
                                                                            
                                                                           
                                                                           
                                                                              
                                                                        
              
  
                                                                       
                                                                        
                                       
  
                                                                        
                                                                          
                                                                         
                                                                             
                                                                          
                                                                           
                                             
  
                                                                            
                                                                            
                           
  
                                                                             
  
                                                                         
                                          
                                                        
   
extern DECLSPEC int SDLCALL Mix_SetPosition(int channel, Sint16 angle, Uint8 distance);


                                                                          
                                                                         
                                                                              
                                                                           
                                                                             
                                          
                                                                           
                                                                              
                                                                       
                                                                          
                      
                                                                           
              
                                                                       
                                                                        
                                       
  
                                                                            
                                                                          
                                      
  
                                                     
  
                                                                         
                                          
                                                        
   
extern DECLSPEC int SDLCALL Mix_SetDistance(int channel, Uint8 distance);


  
                                                                  
                                                                       
                        
   
#if 0
                                                                       
                                                                   
                   
  
                                                                            
                                                                             
                        
  
                                                                            
                                                                              
  
                                                                         
                                           
                                                        
   
extern no_parse_DECLSPEC int SDLCALL Mix_SetReverb(int channel, Uint8 echo);
#endif

                                                                            
                                                                          
                                                                          
                                                                            
                               
  
                                                                          
                                                                     
                                                                            
                                                                     
  
                                                                              
                                                                         
                
  
                                                                         
                                                                             
                                                                       
                                                        
   
extern DECLSPEC int SDLCALL Mix_SetReverseStereo(int channel, int flip);

 


                                                                                 
                                                                          
                                           
   
extern DECLSPEC int SDLCALL Mix_ReserveChannels(int num);

 

                                                                    
                                        
                                                                     
                                            
                                     
   
extern DECLSPEC int SDLCALL Mix_GroupChannel(int which, int tag);
 
extern DECLSPEC int SDLCALL Mix_GroupChannels(int from, int to, int tag);
                                                            
                                      
   
extern DECLSPEC int SDLCALL Mix_GroupAvailable(int tag);
                                                                   
                                                           
   
extern DECLSPEC int SDLCALL Mix_GroupCount(int tag);
 
extern DECLSPEC int SDLCALL Mix_GroupOldest(int tag);
 
extern DECLSPEC int SDLCALL Mix_GroupNewer(int tag);

                                             
                                                                  
                                                                   
                                                     
                                                    
  
#define Mix_PlayChannel(channel,chunk,loops) Mix_PlayChannelTimed(channel,chunk,loops,-1)
 
extern DECLSPEC int SDLCALL Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
extern DECLSPEC int SDLCALL Mix_PlayMusic(Mix_Music *music, int loops);
#define Mix_PlayChannelVol(channel,chunk,loops,vol) Mix_PlayChannelTimedVolume(channel,chunk,loops,-1,vol) 
extern DECLSPEC int SDLCALL Mix_PlayChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ticks, int volume); 

 
extern DECLSPEC int SDLCALL Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
extern DECLSPEC int SDLCALL Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position);
#define Mix_FadeInChannel(channel,chunk,loops,ms) Mix_FadeInChannelTimed(channel,chunk,loops,ms,-1)
extern DECLSPEC int SDLCALL Mix_FadeInChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ms, int ticks);
#define Mix_FadeInChannelVolume(channel,chunk,loops,ms,vol) Mix_FadeInChannelTimed(channel,chunk,loops,ms,-1,vol) 
extern DECLSPEC int SDLCALL Mix_FadeInChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ms, int ticks, int volume); 


                                                                       
                                                               
                               
                                                                 
  
extern DECLSPEC int SDLCALL Mix_Volume(int channel, int volume);
extern DECLSPEC int SDLCALL Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
extern DECLSPEC int SDLCALL Mix_VolumeMusic(int volume);

 
extern DECLSPEC int SDLCALL Mix_HaltChannel(int channel);
extern DECLSPEC int SDLCALL Mix_HaltGroup(int tag);
extern DECLSPEC int SDLCALL Mix_HaltMusic(void);

                                                        
                                                                            
                                            
  
extern DECLSPEC int SDLCALL Mix_ExpireChannel(int channel, int ticks);

                                                               
                                                                   
             
   
extern DECLSPEC int SDLCALL Mix_FadeOutChannel(int which, int ms);
extern DECLSPEC int SDLCALL Mix_FadeOutGroup(int tag, int ms);
extern DECLSPEC int SDLCALL Mix_FadeOutMusic(int ms);

 
extern DECLSPEC Mix_Fading SDLCALL Mix_FadingMusic(void);
extern DECLSPEC Mix_Fading SDLCALL Mix_FadingChannel(int which);

 
extern DECLSPEC void SDLCALL Mix_Pause(int channel);
extern DECLSPEC void SDLCALL Mix_Resume(int channel);
extern DECLSPEC int SDLCALL Mix_Paused(int channel);

 
extern DECLSPEC void SDLCALL Mix_PauseMusic(void);
extern DECLSPEC void SDLCALL Mix_ResumeMusic(void);
extern DECLSPEC void SDLCALL Mix_RewindMusic(void);
extern DECLSPEC int SDLCALL Mix_PausedMusic(void);

                                                
                                                                         
                                                                       
                                                                            
                                            
  
extern DECLSPEC int SDLCALL Mix_SetMusicPosition(double position);
  
                                                 
                                                                
   
extern DECLSPEC double SDLCALL Mix_GetMusicPosition(Mix_Music *music);
  
                                             
                                                                
   
extern DECLSPEC double SDLCALL Mix_GetMusicTotalTime(Mix_Music *music);

  
                                                    
                                                                                           
   
extern DECLSPEC double SDLCALL Mix_GetMusicLoopStartTime(Mix_Music *music);
  
                                                  
                                                                                           
   
extern DECLSPEC double SDLCALL Mix_GetMusicLoopEndTime(Mix_Music *music);
  
                                            
                                                                                           
   
extern DECLSPEC double SDLCALL Mix_GetMusicLoopLengthTime(Mix_Music *music);


                                          
                                                      
  
extern DECLSPEC int SDLCALL Mix_Playing(int channel);
extern DECLSPEC int SDLCALL Mix_PlayingMusic(void);

 
extern DECLSPEC int SDLCALL Mix_SetMusicCMD(const char *command);

 
extern DECLSPEC int SDLCALL Mix_SetSynchroValue(int value);
extern DECLSPEC int SDLCALL Mix_GetSynchroValue(void);

 
extern DECLSPEC int SDLCALL Mix_SetSoundFonts(const char *paths);
extern DECLSPEC const char* SDLCALL Mix_GetSoundFonts(void);
extern DECLSPEC int SDLCALL Mix_EachSoundFont(int (SDLCALL *function)(const char*, void*), void *data);

                                                              
                                                                            
  
extern DECLSPEC Mix_Chunk * SDLCALL Mix_GetChunk(int channel);

 
extern DECLSPEC void SDLCALL Mix_CloseAudio(void);

 
extern DECLSPEC void SDLCALL Mix_Timidity_addToPathList(const char *path);

 
 
extern DECLSPEC int    SDLCALL Mix_ADLMIDI_getTotalBanks(void);
 
extern DECLSPEC const char *const *SDLCALL Mix_ADLMIDI_getBankNames(void);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getBankID(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setBankID(int bnk);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getTremolo(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setTremolo(int tr);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getVibrato(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setVibrato(int vib);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getScaleMod(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setScaleMod(int sc);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getAdLibMode(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setAdLibMode(int tr);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getLogarithmicVolumes(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setLogarithmicVolumes(int lv);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getVolumeModel(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setVolumeModel(int vm);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getFullRangeBrightness(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setFullRangeBrightness(int frb);
 
extern DECLSPEC int  SDLCALL Mix_ADLMIDI_getEmulator(void);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setEmulator(int emu);
 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setSetDefaults(void);

 
extern DECLSPEC void SDLCALL Mix_ADLMIDI_setCustomBankFile(const char *bank_wonl_path);

 
extern DECLSPEC void SDLCALL Mix_OPNMIDI_setSetDefaults(void);
 
extern DECLSPEC int  SDLCALL Mix_OPNMIDI_getFullRangeBrightness(void);
 
extern DECLSPEC void SDLCALL Mix_OPNMIDI_setFullRangeBrightness(int frb);
 
extern DECLSPEC int  SDLCALL Mix_OPNMIDI_getEmulator(void);
 
extern DECLSPEC void SDLCALL Mix_OPNMIDI_setEmulator(int emu);
 
extern DECLSPEC void SDLCALL Mix_OPNMIDI_setCustomBankFile(const char *bank_wonp_path);

 
extern DECLSPEC int  SDLCALL Mix_GetMidiPlayer(void);

 
extern DECLSPEC int  SDLCALL Mix_GetNextMidiPlayer(void);

 
extern DECLSPEC int  SDLCALL Mix_SetMidiPlayer(int player);

 
extern DECLSPEC void SDLCALL Mix_SetLockMIDIArgs(int lock_midiargs);

                                                        
                                                                         
                                                                        
                                                                                       
                                                                                                 
  
DEPRECATED(extern DECLSPEC int  SDLCALL Mix_GetMidiDevice(void));
DEPRECATED(extern DECLSPEC int  SDLCALL Mix_GetNextMidiDevice(void));
DEPRECATED(extern DECLSPEC int  SDLCALL Mix_SetMidiDevice(int player));

 
#define Mix_SetError    SDL_SetError
#define Mix_GetError    SDL_GetError
#define Mix_ClearError  SDL_ClearError

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
