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



#ifndef __R_DEFS__
#define __R_DEFS__


#include "m_fixed.h"
#include "m_vector.h"


#include "d_think.h"

#include "p_mobj.h"

#include "screen.h"  

#ifdef HWRENDER
#include "m_aatree.h"
#endif

#include "taglist.h"






typedef struct
{
	INT32 first;
	INT32 last;
} cliprange_t;


#define SIL_NONE   0
#define SIL_BOTTOM 1
#define SIL_TOP    2
#define SIL_BOTH   3





typedef UINT8 lighttable_t;

#define NUM_PALETTE_ENTRIES 256
#define DEFAULT_STARTTRANSCOLOR 96

#define CMF_FADEFULLBRIGHTSPRITES  1
#define CMF_FOG 4

#define TEXTURE_255_IS_TRANSPARENT


typedef struct extracolormap_s
{
	UINT8 fadestart, fadeend;
	UINT8 flags;



	INT32 rgba;  
	INT32 fadergba;  

	lighttable_t *colormap;

#ifdef HWRENDER
	struct {
		UINT32 id;  
		RGBA_t *data;  
		boolean needs_update;  
	} gl_lighttable;
#endif

#ifdef EXTRACOLORMAPLUMPS
	lumpnum_t lump;  
	char lumpname[9];  
#endif

	struct extracolormap_s *next;
	struct extracolormap_s *prev;
} extracolormap_t;





                              
    
typedef struct
{
	fixed_t x, y;
	boolean floorzset, ceilingzset;
	fixed_t floorz, ceilingz;
} vertex_t;


struct line_s;

                                                            
                                                                           
                                                                            
                               
    
typedef struct
{
	thinker_t thinker;  
	fixed_t x;          
	fixed_t y;          
	fixed_t z;          
} degenmobj_t;

#include "p_polyobj.h"



                                                     
    
typedef enum
{
	FOF_EXISTS            = 0x1,         
	FOF_BLOCKPLAYER       = 0x2,         
	FOF_BLOCKOTHERS       = 0x4,         
	FOF_SOLID             = 0x6,         
	FOF_RENDERSIDES       = 0x8,         
	FOF_RENDERPLANES      = 0x10,        
	FOF_RENDERALL         = 0x18,        
	FOF_SWIMMABLE         = 0x20,        
	FOF_NOSHADE           = 0x40,        
	FOF_CUTSOLIDS         = 0x80,        
	FOF_CUTEXTRA          = 0x100,       
	FOF_CUTLEVEL          = 0x180,       
	FOF_CUTSPRITES        = 0x200,       
	FOF_BOTHPLANES        = 0x400,       
	FOF_EXTRA             = 0x800,       
	FOF_TRANSLUCENT       = 0x1000,      
	FOF_FOG               = 0x2000,      
	FOF_INVERTPLANES      = 0x4000,      
	FOF_ALLSIDES          = 0x8000,      
	FOF_INVERTSIDES       = 0x10000,     
	FOF_DOUBLESHADOW      = 0x20000,     
	FOF_FLOATBOB          = 0x40000,     
	FOF_NORETURN          = 0x80000,     
	FOF_CRUMBLE           = 0x100000,    
	FOF_GOOWATER          = 0x200000,    
	FOF_MARIO             = 0x400000,    
	FOF_BUSTUP            = 0x800000,    
	FOF_QUICKSAND         = 0x1000000,   
	FOF_PLATFORM          = 0x2000000,   
	FOF_REVERSEPLATFORM   = 0x4000000,   
	FOF_INTANGIBLEFLATS   = 0x6000000,   
	FOF_RIPPLE            = 0x8000000,   
	FOF_COLORMAPONLY      = 0x10000000,  
	FOF_BOUNCY            = 0x20000000,  
	FOF_SPLAT             = 0x40000000,  
} ffloortype_e;

