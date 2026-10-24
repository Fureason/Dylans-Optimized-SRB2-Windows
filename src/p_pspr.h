// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------






#ifndef __P_PSPR__
#define __P_PSPR__



#include "m_fixed.h"
#include "tables.h"








#include "info.h"

#ifdef __GNUG__
#pragma interface
#endif


#define FF_FRAMEMASK 0xff


#define FF_SPR2SUPER SPR2F_SUPER  

#define FF_SPR2ENDSTATE 0x100

#define FF_SPR2MIDSTART 0x200


#define FF_BLENDMASK 0x7000

#define FF_BLENDSHIFT 12

#define FF_ADD             ((AST_ADD-1)<<FF_BLENDSHIFT)
#define FF_SUBTRACT        ((AST_SUBTRACT-1)<<FF_BLENDSHIFT)
#define FF_REVERSESUBTRACT ((AST_REVERSESUBTRACT-1)<<FF_BLENDSHIFT)
#define FF_MODULATE        ((AST_MODULATE-1)<<FF_BLENDSHIFT)
#define FF_OVERLAY         ((AST_OVERLAY-1)<<FF_BLENDSHIFT)


#define FF_TRANSMASK 0xf0000

#define FF_TRANSSHIFT 16

#define FF_TRANS10 (tr_trans10<<FF_TRANSSHIFT)
#define FF_TRANS20 (tr_trans20<<FF_TRANSSHIFT)
#define FF_TRANS30 (tr_trans30<<FF_TRANSSHIFT)
#define FF_TRANS40 (tr_trans40<<FF_TRANSSHIFT)
#define FF_TRANS50 (tr_trans50<<FF_TRANSSHIFT)
#define FF_TRANS60 (tr_trans60<<FF_TRANSSHIFT)
#define FF_TRANS70 (tr_trans70<<FF_TRANSSHIFT)
#define FF_TRANS80 (tr_trans80<<FF_TRANSSHIFT)
#define FF_TRANS90 (tr_trans90<<FF_TRANSSHIFT)


#define FF_BRIGHTMASK	0x00300000

#define FF_FULLBRIGHT	0x00100000

#define FF_FULLDARK		0x00200000

#define FF_SEMIBRIGHT	(FF_FULLBRIGHT|FF_FULLDARK)


#define FF_PAPERSPRITE 0x00400000

#define FF_FLOORSPRITE 0x00800000


#define FF_VERTICALFLIP 0x01000000

#define FF_HORIZONTALFLIP 0x02000000


#define FF_ANIMATE 0x10000000

#define FF_GLOBALANIM 0x20000000

#define FF_RANDOMANIM 0x40000000


#define SPR2F_MASK 0x3FF

#define SPR2F_SUPER 0x400

                              

                                                                     
                                                               
                                                                   

                                                                
                                      
  
typedef enum
{
	tr_trans10 = 1,
	tr_trans20,
	tr_trans30,
	tr_trans40,
	tr_trans50,
	tr_trans60,
	tr_trans70,
	tr_trans80,
	tr_trans90,
	NUMTRANSMAPS
} transnum_t;

#endif
