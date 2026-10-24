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

   
                      
  
                                        
   

#ifndef SDL_events_h_
#define SDL_events_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"
#include "SDL_quit.h"
#include "SDL_gesture.h"
#include "SDL_touch.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 
#define SDL_RELEASED    0
#define SDL_PRESSED 1

   
                                             
   
typedef enum
{
    SDL_FIRSTEVENT     = 0,      

     
    SDL_QUIT           = 0x100,  

     
    SDL_APP_TERMINATING,                                                          
                                                                                
                                                                     
                                  
    SDL_APP_LOWMEMORY,                                                                         
                                                                                          
                                                                       
                                  
    SDL_APP_WILLENTERBACKGROUND,                                                      
                                                                                   
                                                                   
                                  
    SDL_APP_DIDENTERBACKGROUND,                                                                                
                                                                                     
                                                                   
                                  
    SDL_APP_WILLENTERFOREGROUND,                                                      
                                                                                      
                                                                    
                                  
    SDL_APP_DIDENTERFOREGROUND,                                        
                                                                                  
                                                                    
                                  

    SDL_LOCALECHANGED,   

     
    SDL_DISPLAYEVENT   = 0x150,   

     
    SDL_WINDOWEVENT    = 0x200,  
    SDL_SYSWMEVENT,              

     
    SDL_KEYDOWN        = 0x300,  
    SDL_KEYUP,                   
    SDL_TEXTEDITING,             
    SDL_TEXTINPUT,               
    SDL_KEYMAPCHANGED,                                                              
                                                                              
                                  
    SDL_TEXTEDITING_EXT,        

     
    SDL_MOUSEMOTION    = 0x400,  
    SDL_MOUSEBUTTONDOWN,         
    SDL_MOUSEBUTTONUP,           
    SDL_MOUSEWHEEL,              

     
    SDL_JOYAXISMOTION  = 0x600,  
    SDL_JOYBALLMOTION,           
    SDL_JOYHATMOTION,            
    SDL_JOYBUTTONDOWN,           
    SDL_JOYBUTTONUP,             
    SDL_JOYDEVICEADDED,          
    SDL_JOYDEVICEREMOVED,        
    SDL_JOYBATTERYUPDATED,       

     
    SDL_CONTROLLERAXISMOTION  = 0x650,  
    SDL_CONTROLLERBUTTONDOWN,           
    SDL_CONTROLLERBUTTONUP,             
    SDL_CONTROLLERDEVICEADDED,          
    SDL_CONTROLLERDEVICEREMOVED,        
    SDL_CONTROLLERDEVICEREMAPPED,       
    SDL_CONTROLLERTOUCHPADDOWN,         
    SDL_CONTROLLERTOUCHPADMOTION,       
    SDL_CONTROLLERTOUCHPADUP,           
    SDL_CONTROLLERSENSORUPDATE,         

     
    SDL_FINGERDOWN      = 0x700,
    SDL_FINGERUP,
    SDL_FINGERMOTION,

     
    SDL_DOLLARGESTURE   = 0x800,
    SDL_DOLLARRECORD,
    SDL_MULTIGESTURE,

     
    SDL_CLIPBOARDUPDATE = 0x900,  

     
    SDL_DROPFILE        = 0x1000,  
    SDL_DROPTEXT,                  
    SDL_DROPBEGIN,                 
    SDL_DROPCOMPLETE,              

     
    SDL_AUDIODEVICEADDED = 0x1100,  
    SDL_AUDIODEVICEREMOVED,         

     
    SDL_SENSORUPDATE = 0x1200,      

     
    SDL_RENDER_TARGETS_RESET = 0x2000,  
    SDL_RENDER_DEVICE_RESET,  

     
    SDL_POLLSENTINEL = 0x7F00,  

                                                                        
                                                         
       
    SDL_USEREVENT    = 0x8000,

       
                                                            
       
    SDL_LASTEVENT    = 0xFFFF
} SDL_EventType;

   
                                       
   
typedef struct SDL_CommonEvent
{
    Uint32 type;
    Uint32 timestamp;    
} SDL_CommonEvent;

   
                                                            
   
typedef struct SDL_DisplayEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 display;      
    Uint8 event;         
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint32 data1;        
} SDL_DisplayEvent;

   
                                                          
   
typedef struct SDL_WindowEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 windowID;     
    Uint8 event;         
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint32 data1;        
    Sint32 data2;        
} SDL_WindowEvent;

   
                                                        
   
typedef struct SDL_KeyboardEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 windowID;     
    Uint8 state;         
    Uint8 repeat;        
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;   
} SDL_KeyboardEvent;