typedef enum
{
	FF_OLD_EXISTS            = 0x1,
	FF_OLD_BLOCKPLAYER       = 0x2,
	FF_OLD_BLOCKOTHERS       = 0x4,
	FF_OLD_SOLID             = 0x6,
	FF_OLD_RENDERSIDES       = 0x8,
	FF_OLD_RENDERPLANES      = 0x10,
	FF_OLD_RENDERALL         = 0x18,
	FF_OLD_SWIMMABLE         = 0x20,
	FF_OLD_NOSHADE           = 0x40,
	FF_OLD_CUTSOLIDS         = 0x80,
	FF_OLD_CUTEXTRA          = 0x100,
	FF_OLD_CUTLEVEL          = 0x180,
	FF_OLD_CUTSPRITES        = 0x200,
	FF_OLD_BOTHPLANES        = 0x400,
	FF_OLD_EXTRA             = 0x800,
	FF_OLD_TRANSLUCENT       = 0x1000,
	FF_OLD_FOG               = 0x2000,
	FF_OLD_INVERTPLANES      = 0x4000,
	FF_OLD_ALLSIDES          = 0x8000,
	FF_OLD_INVERTSIDES       = 0x10000,
	FF_OLD_DOUBLESHADOW      = 0x20000,
	FF_OLD_FLOATBOB          = 0x40000,
	FF_OLD_NORETURN          = 0x80000,
	FF_OLD_CRUMBLE           = 0x100000,
	FF_OLD_SHATTERBOTTOM     = 0x200000,
	FF_OLD_GOOWATER          = 0x200000,
	FF_OLD_MARIO             = 0x400000,
	FF_OLD_BUSTUP            = 0x800000,
	FF_OLD_QUICKSAND         = 0x1000000,
	FF_OLD_PLATFORM          = 0x2000000,
	FF_OLD_REVERSEPLATFORM   = 0x4000000,
	FF_OLD_INTANGIBLEFLATS   = 0x6000000,
	FF_OLD_SHATTER           = 0x8000000,
	FF_OLD_SPINBUST          = 0x10000000,
	FF_OLD_STRONGBUST        = 0x20000000,
	FF_OLD_RIPPLE            = 0x40000000,
	FF_OLD_COLORMAPONLY      = 0x80000000,
} oldffloortype_e;

typedef enum
{
	FB_PUSHABLES   = 0x1,  
	FB_EXECUTOR    = 0x2,  
	FB_ONLYBOTTOM  = 0x4,  
} ffloorbustflags_e;

typedef enum
{
	BT_TOUCH,
	BT_SPINBUST,
	BT_REGULAR,
	BT_STRONG,
} busttype_e;

typedef enum
{
	SECPORTAL_LINE,     
	SECPORTAL_SKYBOX,   
	SECPORTAL_PLANE,    
	SECPORTAL_HORIZON,  
	SECPORTAL_OBJECT,   
	SECPORTAL_FLOOR,    
	SECPORTAL_CEILING,  
	SECPORTAL_NONE = 0xFF
} secportaltype_e;

typedef struct sectorportal_s
{
	secportaltype_e type;
	union {
		struct {
			struct line_s *start;
			struct line_s *dest;
		} line;
		struct sector_s *sector;
		struct mobj_s *mobj;
	};
	struct sector_s *target;
	boolean ceiling;
} sectorportal_t;

typedef struct ffloor_s
{
	fixed_t *topheight;
	INT32 *toppic;
	INT16 *toplightlevel;
	fixed_t *topxoffs;
	fixed_t *topyoffs;
	fixed_t *topxscale;
	fixed_t *topyscale;
	angle_t *topangle;

	fixed_t *bottomheight;
	INT32 *bottompic;
	fixed_t *bottomxoffs;
	fixed_t *bottomyoffs;
	fixed_t *bottomxscale;
	fixed_t *bottomyscale;
	angle_t *bottomangle;


	struct pslope_s **t_slope;
	struct pslope_s **b_slope;

	size_t secnum;
	ffloortype_e fofflags;
	struct line_s *master;

	struct sector_s *target;

	struct ffloor_s *next;
	struct ffloor_s *prev;

	INT32 lastlight;
	INT32 alpha;
	UINT8 blend;  
	tic_t norender;  


	ffloorbustflags_e bustflags;
	UINT8 busttype;
	INT16 busttag;


	fixed_t sinkspeed;
	fixed_t friction;


	fixed_t bouncestrength;


	ffloortype_e spawnflags;  
	INT32 spawnalpha;  

	void *fadingdata;  
} ffloor_t;





