// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_light.h"
#include "hw_drv.h"
#include "../i_video.h"
#include "../z_zone.h"
#include "../m_random.h"
#include "../m_bbox.h"
#include "../w_wad.h"
#include "../r_state.h"
#include "../r_main.h"
#include "../p_local.h"





#define DL_SQRRADIUS(x) dynlights->p_lspr[(x)]->dynamic_sqrradius
#define DL_RADIUS(x) dynlights->p_lspr[(x)]->dynamic_radius
#define LIGHT_POS(i) dynlights->position[(i)]

#define DL_HIGH_QUALITY

#define LIGHTMAPFLAGS (PF_Modulated|PF_Additive)

#ifdef ALAM_LIGHTING
static dynlights_t view_dynlights[2];  
static dynlights_t *dynlights = &view_dynlights[0];
#endif

#define UNDEFINED_SPR   0x0  
#define CORONA_SPR      0x1  
#define DYNLIGHT_SPR    0x2  
#define LIGHT_SPR       (DYNLIGHT_SPR|CORONA_SPR)
#define ROCKET_SPR      (DYNLIGHT_SPR|CORONA_SPR|0x10)





light_t lspr[NUMLIGHTS] =
{


	{ UNDEFINED_SPR,  0.0f,   0.0f, 0x00000000,  24.0f, 0x00000000,   0.0f, 0.0f},


	{ LIGHT_SPR,      0.0f,   0.0f, 0x0000e0ff,  16.0f, 0x0000e0ff,  32.0f, 0.0f},  

	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0xff00e0ff,  32.0f, 0xff00e0ff, 128.0f, 0.0f},  

	{ LIGHT_SPR,      0.0f,   0.0f, 0xe0ffffff,   8.0f, 0xe0ffffff,  64.0f, 0.0f},

	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x10ffaaaa,  16.0f, 0x10ffaaaa, 128.0f, 0.0f},

	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x602b7337,  32.0f, 0x602b7337, 128.0f, 0.0f},

	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60cb0000,  32.0f, 0x60cb0000, 128.0f, 0.0f},



	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x601f7baf,  32.0f, 0x601f7baf, 128.0f, 0.0f},


	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x600000cb,  32.0f, 0x600000cb, 128.0f, 0.0f},


	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60010101,  32.0f, 0x60ff00ff, 128.0f, 0.0f},


	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60ffffff,  32.0f, 0x60ffffff, 128.0f, 0.0f},


	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x606060f0,   0.0f, 0x302070ff,  32.0f, 0.0f},



	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60b0f0f0,   0.0f, 0x30b0f0f0, 100.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,  14.0f, 0x6070ff70,  60.0f, 0x4070ff70, 100.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,  14.0f, 0x705070ff,  60.0f, 0x405070ff, 100.0f, 0.0f},



	{    LIGHT_SPR,   0.0f,   0.0f, 0xff00ff00,  64.0f, 0xff00ff00, 256.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,   0.0f, 0xff0080ff,  64.0f, 0xff0080ff, 256.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,   0.0f, 0xffe080ff,  64.0f, 0xffe080ff, 256.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,   0.0f, 0xffff0000,  64.0f, 0xffff0000, 256.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,   0.0f, 0xff0000ff,  64.0f, 0xff0000ff, 256.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,   0.0f, 0xffff8080,  64.0f, 0xffff8080, 256.0f, 0.0f},

	{    LIGHT_SPR,   0.0f,   0.0f, 0xffe0e0e0,  64.0f, 0xffe0e0e0, 256.0f, 0.0f},



	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x606060ff,   0.0f, 0x606060ff, 100.0f, 0.0f},

	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x6060ff60, 120.0f, 0x6060ff60, 100.0f, 0.0f},

	{ DYNLIGHT_SPR,   0.0f,   0.0f, 0x60ff6060, 120.0f, 0x60ff6060, 100.0f, 0.0f},


	{    LIGHT_SPR,   0.0f,   6.0f, 0x60ffffff,  16.0f, 0x30ffffff,  32.0f, 0.0f},


	{ DYNLIGHT_SPR,   0.0f,   6.0f, 0x60ffaaaa,  16.0f, 0x30ffaaaa,  64.0f, 0.0f},


	{ DYNLIGHT_SPR,   0.0f,   6.0f, 0x60ff00ff,  16.0f, 0x30ff00ff,  32.0f, 0.0f},


	{ LIGHT_SPR,      0.0f,   0.0f, 0xe0ffffff,  64.0f, 0xe0ffffff, 384.0f, 0.0f},
};

