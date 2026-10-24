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

   
                        
  
                                                
  
                                                                                                                                     
                                                                           
  
                                                                                                                                          
                                                                                                                         
  
                                                                           
                                                                           
                                                                
  
                                                                                                                                    
                                                                                             
   

#ifndef SDL_joystick_h_
#define SDL_joystick_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_guid.h"
#include "SDL_mutex.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                        
  
                                                                     
                                                                          
                                                
  
                                                                       
                                                                          
                                                         
   

   
                                                          
   
#ifdef SDL_THREAD_SAFETY_ANALYSIS
extern SDL_mutex *SDL_joystick_lock;
#endif
struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;

 
typedef SDL_GUID SDL_JoystickGUID;

   
                                                                                 
                                                                              
                                                      
  
                                                                                     
   
typedef Sint32 SDL_JoystickID;

typedef enum
{
    SDL_JOYSTICK_TYPE_UNKNOWN,
    SDL_JOYSTICK_TYPE_GAMECONTROLLER,
    SDL_JOYSTICK_TYPE_WHEEL,
    SDL_JOYSTICK_TYPE_ARCADE_STICK,
    SDL_JOYSTICK_TYPE_FLIGHT_STICK,
    SDL_JOYSTICK_TYPE_DANCE_PAD,
    SDL_JOYSTICK_TYPE_GUITAR,
    SDL_JOYSTICK_TYPE_DRUM_KIT,
    SDL_JOYSTICK_TYPE_ARCADE_PAD,
    SDL_JOYSTICK_TYPE_THROTTLE
} SDL_JoystickType;

typedef enum
{
    SDL_JOYSTICK_POWER_UNKNOWN = -1,
    SDL_JOYSTICK_POWER_EMPTY,    
    SDL_JOYSTICK_POWER_LOW,      
    SDL_JOYSTICK_POWER_MEDIUM,   
    SDL_JOYSTICK_POWER_FULL,     
    SDL_JOYSTICK_POWER_WIRED,
    SDL_JOYSTICK_POWER_MAX
} SDL_JoystickPowerLevel;

                                                
                                                                           
   
#define SDL_IPHONE_MAX_GFORCE 5.0


 

   
                                                        
  
                                                                             
                                                                             
  
                                                                            
                                                                           
                                                    
  
                                                                             
                                                                           
                                                                          
                                        
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_LockJoysticks(void) SDL_ACQUIRE(SDL_joystick_lock);


   
                                                          
  
                                                                             
                                                                             
  
                                                                            
                                                                           
                                                    
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_UnlockJoysticks(void) SDL_RELEASE(SDL_joystick_lock);

   
                                                        
  
                                                                           
                                                                      
  
                                                     
  
                       
                       
                       
   
extern DECLSPEC int SDLCALL SDL_NumJoysticks(void);

   
                                                       
  
                                                      
  
                                                                            
                                     
                                                                            
                                                                            
  
                                                     
  
                       
                       
   
extern DECLSPEC const char *SDLCALL SDL_JoystickNameForIndex(int device_index);

   
                                                       
  
                                                      
  
                                                                            
                                     
                                                                            
                                                                            
  
                                                      
  
                       
                       
   
extern DECLSPEC const char *SDLCALL SDL_JoystickPathForIndex(int device_index);

   
                                                                              
                                          
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_JoystickGetDevicePlayerIndex(int device_index);

   
                                                                           
         
  
                                                               
  
                                                                            
                                    
                                                                             
                                             
  
                                                     
  
                          
                                
   
extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetDeviceGUID(int device_index);

   
                                                     
  
                                                                             
                                     
  
                                                                            
                                    
                                                                       
                                                     
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetDeviceVendor(int device_index);

   
                                                      
  
                                                                              
                                     
  
                                                                            
                                    
                                                                        
                                                     
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetDeviceProduct(int device_index);

   
                                                       
  
                                                                             
                                           
  
                                                                            
                                    
                                                                         
                                                     
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetDeviceProductVersion(int device_index);

   
                                            
  
                                                      
  
                                                                            
                                    
                                                                          
                                                                            
  
                                                     
   