typedef struct lightlist_s
{
	fixed_t height;
	INT16 *lightlevel;
	extracolormap_t **extra_colormap;  
	INT32 flags;
	ffloor_t *caster;
	struct pslope_s *slope;  
} lightlist_t;



typedef struct r_lightlist_s
{
	fixed_t height;
	fixed_t heightstep;
	fixed_t botheight;
	fixed_t botheightstep;
	fixed_t startheight;  
	INT16 lightlevel;
	extracolormap_t *extra_colormap;
	lighttable_t *rcolormap;
	ffloortype_e flags;
	INT32 lightnum;
} r_lightlist_t;


typedef enum {
	SL_NOPHYSICS = 1,  
	SL_DYNAMIC = 1<<1,  
} slopeflags_t;

typedef struct pslope_s
{
	UINT16 id;  
	struct pslope_s *next;  


	vector3_t o;		 
	vector3_t normal;	 

	vector2_t d;		 
	fixed_t zdelta;		 


	angle_t zangle;		 
	angle_t xydirection; 

	dvector3_t dorigin;
	dvector3_t dnormdir;

	double dzdelta;

	boolean moved;

	UINT8 flags;  
} pslope_t;

typedef enum
{

	MSF_FLIPSPECIAL_FLOOR       =  1,
	MSF_FLIPSPECIAL_CEILING     =  1<<1,
	MSF_FLIPSPECIAL_BOTH        =  (MSF_FLIPSPECIAL_FLOOR|MSF_FLIPSPECIAL_CEILING),

	MSF_TRIGGERSPECIAL_TOUCH    =  1<<2,
	MSF_TRIGGERSPECIAL_HEADBUMP =  1<<3,

	MSF_TRIGGERLINE_PLANE       =  1<<4,  
	MSF_TRIGGERLINE_MOBJ        =  1<<5,  

	MSF_INVERTPRECIP            =  1<<6,
	MSF_GRAVITYFLIP             =  1<<7,
	MSF_HEATWAVE                =  1<<8,
	MSF_NOCLIPCAMERA            =  1<<9,
} sectorflags_t;

typedef enum
{
	SSF_OUTERSPACE = 1,
	SSF_DOUBLESTEPUP = 1<<1,
	SSF_NOSTEPDOWN = 1<<2,
	SSF_WINDCURRENT = 1<<3,
	SSF_CONVEYOR = 1<<4,
	SSF_SPEEDPAD = 1<<5,
	SSF_STARPOSTACTIVATOR = 1<<6,
	SSF_EXIT = 1<<7,
	SSF_SPECIALSTAGEPIT = 1<<8,
	SSF_RETURNFLAG = 1<<9,
	SSF_REDTEAMBASE = 1<<10,
	SSF_BLUETEAMBASE = 1<<11,
	SSF_FAN = 1<<12,
	SSF_SUPERTRANSFORM = 1<<13,
	SSF_FORCESPIN = 1<<14,
	SSF_ZOOMTUBESTART = 1<<15,
	SSF_ZOOMTUBEEND = 1<<16,
	SSF_FINISHLINE = 1<<17,
	SSF_ROPEHANG = 1<<18,
	SSF_JUMPFLIP = 1<<19,
	SSF_GRAVITYOVERRIDE = 1<<20,  
	SSF_NOPHYSICSFLOOR = 1<<21,
	SSF_NOPHYSICSCEILING = 1<<22,
} sectorspecialflags_t;

