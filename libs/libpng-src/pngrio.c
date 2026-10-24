
/* pngrio.c - functions for data input
 *
 * Copyright (c) 2018 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2016,2018 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * This file provides a location for all input.  Users who need
 * special handling are expected to write a function that has the same
 * arguments as this and performs a similar function, but that possibly
 * has a different input method.  Note that you shouldn't change this
 * function, but rather write a replacement function and then make
 * libpng use it at run time with png_set_read_fn(...).
 */

#include "pngpriv.h"

#ifdef PNG_READ_SUPPORTED

                                                                        
                                                                           
                                                                       
                                                                           
                                             
   
void  
png_read_data(png_structrp png_ptr, png_bytep data, size_t length)
{
   png_debug1(4, "reading %d bytes", (int)length);

   if (png_ptr->read_data_fn != NULL)
      (*(png_ptr->read_data_fn))(png_ptr, data, length);

   else
      png_error(png_ptr, "Call to NULL read function");
}

#ifdef PNG_STDIO_SUPPORTED
                                                                         
                                                                        
                                                                           
                             
   
void PNGCBAPI
png_default_read_data(png_structp png_ptr, png_bytep data, size_t length)
{
   size_t check;

   if (png_ptr == NULL)
      return;

                                                                       
                                                                
      
   check = fread(data, 1, length, png_voidcast(png_FILE_p, png_ptr->io_ptr));

   if (check != length)
      png_error(png_ptr, "Read Error");
}
#endif

                                                                      
                                                      
  
                                        
  
                                                       
  
                                                                          
                                                    
  
                                                                   
                                                                   
                                                                         
                                                                      
                                                                           
                                                                       
                                                                           
                          
   
void PNGAPI
png_set_read_fn(png_structrp png_ptr, png_voidp io_ptr,
    png_rw_ptr read_data_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->io_ptr = io_ptr;

#ifdef PNG_STDIO_SUPPORTED
   if (read_data_fn != NULL)
      png_ptr->read_data_fn = read_data_fn;

   else
      png_ptr->read_data_fn = png_default_read_data;
#else
   png_ptr->read_data_fn = read_data_fn;
#endif

#ifdef PNG_WRITE_SUPPORTED
    
   if (png_ptr->write_data_fn != NULL)
   {
      png_ptr->write_data_fn = NULL;
      png_warning(png_ptr,
          "Can't set both read_data_fn and write_data_fn in the"
          " same structure");
   }
#endif

#ifdef PNG_WRITE_FLUSH_SUPPORTED
   png_ptr->output_flush_fn = NULL;
#endif
}
#endif  
