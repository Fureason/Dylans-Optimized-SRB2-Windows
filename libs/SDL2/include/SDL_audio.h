/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

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

 

   
                     
  
                                                              
   

#ifndef SDL_audio_h_
#define SDL_audio_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_endian.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"
#include "SDL_rwops.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                              
  
                                                                   
                                       
  
             
                                                    
      
                                                    
               
                                                    
                           
                                                   
                                                   
                                                   
                
  
                                                              
   
typedef Uint16 SDL_AudioFormat;

   
                     
   
 

#define SDL_AUDIO_MASK_BITSIZE       (0xFF)
#define SDL_AUDIO_MASK_DATATYPE      (1<<8)
#define SDL_AUDIO_MASK_ENDIAN        (1<<12)
#define SDL_AUDIO_MASK_SIGNED        (1<<15)
#define SDL_AUDIO_BITSIZE(x)         (x & SDL_AUDIO_MASK_BITSIZE)
#define SDL_AUDIO_ISFLOAT(x)         (x & SDL_AUDIO_MASK_DATATYPE)
#define SDL_AUDIO_ISBIGENDIAN(x)     (x & SDL_AUDIO_MASK_ENDIAN)
#define SDL_AUDIO_ISSIGNED(x)        (x & SDL_AUDIO_MASK_SIGNED)
#define SDL_AUDIO_ISINT(x)           (!SDL_AUDIO_ISFLOAT(x))
#define SDL_AUDIO_ISLITTLEENDIAN(x)  (!SDL_AUDIO_ISBIGENDIAN(x))
#define SDL_AUDIO_ISUNSIGNED(x)      (!SDL_AUDIO_ISSIGNED(x))

   
                            
  
                               
   
 
#define AUDIO_U8        0x0008   
#define AUDIO_S8        0x8008   
#define AUDIO_U16LSB    0x0010   
#define AUDIO_S16LSB    0x8010   
#define AUDIO_U16MSB    0x1010   
#define AUDIO_S16MSB    0x9010   
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB
 

   
                       
   
 
#define AUDIO_S32LSB    0x8020   
#define AUDIO_S32MSB    0x9020   
#define AUDIO_S32       AUDIO_S32LSB
 

   
                         
   
 
#define AUDIO_F32LSB    0x8120   
#define AUDIO_F32MSB    0x9120   
#define AUDIO_F32       AUDIO_F32LSB
 

   
                                    
   
 
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define AUDIO_U16SYS    AUDIO_U16LSB
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_S32SYS    AUDIO_S32LSB
#define AUDIO_F32SYS    AUDIO_F32LSB
#else
#define AUDIO_U16SYS    AUDIO_U16MSB
#define AUDIO_S16SYS    AUDIO_S16MSB
#define AUDIO_S32SYS    AUDIO_S32MSB
#define AUDIO_F32SYS    AUDIO_F32MSB
#endif
 

   
                            
  
                                                                 
   
 
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE    0x00000001
#define SDL_AUDIO_ALLOW_FORMAT_CHANGE       0x00000002
#define SDL_AUDIO_ALLOW_CHANNELS_CHANGE     0x00000004
#define SDL_AUDIO_ALLOW_SAMPLES_CHANGE      0x00000008
#define SDL_AUDIO_ALLOW_ANY_CHANGE          (SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_FORMAT_CHANGE|SDL_AUDIO_ALLOW_CHANNELS_CHANGE|SDL_AUDIO_ALLOW_SAMPLES_CHANGE)
 

  

   
                                                                  
  
                                                              
                                               
                                                     
                                                     
  
                                                                  
                                                   
  
                                                                          
                                                               
   
typedef void (SDLCALL * SDL_AudioCallback) (void *userdata, Uint8 * stream,
                                            int len);

   
                                                                              
  
                                                                
                                                
                                                      
                                              
                                                      
                                                                                   
                                                      
                                                      
   
typedef struct SDL_AudioSpec
{
    int freq;                    
    SDL_AudioFormat format;      
    Uint8 channels;              
    Uint8 silence;               
    Uint16 samples;              
    Uint16 padding;              
    Uint32 size;                 
    SDL_AudioCallback callback;  
    void *userdata;              
} SDL_AudioSpec;


struct SDL_AudioCVT;
typedef void (SDLCALL * SDL_AudioFilter) (struct SDL_AudioCVT * cvt,
                                          SDL_AudioFormat format);

   
                                                 
  
                                                                      
                                                                           
                                                 
   