typedef enum
{
	SD_NONE = 0,
	SD_GENERIC = 1,
	SD_WATER = 2,
	SD_FIRE = 3,
	SD_LAVA = 4,
	SD_ELECTRIC = 5,
	SD_SPIKE = 6,
	SD_DEATHPITTILT = 7,
	SD_DEATHPITNOTILT = 8,
	SD_INSTAKILL = 9,
	SD_SPECIALSTAGE = 10,
} sectordamage_t;

typedef enum
{
	TO_PLAYER = 0,
	TO_ALLPLAYERS = 1,
	TO_MOBJ = 2,
	TO_PLAYEREMERALDS = 3,  
	TO_PLAYERNIGHTS = 4,  
} triggerobject_t;

typedef enum
{
	CRUMBLE_NONE,  
	CRUMBLE_WAIT,  
	CRUMBLE_ACTIVATED,  
	CRUMBLE_FALL,  
	CRUMBLE_RESTORE,  
} crumblestate_t;





typedef struct sector_s
{
	fixed_t floorheight;
	fixed_t ceilingheight;
	INT32 floorpic;
	INT32 ceilingpic;
	INT16 lightlevel;
	INT16 special;
	taglist_t tags;



	degenmobj_t soundorg;


	size_t validcount;


	mobj_t *thinglist;


	void *floordata;  
	void *ceilingdata;  
	void *lightingdata;  
	void *fadecolormapdata;  


	fixed_t floorxoffset, flooryoffset;
	fixed_t ceilingxoffset, ceilingyoffset;


	fixed_t floorxscale, flooryscale;
	fixed_t ceilingxscale, ceilingyscale;


	angle_t floorangle;
	angle_t ceilingangle;

	INT32 heightsec;  
	INT32 camsec;  


	INT16 floorlightlevel, ceilinglightlevel;
	boolean floorlightabsolute, ceilinglightabsolute;  
	INT32 floorlightsec, ceilinglightsec;  

	INT32 crumblestate;  



	struct msecnode_s *touching_thinglist;

	size_t linecount;
	struct line_s **lines;  


	ffloor_t *ffloors;
	size_t *attached;
	boolean *attachedsolid;
	size_t numattached;
	size_t maxattached;
	lightlist_t *lightlist;
	INT32 numlights;
	boolean moved;


	extracolormap_t *extra_colormap;
	boolean colormap_protected;

	fixed_t gravity;  
	fixed_t *gravityptr;  

	sectorflags_t flags;
	sectorspecialflags_t specialflags;
	UINT8 damagetype;


	mtag_t triggertag;  
	UINT8 triggerer;  

	fixed_t friction;


	struct line_s *cullheight;


	fixed_t floorspeed, ceilspeed;


	precipmobj_t *preciplist;
	struct mprecipsecnode_s *touching_preciplist;


	pslope_t *f_slope;  
	pslope_t *c_slope;  
	boolean hasslope;  


	INT16 spawn_lightlevel;


	extracolormap_t *spawn_extra_colormap;


	UINT32 portal_floor;
	UINT32 portal_ceiling;
} sector_t;




typedef enum
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
} slopetype_t;

#define SPECIAL_HORIZON_LINE 41

#define SPECIAL_SECTOR_SETPORTAL 6

#define NUMLINEARGS 10
#define NUMLINESTRINGARGS 2

#define NO_SIDEDEF 0xFFFFFFFF

typedef struct line_s
{

	vertex_t *v1;
	vertex_t *v2;

	fixed_t dx, dy;  
	angle_t angle;  


	INT16 flags;
	INT16 special;
	taglist_t tags;
	INT32 args[NUMLINEARGS];
	char *stringargs[NUMLINESTRINGARGS];


	UINT32 sidenum[2];  
	fixed_t alpha;  
	UINT8 blendmode;  
	INT32 executordelay;

	fixed_t bbox[4];  


	slopetype_t slopetype;



	sector_t *frontsector;
	sector_t *backsector;

	size_t validcount;  
	polyobj_t *polyobj;  

	INT16 callcount;  

	UINT32 secportal;  

	struct pslope_s *midtexslope;
} line_t;

