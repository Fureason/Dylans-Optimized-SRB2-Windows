/* Message catalogs for internationalization.
   Copyright (C) 1995-1997, 2000-2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.  */

#ifndef _LIBINTL_H
#define _LIBINTL_H 1

#include <locale.h>
#if (defined __APPLE__ && defined __MACH__) && 0
# include <xlocale.h>
#endif

                                                                        
                                                                          
                                                                   
                                                                          
                                                                    
                                                                          
                    
#if !defined LC_MESSAGES && !(defined __LOCALE_H || (defined _LOCALE_H && defined __sun))
# define LC_MESSAGES 1729
#endif

                                                               
                                 
#define __USE_GNU_GETTEXT 1

                                                                     
                                                                          
#define __GNU_GETTEXT_SUPPORTED_REVISION(major) \
  ((major) == 0 || (major) == 1 ? 1 : -1)

                                                                    
                                      
#ifdef __DJGPP__
# undef gettext
#endif

#ifdef __cplusplus
extern "C" {
#endif


 
#define LIBINTL_VERSION 0x001201
extern int libintl_version;


                                                                        
                                                                         
                                                                       
                                                                  
                                                                       
                                                                      
                             
                          
                                                                             
                                                                         
                     
                                                                           
                   
                                                                          
         
                                                                 
                                                              
                                                                        
                                             
                                                                       
                         

                                                                           
                                                                          
                                                                           
                                      

                                                                      
                                                                   
                                                 
#if !(defined _INTL_REDIRECT_INLINE || defined _INTL_REDIRECT_MACROS)
# if defined __GNUC__ && __GNUC__ >= 2 && !(defined __APPLE_CC__ && __APPLE_CC__ > 1) && !defined __MINGW32__ && !(__GNUC__ == 2 && defined _AIX) && (defined __STDC__ || defined __cplusplus)
#  define _INTL_REDIRECT_ASM
# else
#  ifdef __cplusplus
#   define _INTL_REDIRECT_INLINE
#  else
#   define _INTL_REDIRECT_MACROS
#  endif
# endif
#endif
 
#ifdef _INTL_REDIRECT_ASM
# define _INTL_ASM(cname) __asm__ (_INTL_ASMNAME (__USER_LABEL_PREFIX__, #cname))
# define _INTL_ASMNAME(prefix,cnamestring) _INTL_STRINGIFY (prefix) cnamestring
# define _INTL_STRINGIFY(prefix) #prefix
#else
# define _INTL_ASM(cname)
#endif

                                                                             
                                                                           
                                   
#if defined __GNUC__ && __GNUC__ >= 3 && !(defined __APPLE_CC__ && __APPLE_CC__ > 1 && defined __cplusplus)
# define _INTL_MAY_RETURN_STRING_ARG(n) __attribute__ ((__format_arg__ (n)))
#else
# define _INTL_MAY_RETURN_STRING_ARG(n)
#endif

                                                                       
                                                                       
             
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_gettext (const char *__msgid)
       _INTL_MAY_RETURN_STRING_ARG (1);
static inline char *gettext (const char *__msgid)
{
  return libintl_gettext (__msgid);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define gettext libintl_gettext
#endif
extern char *gettext (const char *__msgid)
       _INTL_ASM (libintl_gettext)
       _INTL_MAY_RETURN_STRING_ARG (1);
#endif

                                                                  
                          
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_dgettext (const char *__domainname, const char *__msgid)
       _INTL_MAY_RETURN_STRING_ARG (2);
static inline char *dgettext (const char *__domainname, const char *__msgid)
{
  return libintl_dgettext (__domainname, __msgid);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define dgettext libintl_dgettext
#endif
extern char *dgettext (const char *__domainname, const char *__msgid)
       _INTL_ASM (libintl_dgettext)
       _INTL_MAY_RETURN_STRING_ARG (2);
#endif

                                                                           
              
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_dcgettext (const char *__domainname, const char *__msgid,
                                int __category)
       _INTL_MAY_RETURN_STRING_ARG (2);
static inline char *dcgettext (const char *__domainname, const char *__msgid,
                               int __category)
{
  return libintl_dcgettext (__domainname, __msgid, __category);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define dcgettext libintl_dcgettext
#endif
extern char *dcgettext (const char *__domainname, const char *__msgid,
                        int __category)
       _INTL_ASM (libintl_dcgettext)
       _INTL_MAY_RETURN_STRING_ARG (2);
#endif


                                                                       
                
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_ngettext (const char *__msgid1, const char *__msgid2,
                               unsigned long int __n)
       _INTL_MAY_RETURN_STRING_ARG (1) _INTL_MAY_RETURN_STRING_ARG (2);
static inline char *ngettext (const char *__msgid1, const char *__msgid2,
                              unsigned long int __n)
{
  return libintl_ngettext (__msgid1, __msgid2, __n);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define ngettext libintl_ngettext
#endif
extern char *ngettext (const char *__msgid1, const char *__msgid2,
                       unsigned long int __n)
       _INTL_ASM (libintl_ngettext)
       _INTL_MAY_RETURN_STRING_ARG (1) _INTL_MAY_RETURN_STRING_ARG (2);
#endif

                                                                        
                
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_dngettext (const char *__domainname, const char *__msgid1,
                                const char *__msgid2, unsigned long int __n)
       _INTL_MAY_RETURN_STRING_ARG (2) _INTL_MAY_RETURN_STRING_ARG (3);
static inline char *dngettext (const char *__domainname, const char *__msgid1,
                               const char *__msgid2, unsigned long int __n)
{
  return libintl_dngettext (__domainname, __msgid1, __msgid2, __n);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define dngettext libintl_dngettext
#endif
extern char *dngettext (const char *__domainname,
                        const char *__msgid1, const char *__msgid2,
                        unsigned long int __n)
       _INTL_ASM (libintl_dngettext)
       _INTL_MAY_RETURN_STRING_ARG (2) _INTL_MAY_RETURN_STRING_ARG (3);
#endif

                                                                         
                
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_dcngettext (const char *__domainname,
                                 const char *__msgid1, const char *__msgid2,
                                 unsigned long int __n, int __category)
       _INTL_MAY_RETURN_STRING_ARG (2) _INTL_MAY_RETURN_STRING_ARG (3);
static inline char *dcngettext (const char *__domainname,
                                const char *__msgid1, const char *__msgid2,
                                unsigned long int __n, int __category)
{
  return libintl_dcngettext (__domainname, __msgid1, __msgid2, __n, __category);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define dcngettext libintl_dcngettext
#endif
extern char *dcngettext (const char *__domainname,
                         const char *__msgid1, const char *__msgid2,
                         unsigned long int __n, int __category)
       _INTL_ASM (libintl_dcngettext)
       _INTL_MAY_RETURN_STRING_ARG (2) _INTL_MAY_RETURN_STRING_ARG (3);
#endif



                                                         
                                                     
                                                               
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_textdomain (const char *__domainname);
static inline char *textdomain (const char *__domainname)
{
  return libintl_textdomain (__domainname);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define textdomain libintl_textdomain
#endif
extern char *textdomain (const char *__domainname)
       _INTL_ASM (libintl_textdomain);
#endif

                                                            
                                                             
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_bindtextdomain (const char *__domainname,
                                     const char *__dirname);
static inline char *bindtextdomain (const char *__domainname,
                                    const char *__dirname)
{
  return libintl_bindtextdomain (__domainname, __dirname);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define bindtextdomain libintl_bindtextdomain
#endif
extern char *bindtextdomain (const char *__domainname, const char *__dirname)
       _INTL_ASM (libintl_bindtextdomain);
#endif

                                                                
                                                   
#ifdef _INTL_REDIRECT_INLINE
extern char *libintl_bind_textdomain_codeset (const char *__domainname,
                                              const char *__codeset);
static inline char *bind_textdomain_codeset (const char *__domainname,
                                             const char *__codeset)
{
  return libintl_bind_textdomain_codeset (__domainname, __codeset);
}
#else
#ifdef _INTL_REDIRECT_MACROS
# define bind_textdomain_codeset libintl_bind_textdomain_codeset
#endif
extern char *bind_textdomain_codeset (const char *__domainname,
                                      const char *__codeset)
       _INTL_ASM (libintl_bind_textdomain_codeset);
#endif



                                                                        
                           
                                                                        
                                                                     
                                                                      
                                                                         
                                                               

#if !0

#include <stdio.h>
#include <stddef.h>

 
#if (defined __STDC__ && __STDC__) || defined __cplusplus || defined _MSC_VER
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#if !(defined fprintf && defined _GL_STDIO_H)  
#undef fprintf
#define fprintf libintl_fprintf
extern int fprintf (FILE *, const char *, ...);
#endif
#if !(defined vfprintf && defined _GL_STDIO_H)  
#undef vfprintf
#define vfprintf libintl_vfprintf
extern int vfprintf (FILE *, const char *, va_list);
#endif

#if !(defined printf && defined _GL_STDIO_H)  
#undef printf
#if defined __NetBSD__ || defined __BEOS__ || defined __CYGWIN__ || defined __MINGW32__
                                                   
                                                                         
                                             
                                                                              
                                                
                                            
                                                                
                                                                         
                                                        
# define libintl_printf __printf__
#endif
#define printf libintl_printf
extern int printf (const char *, ...);
#endif
#if !(defined vprintf && defined _GL_STDIO_H)  
#undef vprintf
#define vprintf libintl_vprintf
extern int vprintf (const char *, va_list);
#endif

#if !(defined sprintf && defined _GL_STDIO_H)  
#undef sprintf
#define sprintf libintl_sprintf
extern int sprintf (char *, const char *, ...);
#endif
#if !(defined vsprintf && defined _GL_STDIO_H)  
#undef vsprintf
#define vsprintf libintl_vsprintf
extern int vsprintf (char *, const char *, va_list);
#endif

#if 1

#if !(defined snprintf && defined _GL_STDIO_H)  
#undef snprintf
#define snprintf libintl_snprintf
extern int snprintf (char *, size_t, const char *, ...);
#endif
#if !(defined vsnprintf && defined _GL_STDIO_H)  
#undef vsnprintf
#define vsnprintf libintl_vsnprintf
extern int vsnprintf (char *, size_t, const char *, va_list);
#endif

#endif

#if 0

#if !(defined asprintf && defined _GL_STDIO_H)  
#undef asprintf
#define asprintf libintl_asprintf
extern int asprintf (char **, const char *, ...);
#endif
#if !(defined vasprintf && defined _GL_STDIO_H)  
#undef vasprintf
#define vasprintf libintl_vasprintf
extern int vasprintf (char **, const char *, va_list);
#endif

#endif

#if 0

#undef fwprintf
#define fwprintf libintl_fwprintf
extern int fwprintf (FILE *, const wchar_t *, ...);
#undef vfwprintf
#define vfwprintf libintl_vfwprintf
extern int vfwprintf (FILE *, const wchar_t *, va_list);

#undef wprintf
#define wprintf libintl_wprintf
extern int wprintf (const wchar_t *, ...);
#undef vwprintf
#define vwprintf libintl_vwprintf
extern int vwprintf (const wchar_t *, va_list);

#undef swprintf
#define swprintf libintl_swprintf
extern int swprintf (wchar_t *, size_t, const wchar_t *, ...);
#undef vswprintf
#define vswprintf libintl_vswprintf
extern int vswprintf (wchar_t *, size_t, const wchar_t *, va_list);

#endif

#endif


 
#if (defined __APPLE__ && defined __MACH__) || defined _WIN32 || defined __WIN32__ || defined __CYGWIN__

#undef setlocale
#define setlocale libintl_setlocale
extern char *setlocale (int, const char *);

#if 0

#undef newlocale
#define newlocale libintl_newlocale
extern locale_t newlocale (int, const char *, locale_t);

#endif

#endif


 

                                                                        
                                                                          
                                                                       
                                                                         
                       
#define libintl_set_relocation_prefix libintl_set_relocation_prefix
extern void
       libintl_set_relocation_prefix (const char *orig_prefix,
                                      const char *curr_prefix);


#ifdef __cplusplus
}
#endif

#endif  