#define SDL_TEXTEDITINGEVENT_TEXT_SIZE (32)
   
                                                               
   
typedef struct SDL_TextEditingEvent
{
    Uint32 type;                                 
    Uint32 timestamp;                            
    Uint32 windowID;                             
    char text[SDL_TEXTEDITINGEVENT_TEXT_SIZE];   
    Sint32 start;                                
    Sint32 length;                               
} SDL_TextEditingEvent;

   
                                                                                              
                                                               
   
typedef struct SDL_TextEditingExtEvent
{
    Uint32 type;                                 
    Uint32 timestamp;                            
    Uint32 windowID;                             
    char* text;                                  
    Sint32 start;                                
    Sint32 length;                               
} SDL_TextEditingExtEvent;

#define SDL_TEXTINPUTEVENT_TEXT_SIZE (32)
   
                                                             
   
typedef struct SDL_TextInputEvent
{
    Uint32 type;                               
    Uint32 timestamp;                          
    Uint32 windowID;                           
    char text[SDL_TEXTINPUTEVENT_TEXT_SIZE];   
} SDL_TextInputEvent;

   
                                                        
   
typedef struct SDL_MouseMotionEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 windowID;     
    Uint32 which;        
    Uint32 state;        
    Sint32 x;            
    Sint32 y;            
    Sint32 xrel;         
    Sint32 yrel;         
} SDL_MouseMotionEvent;

   
                                                        
   
typedef struct SDL_MouseButtonEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 windowID;     
    Uint32 which;        
    Uint8 button;        
    Uint8 state;         
    Uint8 clicks;        
    Uint8 padding1;
    Sint32 x;            
    Sint32 y;            
} SDL_MouseButtonEvent;

   
                                                      
   
typedef struct SDL_MouseWheelEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 windowID;     
    Uint32 which;        
    Sint32 x;            
    Sint32 y;            
    Uint32 direction;    
    float preciseX;      
    float preciseY;      
    Sint32 mouseX;       
    Sint32 mouseY;       
} SDL_MouseWheelEvent;

   
                                                               
   
typedef struct SDL_JoyAxisEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Uint8 axis;          
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint16 value;        
    Uint16 padding4;
} SDL_JoyAxisEvent;

   
                                                                    
   
typedef struct SDL_JoyBallEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Uint8 ball;          
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint16 xrel;         
    Sint16 yrel;         
} SDL_JoyBallEvent;

   
                                                                      
   
typedef struct SDL_JoyHatEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Uint8 hat;           
    Uint8 value;                                    
                                                                                
                                                                                  
                                                                                      
                          
                                                                      
                           
    Uint8 padding1;
    Uint8 padding2;
} SDL_JoyHatEvent;

   
                                                            
   
typedef struct SDL_JoyButtonEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Uint8 button;        
    Uint8 state;         
    Uint8 padding1;
    Uint8 padding2;
} SDL_JoyButtonEvent;

   
                                                            
   
typedef struct SDL_JoyDeviceEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Sint32 which;        
} SDL_JoyDeviceEvent;

   
                                                                          
   
typedef struct SDL_JoyBatteryEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    SDL_JoystickPowerLevel level;  
} SDL_JoyBatteryEvent;

   
                                                                      
   
typedef struct SDL_ControllerAxisEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Uint8 axis;          
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint16 value;        
    Uint16 padding4;
} SDL_ControllerAxisEvent;


   
                                                                   
   
typedef struct SDL_ControllerButtonEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Uint8 button;        
    Uint8 state;         
    Uint8 padding1;
    Uint8 padding2;
} SDL_ControllerButtonEvent;


   
                                                              
   
typedef struct SDL_ControllerDeviceEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Sint32 which;        
} SDL_ControllerDeviceEvent;

   
                                                                       
   
typedef struct SDL_ControllerTouchpadEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Sint32 touchpad;     
    Sint32 finger;       
    float x;             
    float y;             
    float pressure;      
} SDL_ControllerTouchpadEvent;

   
                                                                   
   
typedef struct SDL_ControllerSensorEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_JoystickID which;  
    Sint32 sensor;       
    float data[3];       
    Uint64 timestamp_us;  
} SDL_ControllerSensorEvent;

   
                                                         
   
typedef struct SDL_AudioDeviceEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 which;        
    Uint8 iscapture;     
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
} SDL_AudioDeviceEvent;


   
                                                         
   
typedef struct SDL_TouchFingerEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_TouchID touchId;  
    SDL_FingerID fingerId;
    float x;             
    float y;             
    float dx;            
    float dy;            
    float pressure;      
    Uint32 windowID;     
} SDL_TouchFingerEvent;


   
                                                           
   
typedef struct SDL_MultiGestureEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_TouchID touchId;  
    float dTheta;
    float dDist;
    float x;
    float y;
    Uint16 numFingers;
    Uint16 padding;
} SDL_MultiGestureEvent;


   
                                                 
   
typedef struct SDL_DollarGestureEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_TouchID touchId;  
    SDL_GestureID gestureId;
    Uint32 numFingers;
    float error;
    float x;             
    float y;             
} SDL_DollarGestureEvent;


   
                                                                            
                                                                                      
                                                                            
   
typedef struct SDL_DropEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    char *file;          
    Uint32 windowID;     
} SDL_DropEvent;


   
                                                  
   
typedef struct SDL_SensorEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Sint32 which;        
    float data[6];       
    Uint64 timestamp_us;  
} SDL_SensorEvent;

   
                                     
   
typedef struct SDL_QuitEvent
{
    Uint32 type;         
    Uint32 timestamp;    
} SDL_QuitEvent;

   
                            
   
typedef struct SDL_OSEvent
{
    Uint32 type;         
    Uint32 timestamp;    
} SDL_OSEvent;

   
                                                   
   
typedef struct SDL_UserEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    Uint32 windowID;     
    Sint32 code;         
    void *data1;         
    void *data2;         
} SDL_UserEvent;