typedef struct
{

	fixed_t textureoffset;


	fixed_t rowoffset;


	fixed_t offsetx_top, offsetx_mid, offsetx_bottom;
	fixed_t offsety_top, offsety_mid, offsety_bottom;

	fixed_t scalex_top, scalex_mid, scalex_bottom;
	fixed_t scaley_top, scaley_mid, scaley_bottom;



	INT16 light, light_top, light_mid, light_bottom;
	boolean lightabsolute, lightabsolute_top, lightabsolute_mid, lightabsolute_bottom;



	INT32 toptexture, bottomtexture, midtexture;


	line_t *line;


	sector_t *sector;

	INT16 special;  
	INT16 repeatcnt;  

	extracolormap_t *colormap_data;  
} side_t;







typedef struct subsector_s
{
	sector_t *sector;
	INT16 numlines;
	UINT32 firstline;
	struct polyobj_s *polyList;  
	size_t validcount;
} subsector_t;















typedef struct msecnode_s
{
	sector_t *m_sector;  
	struct mobj_s *m_thing;   
	struct msecnode_s *m_sectorlist_prev;   
	struct msecnode_s *m_sectorlist_next;   
	struct msecnode_s *m_thinglist_prev;   
	struct msecnode_s *m_thinglist_next;   
	boolean visited;  
} msecnode_t;

typedef struct mprecipsecnode_s
{
	sector_t *m_sector;  
	struct precipmobj_s *m_thing;   
	struct mprecipsecnode_s *m_sectorlist_prev;   
	struct mprecipsecnode_s *m_sectorlist_next;   
	struct mprecipsecnode_s *m_thinglist_prev;   
	struct mprecipsecnode_s *m_thinglist_next;   
	boolean visited;  
} mprecipsecnode_t;




typedef struct light_s
{
	UINT16 type;           

	float light_xoffset;
	float light_yoffset;   

	UINT32 corona_color;    
	float corona_radius;   

	UINT32 dynamic_color;   
	float dynamic_radius;  
	float dynamic_sqrradius;  
} light_t;

typedef struct lightmap_s
{
	float s[2], t[2];
	light_t *light;
	struct lightmap_s *next;
} lightmap_t;




typedef struct seg_s
{
	vertex_t *v1;
	vertex_t *v2;

	INT32 side;

	fixed_t offset;

	angle_t angle;

	side_t *sidedef;
	line_t *linedef;



	sector_t *frontsector;
	sector_t *backsector;

	fixed_t length;	 
#ifdef HWRENDER

	void *pv1;  
	void *pv2;  
	float flength;  

	lightmap_t *lightmaps;  
#endif

	polyobj_t *polyseg;
	boolean dontrenderme;
	boolean glseg;
} seg_t;




typedef struct
{

	fixed_t x, y;
	fixed_t dx, dy;


	fixed_t bbox[2][4];


	UINT16 children[2];
} node_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif


typedef struct
{
	UINT8 topdelta;  
	UINT8 length;    
} ATTRPACK doompost_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	unsigned topdelta;
	unsigned length;
	size_t data_offset;
} post_t;


typedef struct
{
	unsigned num_posts;
	post_t *posts;
	UINT8 *pixels;
} column_t;





#ifndef MAXFFLOORS
#define MAXFFLOORS 40
#endif




