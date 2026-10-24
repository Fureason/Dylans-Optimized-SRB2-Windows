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

   
                              
  
                                                       
   

#ifndef SDL_gamecontroller_h_
#define SDL_gamecontroller_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_rwops.h"
#include "SDL_sensor.h"
#include "SDL_joystick.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                              
  
                                                                     
                                                                                
                                                       
  
                                                                         
                                                                          
                                                         
   

   
                                                                       
   
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

typedef enum
{
    SDL_CONTROLLER_TYPE_UNKNOWN = 0,
    SDL_CONTROLLER_TYPE_XBOX360,
    SDL_CONTROLLER_TYPE_XBOXONE,
    SDL_CONTROLLER_TYPE_PS3,
    SDL_CONTROLLER_TYPE_PS4,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO,
    SDL_CONTROLLER_TYPE_VIRTUAL,
    SDL_CONTROLLER_TYPE_PS5,
    SDL_CONTROLLER_TYPE_AMAZON_LUNA,
    SDL_CONTROLLER_TYPE_GOOGLE_STADIA,
    SDL_CONTROLLER_TYPE_NVIDIA_SHIELD,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR
} SDL_GameControllerType;

typedef enum
{
    SDL_CONTROLLER_BINDTYPE_NONE = 0,
    SDL_CONTROLLER_BINDTYPE_BUTTON,
    SDL_CONTROLLER_BINDTYPE_AXIS,
    SDL_CONTROLLER_BINDTYPE_HAT
} SDL_GameControllerBindType;

   
                                                                              
   
typedef struct SDL_GameControllerButtonBind
{
    SDL_GameControllerBindType bindType;
    union
    {
        int button;
        int axis;
        struct {
            int hat;
            int hat_mask;
        } hat;
    } value;

} SDL_GameControllerButtonBind;


   
                                                                            
  
        
                                        
                             
                                          
                                      
                               
         
     
       
  
                                                                                                                                                                                                                        
                      
  
                                                                                                                                                                            
                                                                                      
                                       
                                       
                                 
                                   
                                                            
  
                                                                    
  
       
                                                                                                                                                                                                                                                                                      
      
   

   
                                                                          
  
                                                                         
                  
  
                                                                             
                                                   
  
                                                                           
                                                                        
                 
  
                                                                      
                                                                        
                           
  
                                                         
                                                              
                                                                            
                                 
  
                                                     
  
                                   
                                            
                                       
   
extern DECLSPEC int SDLCALL SDL_GameControllerAddMappingsFromRW(SDL_RWops * rw, int freerw);

   
                                                                                 
  
                      
   
#define SDL_GameControllerAddMappingsFromFile(file)   SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile(file, "rb"), 1)

   
                                                                             
                                          
  
                                                                           
                                                                            
                                                                         
                                                                           
                                                                         
                                                                              
                                                              
  
                                                                    
  
       
                                                                                                                                                                                                                                                                                               
      
  
                                          
                                                                             
                                                                  
  
                                                     
  
                                
                                       
   
extern DECLSPEC int SDLCALL SDL_GameControllerAddMapping(const char* mappingString);

   
                                        
  
                                   
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_GameControllerNumMappings(void);

   
                                         
  
                                                                              
                                      
  
                                                     
   
extern DECLSPEC char * SDLCALL SDL_GameControllerMappingForIndex(int mapping_index);

   
                                                           
  
                                                     
  
                                                                             
                                                                           
                        
  
                                                     
  
                                
                          
   
extern DECLSPEC char * SDLCALL SDL_GameControllerMappingForGUID(SDL_JoystickGUID guid);

   
                                                
  
                                                     
  
                                                                            
  
                                                                        
                                    
                                                                            
                                                                   
  
                                                     
  
                                   
                                       
   
extern DECLSPEC char * SDLCALL SDL_GameControllerMapping(SDL_GameController *gamecontroller);

   
                                                                             
  
                                                               
                      
  
                                                            
                                           
                                                                              
                                                                      
  
                                                     
  
                                     
                             
   
