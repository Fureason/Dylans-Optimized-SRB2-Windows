// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2005 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __HWS_DATA_H__
#define __HWS_DATA_H__

#define NORMAL_SEP 128


#define STATIC_SOURCES_NUM      6        

#define MIN_DISTANCE 160.0f
#define MAX_DISTANCE 1200.0f

typedef struct source_pos_s
{
	double   x;
	double   y;
	double   z;
} source_pos_t;

typedef struct source3D_pos_s
{
	float   x;
	float   y;
	float   z;

	float   momx;
	float   momy;
	float   momz;

} source3D_pos_t;


enum {NORMAL_PITCH = 128};

                                
 
                  
               

                    



typedef struct source3D_data_s
{
	float           min_distance;        
	float           max_distance;        
	INT32           head_relative;       
	INT32           permanent;           
	source3D_pos_t  pos;                 

} source3D_data_t;



typedef struct sfx_data_s
{
	size_t  length;
	void    *data;
	INT32   priority;
	INT32   sep;                     
} sfx_data_t;



typedef struct cone_def_s
{
	float   inner;
	float   outer;
	INT32   outer_gain;
	                  
                   
} cone_def_t;


typedef struct listener_data_s
{

	double  x;
	double  y;
	double  z;


	double  f_angle;
	double  h_angle;


	double  momx;
	double  momy;
	double  momz;
} listener_data_t;

typedef struct snddev_s
{
	INT32   sample_rate;
	INT32   bps;
	size_t  numsfxs;


#ifdef _WIN32
	UINT32   cooplevel;
	HWND    hWnd;
#endif
} snddev_t;

#endif  
