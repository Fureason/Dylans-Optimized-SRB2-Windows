/* Public API for GNU gettext PO files - contained in libgettextpo.
   Copyright (C) 2003-2008, 2010 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _GETTEXT_PO_H
#define _GETTEXT_PO_H 1

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


 

 
#define LIBGETTEXTPO_VERSION 0x001201
extern __declspec (dllimport) int libgettextpo_version;

 

 
typedef struct po_file *po_file_t;

                                                                       
               
typedef struct po_message_iterator *po_message_iterator_t;

 
typedef struct po_message *po_message_t;

 
typedef struct po_filepos *po_filepos_t;

 
struct po_error_handler
{
                                                                               
                                                       
                                                                         
                                              
  void (*error) (int status, int errnum,
                 const char *format, ...)
#if ((__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || __GNUC__ > 3) && !__STRICT_ANSI__
  __attribute__ ((__format__ (__printf__, 3, 4)))
#endif
  ;

                                                                               
                                                                          
                                
                                                                         
                                              
  void (*error_at_line) (int status, int errnum,
                         const char *filename, unsigned int lineno,
                         const char *format, ...)
#if ((__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || __GNUC__ > 3) && !__STRICT_ANSI__
  __attribute__ ((__format__ (__printf__, 5, 6)))
#endif
  ;

                                                                        
                                                         
  void (*multiline_warning) (char *prefix, char *message);

                                                                      
                                                     
                                                                           
                            
  void (*multiline_error) (char *prefix, char *message);
};
typedef const struct po_error_handler *po_error_handler_t;

 
#define PO_SEVERITY_WARNING     0  
#define PO_SEVERITY_ERROR       1  
#define PO_SEVERITY_FATAL_ERROR 2  
struct po_xerror_handler
{
                                            
                                                                          
                                                                           
                                                                    
                                                     
                                                                     
                                                                      
                            
                                                                
  void (*xerror) (int severity,
                  po_message_t message,
                  const char *filename, size_t lineno, size_t column,
                  int multiline_p, const char *message_text);
                                                  
                                    
                                                                           
                       
  void (*xerror2) (int severity,
                   po_message_t message1,
                   const char *filename1, size_t lineno1, size_t column1,
                   int multiline_p1, const char *message_text1,
                   po_message_t message2,
                   const char *filename2, size_t lineno2, size_t column2,
                   int multiline_p2, const char *message_text2);
};
typedef const struct po_xerror_handler *po_xerror_handler_t;

                     
                                                                     
                                                              
                                                                        
                                                                       


 

 
extern po_file_t po_file_create (void);

                              
                                                                     
#define po_file_read po_file_read_v3
extern po_file_t po_file_read (const char *filename,
                               po_xerror_handler_t handler);

                                        
                                               
#define po_file_write po_file_write_v2
extern po_file_t po_file_write (po_file_t file, const char *filename,
                                po_xerror_handler_t handler);

 
extern void po_file_free (po_file_t file);

 
extern const char * const * po_file_domains (po_file_t file);


 

                                                              
                                              
                                               
extern const char * po_file_domain_header (po_file_t file, const char *domain);

                                                 
                                                                            
                       
extern char * po_header_field (const char *header, const char *field);

                                                                              
                         
                                                      
extern char * po_header_set_field (const char *header, const char *field, const char *value);


 

                                                                     
                                                  
extern po_message_iterator_t po_message_iterator (po_file_t file, const char *domain);

 
extern void po_message_iterator_free (po_message_iterator_t iterator);

                                                     
                                                  
extern po_message_t po_next_message (po_message_iterator_t iterator);

                                                                             
                                                                             
                        
extern void po_message_insert (po_message_iterator_t iterator, po_message_t message);


 

                                        
                                                                             
extern po_message_t po_message_create (void);

                                                                             
               
extern const char * po_message_msgctxt (po_message_t message);

                                                                            
               
extern void po_message_set_msgctxt (po_message_t message, const char *msgctxt);

 
extern const char * po_message_msgid (po_message_t message);

 
extern void po_message_set_msgid (po_message_t message, const char *msgid);

                                                                             
                                            
extern const char * po_message_msgid_plural (po_message_t message);

                                                                             
                                           
extern void po_message_set_msgid_plural (po_message_t message, const char *msgid_plural);

                                                
                                                           
extern const char * po_message_msgstr (po_message_t message);

                                                
                                                             
extern void po_message_set_msgstr (po_message_t message, const char *msgstr);

                                                                  
                                                                           
extern const char * po_message_msgstr_plural (po_message_t message, int index);

                                                               
                                                                        
extern void po_message_set_msgstr_plural (po_message_t message, int index, const char *msgstr);

 
extern const char * po_message_comments (po_message_t message);

                                     
                                                                            
extern void po_message_set_comments (po_message_t message, const char *comments);

 
extern const char * po_message_extracted_comments (po_message_t message);

                                               
                                                                            
extern void po_message_set_extracted_comments (po_message_t message, const char *comments);

                                                                      
             
extern po_filepos_t po_message_filepos (po_message_t message, int i);

                                                
                                                                              
              
extern void po_message_remove_filepos (po_message_t message, int i);

                                                                         
           
                         
                                                                               
                                
extern void po_message_add_filepos (po_message_t message, const char *file, size_t start_line);

 
extern const char * po_message_prev_msgctxt (po_message_t message);

 
extern void po_message_set_prev_msgctxt (po_message_t message, const char *prev_msgctxt);

                                                                           
                     
extern const char * po_message_prev_msgid (po_message_t message);

                                                                        
                       
extern void po_message_set_prev_msgid (po_message_t message, const char *prev_msgid);

                                                                             
                                 
extern const char * po_message_prev_msgid_plural (po_message_t message);

                                                                             
                                 
extern void po_message_set_prev_msgid_plural (po_message_t message, const char *prev_msgid_plural);

 
extern int po_message_is_obsolete (po_message_t message);

 
extern void po_message_set_obsolete (po_message_t message, int obsolete);

 
extern int po_message_is_fuzzy (po_message_t message);

 
extern void po_message_set_fuzzy (po_message_t message, int fuzzy);

                                                                             
                              
extern int po_message_is_format (po_message_t message, const char *format_type);

 
extern void po_message_set_format (po_message_t message, const char *format_type,  int value);

                                                                            
                                            
extern int po_message_is_range (po_message_t message, int *minp, int *maxp);

                                                                            
                                                                       
               
extern void po_message_set_range (po_message_t message, int min, int max);


 

 
extern const char * po_filepos_file (po_filepos_t filepos);

                                                                             
                           
extern size_t po_filepos_start_line (po_filepos_t filepos);


 

 
extern const char * const * po_format_list (void);

                                                        
                                                 
                                                                  
extern const char * po_format_pretty_name (const char *format_type);


 

                                                                     
                                                         
extern void po_file_check_all (po_file_t file, po_xerror_handler_t handler);

                                                                            
                                                                             
                                                                               
extern void po_message_check_all (po_message_t message, po_message_iterator_t iterator, po_xerror_handler_t handler);

                                                                               
                                                                             
                   
#define po_message_check_format po_message_check_format_v2
extern void po_message_check_format (po_message_t message, po_xerror_handler_t handler);


#ifdef __cplusplus
}
#endif

#endif  