extern DECLSPEC SDL_bool SDLCALL SDL_IsGameController(int joystick_index);

   
                                                                 
  
                                                                 
  
                                                               
                      
  
                                                                   
                                             
                                                                              
                                                        
  
                                                     
  
                             
                             
                           
   
extern DECLSPEC const char *SDLCALL SDL_GameControllerNameForIndex(int joystick_index);

   
                                                                 
  
                                                                 
  
                                                               
                      
  
                                                                   
                                             
                                                                              
                                                        
  
                                                      
  
                             
   
extern DECLSPEC const char *SDLCALL SDL_GameControllerPathForIndex(int joystick_index);

   
                                     
  
                                                        
  
                                                                   
                                             
                                
  
                                                      
   
extern DECLSPEC SDL_GameControllerType SDLCALL SDL_GameControllerTypeForIndex(int joystick_index);

   
                                        
  
                                                        
  
                                                                   
                                             
                                                                              
                                    
  
                                                     
   
extern DECLSPEC char *SDLCALL SDL_GameControllerMappingForDeviceIndex(int joystick_index);

   
                                  
  
                                                               
                      
  
                                                                            
                                                                             
                                                                             
                         
  
                                                            
                                           
                                                                          
                                                
  
                                                     
  
                              
                                     
                           
   
extern DECLSPEC SDL_GameController *SDLCALL SDL_GameControllerOpen(int joystick_index);

   
                                                             
  
                                                                 
                                                                     
                                                
  
                                                     
   
extern DECLSPEC SDL_GameController *SDLCALL SDL_GameControllerFromInstanceID(SDL_JoystickID joyid);

   
                                                             
  
                                                                             
               
  
                                                                             
                                   
                                                                  
  
                                                      
  
                                       
                                       
   
extern DECLSPEC SDL_GameController *SDLCALL SDL_GameControllerFromPlayerIndex(int player_index);

   
                                                                       
  
                                                                             
                                                                           
  
                                                                            
                                                 
                                                                              
                                                                    
  
                                                     
  
                                     
                             
   
extern DECLSPEC const char *SDLCALL SDL_GameControllerName(SDL_GameController *gamecontroller);

   
                                                                       
  
                                                                             
                                                                           
  
                                                                            
                                                 
                                                                              
                                                                    
  
                                                      
  
                                     
   
extern DECLSPEC const char *SDLCALL SDL_GameControllerPath(SDL_GameController *gamecontroller);

   
                                                   
  
                                                                             
                                                                           
  
                                                             
                                
  
                                                      
   
extern DECLSPEC SDL_GameControllerType SDLCALL SDL_GameControllerGetType(SDL_GameController *gamecontroller);

   
                                                     
  
                                                             
  
                                                             
                                                                         
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_GameControllerGetPlayerIndex(SDL_GameController *gamecontroller);

   
                                                     
  
                                                              
                                                                          
                                                                       
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_GameControllerSetPlayerIndex(SDL_GameController *gamecontroller, int player_index);

   
                                                               
  
                                                            
  
                                                             
                                                     
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_GameControllerGetVendor(SDL_GameController *gamecontroller);

   
                                                                
  
                                                             
  
                                                             
                                                      
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_GameControllerGetProduct(SDL_GameController *gamecontroller);

   
                                                                 
  
                                                                  
  
                                                             
                                                           
  
                                                     
   
extern DECLSPEC Uint16 SDLCALL SDL_GameControllerGetProductVersion(SDL_GameController *gamecontroller);

   
                                                                  
  
                                                                   
  
                                                             
                                                                   
  
                                                      
   