light_t *t_lspr[NUMSPRITES] =
{
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],      
	&lspr[SUPERSONIC_L], 


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[JETLIGHT_L],   
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[JETLIGHT_L],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[LBLUESHINE_L], 


	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],       
	&lspr[NOLIGHT],       
	&lspr[NOLIGHT],       
	&lspr[INVINCIBLE_L],  
	&lspr[INVINCIBLE_L],  
	&lspr[NOLIGHT],       
	&lspr[NOLIGHT],       
	&lspr[REDBALL_L],     
	&lspr[NOLIGHT],       


	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[BLUEBALL_L],   
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[SMALLREDBALL_L],  
	&lspr[REDSHINE_L],   
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[SMALLREDBALL_L],  
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[REDBALL_L],    
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      






	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[RINGLIGHT_L],  
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[GREENBALL_L],  


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[INVINCIBLE_L],  
	&lspr[SUPERSPARK_L],  

	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[RINGLIGHT_L],  
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[SUPERSPARK_L],  
	&lspr[NIGHTSLIGHT_L],  
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      

	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      

	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      

	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      
	&lspr[RINGLIGHT_L],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],      
	&lspr[SMALLREDBALL_L],  
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[REDBALL_L],    
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[SUPERSONIC_L],  
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[INVINCIBLE_L],  
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[BLUEBALL_L],   
	&lspr[NOLIGHT],      
	&lspr[BLUEBALL_L],   
	&lspr[BLUEBALL_L],   
	&lspr[NOLIGHT],      


	&lspr[RINGSPARK_L],   
	&lspr[NOLIGHT],       
	&lspr[SUPERSPARK_L],  
	&lspr[SUPERSPARK_L],  
	&lspr[NOLIGHT],       
	&lspr[REDBALL_L],     


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  
	&lspr[NOLIGHT],  


	&lspr[NOLIGHT],      
	&lspr[NOLIGHT],      


	&lspr[NOLIGHT],		 


	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
	&lspr[NOLIGHT],
};

#ifdef ALAM_LIGHTING





static void HWR_SetLight(void);






static float HWR_DistP2D(FOutVector *p1, FOutVector *p2, FVector *p3, FVector *inter)
{
	if (p1->z == p2->z)
	{
		inter->x = p3->x;
		inter->z = p1->z;
	}
	else if (p1->x == p2->x)
	{
		inter->x = p1->x;
		inter->z = p3->z;
	}
	else
	{
		register float local, pente;

		pente = (p1->z-p2->z) / (p1->x-p2->x);
		local = p1->z - p1->x*pente;
		inter->x = (p3->z - local + p3->x/pente) * (pente/(pente*pente+1));
		inter->z = inter->x*pente + local;
	}

	return (p3->x-inter->x)*(p3->x-inter->x) + (p3->z-inter->z)*(p3->z-inter->z);
}


static boolean SphereTouchBBox3D(FOutVector *p1, FOutVector *p2, FVector *p3, float r)
{
	float minx = p1->x,maxx = p2->x,miny = p2->y,maxy = p1->y,minz = p2->z,maxz = p1->z;

	if (minx > maxx)
	{
		minx = maxx;
		maxx = p1->x;
	}
	if (miny > maxy)
	{
		miny = maxy;
		maxy = p2->y;
	}
	if (minz > maxz)
	{
		minz = maxz;
		maxz = p2->z;
	}

	if (minx-r > p3->x) return false;
	if (maxx+r < p3->x) return false;
	if (miny-r > p3->y) return false;
	if (maxy+r < p3->y) return false;
	if (minz-r > p3->z) return false;
	if (maxz+r < p3->z) return false;
	return true;
}








