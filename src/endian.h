// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __ENDIAN__
#define __ENDIAN__

#if defined(SRB2_BIG_ENDIAN) || defined(SRB2_LITTLE_ENDIAN)

#else
  #if defined(__FreeBSD__)


    #include <sys/endian.h>
    #if _BYTE_ORDER == _BIG_ENDIAN
      #define SRB2_BIG_ENDIAN
    #else
      #define SRB2_LITTLE_ENDIAN
    #endif
  #elif defined(__BYTE_ORDER__)

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define SRB2_BIG_ENDIAN
    #else
      #define SRB2_LITTLE_ENDIAN
    #endif
  #else


    #if defined(_BIG_ENDIAN)
      #define SRB2_BIG_ENDIAN
    #else
      #define SRB2_LITTLE_ENDIAN
    #endif
  #endif
#endif

#endif  
