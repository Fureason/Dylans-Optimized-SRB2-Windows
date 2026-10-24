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

   
                      
  
                                                                                 
                   
  
                                  
                                                    
                            
                                            
                                                                       
                                             
                                                    
                                                 
                                                                 
                                                      
  
                              
        
                         
  
                        
                                   
                         
                   
  
                                 
                                              
                   
  
                                                  
                                                         
                   
                        
  
                 
                                
           
  
                         
        
                                               
                         
                              
                    
  
                        
                                                      
                                                                         
  
                                    
                                                           
                                                   
                   
       
  
                          
                                                                              
                                    
                                                                             
                                                                         
                                               
                                                                
                                                      
                                                                                 
                                                                        
  
                          
                                                         
  
                        
                                                  
                                                        
  
                                                                          
                                                   
  
                         
                              
  
                          
    
           
   

#ifndef SDL_haptic_h_
#define SDL_haptic_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_joystick.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif  

                                                                                
  
                                                                               
                                                                                
  
                                                                                
                                                                            
                                                     
          
   

   
                       
  
                                                               
  
                      
                                  
                       
   
struct _SDL_Haptic;
typedef struct _SDL_Haptic SDL_Haptic;


   
                         
  
                                                
   
 

   
                        
   
 

   
                                     
  
                           
  
                           
   
#define SDL_HAPTIC_CONSTANT   (1u<<0)

   
                                      
  
                                                     
  
                          
   
#define SDL_HAPTIC_SINE       (1u<<1)

   
                                       
  
                                                                    
  
                           
                                                                               
                                                                         
   
#define SDL_HAPTIC_LEFTRIGHT     (1u<<2)

 
 

   
                                          
  
                                                           
  
                          
   
#define SDL_HAPTIC_TRIANGLE   (1u<<3)

   
                                            
  
                                                             
  
                          
   
#define SDL_HAPTIC_SAWTOOTHUP (1u<<4)

   
                                              
  
                                                               
  
                          
   
#define SDL_HAPTIC_SAWTOOTHDOWN (1u<<5)

   
                                 
  
                       
  
                      
   
#define SDL_HAPTIC_RAMP       (1u<<6)

   
                                                        
  
                                                                            
                  
  
                           
   
#define SDL_HAPTIC_SPRING     (1u<<7)

   
                                                        
  
                                                                             
                  
  
                           
   
#define SDL_HAPTIC_DAMPER     (1u<<8)

   
                                                             
  
                                                                                
                 
  
                           
   
#define SDL_HAPTIC_INERTIA    (1u<<9)

   
                                                          
  
                                                                            
                  
  
                           
   
#define SDL_HAPTIC_FRICTION   (1u<<10)

   
                                      
  
                                      
   
#define SDL_HAPTIC_CUSTOM     (1u<<11)

  

 

   
                                      
  
                                            
  
                         
   
#define SDL_HAPTIC_GAIN       (1u<<12)

   
                                     
  
                                       
  
                               
   
#define SDL_HAPTIC_AUTOCENTER (1u<<13)

   
                                                   
  
                                           
  
                                 
   
#define SDL_HAPTIC_STATUS     (1u<<14)

   
                                
  
                                  
  
                       
                         
   
#define SDL_HAPTIC_PAUSE      (1u<<15)


   
                            
   
 

   
                                                    
  
                           
   
#define SDL_HAPTIC_POLAR      0

   
                                                        
  
                           
   
#define SDL_HAPTIC_CARTESIAN  1

   
                                                        
  
                           
   
#define SDL_HAPTIC_SPHERICAL  2

   
                                                                             
                                                                                 
                     
                           
   
#define SDL_HAPTIC_STEERING_AXIS 3

  

  

  
                
   

   
                                                            
  
                          
   
#define SDL_HAPTIC_INFINITY   4294967295U


   
                                                        
  
                                                     
                                                           
  
                                   
                                                           
                                                                   
                                                                   
  
                                                                            
                                                                 
  
                                                             
             
                     
                               
                               
                               
                               
                               
                               


                                 
                          
                          
                          
                                               
                          
                          
                          
                                 


                              
                             
                             
                                   
                
  
                                                                          
                                                                              
                                                                  
                           
                              
                                 
                                
  
                                                                              
                                                                           
                                                                         
                      
                      
                      
                      
  
                                                                          
                                                                               
                                                              
  
                                                                             
                                                                           
                                                      
                                                  
                                                                
  
  
                                                                           
                                                                             
         
                                  
  
                           
                                                                                 
                                       
                                       
                                                                                
  
                       
                                                                                  
                                                                
  
                            
                                                                
                                                                                          
            
  
                        
                            
                            
                                
                        
                         
   