void HWR_WallLighting(FOutVector *wlVerts)
{
	int             i, j;


	for (j = 0; j < dynlights->nb; j++)
	{
		FVector         inter;
		FSurfaceInfo    Surf;
		float           dist_p2d, d[4], s;

		if (!dynlights->mo[j])
			continue;
		if (P_MobjWasRemoved(dynlights->mo[j]))
		{
			P_SetTarget(&dynlights->mo[j], NULL);
			continue;
		}


		if (SphereTouchBBox3D(&wlVerts[2], &wlVerts[0], &LIGHT_POS(j), DL_RADIUS(j))==false)
			continue;
		d[0] = wlVerts[2].x - wlVerts[0].x;
		d[1] = wlVerts[2].z - wlVerts[0].z;
		d[2] = LIGHT_POS(j).x - wlVerts[0].x;
		d[3] = LIGHT_POS(j).z - wlVerts[0].z;

		if (d[2]*d[1] - d[3]*d[0] < 0)
			continue;

		dist_p2d = HWR_DistP2D(&wlVerts[2], &wlVerts[0],  &LIGHT_POS(j), &inter);
		if (dist_p2d >= DL_SQRRADIUS(j))
			continue;

		d[0] = (float)sqrt((wlVerts[0].x-inter.x)*(wlVerts[0].x-inter.x)
			+ (wlVerts[0].z-inter.z)*(wlVerts[0].z-inter.z));
		d[1] = (float)sqrt((wlVerts[2].x-inter.x)*(wlVerts[2].x-inter.x)
			+ (wlVerts[2].z-inter.z)*(wlVerts[2].z-inter.z));






		if ((wlVerts[0].x < inter.x && wlVerts[2].x > inter.x) ||
		     (wlVerts[0].x > inter.x && wlVerts[2].x < inter.x) ||
		     (wlVerts[0].z < inter.z && wlVerts[2].z > inter.z) ||
		     (wlVerts[0].z > inter.z && wlVerts[2].z < inter.z))
		{
			d[0] = -d[0];  
		}
		d[2] = d[1]; d[3] = d[0];
#ifdef DL_HIGH_QUALITY
		s = 0.5f / DL_RADIUS(j);
#else
		s = 0.5f / sqrtf(DL_SQRRADIUS(j)-dist_p2d);
#endif
		for (i = 0; i < 4; i++)
		{
			wlVerts[i].s = (float)(0.5f + d[i]*s);
			wlVerts[i].t = (float)(0.5f + (wlVerts[i].y-LIGHT_POS(j).y)*s*1.2f);
		}

		HWR_SetLight();

		Surf.PolyColor.rgba = LONG(dynlights->p_lspr[j]->dynamic_color);
#ifdef DL_HIGH_QUALITY
		Surf.PolyColor.s.alpha = (UINT8)((1-dist_p2d/DL_SQRRADIUS(j))*Surf.PolyColor.s.alpha);
#endif

		if (dynlights->mo[j]->state->nextstate == S_NULL)
			Surf.PolyColor.s.alpha = (UINT8)(((float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics)*Surf.PolyColor.s.alpha);

		HWD.pfnDrawPolygon (&Surf, wlVerts, 4, LIGHTMAPFLAGS);

	}  
}





void HWR_PlaneLighting(FOutVector *clVerts, int nrClipVerts)
{
	int     i, j;
	FOutVector p1,p2;

	p1.z = FIXED_TO_FLOAT(hwbbox[BOXTOP   ]);
	p1.x = FIXED_TO_FLOAT(hwbbox[BOXLEFT  ]);
	p2.z = FIXED_TO_FLOAT(hwbbox[BOXBOTTOM]);
	p2.x = FIXED_TO_FLOAT(hwbbox[BOXRIGHT ]);
	p2.y = clVerts[0].y;
	p1.y = clVerts[0].y;

	for (j = 0; j < dynlights->nb; j++)
	{
		FSurfaceInfo    Surf;
		float           dist_p2d, s;

		if (!dynlights->mo[j])
			continue;
		if (P_MobjWasRemoved(dynlights->mo[j]))
		{
			P_SetTarget(&dynlights->mo[j], NULL);
			continue;
		}


		if (SphereTouchBBox3D(&p1, &p2, &dynlights->position[j], DL_RADIUS(j))==false)
			continue;


		if ((clVerts[0].y > atransform.z)        
			 ^ (LIGHT_POS(j).y < clVerts[0].y))  
			 continue;
		dist_p2d = (clVerts[0].y-LIGHT_POS(j).y);
		dist_p2d *= dist_p2d;




#ifdef DL_HIGH_QUALITY
		s = 0.5f / DL_RADIUS(j);
#else
		s = 0.5f / sqrtf(DL_SQRRADIUS(j)-dist_p2d);
#endif
		for (i = 0; i < nrClipVerts; i++)
		{
			clVerts[i].s = 0.5f + (clVerts[i].x-LIGHT_POS(j).x)*s;
			clVerts[i].t = 0.5f + (clVerts[i].z-LIGHT_POS(j).z)*s*1.2f;
		}

		HWR_SetLight();

		Surf.PolyColor.rgba = LONG(dynlights->p_lspr[j]->dynamic_color);
#ifdef DL_HIGH_QUALITY
		Surf.PolyColor.s.alpha = (unsigned char)((1 - dist_p2d/DL_SQRRADIUS(j))*Surf.PolyColor.s.alpha);
#endif

		if ((dynlights->mo[j]->state->nextstate == S_NULL))
			Surf.PolyColor.s.alpha = (unsigned char)(((float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics)*Surf.PolyColor.s.alpha);

		HWD.pfnDrawPolygon (&Surf, clVerts, nrClipVerts, LIGHTMAPFLAGS);

	}  
}


static lumpnum_t coronalumpnum = LUMPERROR;
#ifndef NEWCORONAS



void HWR_DoCoronasLighting(FOutVector *outVerts, gl_vissprite_t *spr)
{
	light_t   *p_lspr;

	if (coronalumpnum == LUMPERROR)
		return;


	p_lspr = t_lspr[spr->mobj->sprite];
	if ((spr->mobj->state>=&states[S_EXPLODE1] && spr->mobj->state<=&states[S_EXPLODE3])
	 || (spr->mobj->state>=&states[S_FATSHOTX1] && spr->mobj->state<=&states[S_FATSHOTX3]))
	{
		p_lspr = &lspr[ROCKETEXP_L];
	}

	if (cv_glcoronas.value && (p_lspr->type & CORONA_SPR))
	{  
		FOutVector      light[4];
		FSurfaceInfo    Surf;
		float           cx = 0.0f, cy = 0.0f, cz = 0.0f;  
		float           size;
		UINT8           i;

		switch (p_lspr->type)
		{
			case LIGHT_SPR:
				size  = p_lspr->corona_radius  * ((outVerts[0].z+120.0f)/950.0f);  
				break;
			case ROCKET_SPR:
				p_lspr->corona_color = (((M_RandomByte()>>1)&0xff)<<24)|0x0040ff;

			case CORONA_SPR:
				size  = p_lspr->corona_radius  * ((outVerts[0].z+60.0f)/100.0f);  
				break;
			default:
				I_Error("HWR_DoCoronasLighting: unknow light type %d",p_lspr->type);
				return;
		}
		if (size > p_lspr->corona_radius)
			size = p_lspr->corona_radius;
		size *= FIXED_TO_FLOAT(cv_glcoronasize.value<<1);


		for (i = 0; i < 4; i++)
		{
			cx += outVerts[i].x;
			cy += outVerts[i].y;
			cz += outVerts[i].z;
		}
		cx /= 4.0f;  cy /= 4.0f;  cz /= 4.0f;


		if (cz >= 255*8+250)
			return;
		Surf.PolyColor.rgba = p_lspr->corona_color;
		if (cz > 250.0f)
			Surf.PolyColor.s.alpha = 0xff-((int)cz-250)/8;
		else
			Surf.PolyColor.s.alpha = 0xff;


		cz = cz - 2.0f;






		light[0].x = cx-size;  light[0].z = cz;
		light[0].y = cy-size*1.33f+p_lspr->light_yoffset;
		light[0].s = 0.0f;   light[0].t = 0.0f;

		light[1].x = cx+size;  light[1].z = cz;
		light[1].y = cy-size*1.33f+p_lspr->light_yoffset;
		light[1].s = 1.0f;   light[1].t = 0.0f;

		light[2].x = cx+size;  light[2].z = cz;
		light[2].y = cy+size*1.33f+p_lspr->light_yoffset;
		light[2].s = 1.0f;   light[2].t = 1.0f;

		light[3].x = cx-size;  light[3].z = cz;
		light[3].y = cy+size*1.33f+p_lspr->light_yoffset;
		light[3].s = 0.0f;   light[3].t = 1.0f;



		HWD.pfnDrawPolygon (&Surf, light, 4, PF_Modulated | PF_Additive | PF_Corona | PF_NoDepthTest);
	}
}
#endif

#ifdef NEWCORONAS

void HWR_DrawCoronas(void)
{
	int       j;

	if (!cv_glcoronas.value || dynlights->nb <= 0 || coronalumpnum == LUMPERROR)
		return;


	for (j = 0;j < dynlights->nb;j++)
	{
		FOutVector      light[4];
		FSurfaceInfo    Surf;
		float           cx = LIGHT_POS(j).x;
		float           cy = LIGHT_POS(j).y;
		float           cz = LIGHT_POS(j).z;  
		float           size;
		light_t         *p_lspr = dynlights->p_lspr[j];


		if (!(p_lspr->type & CORONA_SPR))
			continue;

		if (!dynlights->mo[j])
			continue;
		if (P_MobjWasRemoved(dynlights->mo[j]))
		{
			P_SetTarget(&dynlights->mo[j], NULL);
			continue;
		}

		transform(&cx,&cy,&cz);


		if (cz >= 255*8+250)
			continue;
		Surf.PolyColor.rgba = p_lspr->corona_color;
		if (cz > 250.0f)
			Surf.PolyColor.s.alpha = (UINT8)(0xff-(UINT8)(((int)cz-250)/8));
		else
			Surf.PolyColor.s.alpha = 0xff;

		switch (p_lspr->type)
		{
			case LIGHT_SPR:
				size  = p_lspr->corona_radius  * ((cz+120.0f)/950.0f);  
				break;
			case ROCKET_SPR:
				Surf.PolyColor.s.alpha = (UINT8)((M_RandomByte()>>1)&0xff);

			case CORONA_SPR:
				size  = p_lspr->corona_radius  * ((cz+60.0f)/100.0f);  
				break;
			default:
				I_Error("HWR_DoCoronasLighting: unknow light type %d",p_lspr->type);
				continue;
		}
		if (size > p_lspr->corona_radius)
			size = p_lspr->corona_radius;
		size = (float)(FIXED_TO_FLOAT(cv_glcoronasize.value<<1)*size);




		cz = cz - 5.0f;

		light[0].x = cx-size;  light[0].z = cz;
		light[0].y = cy-size*1.33f;
		light[0].s = 0.0f;   light[0].t = 0.0f;

		light[1].x = cx+size;  light[1].z = cz;
		light[1].y = cy-size*1.33f;
		light[1].s = 1.0f;   light[1].t = 0.0f;

		light[2].x = cx+size;  light[2].z = cz;
		light[2].y = cy+size*1.33f;
		light[2].s = 1.0f;   light[2].t = 1.0f;

		light[3].x = cx-size;  light[3].z = cz;
		light[3].y = cy+size*1.33f;
		light[3].s = 0.0f;   light[3].t = 1.0f;

		HWD.pfnDrawPolygon (&Surf, light, 4, PF_Modulated | PF_Additive | PF_NoDepthTest | PF_Corona);
	}
}
#endif




void HWR_ResetLights(void)
{
	while (dynlights->nb)
		P_SetTarget(&dynlights->mo[--dynlights->nb], NULL);
}




void HWR_SetLights(int viewnumber)
{
	dynlights = &view_dynlights[viewnumber];
}





void HWR_DL_AddLight(gl_vissprite_t *spr, GLPatch_t *patch)
{
	light_t   *p_lspr;


	(void)patch;
	if (!cv_gldynamiclighting.value)
		return;

	if (!spr->mobj)
#ifdef PARANOIA
		I_Error("vissprite without mobj !!!");
#else
		return;
#endif

	if (dynlights->nb >= DL_MAX_LIGHT)
		return;


	p_lspr = t_lspr[spr->mobj->sprite];
	if (!(p_lspr->type & DYNLIGHT_SPR))
		return;
	if ((p_lspr->type != LIGHT_SPR) || cv_glstaticlighting.value)
		return;

	LIGHT_POS(dynlights->nb).x = FIXED_TO_FLOAT(spr->mobj->x);
	LIGHT_POS(dynlights->nb).y = FIXED_TO_FLOAT(spr->mobj->z)+FIXED_TO_FLOAT(spr->mobj->height>>1)+p_lspr->light_yoffset;
	LIGHT_POS(dynlights->nb).z = FIXED_TO_FLOAT(spr->mobj->y);

	P_SetTarget(&dynlights->mo[dynlights->nb], spr->mobj);

	dynlights->p_lspr[dynlights->nb] = p_lspr;

	dynlights->nb++;
}

static GLMipmap_t lightmappatchmipmap;
static GLPatch_t lightmappatch = { .mipmap = &lightmappatchmipmap };

void HWR_InitLight(void)
{
	size_t i;


	for (i = 0;i < NUMLIGHTS;i++)
		lspr[i].dynamic_sqrradius = lspr[i].dynamic_radius*lspr[i].dynamic_radius;

	lightmappatch.mipmap->downloaded = false;
	coronalumpnum = W_CheckNumForName("CORONA");
}




static void HWR_SetLight(void)
{
	int    i, j;

	if (!lightmappatch.mipmap->downloaded && !lightmappatch.mipmap->data)
	{

		UINT16 *Data = Z_Malloc(129*128*sizeof (UINT16), PU_HWRCACHE, &lightmappatch.mipmap->data);

		for (i = 0; i < 128; i++)
		{
			for (j = 0; j < 128; j++)
			{
				int pos = ((i-64)*(i-64))+((j-64)*(j-64));
				if (pos <= 63*63)
					Data[i*128+j] = (UINT16)(((UINT8)(255-(4*(float)sqrt((float)pos)))) << 8 | 0xff);
				else
					Data[i*128+j] = 0;
			}
		}
		lightmappatch.mipmap->format = GL_TEXFMT_ALPHA_INTENSITY_88;

		lightmappatch.width = 128;
		lightmappatch.height = 128;
		lightmappatch.mipmap->width = 128;
		lightmappatch.mipmap->height = 128;
		lightmappatch.mipmap->flags = 0;  
	}
	HWD.pfnSetTexture(lightmappatch.mipmap);


	Z_ChangeTag(lightmappatch.mipmap->data, PU_HWRCACHE_UNLOCKED);
}





#ifdef STATICLIGHT

static sector_t *lgl_backsector;
static seg_t *lgl_curline;
#endif


static inline void HWR_BuildWallLightmaps(FVector *p1, FVector *p2, int lighnum, seg_t *line)
{
	lightmap_t *lp;








	(void)lighnum;
	(void)p1;
	(void)p2;
	lp = malloc(sizeof (*lp));
	lp->next = line->lightmaps;
	line->lightmaps = lp;


}

#ifdef STATICLIGHT
static void HWR_AddLightMapForLine(int lightnum, seg_t *line)
{
	  
                        
                        
                            
                            
                          
                           
   
	FVector             p1,p2;

	lgl_curline = line;
	lgl_backsector = line->backsector;





  
                                                              
                                                          
                                                              
                                            
  
         
  
  

	p1.y = FIXED_TO_FLOAT(lgl_curline->v1->y);
	p1.x = FIXED_TO_FLOAT(lgl_curline->v1->x);
	p2.y = FIXED_TO_FLOAT(lgl_curline->v2->y);
	p2.x = FIXED_TO_FLOAT(lgl_curline->v2->x);





	HWR_BuildWallLightmaps(&p1, &p2, lightnum, line);
}


static void HWR_CheckSubsector(size_t num, fixed_t *bbox)
{
	int         count;
	seg_t       *line;
	subsector_t *sub;
	FVector     p1,p2;
	int         lightnum;

	p1.y = FIXED_TO_FLOAT(bbox[BOXTOP   ]);
	p1.x = FIXED_TO_FLOAT(bbox[BOXLEFT  ]);
	p2.y = FIXED_TO_FLOAT(bbox[BOXBOTTOM]);
	p2.x = FIXED_TO_FLOAT(bbox[BOXRIGHT ]);


	if (num < numsubsectors)
	{
		sub = &subsectors[num];          
		for (lightnum = 0; lightnum < dynlights->nb; lightnum++)
		{



			if (!dynlights->mo[lightnum])
				continue;
			if (P_MobjWasRemoved(dynlights->mo[lightnum]))
			{
				P_SetTarget(&dynlights->mo[lightnum], NULL);
				continue;
			}

			count = sub->numlines;           
			line = &segs[sub->firstline];    
			while (count--)
			{
				HWR_AddLightMapForLine (lightnum, line);        
				line++;
			}
		}
	}
}





static void HWR_AddMobjLights(mobj_t *thing)
{
	if (dynlights->nb >= DL_MAX_LIGHT)
		return;
	if (!(t_lspr[thing->sprite]->type & CORONA_SPR))
		return;

	LIGHT_POS(dynlights->nb).x = FIXED_TO_FLOAT(thing->x);
	LIGHT_POS(dynlights->nb).y = FIXED_TO_FLOAT(thing->z) + t_lspr[thing->sprite]->light_yoffset;
	LIGHT_POS(dynlights->nb).z = FIXED_TO_FLOAT(thing->y);

	P_SetTarget(&dynlights->mo[dynlights->nb], thing);

	dynlights->p_lspr[dynlights->nb] = t_lspr[thing->sprite];

	dynlights->nb++;
}




static void HWR_ComputeLightMapsInBSPNode(int bspnum, fixed_t *bbox)
{
	if (bspnum & NF_SUBSECTOR)  
	{
		if (bspnum == -1)
			HWR_CheckSubsector(0, bbox);   
		else
			HWR_CheckSubsector(bspnum&(~NF_SUBSECTOR), bbox);
		return;
	}
	HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[0], nodes[bspnum].bbox[0]);
	HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[1], nodes[bspnum].bbox[1]);
}

static void HWR_SearchLightsInMobjs(void)
{
	thinker_t *         th;



	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			HWR_AddMobjLights((mobj_t *)th);
}
#endif




void HWR_CreateStaticLightmaps(int bspnum)
{
#ifdef STATICLIGHT
	CONS_Debug(DBG_RENDER, "HWR_CreateStaticLightmaps\n");

	HWR_ResetLights();




	HWR_SearchLightsInMobjs();
	CONS_Debug(DBG_RENDER, "%d lights found\n", dynlights->nb);


	validcount++;  
	HWR_ComputeLightMapsInBSPNode(bspnum, NULL);
#else
	(void)bspnum;
#endif
}

   
      

                                                                                        
                                                                                
                        
                                                                            
                                                                                    
                                                                      
                                                                                  
                                                                                  
                                                                             
                                                                                
      
                                                                                   
                                                                                 
                                                                                 
                            
                                                                    
  
#endif
#endif  
