/* Class autosprintf - formatted output to an ostream.
   Copyright (C) 2002 Free Software Foundation, Inc.

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

#ifndef _AUTOSPRINTF_H
#define _AUTOSPRINTF_H

#ifndef __attribute__
 
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(Spec)  
# endif
                                                                
                                                                      
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

#include <string>
#include <iostream>

namespace gnu
{
                                                                     
                                          
  class autosprintf
  {
  public:
     
    autosprintf (const char *format, ...)
                __attribute__ ((__format__ (__printf__, 2, 3)));
     
    autosprintf (const autosprintf& src);
     
    ~autosprintf ();
     
    operator char * () const;
    operator std::string () const;
     
    friend inline std::ostream& operator<< (std::ostream& stream, const autosprintf& tmp)
    {
      stream << (tmp.str ? tmp.str : "(error in autosprintf)");
      return stream;
    }
  private:
    char *str;
  };
}

#endif  