extern DECLSPEC SDL_JoystickType SDLCALL SDL_JoystickGetDeviceType(int device_index);

   
                                     
  
                                                      
  
                                                                            
                                    
                                                                             
                                            
  
                                                     
   
extern DECLSPEC SDL_JoystickID SDLCALL SDL_JoystickGetDeviceInstanceID(int device_index);

   
                           
  
                                                                    
                                                                             
                                                      
                                                                
  
                                                                             
           
  
                                                         
                                                                    
                                                
  
                                                     
  
                        
                             
   
extern DECLSPEC SDL_Joystick *SDLCALL SDL_JoystickOpen(int device_index);

   
                                                       
  
                                                                 
                                                                              
                                 
  
                                                     
   
extern DECLSPEC SDL_Joystick *SDLCALL SDL_JoystickFromInstanceID(SDL_JoystickID instance_id);

   
                                                       
  
                                                                   
                                                                              
                                 
  
                                                      
   
extern DECLSPEC SDL_Joystick *SDLCALL SDL_JoystickFromPlayerIndex(int player_index);

   
                                 
  
                                                                    
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickAttachVirtual(SDL_JoystickType type,
                                                      int naxes,
                                                      int nbuttons,
                                                      int nhats);

   
                                                                      
  
                                                                                                                                                                 
                                                                        
  
                                  
   
typedef struct SDL_VirtualJoystickDesc
{
    Uint16 version;      
    Uint16 type;         
    Uint16 naxes;        
    Uint16 nbuttons;     
    Uint16 nhats;        
    Uint16 vendor_id;    
    Uint16 product_id;   
    Uint16 padding;      
    Uint32 button_mask;                                                           
                                                                   
    Uint32 axis_mask;                                                          
                                                                     
    const char *name;    

    void *userdata;      
    void (SDLCALL *Update)(void *userdata);  
    void (SDLCALL *SetPlayerIndex)(void *userdata, int player_index);  
    int (SDLCALL *Rumble)(void *userdata, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble);  
    int (SDLCALL *RumbleTriggers)(void *userdata, Uint16 left_rumble, Uint16 right_rumble);  
    int (SDLCALL *SetLED)(void *userdata, Uint8 red, Uint8 green, Uint8 blue);  
    int (SDLCALL *SendEffect)(void *userdata, const void *data, int size);  

} SDL_VirtualJoystickDesc;

   
                                                                      
   
#define SDL_VIRTUAL_JOYSTICK_DESC_VERSION   1

   
                                                          
  
                                                                    
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickAttachVirtualEx(const SDL_VirtualJoystickDesc *desc);

   
                             
  
                                                       
                                                  
                                                     
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickDetachVirtual(int device_index);

   
                                                                        
  
                                               
                                                                     
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_JoystickIsVirtual(int device_index);

   
                                                    
  
                                                                              
                                                                            
                                                                           
                                                                      
                 
  
                                                                              
                                                                          
                           
  
                                                              
                                                                
                                                     
                                      
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickSetVirtualAxis(SDL_Joystick *joystick, int axis, Sint16 value);

   
                                                      
  
                                                                              
                                                                            
                                                                           
                                                                      
                 
  
                                                              
                                                                    
                                                       
                                      
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickSetVirtualButton(SDL_Joystick *joystick, int button, Uint8 value);

   
                                                   
  
                                                                              
                                                                            
                                                                           
                                                                      
                 
  
                                                              
                                                              
                                                    
                                      
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickSetVirtualHat(SDL_Joystick *joystick, int hat, Uint8 value);

   
                                                       
  
                                                                    
                                                                            
                                                                            
  
                                                     
  
                               
                       
   
