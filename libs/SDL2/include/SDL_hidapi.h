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

/**
 *  \file SDL_hidapi.h
 *
 *  Header file for SDL HIDAPI functions.
 *
 *  This is an adaptation of the original HIDAPI interface by Alan Ott,
 *  and includes source code licensed under the following BSD license:
 *
    Copyright (c) 2010, Alan Ott, Signal 11 Software
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Signal 11 Software nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
 *
 * If you would like a version of SDL without this code, you can build SDL
 * with SDL_HIDAPI_DISABLED defined to 1. You might want to do this for example
 * on iOS or tvOS to avoid a dependency on the CoreBluetooth framework.
 */

#ifndef SDL_hidapi_h_
#define SDL_hidapi_h_

#include "SDL_stdinc.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

   
                                                    
   
struct SDL_hid_device_;
typedef struct SDL_hid_device_ SDL_hid_device;  

 
   
                                                    
   
typedef struct SDL_hid_device_info
{
     
    char *path;
     
    unsigned short vendor_id;
     
    unsigned short product_id;
     
    wchar_t *serial_number;
                                                      
                                              
    unsigned short release_number;
     
    wchar_t *manufacturer_string;
     
    wchar_t *product_string;
                                            
                              
    unsigned short usage_page;
                                       
                             
    unsigned short usage;
                                                   
                   

                                                           
                                                                
                                              
    int interface_number;

                                                       
                                                       
    int interface_class;
    int interface_subclass;
    int interface_protocol;

     
    struct SDL_hid_device_info *next;
} SDL_hid_device_info;


   
                                 
  
                                                                           
                                                                           
                                                                              
                                                                           
                                                                   
  
                                                                           
  
                                         
  
                                                      
  
                   
   
extern DECLSPEC int SDLCALL SDL_hid_init(void);

   
                               
  
                                                                        
                                                                  
  
                                         
  
                                                      
  
                   
   
extern DECLSPEC int SDLCALL SDL_hid_exit(void);

   
                                                          
  
                                                                              
                                                                             
                                                                           
                                                                            
                       
  
                                                                              
                                                                 
  
                                                                           
                                                                    
  
                                                      
  
                        
   
extern DECLSPEC Uint32 SDLCALL SDL_hid_device_change_count(void);

   
                             
  
                                                                             
                                                                          
                                                                        
                                                                           
                            
  
                                                                       
                                                                         
                                                                              
                                                                             
                                                                    
                                       
  
                                                      
  
                                  
   
extern DECLSPEC SDL_hid_device_info * SDLCALL SDL_hid_enumerate(unsigned short vendor_id, unsigned short product_id);

   
                                  
  
                                                                    
  
                                                               
                                   
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_hid_free_enumeration(SDL_hid_device_info *devs);

   
                                                                             
                   
  
                                                                              
             
  
                                                              
                                                                
                                                                           
                              
                                                                      
                    
  
                                                      
   
extern DECLSPEC SDL_hid_device * SDLCALL SDL_hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number);

   
                                      
  
                                                                   
                                                                       
  
                                                  
                                                                      
                    
  
                                                      
   
extern DECLSPEC SDL_hid_device * SDLCALL SDL_hid_open_path(const char *path, int bExclusive  );

   
                                          
  
                                                                              
                                                                        
                                                                      
                                                                              
                                                                            
                                                                             
                                                                             
                  
  
                                                                       
                                                                             
                
  
                                                           
                                                                         
                    
                                                         
                                                               
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_write(SDL_hid_device *dev, const unsigned char *data, size_t length);

   
                                                       
  
                                                                            
                                                                            
           
  
                                                           
                                                  
                                                                       
                                                                        
                        
                                                                       
                                                                             
                                                                         
                      
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_read_timeout(SDL_hid_device *dev, unsigned char *data, size_t length, int milliseconds);

   
                                          
  
                                                                            
                                                                            
           
  
                                                           
                                                  
                                                                       
                                                                        
                        
                                                                             
                                                                             
                               
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_read(SDL_hid_device *dev, unsigned char *data, size_t length);

   
                                            
  
                                                                              
                                                                              
                                                                  
  
                                                    
  
                                                           
                                                                    
                                                          
                                         
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_set_nonblocking(SDL_hid_device *dev, int nonblock);

   
                                       
  
                                                                     
                                                                             
                                                                             
                                                                            
                                                                           
                                                                           
                                                                              
                                                                              
                                                             
  
                                                           
                                                                         
                    
                                                                              
                        
                                                               
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_send_feature_report(SDL_hid_device *dev, const unsigned char *data, size_t length);

   
                                          
  
                                                                          
                                                                           
                                                                              
              
  
                                                           
                                                                           
                                                                             
                                                                              
                       
                                                                             
                                                                            
                                                                         
                                                     
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_get_feature_report(SDL_hid_device *dev, unsigned char *data, size_t length);

   
                      
  
                                                           
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_hid_close(SDL_hid_device *dev);

   
                                                 
  
                                                           
                                                           
                                                                  
                                         
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_get_manufacturer_string(SDL_hid_device *dev, wchar_t *string, size_t maxlen);

   
                                            
  
                                                           
                                                           
                                                                  
                                         
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_get_product_string(SDL_hid_device *dev, wchar_t *string, size_t maxlen);

   
                                                  
  
                                                           
                                                           
                                                                  
                                         
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_get_serial_number_string(SDL_hid_device *dev, wchar_t *string, size_t maxlen);

   
                                                             
  
                                                           
                                                      
                                                           
                                                                  
                                         
  
                                                      
   
extern DECLSPEC int SDLCALL SDL_hid_get_indexed_string(SDL_hid_device *dev, int string_index, wchar_t *string, size_t maxlen);

   
                                                                     
  
                                                                       
  
                                                      
   
extern DECLSPEC void SDLCALL SDL_hid_ble_scan(SDL_bool active);

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