#define SDL_AUDIOCVT_MAX_FILTERS 9

   
                        
                                                                             
  
                                                                         
                                                                             
                                                                        
                                                                              
                                                                  
   
#if defined(__GNUC__) && !defined(__CHERI_PURE_CAPABILITY__)
                                                                            
                                                                           
                                                                              
                                                                            
                                                                          
      
                                                                            
  
#define SDL_AUDIOCVT_PACKED __attribute__((packed))
#else
#define SDL_AUDIOCVT_PACKED
#endif
 
typedef struct SDL_AudioCVT
{
    int needed;                  
    SDL_AudioFormat src_format;  
    SDL_AudioFormat dst_format;  
    double rate_incr;            
    Uint8 *buf;                  
    int len;                     
    int len_cvt;                 
    int len_mult;                
    double len_ratio;            
    SDL_AudioFilter filters[SDL_AUDIOCVT_MAX_FILTERS + 1];  
    int filter_index;            
} SDL_AUDIOCVT_PACKED SDL_AudioCVT;


 

   
                                    
  
                                                                     
                                                        
   
 

   
                                                                 
  
                                                                          
                                                                       
                                                                             
                                                                          
                                                                           
                                                                              
  
                                                                          
                      
  
                                                 
  
                                                     
  
                         
   
extern DECLSPEC int SDLCALL SDL_GetNumAudioDrivers(void);

   
                                                                
  
                                                                         
                                                                          
                                                                        
  
                                                                           
                                                                             
                            
  
                                                                         
                                            
                                                                              
                                        
  
                                                     
  
                             
   
extern DECLSPEC const char *SDLCALL SDL_GetAudioDriver(int index);
 

   
                                    
  
                                                                                
                                                                              
                                                                              
   
 

   
                                                             
  
                                                                             
                                                                          
                                                  
  
                                                          
                                                                  
                                                
  
                                                     
  
                    
   
extern DECLSPEC int SDLCALL SDL_AudioInit(const char *driver_name);

   
                                                                  
                   
  
                                                                             
                                                                        
                                                  
  
                                                     
  
                    
   
extern DECLSPEC void SDLCALL SDL_AudioQuit(void);
 

   
                                            
  
                                                                              
                                                                            
                                                                            
                                                                            
                       
  
                                                                              
                        
  
                                                     
  
                    
   
extern DECLSPEC const char *SDLCALL SDL_GetCurrentAudioDriver(void);

   
                                                               
  
                                                                              
                                                                          
                                                                   
  
                                          
  
       
                                                                               
      
  
                               
  
                                                                          
                                                                           
                                                                          
                                                
                                                                              
                                                                          
  
                                                                            
                                                                 
                                                                             
                                                                       
                                       
                                                                          
                                               
  
                                                                        
                                                                          
                                                                        
                                                                           
                            
  
                                                                              
                                                                    
                                                
  
                                                     
  
                     
                    
                     
                      
   
extern DECLSPEC int SDLCALL SDL_OpenAudio(SDL_AudioSpec * desired,
                                          SDL_AudioSpec * obtained);

   
                         
  
                                                                          
                                                                              
                                                                          
                                                                       
                                 
   
typedef Uint32 SDL_AudioDeviceID;

   
                                            
  
                                                                        
             
  
                                                                             
                                                                              
       
  
                                                                       
                                                                             
                                                                          
                                                                       
                         
  
                                                                              
                                                                   
                          
  
                                                                          
                                                                            
                   
  
       
                    
                                                      
  
                      
                                               
                                                           
      
  
                                                                         
                                     
                                                                            
                                                                           
                                                                     
  
                                                     
  
                             
                          
   
extern DECLSPEC int SDLCALL SDL_GetNumAudioDevices(int iscapture);

   
                                                          
  
                                                                        
                                                                             
                                                                        
            
  
                                                                        
                                                                             
                                                                             
                                                                   
  
                                                                           
                                            
                                                                            
                                                     
                                                                           
                  
  
                                                     
  
                             
                              
   
extern DECLSPEC const char *SDLCALL SDL_GetAudioDeviceName(int index,
                                                           int iscapture);

   
                                                             
  
                                                                          
                                                                             
                                                                        
            
  
                                                                         
         
  
                                                                           
                                            
                                                                            
                                                     
                                                                    
                                          
  
                                                      
  
                             
                              
   
