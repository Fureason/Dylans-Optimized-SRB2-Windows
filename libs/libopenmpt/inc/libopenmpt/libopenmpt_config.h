/*
 * libopenmpt_config.h
 * -------------------
 * Purpose: libopenmpt public interface configuration
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_CONFIG_H
#define LIBOPENMPT_CONFIG_H

 

                          
    
  

 
#define LIBOPENMPT_API
#undef LIBOPENMPT_API
#define LIBOPENMPT_CXX_API
#undef LIBOPENMPT_CXX_API

 
#define LIBOPENMPT_STREAM_CALLBACKS_BUFFER

                                                                         
             
           
                                                 
          
                                       
                                                                                                                                                                
                                                           
           
             
   
#define LIBOPENMPT_STREAM_CALLBACKS_FD

                                                                           
             
           
                                                 
          
                                       
                                                                                                                                                                  
                                                             
           
             
   
#define LIBOPENMPT_STREAM_CALLBACKS_FILE

#if defined(__DOXYGEN__)

#define LIBOPENMPT_API_HELPER_EXPORT
#define LIBOPENMPT_API_HELPER_IMPORT
#define LIBOPENMPT_API_HELPER_PUBLIC
#define LIBOPENMPT_API_HELPER_LOCAL

#elif defined(_MSC_VER)

#define LIBOPENMPT_API_HELPER_EXPORT __declspec(dllexport)
#define LIBOPENMPT_API_HELPER_IMPORT __declspec(dllimport)
#define LIBOPENMPT_API_HELPER_PUBLIC 
#define LIBOPENMPT_API_HELPER_LOCAL  

#elif defined(__EMSCRIPTEN__)

#define LIBOPENMPT_API_HELPER_EXPORT __attribute__((visibility("default"))) __attribute__((used))
#define LIBOPENMPT_API_HELPER_IMPORT __attribute__((visibility("default"))) __attribute__((used))
#define LIBOPENMPT_API_HELPER_PUBLIC __attribute__((visibility("default"))) __attribute__((used))
#define LIBOPENMPT_API_HELPER_LOCAL  __attribute__((visibility("hidden")))

#elif (defined(__GNUC__) || defined(__clang__)) && defined(_WIN32)

#define LIBOPENMPT_API_HELPER_EXPORT __declspec(dllexport)
#define LIBOPENMPT_API_HELPER_IMPORT __declspec(dllimport)
#define LIBOPENMPT_API_HELPER_PUBLIC __attribute__((visibility("default")))
#define LIBOPENMPT_API_HELPER_LOCAL  __attribute__((visibility("hidden")))

#elif defined(__GNUC__) || defined(__clang__)

#define LIBOPENMPT_API_HELPER_EXPORT __attribute__((visibility("default")))
#define LIBOPENMPT_API_HELPER_IMPORT __attribute__((visibility("default")))
#define LIBOPENMPT_API_HELPER_PUBLIC __attribute__((visibility("default")))
#define LIBOPENMPT_API_HELPER_LOCAL  __attribute__((visibility("hidden")))

#elif defined(_WIN32)

#define LIBOPENMPT_API_HELPER_EXPORT __declspec(dllexport)
#define LIBOPENMPT_API_HELPER_IMPORT __declspec(dllimport)
#define LIBOPENMPT_API_HELPER_PUBLIC 
#define LIBOPENMPT_API_HELPER_LOCAL  

#else

#define LIBOPENMPT_API_HELPER_EXPORT 
#define LIBOPENMPT_API_HELPER_IMPORT 
#define LIBOPENMPT_API_HELPER_PUBLIC 
#define LIBOPENMPT_API_HELPER_LOCAL  

#endif

#if defined(LIBOPENMPT_BUILD_DLL)
#define LIBOPENMPT_API     LIBOPENMPT_API_HELPER_EXPORT
#elif defined(LIBOPENMPT_USE_DLL)
#define LIBOPENMPT_API     LIBOPENMPT_API_HELPER_IMPORT
#else
#define LIBOPENMPT_API     LIBOPENMPT_API_HELPER_PUBLIC
#endif

#ifdef __cplusplus

#define LIBOPENMPT_CXX_API LIBOPENMPT_API

#if defined(LIBOPENMPT_USE_DLL)
#if defined(_MSC_VER) && !defined(_DLL)
#error "C++ interface is disabled if libopenmpt is built as a DLL and the runtime is statically linked. This is not supported by microsoft and cannot possibly work. Ever."
#undef LIBOPENMPT_CXX_API
#define LIBOPENMPT_CXX_API LIBOPENMPT_API_HELPER_LOCAL
#endif
#endif

#if defined(__EMSCRIPTEN__)

 
#undef LIBOPENMPT_CXX_API
#define LIBOPENMPT_CXX_API LIBOPENMPT_API_HELPER_LOCAL 
#endif

#endif

   
    
  


 

#if !defined(LIBOPENMPT_NO_DEPRECATE)
#if defined(__clang__)
#define LIBOPENMPT_DEPRECATED __attribute__((deprecated))
#elif defined(__GNUC__)
#define LIBOPENMPT_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define LIBOPENMPT_DEPRECATED __declspec(deprecated)
#else
#define LIBOPENMPT_DEPRECATED
#endif
#endif

#ifndef __cplusplus
#if !defined(LIBOPENMPT_NO_DEPRECATE)
LIBOPENMPT_DEPRECATED static const int LIBOPENMPT_DEPRECATED_STRING_CONSTANT = 0;
#define LIBOPENMPT_DEPRECATED_STRING( str ) ( LIBOPENMPT_DEPRECATED_STRING_CONSTANT ? ( str ) : ( str ) )
#else
#define LIBOPENMPT_DEPRECATED_STRING( str ) str
#endif
#endif


 

#ifdef __cplusplus

#ifndef LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED
 
#endif

#if defined(LIBOPENMPT_ASSUME_CPLUSPLUS)
#ifndef LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED
#define LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED LIBOPENMPT_ASSUME_CPLUSPLUS
#endif
#endif

#if !defined(LIBOPENMPT_NO_DEPRECATE)
#if defined(LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED)
#if (LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED >= 201402L)
#define LIBOPENMPT_ATTR_DEPRECATED [[deprecated]]
#undef LIBOPENMPT_DEPRECATED
#define LIBOPENMPT_DEPRECATED
#else
#define LIBOPENMPT_ATTR_DEPRECATED
#endif
#elif (__cplusplus >= 201402L)
#define LIBOPENMPT_ATTR_DEPRECATED [[deprecated]]
#undef LIBOPENMPT_DEPRECATED
#define LIBOPENMPT_DEPRECATED
#else
#define LIBOPENMPT_ATTR_DEPRECATED
#endif
#else
#undef LIBOPENMPT_DEPRECATED
#define LIBOPENMPT_DEPRECATED
#define LIBOPENMPT_ATTR_DEPRECATED
#endif

#endif


#include "libopenmpt_version.h"

#endif  