struct SDL_SysWMmsg;
typedef struct SDL_SysWMmsg SDL_SysWMmsg;

   
                                                                
                                                                                     
  
                                                                        
   
typedef struct SDL_SysWMEvent
{
    Uint32 type;         
    Uint32 timestamp;    
    SDL_SysWMmsg *msg;   
} SDL_SysWMEvent;

   
                                  
   
typedef union SDL_Event
{
    Uint32 type;                             
    SDL_CommonEvent common;                  
    SDL_DisplayEvent display;                
    SDL_WindowEvent window;                  
    SDL_KeyboardEvent key;                   
    SDL_TextEditingEvent edit;               
    SDL_TextEditingExtEvent editExt;         
    SDL_TextInputEvent text;                 
    SDL_MouseMotionEvent motion;             
    SDL_MouseButtonEvent button;             
    SDL_MouseWheelEvent wheel;               
    SDL_JoyAxisEvent jaxis;                  
    SDL_JoyBallEvent jball;                  
    SDL_JoyHatEvent jhat;                    
    SDL_JoyButtonEvent jbutton;              
    SDL_JoyDeviceEvent jdevice;              
    SDL_JoyBatteryEvent jbattery;            
    SDL_ControllerAxisEvent caxis;           
    SDL_ControllerButtonEvent cbutton;       
    SDL_ControllerDeviceEvent cdevice;       
    SDL_ControllerTouchpadEvent ctouchpad;   
    SDL_ControllerSensorEvent csensor;       
    SDL_AudioDeviceEvent adevice;            
    SDL_SensorEvent sensor;                  
    SDL_QuitEvent quit;                      
    SDL_UserEvent user;                      
    SDL_SysWMEvent syswm;                    
    SDL_TouchFingerEvent tfinger;            
    SDL_MultiGestureEvent mgesture;          
    SDL_DollarGestureEvent dgesture;         
    SDL_DropEvent drop;                      

                                                                          
                                                                             
                                                                        
                                                                           
                                                                    
                     

                                                                         

                                                                              
                                                                           
                                                                               
      
    Uint8 padding[sizeof(void *) <= 8 ? 56 : sizeof(void *) == 16 ? 64 : 3 * sizeof(void *)];
} SDL_Event;

 
SDL_COMPILE_TIME_ASSERT(SDL_Event, sizeof(SDL_Event) == sizeof(((SDL_Event *)NULL)->padding));


 

   
                                                                
  
                                                                         
  
                                                                          
                                                                              
                                         
  
                                                                              
                                                                            
                                                                 
                                                                     
                                                                            
                                                                             
                                                        
  
                                                     
  
                    
                    
   
