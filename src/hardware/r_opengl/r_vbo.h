/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/
#ifndef _R_VBO_H_
#define _R_VBO_H_

typedef struct
{
	float x, y, z;		 
	float nx, ny, nz;	 
	float s0, t0;		 
} vbo32_t;

typedef struct
{
	float x, y, z;	 
	float s0, t0;	 
	unsigned char r, g, b, a;  
	float pad[2];  
} vbo2d32_t;

typedef struct
{
	float x, y;  
	float s0, t0;  
} vbofont_t;

typedef struct
{
	short x, y, z;  
	char nx, ny, nz;  
	char tanx, tany, tanz;  
	float s0, t0;  
} vbotiny_t;

typedef struct
{
	float x, y, z;       
	float nx, ny, nz;    
	float s0, t0;        
	float s1, t1;        
	float s2, t2;        
	float tan0, tan1, tan2;  
	unsigned char r, g, b, a;	 
} vbo64_t;

#endif