extern DECLSPEC int SDLCALL SDL_GetAudioDeviceSpec(int index,
                                                   int iscapture,
                                                   SDL_AudioSpec *spec);


   
                                                                 
  
                                                                              
                                                                          
                                                                              
                                                                           
  
                                                                             
                                                                              
                                       
  
                                                                         
                                                                          
                                                                           
  
                                                                              
                                                                           
                       
                                                                    
                                                                           
                                                    
                                          
  
                                                      
  
                             
                             
                          
   
extern DECLSPEC int SDLCALL SDL_GetDefaultAudioInfo(char **name,
                                                    SDL_AudioSpec *spec,
                                                    int iscapture);


   
                                
  
                                                                              
                                                                            
            
  
                                                                            
                                                                          
                                                                  
  
                                                                          
                                                                           
                                                                            
                                                                      
                                                                      
                    
  
                                                                              
                                                                              
                                                                        
                                                                           
                                           
  
                                                                              
                                                                        
                                                                              
                               
  
                                                        
  
                                                    
  
                                                                              
                                                                        
                                                                           
                                                                           
                                                                           
                                                                            
                                                                           
                                                                          
                                                                              
                                                                            
                                                             
                              
                                                                       
                                                                    
                                                                              
                                                                    
                                                                             
                                                                           
                                                                             
                                                                      
                                                                     
                                                                           
                                                                          
                                                                  
                                                                      
                                                                          
                                                                    
  
                                                                
  
                                       
                                    
                                      
                                     
                                 
  
                                                                         
                                                                            
                                                                    
  
                                                                            
                                                                     
                                                                              
                                                                          
                                                                          
                                                                         
  
                                                                       
                                                                 
  
                                                                            
                                                                          
  
                                                                         
                                                                            
                                           
                                                                     
                                           
                                                                            
                                                                  
                                                                              
                                                                   
                                                               
                                                                          
                                                
  
                                                                           
                                                                         
  
                                                     
  
                           
                             
                          
                    
                           
                            
   
extern DECLSPEC SDL_AudioDeviceID SDLCALL SDL_OpenAudioDevice(
                                                  const char *device,
                                                  int iscapture,
                                                  const SDL_AudioSpec *desired,
                                                  SDL_AudioSpec *obtained,
                                                  int allowed_changes);



   
                     
  
                                
   
 
typedef enum
{
    SDL_AUDIO_STOPPED = 0,
    SDL_AUDIO_PLAYING,
    SDL_AUDIO_PAUSED
} SDL_AudioStatus;

   
                                                                
  
                                                                          
                                       
  
       
                               
      
  
                                                                         
  
                                                                              
  
                                                     
  
                               
   
extern DECLSPEC SDL_AudioStatus SDLCALL SDL_GetAudioStatus(void);

   
                                                                       
  
                                                              
                                   
                                                              
  
                                                     
  
                           
   
extern DECLSPEC SDL_AudioStatus SDLCALL SDL_GetAudioDeviceStatus(SDL_AudioDeviceID dev);
  

   
                               
  
                                                                    
                                                                       
                                                                        
                                                                   
                                                                 
   
 

   
                                                               
  
                                                                      
                                       
  
       
                                     
      
  
                                                                         
  
                                                  
  
                                                     
  
                         
                           
   
extern DECLSPEC void SDLCALL SDL_PauseAudio(int pause_on);

   
                                                                       
          
  
                                                                              
                                                                            
                                                                           
                                                                           
                                                                             
                                                                      
                                                                         
                                                 
  
                                                                            
                                                                              
                                                  
  
                                                                           
                                                                              
                                                                        
  
                                                      
                                                  
  
                                                     
  
                          
   
extern DECLSPEC void SDLCALL SDL_PauseAudioDevice(SDL_AudioDeviceID dev,
                                                  int pause_on);
  

   
                                                  
  
                                                                             
                                                                             
                                   
  
                                                                          
                                     
  
                                                                             
                                                                              
                                                                         
                  
  
                                                                           
                                                                            
                                                                        
                                                                              
                                                                            
                          
  
                                                                         
                                         
  
                                                                       
                                                                           
                                                                             
                                                                          
                                                    
                                                                            
                                            
  
                                                                              
                                                                           
                                                                             
                                                                           
                                                                           
       
  
                                                        
  
           
  
       
                                                                            
      
  
                                                                              
             
  
       
                                                
      
  
                                               
                                                                     
                                                                           
                             
                                                                          
                             
                                                                             
                            
                                                                             
                                                                         
                                                                            
                                                                         
                                                 
  
                                                                              
                                                                          
                             
  
                                                                  
                                                                       
  
                                                     
  
                  
                  
   