typedef struct SDL_HapticDirection
{
    Uint8 type;          
    Sint32 dir[3];       
} SDL_HapticDirection;


   
                                                                   
  
                                                                    
  
                                                                         
                    
  
                           
                        
   
typedef struct SDL_HapticConstant
{
     
    Uint16 type;             
    SDL_HapticDirection direction;   

     
    Uint32 length;           
    Uint16 delay;            

     
    Uint16 button;           
    Uint16 interval;         

     
    Sint16 level;            

     
    Uint16 attack_length;    
    Uint16 attack_level;     
    Uint16 fade_length;      
    Uint16 fade_level;       
} SDL_HapticConstant;

   
                                                                   
  
                                             
                        
                             
                            
                              
                                
  
                                                                          
                                                                            
                                         
  
                                                                                 
                                                                        
                                    
                                          
                                          
                                          
                                                                          
  
             
             
                   
                                
                              
                             

                     
                               
                                
                                

                       
                               \
                              
                             

                         
                                
                                
                                

                           
                                
                                
                                
                
  
                       
                            
                           
                             
                               
                        
   
typedef struct SDL_HapticPeriodic
{
     
    Uint16 type;                                                       
                                                                              
                                                         
    SDL_HapticDirection direction;   

     
    Uint32 length;       
    Uint16 delay;        

     
    Uint16 button;       
    Uint16 interval;     

     
    Uint16 period;       
    Sint16 magnitude;    
    Sint16 offset;       
    Uint16 phase;        

     
    Uint16 attack_length;    
    Uint16 attack_level;     
    Uint16 fade_length;  
    Uint16 fade_level;   
} SDL_HapticPeriodic;

   
                                                                    
  
                                             
                                                          
                                                          
                                                               
                                                            
  
                                                                              
                                                                           
                                                                       
                                                                            
                                                                           
                                                                             
                      
  
                           
                         
                         
                          
                           
                        
   
typedef struct SDL_HapticCondition
{
     
    Uint16 type;                                                          
                                                                                 
    SDL_HapticDirection direction;   

     
    Uint32 length;           
    Uint16 delay;            

     
    Uint16 button;           
    Uint16 interval;         

     
    Uint16 right_sat[3];     
    Uint16 left_sat[3];      
    Sint16 right_coeff[3];   
    Sint16 left_coeff[3];    
    Uint16 deadband[3];      
    Sint16 center[3];        
} SDL_HapticCondition;

   
                                                               
  
                                                                
  
                                                                      
                                                                          
                                                                     
                                
  
                       
                        
   
typedef struct SDL_HapticRamp
{
     
    Uint16 type;             
    SDL_HapticDirection direction;   

     
    Uint32 length;           
    Uint16 delay;            

     
    Uint16 button;           
    Uint16 interval;         

     
    Sint16 start;            
    Sint16 end;              

     
    Uint16 attack_length;    
    Uint16 attack_level;     
    Uint16 fade_length;      
    Uint16 fade_level;       
} SDL_HapticRamp;

   
                                                                    
  
                                                                    
  
                                                                          
                                                                             
                                                                  
  
                           
                       
   
