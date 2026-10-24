// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomtype.h"
#include "m_bbox.h"
                                  

                                              

             


  

void M_ClearBox(fixed_t *box)
{
	box[BOXTOP] = box[BOXRIGHT] = INT32_MIN;
	box[BOXBOTTOM] = box[BOXLEFT] = INT32_MAX;
}
                                  

                                                 
                  
                  

             


  

void M_AddToBox(fixed_t *box, fixed_t x, fixed_t y)
{
	if (x < box[BOXLEFT])
		box[BOXLEFT] = x;
	if (x > box[BOXRIGHT])
		box[BOXRIGHT] = x;

	if (y < box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
	if (y > box[BOXTOP])
		box[BOXTOP] = y;
}
                                    

                                                   
                  
                  

                           


  

boolean M_PointInBox(fixed_t *box, fixed_t x, fixed_t y)
{
	if (x < box[BOXLEFT])
		return false;
	if (x > box[BOXRIGHT])
		return false;
	if (y < box[BOXBOTTOM])
		return false;
	if (y > box[BOXTOP])
		return false;

	return true;
}
                                        

                                         
                                           
                                           
                                                

                


  

boolean M_CircleTouchBox(fixed_t *box, fixed_t circlex, fixed_t circley, fixed_t circleradius)
{
	if (box[BOXLEFT] - circleradius > circlex)
		return false;
	if (box[BOXRIGHT] + circleradius < circlex)
		return false;
	if (box[BOXBOTTOM] - circleradius > circley)
		return false;
	if (box[BOXTOP] + circleradius < circley)
		return false;
	return true;
}