extern DECLSPEC SDL_AudioSpec *SDLCALL SDL_LoadWAV_RW(SDL_RWops * src,
                                                      int freesrc,
                                                      SDL_AudioSpec * spec,
                                                      Uint8 ** audio_buf,
                                                      Uint32 * audio_len);

   
                            
                                       
   
#define SDL_LoadWAV(file, spec, audio_buf, audio_len) \
    SDL_LoadWAV_RW(SDL_RWFromFile(file, "rb"),1, spec,audio_buf,audio_len)

   
                                                                         
  
                                                                           
                                                                          
                                     
  
                                                                       
                                    
  
                                                     
  
                  
                     
   
extern DECLSPEC void SDLCALL SDL_FreeWAV(Uint8 * audio_buf);

   
                                                       
  
                                                                             
                                                          
  
                                                                             
                                                                       
  
                                                                    
                                                                              
                                                                           
                                                                   
  
                                                                       
                         
                                                                           
                                    
                                                           
                                                                         
                                                                            
                                        
                                                                
                                                                              
                                                                            
                                                                             
                        
  
                                                     
  
                       
   
extern DECLSPEC int SDLCALL SDL_BuildAudioCVT(SDL_AudioCVT * cvt,
                                              SDL_AudioFormat src_format,
                                              Uint8 src_channels,
                                              int src_rate,
                                              SDL_AudioFormat dst_format,
                                              Uint8 dst_channels,
                                              int dst_rate);

   
                                                
  
                                                                             
                                                                           
                                     
  
                                                                 
                                                                          
                                                                              
                         
  
                                                                        
                                                                             
                                                                        
                                                                            
                                                                          
                                                                           
                                                                         
                                                                           
                                                                       
                                                 
  
                                                                       
                                                                           
                                                                              
                                                                           
                         
  
                                                                     
                                  
                                                                              
                                                                      
  
                                                     
  
                        
   
extern DECLSPEC int SDLCALL SDL_ConvertAudio(SDL_AudioCVT * cvt);

                                                       
                                
                                                                
                                                                    
                                                       
                                                                
   
 
struct _SDL_AudioStream;
typedef struct _SDL_AudioStream SDL_AudioStream;

   
                             
  
                                                   
                                                                 
                                                        
                                                           
                                                                         
                                                                
                                         
  
                                                     
  
                         
                         
                               
                           
                           
                          
   
extern DECLSPEC SDL_AudioStream * SDLCALL SDL_NewAudioStream(const SDL_AudioFormat src_format,
                                           const Uint8 src_channels,
                                           const int src_rate,
                                           const SDL_AudioFormat dst_format,
                                           const Uint8 dst_channels,
                                           const int dst_rate);

   
                                                    
  
                                                            
                                                
                                                        
                                         
  
                                                     
  
                         
                         
                               
                           
                           
                          
   
extern DECLSPEC int SDLCALL SDL_AudioStreamPut(SDL_AudioStream *stream, const void *buf, int len);

   
                                               
  
                                                             
                                              
                                                 
                                                                    
  
                                                     
  
                         
                         
                               
                           
                           
                          
   
extern DECLSPEC int SDLCALL SDL_AudioStreamGet(SDL_AudioStream *stream, void *buf, int len);

   
                                                         
  
                                                                            
                                                                             
                                                                            
  
                                                     
  
                         
                         
                         
                           
                           
                          
   
extern DECLSPEC int SDLCALL SDL_AudioStreamAvailable(SDL_AudioStream *stream);

   
                                                                             
                                                                
  
                                                                             
                                                                            
                                                   
  
                                                     
  
                         
                         
                         
                               
                           
                          
   
extern DECLSPEC int SDLCALL SDL_AudioStreamFlush(SDL_AudioStream *stream);

   
                                                             
  
                                                     
  
                         
                         
                         
                               
                           
                          
   
extern DECLSPEC void SDLCALL SDL_AudioStreamClear(SDL_AudioStream *stream);

   
                       
  
                                                     
  
                         
                         
                         
                               
                           
                           
   
extern DECLSPEC void SDLCALL SDL_FreeAudioStream(SDL_AudioStream *stream);

#define SDL_MIX_MAXVOLUME 128

   
                                                   
  
                                            
  
       
                                                     
      
  
                                                                        
                                   
  
                                                 
                                                 
                                                     
                                                                            
                                      
  
                                                     
  
                         
   
