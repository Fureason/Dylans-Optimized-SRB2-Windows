// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by James Haley
// Copyright (C) 2006-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#ifndef POLYOBJ_H__
#define POLYOBJ_H__

#include "m_dllist.h"
#include "p_mobj.h"
#include "r_defs.h"







#define POLYOBJ_ANCHOR_DOOMEDNUM     760
#define POLYOBJ_SPAWN_DOOMEDNUM      761

#define POLYOBJ_START_LINE    20

typedef enum
{
	POF_CLIPLINES         = 0x1,        
	POF_CLIPPLANES        = 0x2,        
	POF_SOLID             = 0x3,        
	POF_TESTHEIGHT        = 0x4,        
	POF_RENDERSIDES       = 0x8,        
	POF_RENDERTOP         = 0x10,       
	POF_RENDERBOTTOM      = 0x20,       
	POF_RENDERPLANES      = 0x30,       
	POF_RENDERALL         = 0x38,       
	POF_INVERT            = 0x40,       
	POF_INVERTPLANES      = 0x80,       
	POF_INVERTPLANESONLY  = 0x100,      
	POF_PUSHABLESTOP      = 0x200,      
	POF_LDEXEC            = 0x400,      
	POF_ONESIDE           = 0x800,      
	POF_NOSPECIALS        = 0x1000,     
	POF_SPLAT             = 0x2000,     
} polyobjflags_e;

typedef enum
{
	TMPF_NOINSIDES       = 1,
	TMPF_INTANGIBLE      = 1<<1,
	TMPF_PUSHABLESTOP    = 1<<2,
	TMPF_INVISIBLEPLANES = 1<<3,
	TMPF_EXECUTOR        = 1<<4,
	TMPF_CRUSH           = 1<<5,
	TMPF_SPLAT           = 1<<6,

} textmappolyobjectflags_t;





typedef struct polyobj_s
{
	mdllistitem_t link;  

	INT32 id;     
	INT32 first;  
	INT32 next;   

	INT32 parent;  

	size_t segCount;         
	size_t numSegsAlloc;     
	struct seg_s **segs;  

	size_t numVertices;             
	size_t numVerticesAlloc;        
	vertex_t *origVerts;  
	vertex_t *tmpVerts;   
	vertex_t **vertices;  

	size_t numLines;           
	size_t numLinesAlloc;      
	struct line_s **lines;  

	degenmobj_t spawnSpot;  
	vertex_t    centerPt;   
	fixed_t zdist;          
	angle_t angle;          
	UINT8 attached;          

	fixed_t blockbox[4];  
	UINT8 linked;          
	size_t validcount;    
	INT32 damage;         
	fixed_t thrust;       
	INT32 flags;          

	thinker_t *thinker;   

	UINT8 isBad;          
	INT32 translucency;  
	INT16 triggertag;    

	struct visplane_s *visplane;  


	INT32 spawnflags;  
	INT32 spawntrans;  
} polyobj_t;





typedef struct polymaplink_s
{
	mdllistitem_t link;  
	polyobj_t *po;       
} polymaplink_t;





typedef struct polyrotate_s
{
	thinker_t thinker;  

	INT32 polyObjNum;     
	INT32 speed;          
	INT32 distance;       
	UINT8 turnobjs;       
} polyrotate_t;

typedef struct polymove_s
{
	thinker_t thinker;   

	INT32 polyObjNum;    
	INT32 speed;         
	fixed_t momx;        
	fixed_t momy;        
	INT32 distance;      
	UINT32 angle;        
} polymove_t;


typedef enum
{
	PWR_STOP,      
	PWR_WRAP,      
	PWR_COMEBACK,  
} polywaypointreturn_e;

typedef struct polywaypoint_s
{
	thinker_t thinker;  

	INT32 polyObjNum;       
	INT32 speed;            
	INT32 sequence;         
	INT32 pointnum;         
	INT32 direction;        
	UINT8 returnbehavior;   
	UINT8 continuous;       
	UINT8 stophere;         
} polywaypoint_t;

typedef struct polyslidedoor_s
{
	thinker_t thinker;       

	INT32 polyObjNum;          
	INT32 delay;               
	INT32 delayCount;          
	INT32 initSpeed;           
	INT32 speed;               
	INT32 initDistance;        
	INT32 distance;            
	UINT32 initAngle;          
	UINT32 angle;              
	UINT32 revAngle;           
	fixed_t momx;              
	fixed_t momy;              
	UINT8 closing;              
} polyslidedoor_t;

typedef struct polyswingdoor_s
{
	thinker_t thinker;  

	INT32 polyObjNum;     
	INT32 delay;          
	INT32 delayCount;     
	INT32 initSpeed;      
	INT32 speed;          
	INT32 initDistance;   
	INT32 distance;       
	UINT8 closing;         
} polyswingdoor_t;

typedef struct polydisplace_s
{
	thinker_t thinker;  

	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t dx;
	fixed_t dy;
	fixed_t oldHeights;
} polydisplace_t;

typedef struct polyrotdisplace_s
{
	thinker_t thinker;  

	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t rotscale;
	UINT8 turnobjs;
	fixed_t oldHeights;
} polyrotdisplace_t;

