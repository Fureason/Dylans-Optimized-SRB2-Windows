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

   
                     
  
                                                                    
                                                                   
   

#ifndef SDL_rwops_h_
#define SDL_rwops_h_

#include "SDL_stdinc.h"
#include "SDL_error.h"

#include "begin_code.h"
 
#ifdef __cplusplus
extern "C" {
#endif

 
#define SDL_RWOPS_UNKNOWN   0U   
#define SDL_RWOPS_WINFILE   1U   
#define SDL_RWOPS_STDFILE   2U   
#define SDL_RWOPS_JNIFILE   3U   
#define SDL_RWOPS_MEMORY    4U   
#define SDL_RWOPS_MEMORY_RO 5U   

   
                                                            
   
typedef struct SDL_RWops
{
       
                                                                   
       
    Sint64 (SDLCALL * size) (struct SDL_RWops * context);

       
                                                                              
                                             
      
                                                                    
       
    Sint64 (SDLCALL * seek) (struct SDL_RWops * context, Sint64 offset,
                             int whence);

       
                                                                       
                                                
      
                                                                         
       
    size_t (SDLCALL * read) (struct SDL_RWops * context, void *ptr,
                             size_t size, size_t maxnum);

       
                                                                       
                                            
      
                                                                            
       
    size_t (SDLCALL * write) (struct SDL_RWops * context, const void *ptr,
                              size_t size, size_t num);

       
                                                        
      
                                                                        
       
    int (SDLCALL * close) (struct SDL_RWops * context);

    Uint32 type;
    union
    {
#if defined(__ANDROID__)
        struct
        {
            void *asset;
        } androidio;
#elif defined(__WIN32__) || defined(__GDK__)
        struct
        {
            SDL_bool append;
            void *h;
            struct
            {
                void *data;
                size_t size;
                size_t left;
            } buffer;
        } windowsio;
#endif

#ifdef HAVE_STDIO_H
        struct
        {
            SDL_bool autoclose;
            FILE *fp;
        } stdio;
#endif
        struct
        {
            Uint8 *base;
            Uint8 *here;
            Uint8 *stop;
        } mem;
        struct
        {
            void *data1;
            void *data2;
        } unknown;
    } hidden;

} SDL_RWops;


   
                          
  
                                                                       
   
 

   
                                                                         
                                  
  
                                                                      
                                                                          
          
  
                            
  
                                                       
                                                                        
                                                                          
                
                                                                            
                                                    
                                                                         
           
                                                                            
                                                                       
                                 
                                                                            
                                                                            
                                                                            
                                                                          
                                                                       
  
                                                                             
                                                                             
                                                                           
                                                                           
                                                                     
                                                                              
                                                                              
               
  
                                                                              
                                                         
  
                                                                              
                                
  
                                                                              
  
                                                               
                                                                           
                        
                                                                            
                                                              
  
                                                     
  
                  
                         
                   
                    
                 
                 
                 
                  
   
extern DECLSPEC SDL_RWops *SDLCALL SDL_RWFromFile(const char *file,
                                                  const char *mode);

#ifdef HAVE_STDIO_H

extern DECLSPEC SDL_RWops *SDLCALL SDL_RWFromFP(FILE * fp, SDL_bool autoclose);

#else

   
                                                                              
                               
  
                                                                      
                                                                      
           
  
                                                                         
                                                                        
                                                                       
  
                                                        
                                                                             
                                                                         
                          
                                                                            
                                                              
  
                                                     
  
                  
                         
                     
                    
                 
                 
                 
                  
   
extern DECLSPEC SDL_RWops *SDLCALL SDL_RWFromFP(void * fp,
                                                SDL_bool autoclose);
#endif

   
                                                                       
             
  
                                                                        
                                                
  
                                                                              
                                                                            
                       
  
                                                                            
                                                                             
  
                                                               
                                        
                                                                             
                                                
  
                                                     
  
                  
                         
                     
                   
                    
                 
                 
                 
                  
   
extern DECLSPEC SDL_RWops *SDLCALL SDL_RWFromMem(void *mem, int size);

   
                                                                             
  
                                                                        
                                                            
  
                                                                        
                                
  
                                                                              
                                                                            
                       
  
                                                                          
                                            
  
                                                                         
                                        
                                                                             
                                                
  
                                                     
  
                  
                         
                     
                   
                    
                 
                 
                 
   
extern DECLSPEC SDL_RWops *SDLCALL SDL_RWFromConstMem(const void *mem,
                                                      int size);

  


   
                                                                           
  
                                                                          
                                                                      
                                                               
                                                                         
  
                                                                          
                                                                       
                                                                   
                                                                            
                                                                        
                                                                              
                                                     
  
                                                                             
                                                     
  
                                                     
  
                 
   
extern DECLSPEC SDL_RWops *SDLCALL SDL_AllocRW(void);

   
                                                                
                 
  
                                                                          
                                                                      
                                                               
                                                                             
                                                                          
             
  
                                                                              
                                                                              
                                                                              
                                                                     
  
                                                  
  
                                                     
  
                  
   
extern DECLSPEC void SDLCALL SDL_FreeRW(SDL_RWops * area);

#define RW_SEEK_SET 0        
#define RW_SEEK_CUR 1        
#define RW_SEEK_END 2        

   
                                                                        
  
                                                  
  
                                                                       
                                                                          
                                                                            
                                 
  
                                                      
   
extern DECLSPEC Sint64 SDLCALL SDL_RWsize(SDL_RWops *context);

   
                                        
  
                                                              
  
                                               
  
                                                   
                                                       
                                                    
  
                                                  
  
                                                                         
                                                                    
  
                                                  
  
                                                     
                                                                            
                         
                                                                   
                                                                              
  
                                                      
  
                  
                         
                     
                   
                    
                 
                 
                  
   
extern DECLSPEC Sint64 SDLCALL SDL_RWseek(SDL_RWops *context,
                                          Sint64 offset, int whence);

   
                                                                       
  
                                                                              
                                                                    
                           
  
                                                  
  
                                                                              
                        
                                                                              
                          
  
                                                      
  
                  
                         
                     
                   
                    
                 
                 
                  
   
extern DECLSPEC Sint64 SDLCALL SDL_RWtell(SDL_RWops *context);

   
                           
  
                                                                          
                                                                           
                                                                              
                                      
  
                                                                         
                                                                    
  
                                                  
  
                                                     
                                                     
                                                        
                                                         
                                                                          
                                                
  
                                                      
  
                  
                         
                     
                   
                    
                 
                  
   
extern DECLSPEC size_t SDLCALL SDL_RWread(SDL_RWops *context,
                                          void *ptr, size_t size,
                                          size_t maxnum);

   
                                     
  
                                                                          
                                                                              
                                                                         
                             
  
                                                                        
                                                                     
  
                                                  
  
                                                     
                                                            
                                                       
                                            
                                                                             
                                                            
  
                                                      
  
                  
                         
                     
                   
                    
                 
                 
   
extern DECLSPEC size_t SDLCALL SDL_RWwrite(SDL_RWops *context,
                                           const void *ptr, size_t size,
                                           size_t num);

   
                                                   
  
                                                                           
                                                                   
                                                                         
                                      
  
                                                                             
                                                                           
  
                                                  
  
                                              
                                                                  
                                                
  
                                                      
  
                         
                     
                   
                    
                 
                 
                  
   
extern DECLSPEC int SDLCALL SDL_RWclose(SDL_RWops *context);

   
                                             
  
                                                                          
                                                                         
              
  
                                            
  
                                                           
                                                                   
                                                                            
                                                    
  
                                                     
   
extern DECLSPEC void *SDLCALL SDL_LoadFile_RW(SDL_RWops *src,
                                              size_t *datasize,
                                              int freesrc);

   
                                      
  
                                                                          
                                                                         
              
  
                                            
  
                                                                 
                   
  
                                                       
                                                                   
                                                    
  
                                                      
   
extern DECLSPEC void *SDLCALL SDL_LoadFile(const char *file, size_t *datasize);

   
                               
  
                                                                         
   
 

   
                                                      
  
                                        
                                                                             
                             
  
                                                     
  
                  
   
extern DECLSPEC Uint8 SDLCALL SDL_ReadU8(SDL_RWops * src);

   
                                                                            
                               
  
                                                                            
                         
  
                                                
                                                                     
  
                                                     
  
                   
   
extern DECLSPEC Uint16 SDLCALL SDL_ReadLE16(SDL_RWops * src);

   
                                                                             
                           
  
                                                                            
                         
  
                                                
                                                                     
  
                                                     
  
                   
   
extern DECLSPEC Uint16 SDLCALL SDL_ReadBE16(SDL_RWops * src);

   
                                                                            
                               
  
                                                                            
                         
  
                                                
                                                                     
  
                                                     
  
                   
   
extern DECLSPEC Uint32 SDLCALL SDL_ReadLE32(SDL_RWops * src);

   
                                                                             
                           
  
                                                                            
                         
  
                                                
                                                                     
  
                                                     
  
                   
   
extern DECLSPEC Uint32 SDLCALL SDL_ReadBE32(SDL_RWops * src);

   
                                                                            
                               
  
                                                                            
                         
  
                                                
                                                                     
  
                                                     
  
                   
   
extern DECLSPEC Uint64 SDLCALL SDL_ReadLE64(SDL_RWops * src);

   
                                                                             
                           
  
                                                                            
                         
  
                                                
                                                                     
  
                                                     
  
                   
   
extern DECLSPEC Uint64 SDLCALL SDL_ReadBE64(SDL_RWops * src);
  

   
                                
  
                                                               
   
 

   
                                                     
  
                                       
                                       
                                                                      
                        
  
                                                     
  
                 
   
extern DECLSPEC size_t SDLCALL SDL_WriteU8(SDL_RWops * dst, Uint8 value);

   
                                                                        
                      
  
                                                                      
                                                                         
          
  
                                                      
                                                        
                                              
  
                                                     
  
                    
   
extern DECLSPEC size_t SDLCALL SDL_WriteLE16(SDL_RWops * dst, Uint16 value);

   
                                                                        
                   
  
                                                                      
                                                                              
  
                                                      
                                                        
                                              
  
                                                     
  
                    
   
extern DECLSPEC size_t SDLCALL SDL_WriteBE16(SDL_RWops * dst, Uint16 value);

   
                                                                        
                      
  
                                                                      
                                                                         
          
  
                                                      
                                                        
                                              
  
                                                     
  
                    
   
extern DECLSPEC size_t SDLCALL SDL_WriteLE32(SDL_RWops * dst, Uint32 value);

   
                                                                        
                   
  
                                                                      
                                                                              
  
                                                      
                                                        
                                              
  
                                                     
  
                    
   
extern DECLSPEC size_t SDLCALL SDL_WriteBE32(SDL_RWops * dst, Uint32 value);

   
                                                                        
                      
  
                                                                      
                                                                         
          
  
                                                      
                                                        
                                              
  
                                                     
  
                    
   
extern DECLSPEC size_t SDLCALL SDL_WriteLE64(SDL_RWops * dst, Uint64 value);

   
                                                                        
                   
  
                                                                      
                                                                              
  
                                                      
                                                        
                                              
  
                                                     
  
                    
   
extern DECLSPEC size_t SDLCALL SDL_WriteBE64(SDL_RWops * dst, Uint64 value);
  

 
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif  

 