extern DECLSPEC const char *SDLCALL SDL_JoystickName(SDL_Joystick *joystick);

   
                                                       
  
                                                                    
                                                                            
                                                                            
  
                                                      
  
                               
   
extern DECLSPEC const char *SDLCALL SDL_JoystickPath(SDL_Joystick *joystick);

   
                                              
  
                                                                            
                                               
  
                                                                    
                                                          
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_JoystickGetPlayerIndex(SDL_Joystick *joystick);

   
                                              
  
                                                                    
                                                                              
                                                                 
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_JoystickSetPlayerIndex(SDL_Joystick *joystick, int player_index);

   
                                                          
  
                                           
  
                                                                    
                                                                          
                                                                           
                        
  
                                                     
  
                                
                                
   
extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetGUID(SDL_Joystick *joystick);

   
                                                             
  
                                                            
  
                                                                    
                                                                            
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetVendor(SDL_Joystick *joystick);

   
                                                              
  
                                                             
  
                                                                    
                                                                             
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetProduct(SDL_Joystick *joystick);

   
                                                               
  
                                                                  
  
                                                                    
                                                                              
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetProductVersion(SDL_Joystick *joystick);

   
                                                                
  
                                                                   
  
                                                                    
                                                                  
                        
  
                                                      
   
extern DECLSPEC Uint16 SDLCALL SDL_JoystickGetFirmwareVersion(SDL_Joystick *joystick);

   
                                                             
  
                                                                             
  
                                                                    
                                                                  
                        
  
                                                      
   
extern DECLSPEC const char * SDLCALL SDL_JoystickGetSerial(SDL_Joystick *joystick);

   
                                      
  
                                                                    
                                                          
  
                                                     
   
extern DECLSPEC SDL_JoystickType SDLCALL SDL_JoystickGetType(SDL_Joystick *joystick);

   
                                                                   
  
                                                   
  
                                                                 
                                                           
                                    
  
                                                     
  
                                
                          
                                    
   
extern DECLSPEC void SDLCALL SDL_JoystickGetGUIDString(SDL_JoystickGUID guid, char *pszGUID, int cbGUID);

   
                                                           
  
                                                                            
                                                                              
                      
  
                                                                     
                                         
  
                                                     
  
                                
   
extern DECLSPEC SDL_JoystickGUID SDLCALL SDL_JoystickGetGUIDFromString(const char *pchGUID);

   
                                                                     
  
                                                              
                                                                     
                          
                                                                      
                           
                                                                          
                           
                                                                            
                                                                     
  
                                                      
  
                                
   
extern DECLSPEC void SDLCALL SDL_GetJoystickGUIDInfo(SDL_JoystickGUID guid, Uint16 *vendor, Uint16 *product, Uint16 *version, Uint16 *crc16);

   
                                          
  
                                        
                                                                              
                                                     
  
                                                     
  
                        
                       
   
extern DECLSPEC SDL_bool SDLCALL SDL_JoystickGetAttached(SDL_Joystick *joystick);

   
                                             
  
                                                                            
                                                                              
                                                                            
  
                                                     
  
                       
   
extern DECLSPEC SDL_JoystickID SDLCALL SDL_JoystickInstanceID(SDL_Joystick *joystick);

   
                                                         
  
                                                                          
                                                                            
                       
  
                                                                            
                                                                      
                                                                        
                        
  
                                                     
  
                          
                       
   
extern DECLSPEC int SDLCALL SDL_JoystickNumAxes(SDL_Joystick *joystick);

   
                                              
  
                                                                            
                                    
  
                                         
  
                                                                            
                                                                           
                                                              
  
                                                     
  
                          
   
extern DECLSPEC int SDLCALL SDL_JoystickNumBalls(SDL_Joystick *joystick);

   
                                            
  
                                                                            
                                                                         
                                                              
  
                                                     
  
                         
                       
   
extern DECLSPEC int SDLCALL SDL_JoystickNumHats(SDL_Joystick *joystick);

   
                                           
  
                                                                            
                                                                        
                                                              
  
                                                     
  
                            
                       
   
extern DECLSPEC int SDLCALL SDL_JoystickNumButtons(SDL_Joystick *joystick);

   
                                                  
  
                                                                            
           
  
                                                     
  
                             
   
extern DECLSPEC void SDLCALL SDL_JoystickUpdate(void);

   
                                         
  
                                                                      
                                                                      
                        
  
                                                                    
  
                                                                              
               
  
                                                                        
                                                                             
                                                     
  
                                                                         
                                                           
  
                                                     
  
                                   
   
extern DECLSPEC int SDLCALL SDL_JoystickEventState(int state);

