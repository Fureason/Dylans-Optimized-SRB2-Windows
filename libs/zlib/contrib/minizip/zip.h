/* zip.h -- IO on .zip files using zlib
   Version 1.1, February 14h, 2010
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications for Zip64 support
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt

         ---------------------------------------------------------------------------

   Condition of use and distribution are the same than zlib :

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

        ---------------------------------------------------------------------------

        Changes

        See header of zip.h

*/

#ifndef _zip12_H
#define _zip12_H

#ifdef __cplusplus
extern "C" {
#endif



#ifndef _ZLIB_H
#include "zlib.h"
#endif

#ifndef _ZLIBIOAPI_H
#include "ioapi.h"
#endif

#ifdef HAVE_BZIP2
#include "bzlib.h"
#endif

#define Z_BZIP2ED 12

#if defined(STRICTZIP) || defined(STRICTZIPUNZIP)
                                                                         
                                
typedef struct TagzipFile__ { int unused; } zipFile__;
typedef zipFile__ *zipFile;
#else
typedef voidp zipFile;
#endif

#define ZIP_OK                          (0)
#define ZIP_EOF                         (0)
#define ZIP_ERRNO                       (Z_ERRNO)
#define ZIP_PARAMERROR                  (-102)
#define ZIP_BADZIPFILE                  (-103)
#define ZIP_INTERNALERROR               (-104)

#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif
 

 
typedef struct tm_zip_s
{
    int tm_sec;              
    int tm_min;              
    int tm_hour;             
    int tm_mday;             
    int tm_mon;              
    int tm_year;             
} tm_zip;

typedef struct
{
    tm_zip      tmz_date;        
    uLong       dosDate;        
     

    uLong       internal_fa;     
    uLong       external_fa;     
} zip_fileinfo;

typedef const char* zipcharpc;


#define APPEND_STATUS_CREATE        (0)
#define APPEND_STATUS_CREATEAFTER   (1)
#define APPEND_STATUS_ADDINZIP      (2)

extern zipFile ZEXPORT zipOpen(const char *pathname, int append);
extern zipFile ZEXPORT zipOpen64(const void *pathname, int append);
  
                   
                                                                                 
                                           
                                                                              
                                              
                                                           
                                                                           
                                                                                
                                                               
                                                                           
                           
  

                                                     
                                                                                      
                                                                                          
  

extern zipFile ZEXPORT zipOpen2(const char *pathname,
                                int append,
                                zipcharpc* globalcomment,
                                zlib_filefunc_def* pzlib_filefunc_def);

extern zipFile ZEXPORT zipOpen2_64(const void *pathname,
                                   int append,
                                   zipcharpc* globalcomment,
                                   zlib_filefunc64_def* pzlib_filefunc_def);

extern zipFile ZEXPORT zipOpen3(const void *pathname,
                                int append,
                                zipcharpc* globalcomment,
                                zlib_filefunc64_32_def* pzlib_filefunc64_32_def);

extern int ZEXPORT zipOpenNewFileInZip(zipFile file,
                                       const char* filename,
                                       const zip_fileinfo* zipfi,
                                       const void* extrafield_local,
                                       uInt size_extrafield_local,
                                       const void* extrafield_global,
                                       uInt size_extrafield_global,
                                       const char* comment,
                                       int method,
                                       int level);

extern int ZEXPORT zipOpenNewFileInZip64(zipFile file,
                                         const char* filename,
                                         const zip_fileinfo* zipfi,
                                         const void* extrafield_local,
                                         uInt size_extrafield_local,
                                         const void* extrafield_global,
                                         uInt size_extrafield_global,
                                         const char* comment,
                                         int method,
                                         int level,
                                         int zip64);

  
                                     
                                                                         
                                         
                                                                         
                                                     
                                                                            
                                                      
                                                        
                                                                             
                                                                       
                                                                                                   
                                                                               

  


extern int ZEXPORT zipOpenNewFileInZip2(zipFile file,
                                        const char* filename,
                                        const zip_fileinfo* zipfi,
                                        const void* extrafield_local,
                                        uInt size_extrafield_local,
                                        const void* extrafield_global,
                                        uInt size_extrafield_global,
                                        const char* comment,
                                        int method,
                                        int level,
                                        int raw);


extern int ZEXPORT zipOpenNewFileInZip2_64(zipFile file,
                                           const char* filename,
                                           const zip_fileinfo* zipfi,
                                           const void* extrafield_local,
                                           uInt size_extrafield_local,
                                           const void* extrafield_global,
                                           uInt size_extrafield_global,
                                           const char* comment,
                                           int method,
                                           int level,
                                           int raw,
                                           int zip64);
  
                                                                   
   

extern int ZEXPORT zipOpenNewFileInZip3(zipFile file,
                                        const char* filename,
                                        const zip_fileinfo* zipfi,
                                        const void* extrafield_local,
                                        uInt size_extrafield_local,
                                        const void* extrafield_global,
                                        uInt size_extrafield_global,
                                        const char* comment,
                                        int method,
                                        int level,
                                        int raw,
                                        int windowBits,
                                        int memLevel,
                                        int strategy,
                                        const char* password,
                                        uLong crcForCrypting);

extern int ZEXPORT zipOpenNewFileInZip3_64(zipFile file,
                                           const char* filename,
                                           const zip_fileinfo* zipfi,
                                           const void* extrafield_local,
                                           uInt size_extrafield_local,
                                           const void* extrafield_global,
                                           uInt size_extrafield_global,
                                           const char* comment,
                                           int method,
                                           int level,
                                           int raw,
                                           int windowBits,
                                           int memLevel,
                                           int strategy,
                                           const char* password,
                                           uLong crcForCrypting,
                                           int zip64);

  
                                        
                                                                          
                                                       
                                                                  
   

extern int ZEXPORT zipOpenNewFileInZip4(zipFile file,
                                        const char* filename,
                                        const zip_fileinfo* zipfi,
                                        const void* extrafield_local,
                                        uInt size_extrafield_local,
                                        const void* extrafield_global,
                                        uInt size_extrafield_global,
                                        const char* comment,
                                        int method,
                                        int level,
                                        int raw,
                                        int windowBits,
                                        int memLevel,
                                        int strategy,
                                        const char* password,
                                        uLong crcForCrypting,
                                        uLong versionMadeBy,
                                        uLong flagBase);


extern int ZEXPORT zipOpenNewFileInZip4_64(zipFile file,
                                           const char* filename,
                                           const zip_fileinfo* zipfi,
                                           const void* extrafield_local,
                                           uInt size_extrafield_local,
                                           const void* extrafield_global,
                                           uInt size_extrafield_global,
                                           const char* comment,
                                           int method,
                                           int level,
                                           int raw,
                                           int windowBits,
                                           int memLevel,
                                           int strategy,
                                           const char* password,
                                           uLong crcForCrypting,
                                           uLong versionMadeBy,
                                           uLong flagBase,
                                           int zip64);
  
                                        
                                                   
                                                                      
   


extern int ZEXPORT zipWriteInFileInZip(zipFile file,
                                       const void* buf,
                                       unsigned len);
  
                           
  

extern int ZEXPORT zipCloseFileInZip(zipFile file);
  
                                       
  

extern int ZEXPORT zipCloseFileInZipRaw(zipFile file,
                                        uLong uncompressed_size,
                                        uLong crc32);

extern int ZEXPORT zipCloseFileInZipRaw64(zipFile file,
                                          ZPOS64_T uncompressed_size,
                                          uLong crc32);

  
                                                             
                                           
                                                                 
  

extern int ZEXPORT zipClose(zipFile file,
                            const char* global_comment);
  
                   
  


extern int ZEXPORT zipRemoveExtraInfoBlock(char* pData, int* dataLen, short sHeader);
  
                                                      

                                                                                                                    

                                                                                                      

                                                                       

        
                                                                                               
                                                                                                

                                                                                                
                                                                                                    
  

#ifdef __cplusplus
}
#endif

#endif  
