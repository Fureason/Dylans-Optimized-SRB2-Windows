/* infback9.h -- header for using inflateBack9 functions
 * Copyright (C) 2003 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

  
                                                                         
                                                                              
                                                                           
                                                                        
                                                                      
                                                                        
                                                                       
                                                                       
                                                                    
                                                   
   

#ifdef __cplusplus
extern "C" {
#endif

ZEXTERN int ZEXPORT inflateBack9(z_stream FAR *strm,
                                 in_func in, void FAR *in_desc,
                                 out_func out, void FAR *out_desc);
ZEXTERN int ZEXPORT inflateBack9End(z_stream FAR *strm);
ZEXTERN int ZEXPORT inflateBack9Init_(z_stream FAR *strm,
                                      unsigned char FAR *window,
                                      const char *version,
                                      int stream_size);
#define inflateBack9Init(strm, window) \
        inflateBack9Init_((strm), (window), \
        ZLIB_VERSION, sizeof(z_stream))

#ifdef __cplusplus
}
#endif