extern DECLSPEC void SDLCALL SDL_MixAudio(Uint8 * dst, const Uint8 * src,
                                          Uint32 len, int volume);

   
                                        
  
                                                                             
                                                                      
                                                                       
                 
  
                                                                       
  
                                                                        
                                                                            
                                                                            
                                                                             
  
                                                                             
                                                                        
                                                                         
                                         
  
                                                 
                                                 
                                                                             
                       
                                                     
                                                                            
                                      
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_MixAudioFormat(Uint8 * dst,
                                                const Uint8 * src,
                                                SDL_AudioFormat format,
                                                Uint32 len, int volume);

   
                                            
  
                                                                          
                                                                            
                                                             
  
                                                                           
                                                                            
                                                                          
                                                                     
  
                                                                    
                                                                       
                                                                             
                                                                      
                                                                           
                                             
  
                                                                              
                                                                           
                                                                        
                
  
                                                                            
                                                                             
                                                
  
                                                                         
                                                
  
                                                                          
                                                                        
                        
  
                                                        
                                                                 
                                                                       
                                                                  
                                                
  
                                                     
  
                           
                             
   
extern DECLSPEC int SDLCALL SDL_QueueAudio(SDL_AudioDeviceID dev, const void *data, Uint32 len);

   
                                              
  
                                                                          
                                                                            
                                                
  
                                                                              
                                                                        
                                                                             
                                                                       
                              
  
                                                                    
                                                                         
                                                                       
                                                                        
  
                                                                           
                                                                              
                                                                         
                                                                              
                                                                              
                
  
                                                                             
                                                                         
  
                                                           
                                                                            
                                                                         
  
                                                                           
                                                
  
                                                            
                                                               
                                                                       
                                                                             
                                                     
  
                                                     
  
                           
                             
   
extern DECLSPEC Uint32 SDLCALL SDL_DequeueAudio(SDL_AudioDeviceID dev, void *data, Uint32 len);

   
                                                 
  
                                                                              
                                                                              
  
                                                                             
                                                                             
                                                                           
                                                         
  
                                                                              
                                                                             
                                                                   
  
                                                                  
                                                                        
                                                                           
            
  
                                                                         
                                                
  
                                                                    
                                                               
  
                                                     
  
                           
                     
                       
   
extern DECLSPEC Uint32 SDLCALL SDL_GetQueuedAudioSize(SDL_AudioDeviceID dev);

   
                                                                 
  
                                                                           
                                                                              
                                                                         
                                                          
  
                                                                             
                                                                             
                                                                            
                                                                           
                                      
  
                                                                  
                                                                        
                                                                           
            
  
                                                                        
                                                           
  
                                                       
  
                                                             
  
                                                     
  
                             
                     
                       
   
extern DECLSPEC void SDLCALL SDL_ClearQueuedAudio(SDL_AudioDeviceID dev);


   
                              
  
                                                                           
                                                                               
                                                                              
                                        
   
 

   
                                                               
  
                                                                              
                              
  
       
                          
      
  
                                                                         
  
                                                     
  
                          
                      
                            
   
extern DECLSPEC void SDLCALL SDL_LockAudio(void);

   
                                                                            
          
  
                                                                      
                                                        
                                                                            
                                                                         
                                                                      
                                                  
  
                                                                            
                                                                           
                    
  
                                                                             
                                                                           
                                                                             
                                                            
  
                                                                            
                                                                        
                                                                             
                                                                              
                                                                           
            
  
                                                                             
                                                                              
          
  
                                               
  
                                                     
  
                            
   
extern DECLSPEC void SDLCALL SDL_LockAudioDevice(SDL_AudioDeviceID dev);

   
                                                                 
  
                                                                       
                                       
  
       
                            
      
  
                                                                         
  
                                                     
  
                    
                            
   
extern DECLSPEC void SDLCALL SDL_UnlockAudio(void);

   
                                                                          
          
  
                                                                             
  
                                                 
  
                                                     
  
                          
   
extern DECLSPEC void SDLCALL SDL_UnlockAudioDevice(SDL_AudioDeviceID dev);
  

   
                                                               
  
                                            
  
       
                           
      
  
                                                                         
  
                                                     
  
                    
   
extern DECLSPEC void SDLCALL SDL_CloseAudio(void);

   
                                                                              
  
                                                                          
                                                                            
                                                                        
                                                                        
           
  
                                                                            
                                                                         
            
  
                                                                            
                                                             
  
                                                                          
  
                                                     
  
                          
   
extern DECLSPEC void SDLCALL SDL_CloseAudioDevice(SDL_AudioDeviceID dev);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