#define SDL_JOYSTICK_AXIS_MAX   32767
#define SDL_JOYSTICK_AXIS_MIN   -32768

   
                                                          
  
                                                                              
                                                                       
                                                                             
                                                                              
                                                                              
  
                                                                             
                                                                            
                                                                      
  
                                                                            
                                                                   
                                                                            
                                                                           
  
                                                     
  
                          
   
extern DECLSPEC Sint16 SDLCALL SDL_JoystickGetAxis(SDL_Joystick *joystick,
                                                   int axis);

   
                                                          
  
                                                     
  
                                     
  
                                                                            
                                                                   
                                                                
                                                                            
  
                                                     
   
extern DECLSPEC SDL_bool SDLCALL SDL_JoystickGetAxisInitialState(SDL_Joystick *joystick,
                                                   int axis, Sint16 *state);

   
                       
   
 
#define SDL_HAT_CENTERED    0x00
#define SDL_HAT_UP          0x01
#define SDL_HAT_RIGHT       0x02
#define SDL_HAT_DOWN        0x04
#define SDL_HAT_LEFT        0x08
#define SDL_HAT_RIGHTUP     (SDL_HAT_RIGHT|SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN   (SDL_HAT_RIGHT|SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP      (SDL_HAT_LEFT|SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN    (SDL_HAT_LEFT|SDL_HAT_DOWN)
 

   
                                                    
  
                                                             
  
                       
                 
                    
                   
                   
                      
                        
                     
                       
  
                                                                            
                                                                           
                                     
  
                                                     
  
                          
   
extern DECLSPEC Uint8 SDLCALL SDL_JoystickGetHat(SDL_Joystick *joystick,
                                                 int hat);

   
                                                
  
                                                                    
                                                                            
  
                                         
  
                                            
                                                                     
                                                                             
                                                                             
                                                                  
                                                
  
                                                     
  
                           
   
extern DECLSPEC int SDLCALL SDL_JoystickGetBall(SDL_Joystick *joystick,
                                                int ball, int *dx, int *dy);

   
                                                   
  
                                                                            
                                                                         
                        
                                                              
  
                                                     
  
                             
   
extern DECLSPEC Uint8 SDLCALL SDL_JoystickGetButton(SDL_Joystick *joystick,
                                                    int button);

   
                         
  
                                                                             
                                          
  
                                          
                                                                        
                                                             
                                                                           
                                                              
                                                                        
                                                               
  
                                                     
  
                            
   
extern DECLSPEC int SDLCALL SDL_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms);

   
                                                   
  
                                                                             
                                                  
  
                                                                              
                                                                            
                                                                           
           
  
                                          
                                                                            
                               
                                                                              
                                
                                                                        
                                                                       
  
                                                      
  
                                    
   
extern DECLSPEC int SDLCALL SDL_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble, Uint32 duration_ms);

   
                                       
  
                                                                             
                          
  
                                        
                                                                              
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_JoystickHasLED(SDL_Joystick *joystick);

   
                                               
  
                                        
                                                                    
  
                                                      
  
                         
   
extern DECLSPEC SDL_bool SDLCALL SDL_JoystickHasRumble(SDL_Joystick *joystick);

   
                                                           
  
                                        
                                                                            
  
                                                      
  
                                 
   
extern DECLSPEC SDL_bool SDLCALL SDL_JoystickHasRumbleTriggers(SDL_Joystick *joystick);

   
                                 
  
                                                                             
                          
  
                                         
                                          
                                              
                                            
                                                                            
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue);

   
                                         
  
                                         
                                               
                                                           
                                                                              
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size);

   
                                                              
  
                                               
  
                                                     
  
                       
   
extern DECLSPEC void SDLCALL SDL_JoystickClose(SDL_Joystick *joystick);

   
                                                                 
  
                                            
                                                                             
                                                         
  
                                                     
   
extern DECLSPEC SDL_JoystickPowerLevel SDLCALL SDL_JoystickCurrentPowerLevel(SDL_Joystick *joystick);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