typedef struct polyfade_s
{
	thinker_t thinker;  

	INT32 polyObjNum;
	INT32 sourcevalue;
	INT32 destvalue;
	boolean docollision;
	boolean doghostfade;
	boolean ticbased;
	INT32 duration;
	INT32 timer;
} polyfade_t;





typedef enum
{
	TMPR_DONTROTATEOTHERS = 1,
	TMPR_ROTATEPLAYERS    = 1<<1,
	TMPR_CONTINUOUS       = 1<<2,
	TMPR_OVERRIDE         = 1<<3,
} textmappolyrotate_t;

typedef enum
{
	PTF_PLAYERS = 1,     
	PTF_OTHERS = 1<<1,  
} polyturnflags_e;

typedef struct polyrotdata_s
{
	INT32 polyObjNum;    
	INT32 direction;     
	INT32 speed;         
	INT32 distance;      
	UINT8 flags;         
} polyrotdata_t;

typedef struct polymovedata_s
{
	INT32 polyObjNum;    
	fixed_t distance;    
	fixed_t speed;       
	angle_t angle;       
	UINT8 overRide;      
} polymovedata_t;

typedef enum
{
	PWF_REVERSE = 1,     
	PWF_LOOP    = 1<<1,  
} polywaypointflags_e;

typedef struct polywaypointdata_s
{
	INT32 polyObjNum;      
	INT32 sequence;        
	fixed_t speed;         
	UINT8 returnbehavior;  
	UINT8 flags;           
} polywaypointdata_t;

typedef enum
{
	TMPV_NOCHANGE  = 0,
	TMPV_VISIBLE   = 1,
	TMPV_INVISIBLE = 2,
} textmappolyvisibility_t;

typedef enum
{
	TMPT_NOCHANGE   = 0,
	TMPT_TANGIBLE   = 1,
	TMPT_INTANGIBLE = 2,
} textmappolytangibility_t;


typedef enum
{
	POLY_DOOR_SLIDE,
	POLY_DOOR_SWING,
} polydoor_e;

typedef struct polydoordata_s
{
	INT32 polyObjNum;      
	INT32 doorType;        
	INT32 speed;           
	angle_t angle;         
	INT32 distance;        
	INT32 delay;           
} polydoordata_t;

typedef struct polydisplacedata_s
{
	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t dx;
	fixed_t dy;
} polydisplacedata_t;

typedef struct polyrotdisplacedata_s
{
	INT32 polyObjNum;
	struct sector_s *controlSector;
	fixed_t rotscale;
	UINT8 turnobjs;
} polyrotdisplacedata_t;

typedef struct polyflagdata_s
{
	INT32 polyObjNum;
	INT32 speed;
	UINT32 angle;
	fixed_t momx;
} polyflagdata_t;

typedef enum
{
	TMPF_RELATIVE        = 1,
	TMPF_OVERRIDE        = 1<<1,
	TMPF_TICBASED        = 1<<2,
	TMPF_IGNORECOLLISION = 1<<3,
	TMPF_GHOSTFADE       = 1<<4,
} textmappolyfade_t;

typedef struct polyfadedata_s
{
	INT32 polyObjNum;
	INT32 destvalue;
	boolean docollision;
	boolean doghostfade;
	boolean ticbased;
	INT32 speed;
} polyfadedata_t;





boolean Polyobj_moveXY(polyobj_t *po, fixed_t x, fixed_t y, boolean checkmobjs);
boolean Polyobj_rotate(polyobj_t *po, angle_t delta, boolean turnplayers, boolean turnothers, boolean checkmobjs);
polyobj_t *Polyobj_GetForNum(INT32 id);
void Polyobj_InitLevel(void);
void Polyobj_MoveOnLoad(polyobj_t *po, angle_t angle, fixed_t x, fixed_t y);
boolean P_PointInsidePolyobj(polyobj_t *po, fixed_t x, fixed_t y);
boolean P_MobjTouchingPolyobj(polyobj_t *po, mobj_t *mo);
boolean P_MobjInsidePolyobj(polyobj_t *po, mobj_t *mo);
boolean P_BBoxInsidePolyobj(polyobj_t *po, fixed_t *bbox);


void T_PolyObjRotate(polyrotate_t *);
void T_PolyObjMove  (polymove_t *);
void T_PolyObjWaypoint (polywaypoint_t *);
void T_PolyDoorSlide(polyslidedoor_t *);
void T_PolyDoorSwing(polyswingdoor_t *);
void T_PolyObjDisplace  (polydisplace_t *);
void T_PolyObjRotDisplace  (polyrotdisplace_t *);
void T_PolyObjFlag  (polymove_t *);
void T_PolyObjFade  (polyfade_t *);

boolean EV_DoPolyDoor(polydoordata_t *);
boolean EV_DoPolyObjMove(polymovedata_t *);
boolean EV_DoPolyObjWaypoint(polywaypointdata_t *);
boolean EV_DoPolyObjRotate(polyrotdata_t *);
boolean EV_DoPolyObjDisplace(polydisplacedata_t *);
boolean EV_DoPolyObjRotDisplace(polyrotdisplacedata_t *);
boolean EV_DoPolyObjFlag(polyflagdata_t *);
boolean EV_DoPolyObjFade(polyfadedata_t *);






extern polyobj_t *PolyObjects;
extern INT32 numPolyObjects;
extern polymaplink_t **polyblocklinks;  

#endif