extern DECLSPEC void SDLCALL SDL_PumpEvents(void);

 
typedef enum
{
    SDL_ADDEVENT,
    SDL_PEEKEVENT,
    SDL_GETEVENT
} SDL_eventaction;

   
                                                                 
  
                                        
  
                                                                              
                 
                                                                         
                                                                           
                                                     
                                                                              
                                                                           
                                               
  
                                                                      
                                                                      
                    
  
                                
  
                                                            
                                                                          
                                                                          
                                                                          
                                                                    
                                                                   
                                                 
                                                                   
                                                
                                                                            
                                                              
  
                                                     
  
                    
                     
                    
   
extern DECLSPEC int SDLCALL SDL_PeepEvents(SDL_Event * events, int numevents,
                                           SDL_eventaction action,
                                           Uint32 minType, Uint32 maxType);
 

   
                                                                      
  
                                                                       
           
  
                                                                             
                                                                           
                                                   
  
                                                     
  
                    
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasEvent(Uint32 type);


   
                                                                     
  
                                                                            
  
                                                                         
                                           
                                                                          
                                           
                                                                          
                                         
  
                                                     
  
                    
   
extern DECLSPEC SDL_bool SDLCALL SDL_HasEvents(Uint32 minType, Uint32 maxType);

   
                                                        
  
                                                                        
                                                                              
           
  
                                                                            
                                      
  
                                                                          
                                                                             
                                                        
  
                                                                             
  
                                                     
  
                      
   
extern DECLSPEC void SDLCALL SDL_FlushEvent(Uint32 type);

   
                                                         
  
                                                                             
                                                                             
                                            
  
                                                                            
                                      
  
                                                                          
                                                                             
                                                        
  
                                                                         
                                           
                                                                          
                                           
  
                                                     
  
                     
   
extern DECLSPEC void SDLCALL SDL_FlushEvents(Uint32 minType, Uint32 maxType);

   
                                     
  
                                                                              
                                                                             
                                                                            
             
  
                                                                             
                                         
  
                                                                           
                                                       
  
                                                                             
                                                                              
                            
  
                                                                            
                                                            
  
       
                                  
                       
                                                                             
                                                
        
  
                                                   
    
      
  
                                                                             
                                  
                                                                           
  
                                                     
  
                         
                     
                    
                         
                    
                           
   
extern DECLSPEC int SDLCALL SDL_PollEvent(SDL_Event * event);

   
                                                  
  
                                                                              
                                                    
  
                                                                           
                                                                    
  
                                                                           
                                       
                                                                             
                                                     
  
                                                     
  
                    
                     
                           
   