typedef struct SDL_HapticLeftRight
{
     
    Uint16 type;             

     
    Uint32 length;           

     
    Uint16 large_magnitude;  
    Uint16 small_magnitude;  
} SDL_HapticLeftRight;

   
                                                                                
  
                                                                  
  
                                                                            
                                                                          
                                                                             
  
                                                                          
                                                                 
  
                         
                        
   
typedef struct SDL_HapticCustom
{
     
    Uint16 type;             
    SDL_HapticDirection direction;   

     
    Uint32 length;           
    Uint16 delay;            

     
    Uint16 button;           
    Uint16 interval;         

     
    Uint8 channels;          
    Uint16 period;           
    Uint16 samples;          
    Uint16 *data;            

     
    Uint16 attack_length;    
    Uint16 attack_level;     
    Uint16 fade_length;      
    Uint16 fade_level;       
} SDL_HapticCustom;

   
                                                      
  
                                                                          
                                                               
  
                                                                          
                                                                          
                                                                               
  
                                                                             
                          
  
                                                                             
                                                                           
                 
  
                                                                         
                                   
  
                 
         
                                     
                                                     
                                                          
  
                                      
                                                         
                                                                           
  
                                                                
                                                         
                                                              
                                                           
                                                          
            
  
  
                                                                   
             
            
     
     
                                            
                                            \
                                             \
                                              \
                                               \
                                                \
                                                                   
     
                                                             
                                                 
                                                       

                                                 
                              
                
  
                                                                          
                 
  
                          
                          
                           
                      
                           
                        
   
typedef union SDL_HapticEffect
{
     
    Uint16 type;                     
    SDL_HapticConstant constant;     
    SDL_HapticPeriodic periodic;     
    SDL_HapticCondition condition;   
    SDL_HapticRamp ramp;             
    SDL_HapticLeftRight leftright;   
    SDL_HapticCustom custom;         
} SDL_HapticEffect;


 

   
                                                             
  
                                                                             
                                                                            
  
                                                     
  
                     
   
extern DECLSPEC int SDLCALL SDL_NumHaptics(void);

   
                                                            
  
                                                                        
                                     
  
                                                    
                                                                              
                             
  
                                                     
  
                     
   
extern DECLSPEC const char *SDLCALL SDL_HapticName(int device_index);

   
                                
  
                                                                           
          
  
                                                                    
                                                                              
                                 
  
                                                  
                                                                             
                             
  
                                                     
  
                      
                      
                                 
                              
                      
                              
                        
                        
   
extern DECLSPEC SDL_Haptic *SDLCALL SDL_HapticOpen(int device_index);

   
                                                                      
  
                                                       
                                                                       
                                                
  
                                                     
  
                      
                     
   
extern DECLSPEC int SDLCALL SDL_HapticOpened(int device_index);

   
                                    
  
                                               
                                                                             
                                                                 
  
                                                     
  
                     
                       
   
extern DECLSPEC int SDLCALL SDL_HapticIndex(SDL_Haptic * haptic);

   
                                                                  
  
                                                                     
  
                                                     
  
                              
   
extern DECLSPEC int SDLCALL SDL_MouseIsHaptic(void);

   
                                                      
  
                                                                 
                                                
  
                                                     
  
                     
                        
   
extern DECLSPEC SDL_Haptic *SDLCALL SDL_HapticOpenFromMouse(void);

   
                                           
  
                                                                   
                                                                           
                                                                        
                        
  
                                                     
  
                                 
   
extern DECLSPEC int SDLCALL SDL_JoystickIsHaptic(SDL_Joystick * joystick);

   
                                                       
  
                                                                           
                     
  
                                                                              
                                                                          
                                                                              
                  
  
                                                                  
                                                                           
                                                     
  
                                                     
  
                      
                     
                           
   
extern DECLSPEC SDL_Haptic *SDLCALL SDL_HapticOpenFromJoystick(SDL_Joystick *
                                                               joystick);

   
                                                                 
  
                                               
  
                                                     
  
                     
   
extern DECLSPEC void SDLCALL SDL_HapticClose(SDL_Haptic * haptic);

   
                                                       
  
                                                                    
                                                                         
                                                            
  
                                               
                                                                           
                                                                            
  
                                                     
  
                                  
                      
   
extern DECLSPEC int SDLCALL SDL_HapticNumEffects(SDL_Haptic * haptic);

   
                                                                       
  
                                                                          
  
                                                                       
                                                                             
                                                                             
                        
  
                                                     
  
                           
                      
   
extern DECLSPEC int SDLCALL SDL_HapticNumEffectsPlaying(SDL_Haptic * haptic);

   
                                                                
  
                                               
                                                                              
                                                                 
  
                                                     
  
                                
                           
   
extern DECLSPEC unsigned int SDLCALL SDL_HapticQuery(SDL_Haptic * haptic);


   
                                                
  
                                                                
                              
  
                                               
                                                                              
                                                     
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_HapticNumAxes(SDL_Haptic * haptic);

   
                                                             
  
                                               
                                            
                                                                        
                                                                        
                        
  
                                                     
  
                          
                      
   
