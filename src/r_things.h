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



#ifndef __R_THINGS__
#define __R_THINGS__

#include "r_plane.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_portal.h"
#include "r_defs.h"
#include "r_skins.h"





#define FEETADJUST (4<<FRACBITS)  

spritenum_t R_GetSpriteNumByName(const char *name);

boolean R_AddSingleSpriteDef(const char *sprname, spritedef_t *spritedef, UINT16 wadnum, UINT16 startlump, UINT16 endlump, boolean longname);



void R_AddSpriteDefs(UINT16 wadnum);






extern INT16 *mfloorclip;
extern INT16 *mceilingclip;
extern fixed_t spryscale;
extern fixed_t sprtopscreen;
extern fixed_t sprbotscreen;
extern fixed_t windowtop;
extern fixed_t windowbottom;

void R_DrawMaskedColumn(column_t *column, unsigned lengthcol);
void R_DrawFlippedMaskedColumn(column_t *column, unsigned lengthcol);
void R_DrawFlippedPost(UINT8 *source, unsigned length, void (*drawcolfunc)(void));







extern INT16 negonearray[MAXVIDWIDTH];
extern INT16 screenheightarray[MAXVIDWIDTH];

fixed_t R_GetShadowZ(mobj_t *thing, pslope_t **shadowslope);


void R_AddSprites(sector_t *sec, INT32 lightlevel);
void R_InitSprites(void);
void R_ClearSprites(void);

UINT8 R_GetBoundingBoxColor(mobj_t *thing);
boolean R_ThingBoundingBoxVisible(mobj_t *thing);

boolean R_ThingVisible (mobj_t *thing);

boolean R_ThingWithinDist (mobj_t *thing,
		fixed_t        draw_dist,
		fixed_t nights_draw_dist);

boolean R_PrecipThingVisible (precipmobj_t *precipthing,
		fixed_t precip_draw_dist);
boolean R_ShouldCullAdaptiveEffect(const mobj_t *thing);
boolean R_ShouldCullAdaptivePrecipitation(const precipmobj_t *thing);

boolean R_ThingHorizontallyFlipped (mobj_t *thing);
boolean R_ThingVerticallyFlipped (mobj_t *thing);

boolean R_ThingIsPaperSprite (mobj_t *thing);
boolean R_ThingIsFloorSprite (mobj_t *thing);

boolean R_ThingIsFullBright (mobj_t *thing);
boolean R_ThingIsSemiBright (mobj_t *thing);
boolean R_ThingIsFullDark (mobj_t *thing);

boolean R_ThingIsFlashing (mobj_t *thing);

UINT8 *R_GetTranslationForThing(mobj_t *mobj, skincolornum_t color, UINT16 translation);
transnum_t R_GetThingTransTable(fixed_t alpha, transnum_t transmap);

void R_ThingOffsetOverlay (mobj_t *thing, fixed_t *outx, fixed_t *outy);




                                               
                                             
                  
   
typedef struct
{
	size_t drawsegs[2];
	size_t vissprites[2];
	fixed_t viewx, viewy, viewz;			 
	sector_t* viewsector;
} maskcount_t;

void R_DrawMasked(maskcount_t* masks, INT32 nummasks);







#define MAXVISSPRITES 2048  

#define VISSPRITECHUNKBITS 6	 
#define VISSPRITESPERCHUNK (1 << VISSPRITECHUNKBITS)
#define VISSPRITEINDEXMASK (VISSPRITESPERCHUNK - 1)

typedef enum
{

	SC_NONE       = 0,
	SC_TOP        = 1,
	SC_BOTTOM     = 1<<1,
	SC_NOTVISIBLE = 1<<2,

	SC_PRECIP     = 1<<3,
	SC_LINKDRAW   = 1<<4,
	SC_FULLBRIGHT = 1<<5,
	SC_SEMIBRIGHT = 1<<6,
	SC_FULLDARK   = 1<<7,
	SC_VFLIP      = 1<<8,
	SC_ISSCALED   = 1<<9,
	SC_ISROTATED  = 1<<10,
	SC_SHADOW     = 1<<11,
	SC_SHEAR      = 1<<12,
	SC_SPLAT      = 1<<13,
	SC_BBOX       = 1<<14,

	SC_CUTMASK    = SC_TOP|SC_BOTTOM|SC_NOTVISIBLE,
	SC_FLAGMASK   = ~SC_CUTMASK
} spritecut_e;