extern DECLSPEC int SDLCALL SDL_WaitEvent(SDL_Event * event);

   
                                                                            
         
  
                                                                              
                                                    
  
                                                                           
                                                                    
  
                                                                           
                                       
                                                                         
                                 
                                                                             
                                                                            
                                                          
  
                                                     
  
                    
                     
                    
   
extern DECLSPEC int SDLCALL SDL_WaitEventTimeout(SDL_Event * event,
                                                 int timeout);

   
                                   
  
                                                                           
                                                                         
                                                                            
                                                                           
                                                                             
  
                                                                            
                            
  
                                                                             
  
                                                                             
                                                                  
  
                                                                              
                                                                           
                              
  
                                                      
                                                                          
                                                                        
                                                                  
  
                                                     
  
                     
                    
                         
   
extern DECLSPEC int SDLCALL SDL_PushEvent(SDL_Event * event);

   
                                                                    
  
                                                                        
                                   
                                                     
                                                                         
                                                                              
  
                         
                        
   
typedef int (SDLCALL * SDL_EventFilter) (void *userdata, SDL_Event * event);

   
                                                                              
                                          
  
                                                                             
                                                                              
                                                                       
                                                      
  
                                                                            
                                       
  
                                                                     
                                                                       
                                      
  
                                                                            
                                                                           
                                                                             
                                                             
  
                                                                        
                    
  
                                                                             
                               
  
                                                                             
                                                                              
       
  
                                                                          
                                                       
  
                                                     
  
                        
                     
                         
                     
                    
   
extern DECLSPEC void SDLCALL SDL_SetEventFilter(SDL_EventFilter filter,
                                                void *userdata);

   
                                  
  
                                                                              
                                                                        
  
                                                                  
                                                                              
                                 
                                                                             
  
                                                     
  
                         
   
extern DECLSPEC SDL_bool SDLCALL SDL_GetEventFilter(SDL_EventFilter * filter,
                                                    void **userdata);

   
                                                                            
  
                                                                         
           
  
                                                                            
                                       
  
                                                                           
                                                                             
                                 
  
                                                                     
                                                                           
                                                                            
                            
  
                                                                           
                                                       
  
                                                     
  
                        
                         
   
extern DECLSPEC void SDLCALL SDL_AddEventWatch(SDL_EventFilter filter,
                                               void *userdata);

   
                                                                 
  
                                                                            
                                     
  
                                                                      
                                                                       
  
                                                     
  
                        
   
extern DECLSPEC void SDLCALL SDL_DelEventWatch(SDL_EventFilter filter,
                                               void *userdata);

   
                                                                          
                                         
  
                                                                              
                                                                         
                                               
  
                                                                           
                                                       
  
                                                     
  
                         
                         
   
extern DECLSPEC void SDLCALL SDL_FilterEvents(SDL_EventFilter filter,
                                              void *userdata);

 
#define SDL_QUERY   -1
#define SDL_IGNORE   0
#define SDL_DISABLE  0
#define SDL_ENABLE   1

   
                                              
  
                                       
  
                                                                             
                                                                              
                                                  
                                                       
  
                                                               
                                        
                                                                            
                                                                      
  
                                                     
  
                        
   
extern DECLSPEC Uint8 SDLCALL SDL_EventState(Uint32 type, int state);
 
#define SDL_GetEventState(type) SDL_EventState(type, SDL_QUERY)

   
                                                                        
                                 
  
                                                                          
              
  
                                                                       
                                        
  
                                                        
                                                                             
                                     
  
                                                     
  
                    
   
extern DECLSPEC Uint32 SDLCALL SDL_RegisterEvents(int numevents);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