extern DECLSPEC int SDLCALL SDL_HapticEffectSupported(SDL_Haptic * haptic,
                                                      SDL_HapticEffect *
                                                      effect);

   
                                                    
  
                                                             
                                                                           
                                     
                                                                       
                                                              
  
                                                     
  
                              
                          
                             
   
extern DECLSPEC int SDLCALL SDL_HapticNewEffect(SDL_Haptic * haptic,
                                                SDL_HapticEffect * effect);

   
                                      
  
                                                                       
                                                                             
                                                                            
                                    
  
                                                          
                                                       
                                                                      
                                
                                                                  
                                                
  
                                                     
  
                              
                          
                          
   
extern DECLSPEC int SDLCALL SDL_HapticUpdateEffect(SDL_Haptic * haptic,
                                                   int effect,
                                                   SDL_HapticEffect * data);

   
                                                         
  
                                                                       
                                                                           
                                                                              
                                                                            
           
  
                                                           
                                                   
                                                                    
                                                            
                                                                  
                                                
  
                                                     
  
                              
                                
                           
   
extern DECLSPEC int SDLCALL SDL_HapticRunEffect(SDL_Haptic * haptic,
                                                int effect,
                                                Uint32 iterations);

   
                                                          
  
    
  
                                                            
                                                    
                                                                  
                                                
  
                                                     
  
                              
                          
   
extern DECLSPEC int SDLCALL SDL_HapticStopEffect(SDL_Haptic * haptic,
                                                 int effect);

   
                                         
  
                                                                       
                                       
  
                                                               
                                                       
  
                                                     
  
                          
   
extern DECLSPEC void SDLCALL SDL_HapticDestroyEffect(SDL_Haptic * haptic,
                                                     int effect);

   
                                                                       
  
                                                     
  
                                                                        
                                                                
                                                                          
                                                                      
  
                                                     
  
                          
                           
   
extern DECLSPEC int SDLCALL SDL_HapticGetEffectStatus(SDL_Haptic * haptic,
                                                      int effect);

   
                                                      
  
                                                   
  
                                                                            
                                                                        
                                                                             
           
  
                                                         
                                                                              
                                                                  
                                                
  
                                                     
  
                      
   
extern DECLSPEC int SDLCALL SDL_HapticSetGain(SDL_Haptic * haptic, int gain);

   
                                           
  
                                                                       
                 
  
                                                         
  
                                                              
                                                       
                                                                  
                                                
  
                                                     
  
                      
   
extern DECLSPEC int SDLCALL SDL_HapticSetAutocenter(SDL_Haptic * haptic,
                                                    int autocenter);

   
                         
  
                                                           
                                          
  
                                                                              
                                       
  
                                               
                                                                  
                                                
  
                                                     
  
                        
   
extern DECLSPEC int SDLCALL SDL_HapticPause(SDL_Haptic * haptic);

   
                           
  
                                           
  
                                                 
                                                                  
                                                
  
                                                     
  
                      
   
extern DECLSPEC int SDLCALL SDL_HapticUnpause(SDL_Haptic * haptic);

   
                                                             
  
                                              
                                                                  
                                                
  
                                                     
   
extern DECLSPEC int SDLCALL SDL_HapticStopAll(SDL_Haptic * haptic);

   
                                                        
  
                                                          
                                                                        
                                                                        
                        
  
                                                     
  
                           
                           
                           
   
extern DECLSPEC int SDLCALL SDL_HapticRumbleSupported(SDL_Haptic * haptic);

   
                                                         
  
                                                                           
                                                                  
                                                
  
                                                     
  
                     
                           
                           
                                
   
extern DECLSPEC int SDLCALL SDL_HapticRumbleInit(SDL_Haptic * haptic);

   
                                                 
  
                                                               
                                                                      
                                                             
                                                                  
                                                
  
                                                     
  
                           
                           
                                
   
extern DECLSPEC int SDLCALL SDL_HapticRumblePlay(SDL_Haptic * haptic, float strength, Uint32 length );

   
                                             
  
                                                               
                                                                  
                                                
  
                                                     
  
                           
                           
                                
   
extern DECLSPEC int SDLCALL SDL_HapticRumbleStop(SDL_Haptic * haptic);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