typedef struct drawseg_s
{
	seg_t *curline;
	INT32 x1;
	INT32 x2;

	fixed_t scale1;
	fixed_t scale2;
	fixed_t scalestep;

	INT32 silhouette;  

	fixed_t bsilheight;  
	fixed_t tsilheight;  

	fixed_t offsetx;


	INT16 *sprtopclip;
	INT16 *sprbottomclip;
	fixed_t *maskedtexturecol;
	fixed_t *maskedtextureheight;  
	fixed_t *invscale;

	struct visplane_s *ffloorplanes[MAXFFLOORS];
	INT32 numffloorplanes;
	struct ffloor_s *thicksides[MAXFFLOORS];
	fixed_t *thicksidecol;
	INT32 numthicksides;
	fixed_t frontscale[MAXVIDWIDTH];

	UINT8 portalpass;  

	vertex_t leftpos, rightpos;  
} drawseg_t;

#ifdef ROTSPRITE
typedef struct
{
	INT32 angles;
	void **patches;
} rotsprite_t;
#endif






typedef struct
{
	INT16 width, height;
	INT16 leftoffset, topoffset;

	UINT8 *pixels;
	column_t *columns;
	post_t *posts;

#ifdef HWRENDER
	                                                                         
                                                                             
                                                                               
	RGBA_t *hardware_pixels;
	UINT8 *hardware_palette_pixels;
#endif

	void *hardware;  
	void *flats[4];  

#ifdef ROTSPRITE
	rotsprite_t *rotated;  
	size_t rotatedcachecost;  
#endif
} patch_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{
	INT16 width;           
	INT16 height;
	INT16 leftoffset;      
	INT16 topoffset;       
	INT32 columnofs[8];      

} ATTRPACK softwarepatch_t;

#if defined(_MSC_VER)
#pragma pack()
#endif


enum patchalphastyle {AST_COPY, AST_TRANSLUCENT, AST_ADD, AST_SUBTRACT, AST_REVERSESUBTRACT, AST_MODULATE, AST_OVERLAY, AST_FOG};

typedef enum
{
	RF_HORIZONTALFLIP   = 0x0001,    
	RF_VERTICALFLIP     = 0x0002,    
	RF_ABSOLUTEOFFSETS  = 0x0004,    
	RF_FLIPOFFSETS      = 0x0008,    

	RF_SPLATMASK        = 0x00F0,    
	RF_SLOPESPLAT       = 0x0010,    
	RF_OBJECTSLOPESPLAT = 0x0020,    
	RF_NOSPLATBILLBOARD = 0x0040,    
	RF_NOSPLATROLLANGLE = 0x0080,    

	RF_BRIGHTMASK       = 0x00000300,    
	RF_FULLBRIGHT       = 0x00000100,    
	RF_FULLDARK         = 0x00000200,    
	RF_SEMIBRIGHT       = (RF_FULLBRIGHT | RF_FULLDARK),  

	RF_NOCOLORMAPS      = 0x00000400,    

	RF_SPRITETYPEMASK   = 0x00003000,    
	RF_PAPERSPRITE      = 0x00001000,    
	RF_FLOORSPRITE      = 0x00002000,    

	RF_SHADOWDRAW       = 0x00004000,   
	RF_SHADOWEFFECTS    = 0x00008000,   
	RF_DROPSHADOW       = (RF_SHADOWDRAW | RF_SHADOWEFFECTS | RF_FULLDARK),
} renderflags_t;

typedef enum
{
	SRF_SINGLE      = 0,    
	SRF_3D          = 1,    
	SRF_3DGE        = 2,    
	SRF_3DMASK      = SRF_3D|SRF_3DGE,  
	SRF_LEFT        = 4,    
	SRF_RIGHT       = 8,    
	SRF_2D          = SRF_LEFT|SRF_RIGHT,  
	SRF_NONE        = 0xff  
} spriterotateflags_t;      















typedef struct
{



	UINT8 rotate;  


	lumpnum_t lumppat[16];  
	size_t lumpid[16];  


	UINT16 flip;

#ifdef ROTSPRITE
	rotsprite_t *rotated[16];  
#endif
} spriteframe_t;

#define MAXFRAMENUM 256




typedef struct
{
	size_t numframes;
	spriteframe_t *spriteframes;
} spritedef_t;

#endif