typedef struct vissprite_s
{

	struct vissprite_s *prev;
	struct vissprite_s *next;


	struct vissprite_s *linkdraw;

	mobj_t *mobj;  

	INT32 x1, x2;

	fixed_t gx, gy;  
	fixed_t gz, gzt;  
	fixed_t pz, pzt;  

	fixed_t startfrac;  
	fixed_t xscale, scale;  
	fixed_t thingscale;  
	fixed_t sortscale;  
	fixed_t sortsplat;  
	fixed_t linkscale;  
	fixed_t scalestep;  
	fixed_t paperoffset, paperdistance;  
	fixed_t xiscale;  

	angle_t centerangle;  


	struct {
		fixed_t x, y, z;  
		angle_t angle;  
	} viewpoint;

	struct {
		fixed_t tan;  
		INT32 offset;  
	} shear;

	fixed_t texturemid;
	patch_t *patch;

	lighttable_t *colormap;  


	UINT8 *transmap;  

	INT32 mobjflags;

	INT32 heightsec;  

	extracolormap_t *extra_colormap;  

	fixed_t thingheight;  
	sector_t *sector;  


	INT16 sz, szt;

	spritecut_e cut;
	UINT32 renderflags;
	UINT8 rotateflags;

	fixed_t spritexscale, spriteyscale;
	fixed_t spritexoffset, spriteyoffset;

	fixed_t shadowscale;

	skincolornum_t color;
	UINT16 translation;

	INT16 clipbot[MAXVIDWIDTH], cliptop[MAXVIDWIDTH];

	INT32 dispoffset;  
} vissprite_t;

extern UINT32 visspritecount, numvisiblesprites;

void R_ClipSprites(drawseg_t* dsstart, portal_t* portal);

void R_DrawThingBoundingBox(vissprite_t *spr);







typedef struct drawnode_s
{
	visplane_t *plane;
	drawseg_t *seg;
	drawseg_t *thickseg;
	ffloor_t *ffloor;
	vissprite_t *sprite;

	struct drawnode_s *next;
	struct drawnode_s *prev;
} drawnode_t;

void R_InitDrawNodes(void);












FUNCMATH FUNCINLINE static ATTRINLINE char R_Frame2Char(UINT8 frame)
{
#if 0  
	return 'A' + frame;
#else
	if (frame < 26) return 'A' + frame;
	if (frame < 36) return '0' + (frame - 26);
	if (frame < 62) return 'a' + (frame - 36);
	if (frame == 62) return '!';
	if (frame == 63) return '@';
	return '\xFF';
#endif
}

FUNCMATH FUNCINLINE static ATTRINLINE UINT8 R_Char2Frame(char cn)
{
#if 0  
	if (cn == '+') return '\\' - 'A';  
	return cn - 'A';
#else
	if (cn >= 'A' && cn <= 'Z') return (cn - 'A');
	if (cn >= '0' && cn <= '9') return (cn - '0') + 26;
	if (cn >= 'a' && cn <= 'z') return (cn - 'a') + 36;
	if (cn == '!') return 62;
	if (cn == '@') return 63;
	return 255;
#endif
}


#define ROT_L 17
#define ROT_R 18

FUNCMATH FUNCINLINE static ATTRINLINE char R_Rotation2Char(UINT8 rot)
{
	if (rot <=     9) return '0' + rot;
	if (rot <=    16) return 'A' + (rot - 10);
	if (rot == ROT_L) return 'L';
	if (rot == ROT_R) return 'R';
	return '\xFF';
}

FUNCMATH FUNCINLINE static ATTRINLINE UINT8 R_Char2Rotation(char cn)
{
	if (cn >= '0' && cn <= '9') return (cn - '0');
	if (cn >= 'A' && cn <= 'G') return (cn - 'A') + 10;
	if (cn == 'L') return ROT_L;
	if (cn == 'R') return ROT_R;
	return 255;
}

#endif  
