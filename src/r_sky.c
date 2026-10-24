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







#include "doomdef.h"
#include "doomstat.h"
#include "r_sky.h"
#include "r_local.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_maputl.h"  





                                                            
                                                         
  
INT32 skyflatnum;

                                             
  
INT32 skytexture;

                                                    
  
INT32 skytexturemid;

                               
  
fixed_t skyscale;

                                                    
  
INT32 levelskynum;
INT32 globallevelskynum;

                                      

                                                                          

                                                             
                                                       

             
  
void R_SetupSkyDraw(void)
{

	skytexturemid = (textures[skytexture]->height/2)<<FRACBITS;

	R_SetSkyScale();
}

                                     

                                                 

             
  
void R_SetSkyScale(void)
{
	skyscale = FixedDiv(fovtan, FixedDiv(vid.width*FRACUNIT, BASEVIDWIDTH*FRACUNIT));
}
