// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __M_PERFSTATS_H__
#define __M_PERFSTATS_H__

#include "doomdef.h"
#include "lua_script.h"
#include "p_local.h"

typedef struct
{
	union {
		precise_t p;
		INT32 i;
	} value;
	void *history;
} ps_metric_t;

typedef struct
{
	ps_metric_t time_taken;
	char short_src[LUA_IDSIZE];
} ps_hookinfo_t;

#define PS_START_TIMING(metric) metric.value.p = I_GetPreciseTime()
#define PS_STOP_TIMING(metric) metric.value.p = I_GetPreciseTime() - metric.value.p

extern ps_metric_t ps_tictime;

extern ps_metric_t ps_playerthink_time;
extern ps_metric_t ps_thinkertime;

extern ps_metric_t ps_thlist_times[];

extern ps_metric_t ps_checkposition_calls;

extern ps_metric_t ps_lua_prethinkframe_time;
extern ps_metric_t ps_lua_thinkframe_time;
extern ps_metric_t ps_lua_postthinkframe_time;
extern ps_metric_t ps_lua_mobjhooks;
extern ps_metric_t ps_lua_mobjiterates;

extern ps_metric_t ps_otherlogictime;

void PS_SetPreThinkFrameHookInfo(int index, precise_t time_taken, char* short_src);
void PS_SetThinkFrameHookInfo(int index, precise_t time_taken, char* short_src);
void PS_SetPostThinkFrameHookInfo(int index, precise_t time_taken, char* short_src);

                                                                             
                                                                             
                                    
void PS_ArmDeepProfile(void);
void PS_ArmHitchProfile(void);
void PS_BeginTickProfile(void);
boolean PS_DeepProfileActive(void);
void PS_RecordMobjThinker(mobjtype_t type, UINT32 mobjnum, INT32 state,
	fixed_t x, fixed_t y, fixed_t z, UINT32 flags, precise_t time_taken);
void PS_RecordMobjSpawn(mobjtype_t type);
void PS_RecordLuaHook(const char *description, precise_t time_taken);
void PS_FormatHitchContext(char *buffer, size_t buffer_size);
void PS_DeepProfile_f(void);
void PS_PerformanceReport_f(void);

void PS_UpdateTickStats(void);
void PS_UpdateFrameStats(void);

void M_DrawPerfStats(void);

void PS_PerfStats_OnChange(void);
void PS_SampleSize_OnChange(void);

#endif