extern DECLSPEC Uint16 SDLCALL SDL_GameControllerGetFirmwareVersion(SDL_GameController *gamecontroller);

   
                                                               
  
                                                                    
             
  
                                                             
                                                     
  
                                                      
   
extern DECLSPEC const char * SDLCALL SDL_GameControllerGetSerial(SDL_GameController *gamecontroller);

   
                                                                    
  
                                                                            
                                                 
                                                                       
                                           
  
                                                     
  
                              
                             
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerGetAttached(SDL_GameController *gamecontroller);

   
                                              
  
                                                                             
                                                                             
                                                                         
                                                                             
          
  
                                                                          
                                                                          
                      
  
                                                                          
                                      
                                                                            
  
                                                     
   
extern DECLSPEC SDL_Joystick *SDLCALL SDL_GameControllerGetJoystick(SDL_GameController *gamecontroller);

   
                                                           
  
                                                                              
                                                                          
               
  
                                                                              
                                                                   
  
                                                                        
                                                                       
                                                             
  
                                                     
  
                             
   
extern DECLSPEC int SDLCALL SDL_GameControllerEventState(int state);

   
                                                               
  
                                                                        
                                                                           
            
  
                                                     
   
extern DECLSPEC void SDLCALL SDL_GameControllerUpdate(void);


   
                                                
  
                                                                                     
                                                                                     
                                                                  
  
                                                              
   
typedef enum
{
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    SDL_CONTROLLER_AXIS_MAX
} SDL_GameControllerAxis;

   
                                                     
  
                                                                             
                                                                 
                                                                             
                                                                       
  
                                                              
                                                                            
                
  
                                                           
                                                                              
                                                                   
  
                                                     
  
                                         
   
extern DECLSPEC SDL_GameControllerAxis SDLCALL SDL_GameControllerGetAxisFromString(const char *str);

   
                                                           
  
                                                        
  
                                                               
                                                                      
                                                                   
                                               
  
                                                     
  
                                          
   
extern DECLSPEC const char* SDLCALL SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis axis);

   
                                                                    
  
                                          
                                                                            
                                                                          
                                                                             
                                                               
  
                                                     
  
                                         
   
extern DECLSPEC SDL_GameControllerButtonBind SDLCALL
SDL_GameControllerGetBindForAxis(SDL_GameController *gamecontroller,
                                 SDL_GameControllerAxis axis);

   
                                                    
  
                                                                             
                                                                 
  
                                          
                                                                   
                                                                          
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL
SDL_GameControllerHasAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);

   
                                                                 
  
                                     
  
                                                                              
                                                        
  
                                          
                                                                       
                                                                            
                                                
  
                                                     
  
                                  
   
extern DECLSPEC Sint16 SDLCALL
SDL_GameControllerGetAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);

   
                                                   
   
typedef enum
{
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MISC1,     
    SDL_CONTROLLER_BUTTON_PADDLE1,   
    SDL_CONTROLLER_BUTTON_PADDLE2,   
    SDL_CONTROLLER_BUTTON_PADDLE3,   
    SDL_CONTROLLER_BUTTON_PADDLE4,   
    SDL_CONTROLLER_BUTTON_TOUCHPAD,  
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

   
                                                          
  
                                                                             
                                                                 
                                                                             
                                                                       
  
                                                           
                                                                        
                                                                           
  
                                                     
   
extern DECLSPEC SDL_GameControllerButton SDLCALL SDL_GameControllerGetButtonFromString(const char *str);

   
                                                             
  
                                                        
  
                                                                   
                                                                          
                                                                   
                                               
  
                                                     
  
                                            
   
extern DECLSPEC const char* SDLCALL SDL_GameControllerGetStringForButton(SDL_GameControllerButton button);

   
                                                                      
  
                                          
                                                                         
                                                                          
                                                                           
                                                                   
  
                                                     
  
                                       
   
extern DECLSPEC SDL_GameControllerButtonBind SDLCALL
SDL_GameControllerGetBindForButton(SDL_GameController *gamecontroller,
                                   SDL_GameControllerButton button);

   
                                                      
  
                                                                            
                                                                    
  
                                          
                                                                        
                                                                            
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerHasButton(SDL_GameController *gamecontroller,
                                                             SDL_GameControllerButton button);

   
                                                          
  
                                          
                                                                            
                                                                         
                                                
  
                                                     
  
                                
   
extern DECLSPEC Uint8 SDLCALL SDL_GameControllerGetButton(SDL_GameController *gamecontroller,
                                                          SDL_GameControllerButton button);

   
                                                    
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerGetNumTouchpads(SDL_GameController *gamecontroller);

   
                                                                           
              
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerGetNumTouchpadFingers(SDL_GameController *gamecontroller, int touchpad);

   
                                                                        
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerGetTouchpadFinger(SDL_GameController *gamecontroller, int touchpad, int finger, Uint8 *state, float *x, float *y, float *pressure);

   
                                                            
  
                                                
                                          
                                                               
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerHasSensor(SDL_GameController *gamecontroller, SDL_SensorType type);

   
                                                                      
  
                                                 
                                                   
                                                          
                                         
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerSetSensorEnabled(SDL_GameController *gamecontroller, SDL_SensorType type, SDL_bool enabled);

   
                                                                        
  
                                                
                                          
                                                                   
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerIsSensorEnabled(SDL_GameController *gamecontroller, SDL_SensorType type);

   
                                                                       
          
  
                                                
                                          
                                                                    
  
                                                      
   
extern DECLSPEC float SDLCALL SDL_GameControllerGetSensorDataRate(SDL_GameController *gamecontroller, SDL_SensorType type);

   
                                                     
  
                                                                           
                                                            
  
                                                
                                          
                                                             
                                                          
                                        
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerGetSensorData(SDL_GameController *gamecontroller, SDL_SensorType type, float *data, int num_values);

   
                                                                              
               
  
                                                                           
                                                            
  
                                                
                                          
                                                                              
                                                                    
                                                             
                                                          
                                        
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerGetSensorDataWithTimestamp(SDL_GameController *gamecontroller, SDL_SensorType type, Uint64 *timestamp, float *data, int num_values);

   
                                              
  
                                                                             
                                          
  
                                                  
                                                                        
                                                             
                                                                           
                                                              
                                                                        
                                                                 
  
                                                     
  
                                  
   
extern DECLSPEC int SDLCALL SDL_GameControllerRumble(SDL_GameController *gamecontroller, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms);

   
                                                           
  
                                                                             
                                                  
  
                                                                              
                                                                            
                                                      
                                      
  
                                                  
                                                                            
                               
                                                                              
                                
                                                                        
                                                                         
  
                                                      
  
                                          
   
extern DECLSPEC int SDLCALL SDL_GameControllerRumbleTriggers(SDL_GameController *gamecontroller, Uint16 left_rumble, Uint16 right_rumble, Uint32 duration_ms);

   
                                              
  
                                                
                                                                     
                          
  
                                                      
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerHasLED(SDL_GameController *gamecontroller);

   
                                                      
  
                                                
                                                                          
                   
  
                                                      
  
                               
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerHasRumble(SDL_GameController *gamecontroller);

   
                                                                  
  
                                                
                                                                           
                          
  
                                                      
  
                                       
   
extern DECLSPEC SDL_bool SDLCALL SDL_GameControllerHasRumbleTriggers(SDL_GameController *gamecontroller);

   
                                        
  
                                                 
                                          
                                              
                                            
                                                                      
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerSetLED(SDL_GameController *gamecontroller, Uint8 red, Uint8 green, Uint8 blue);

   
                                           
  
                                                 
                                                 
                                                             
                                                                        
                   
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_GameControllerSendEffect(SDL_GameController *gamecontroller, const void *data, int size);

   
                                                                           
  
                                                                            
                                                 
  
                                                     
  
                             
   
extern DECLSPEC void SDLCALL SDL_GameControllerClose(SDL_GameController *gamecontroller);

   
                                                                            
             
  
                                                
                                                
                                                                
  
                                                      
  
                                                     
   
extern DECLSPEC const char* SDLCALL SDL_GameControllerGetAppleSFSymbolsNameForButton(SDL_GameController *gamecontroller, SDL_GameControllerButton button);

   
                                                                          
             
  
                                                
                                             
                                                                
  
                                                      
  
                                                       
   
extern DECLSPEC const char* SDLCALL SDL_GameControllerGetAppleSFSymbolsNameForAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);


 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
