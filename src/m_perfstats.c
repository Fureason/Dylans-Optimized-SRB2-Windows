// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file m_perfstats.c
/// \brief Performance measurement tools.

#include "m_perfstats.h"
#include "v_video.h"
#include "i_video.h"
#include "screen.h"
#include "netcode/d_netcmd.h"
#include "r_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"
#include "r_fps.h"
#include "deh_tables.h"
#include "d_main.h"
#include "m_misc.h"
#include "netcode/d_net.h"
#include "netcode/d_clisrv.h"

#include <time.h>

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

struct perfstatrow;

typedef struct perfstatrow perfstatrow_t;

struct perfstatrow {
	const char  * lores_label;
	const char  * hires_label;
	ps_metric_t * metric;
	UINT8         flags;
};

// perfstatrow_t flags

#define PS_TIME      1  // metric measures time (uses precise_t instead of INT32)
#define PS_LEVEL     2  // metric is valid only when a level is active
#define PS_SW        4  // metric is valid only in software mode
#define PS_HW        8  // metric is valid only in opengl mode
#define PS_BATCHING  16 // metric is valid only when opengl batching is active
#define PS_HIDE_ZERO 32 // hide metric if its value is zero

static ps_metric_t ps_frametime = {0};

ps_metric_t ps_tictime = {0};

ps_metric_t ps_playerthink_time = {0};
ps_metric_t ps_thinkertime = {0};

ps_metric_t ps_thlist_times[NUM_THINKERLISTS];

static ps_metric_t ps_thinkercount = {0};
static ps_metric_t ps_polythcount = {0};
static ps_metric_t ps_mainthcount = {0};
static ps_metric_t ps_mobjcount = {0};
static ps_metric_t ps_regularcount = {0};
static ps_metric_t ps_scenerycount = {0};
static ps_metric_t ps_nothinkcount = {0};
static ps_metric_t ps_dynslopethcount = {0};
static ps_metric_t ps_precipcount = {0};
static ps_metric_t ps_removecount = {0};

ps_metric_t ps_checkposition_calls = {0};

ps_metric_t ps_lua_prethinkframe_time = {0};
ps_metric_t ps_lua_thinkframe_time = {0};
ps_metric_t ps_lua_postthinkframe_time = {0};

ps_metric_t ps_lua_mobjhooks = {0};
ps_metric_t ps_lua_mobjiterates = {0};

ps_metric_t ps_otherlogictime = {0};

/* Automatic deep profiling is deliberately burst-based. Querying the precise
 * timer around every thinker forever costs measurable time in object-heavy
 * add-ons; doing it for a few seconds after a real hitch gives useful blame
 * without taxing normal play. */
static UINT16 ps_deep_profile_tics;
static precise_t ps_mobjtype_times[NUMMOBJTYPES];
static UINT32 ps_mobjtype_calls[NUMMOBJTYPES];
static precise_t ps_mobjtype_totals[NUMMOBJTYPES];
static UINT32 ps_mobjtype_total_calls[NUMMOBJTYPES];
static mobjtype_t ps_last_profiled_mobj = MT_NULL;
static precise_t ps_last_profiled_mobj_time;
static UINT32 ps_last_profiled_mobj_calls;
typedef struct
{
	mobjtype_t type;
	UINT32 mobjnum;
	INT32 state;
	fixed_t x, y, z;
	UINT32 flags;
	precise_t time;
} ps_mobj_instance_sample_t;
static ps_mobj_instance_sample_t ps_current_mobj_instance;
static ps_mobj_instance_sample_t ps_last_mobj_instance;
static UINT32 ps_spawn_counts[NUMMOBJTYPES];
static UINT32 ps_spawn_total;
static char ps_slowest_lua[LUA_IDSIZE + 80];
static precise_t ps_slowest_lua_time;

#define PS_LUA_TOTAL_SLOTS 64
typedef struct
{
	char description[LUA_IDSIZE + 80];
	precise_t total;
	precise_t maximum;
	UINT32 calls;
} ps_lua_total_t;
static ps_lua_total_t ps_lua_totals[PS_LUA_TOTAL_SLOTS];

static const char *PS_MobjTypeName(mobjtype_t type)
{
	if (type >= MT_FIRSTFREESLOT && type < NUMMOBJTYPES
	&& FREE_MOBJS[type - MT_FIRSTFREESLOT])
		return FREE_MOBJS[type - MT_FIRSTFREESLOT];
	if (type >= MT_NULL && type < MT_FIRSTFREESLOT)
		return MOBJTYPE_LIST[type];
	return "MT_UNKNOWN";
}

void PS_ArmDeepProfile(void)
{
	if (!ps_deep_profile_tics)
	{
		memset(ps_mobjtype_totals, 0, sizeof ps_mobjtype_totals);
		memset(ps_mobjtype_total_calls, 0, sizeof ps_mobjtype_total_calls);
	}
	if (ps_deep_profile_tics < 5*TICRATE)
		ps_deep_profile_tics = 5*TICRATE;
}

void PS_ArmHitchProfile(void)
{
	static precise_t next_allowed;
	const precise_t now = I_GetPreciseTime();
	const precise_t precision = I_GetPrecisePrecision();

	/* Automatic hitch attribution only needs a short sample after the event.
	 * Do not extend an active sample on every subsequent hitch: on maps with
	 * thousands of thinkers that feedback kept precise timer calls enabled
	 * permanently and the profiler itself became part of the slowdown. */
	if (!ps_deep_profile_tics && (!next_allowed || now >= next_allowed))
	{
		ps_deep_profile_tics = max(1, TICRATE / 4);
		/* Sample at most once per ten seconds automatically. The console's
		 * perfprofile command still supplies the deliberate full five seconds. */
		next_allowed = now + precision * 10;
	}
}

boolean PS_DeepProfileActive(void)
{
	return ps_deep_profile_tics != 0;
}

void PS_BeginTickProfile(void)
{
	mobjtype_t type;
	precise_t hottest_time = 0;
	const boolean was_active = (ps_deep_profile_tics != 0);

	/* Preserve the previous tick's result until this tick produces a better
	 * snapshot. This is useful when a slow frame is reported between tics. */
	if (ps_deep_profile_tics)
	{
		if (ps_current_mobj_instance.time)
			ps_last_mobj_instance = ps_current_mobj_instance;
		for (type = MT_NULL; type < NUMMOBJTYPES; type++)
		{
			if (ps_mobjtype_times[type] > hottest_time)
			{
				hottest_time = ps_mobjtype_times[type];
				ps_last_profiled_mobj = type;
				ps_last_profiled_mobj_time = hottest_time;
				ps_last_profiled_mobj_calls = ps_mobjtype_calls[type];
			}
		}
		ps_deep_profile_tics--;
	}

	/* These arrays are sizeable in add-on-heavy builds. Clearing and updating
	 * all of them every 35 Hz tic made the diagnostics system permanent work.
	 * Keep it completely dormant until a hitch or perfprofile arms it. */
	if (was_active || ps_deep_profile_tics)
	{
		memset(ps_mobjtype_times, 0, sizeof ps_mobjtype_times);
		memset(ps_mobjtype_calls, 0, sizeof ps_mobjtype_calls);
		memset(ps_spawn_counts, 0, sizeof ps_spawn_counts);
		memset(&ps_current_mobj_instance, 0, sizeof ps_current_mobj_instance);
	}
	ps_spawn_total = 0;
	ps_slowest_lua[0] = '\0';
	ps_slowest_lua_time = 0;
}

void PS_RecordMobjThinker(mobjtype_t type, UINT32 mobjnum, INT32 state,
	fixed_t x, fixed_t y, fixed_t z, UINT32 flags, precise_t time_taken)
{
	if (!ps_deep_profile_tics || type < MT_NULL || type >= NUMMOBJTYPES)
		return;
	ps_mobjtype_times[type] += time_taken;
	ps_mobjtype_calls[type]++;
	ps_mobjtype_totals[type] += time_taken;
	ps_mobjtype_total_calls[type]++;
	if (time_taken > ps_current_mobj_instance.time)
	{
		ps_current_mobj_instance.type = type;
		ps_current_mobj_instance.mobjnum = mobjnum;
		ps_current_mobj_instance.state = state;
		ps_current_mobj_instance.x = x;
		ps_current_mobj_instance.y = y;
		ps_current_mobj_instance.z = z;
		ps_current_mobj_instance.flags = flags;
		ps_current_mobj_instance.time = time_taken;
	}
}

void PS_RecordMobjSpawn(mobjtype_t type)
{
	if (!ps_deep_profile_tics || type < MT_NULL || type >= NUMMOBJTYPES)
		return;
	ps_spawn_counts[type]++;
	ps_spawn_total++;
}

void PS_RecordLuaHook(const char *description, precise_t time_taken)
{
	UINT32 i;
	if (!description)
		return;
	/* The per-tic slowest hook and the multi-second total answer different
	 * questions. Keep accumulating even when this is not a new maximum. */
	if (time_taken > ps_slowest_lua_time)
	{
		ps_slowest_lua_time = time_taken;
		strncpy(ps_slowest_lua, description, sizeof ps_slowest_lua - 1);
		ps_slowest_lua[sizeof ps_slowest_lua - 1] = '\0';
	}

	for (i = 0; i < PS_LUA_TOTAL_SLOTS; i++)
	{
		if (!ps_lua_totals[i].description[0])
		{
			strncpy(ps_lua_totals[i].description, description,
				sizeof ps_lua_totals[i].description - 1);
			ps_lua_totals[i].description[sizeof ps_lua_totals[i].description - 1] = '\0';
		}
		if (!strcmp(ps_lua_totals[i].description, description))
		{
			ps_lua_totals[i].total += time_taken;
			ps_lua_totals[i].maximum = max(ps_lua_totals[i].maximum, time_taken);
			ps_lua_totals[i].calls++;
			break;
		}
	}
}

static void PS_FormatLuaTotals(char *buffer, size_t buffer_size)
{
	boolean used[PS_LUA_TOTAL_SLOTS] = {false};
	UINT32 rank;
	size_t offset = 0;
	const UINT64 precision = I_GetPrecisePrecision();
	if (!buffer || !buffer_size)
		return;
	buffer[0] = '\0';
	for (rank = 0; rank < 5; rank++)
	{
		UINT32 i, best = PS_LUA_TOTAL_SLOTS;
		for (i = 0; i < PS_LUA_TOTAL_SLOTS; i++)
			if (!used[i] && ps_lua_totals[i].calls
			&& (best == PS_LUA_TOTAL_SLOTS
				|| ps_lua_totals[i].total > ps_lua_totals[best].total))
				best = i;
		if (best == PS_LUA_TOTAL_SLOTS)
			break;
		used[best] = true;
		if (offset < buffer_size - 1)
		{
			int added = snprintf(buffer + offset, buffer_size - offset,
				"%u. %s: total %.3f ms, %u calls, max %.3f ms\r\n",
				rank + 1, ps_lua_totals[best].description,
				precision ? (double)ps_lua_totals[best].total * 1000.0 / precision : 0.0,
				ps_lua_totals[best].calls,
				precision ? (double)ps_lua_totals[best].maximum * 1000.0 / precision : 0.0);
			if (added > 0)
				offset += min((size_t)added, buffer_size - offset - 1);
		}
	}
	if (!offset)
		snprintf(buffer, buffer_size, "No attributed Lua hooks were captured.\r\n");
}

static void PS_FormatMobjTotals(char *buffer, size_t buffer_size)
{
	boolean used[NUMMOBJTYPES] = {false};
	UINT32 rank;
	size_t offset = 0;
	const UINT64 precision = I_GetPrecisePrecision();

	if (!buffer || !buffer_size)
		return;
	buffer[0] = '\0';
	for (rank = 0; rank < 5; rank++)
	{
		mobjtype_t type;
		mobjtype_t best = NUMMOBJTYPES;
		for (type = MT_NULL; type < NUMMOBJTYPES; type++)
			if (!used[type] && ps_mobjtype_total_calls[type]
			&& (best == NUMMOBJTYPES
				|| ps_mobjtype_totals[type] > ps_mobjtype_totals[best]))
				best = type;
		if (best == NUMMOBJTYPES)
			break;
		used[best] = true;
		if (offset < buffer_size - 1)
		{
			int added = snprintf(buffer + offset, buffer_size - offset,
				"%u. %s: total %.3f ms, %u calls, max-tic sample tracked separately\r\n",
				rank + 1, PS_MobjTypeName(best),
				precision ? (double)ps_mobjtype_totals[best] * 1000.0 / precision : 0.0,
				ps_mobjtype_total_calls[best]);
			if (added > 0)
				offset += min((size_t)added, buffer_size - offset - 1);
		}
	}
	if (!offset)
		snprintf(buffer, buffer_size,
			"No deep object sample was captured. Run perfprofile, reproduce the slowdown, then run perfreport.\r\n");
}

void PS_FormatHitchContext(char *buffer, size_t buffer_size)
{
	mobjtype_t type;
	mobjtype_t hottest_object = MT_NULL;
	mobjtype_t hottest_spawn = MT_NULL;
	mobjtype_t profiled_object = ps_last_profiled_mobj;
	UINT32 hottest_object_count = 0;
	UINT32 hottest_spawn_count = 0;
	UINT32 profiled_calls = ps_last_profiled_mobj_calls;
	precise_t profiled_time = ps_last_profiled_mobj_time;
	precise_t hottest_phase_time = ps_bsptime.value.p;
	const char *hottest_phase = "bsp";
	UINT64 precision = I_GetPrecisePrecision();
	const ps_mobj_instance_sample_t *instance =
		(ps_current_mobj_instance.time > ps_last_mobj_instance.time)
		? &ps_current_mobj_instance : &ps_last_mobj_instance;

#define PS_CHECK_PHASE(metric, label) do { \
	if ((metric).value.p > hottest_phase_time) { \
		hottest_phase_time = (metric).value.p; hottest_phase = (label); \
	} \
} while (0)
	PS_CHECK_PHASE(ps_uitime, "ui");
	PS_CHECK_PHASE(ps_swaptime, "present");
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		PS_CHECK_PHASE(ps_hw_skyboxtime, "skybox");
		PS_CHECK_PHASE(ps_hw_portaltime, "visuportals");
		PS_CHECK_PHASE(ps_hw_batchsorttime, "batch_sort");
		PS_CHECK_PHASE(ps_hw_batchdrawtime, "batch_draw");
		PS_CHECK_PHASE(ps_hw_spritesorttime, "sprite_sort");
		PS_CHECK_PHASE(ps_hw_spritedrawtime, "sprite_draw");
		PS_CHECK_PHASE(ps_hw_textureuploadtime, "texture_upload");
		PS_CHECK_PHASE(ps_hw_nodesorttime, "drawnode_sort");
		PS_CHECK_PHASE(ps_hw_nodedrawtime, "drawnode_draw");
	}
	else
#endif
	{
		PS_CHECK_PHASE(ps_sw_spritecliptime, "sprite_clip");
		PS_CHECK_PHASE(ps_sw_portaltime, "portals_skybox");
		PS_CHECK_PHASE(ps_sw_planetime, "planes");
		PS_CHECK_PHASE(ps_sw_maskedtime, "masked_sprites");
	}
#undef PS_CHECK_PHASE

	for (type = MT_NULL; type < NUMMOBJTYPES; type++)
	{
		/* Never walk the live thinker list from a hitch report. Reports may be
		 * emitted while map cleanup is unlinking/freeing that list. The call
		 * counters come from P_RunThinkers itself, so they provide the same
		 * useful per-type population clue without touching transitional state. */
		if (ps_mobjtype_calls[type] > hottest_object_count)
		{
			hottest_object = type;
			hottest_object_count = ps_mobjtype_calls[type];
		}
		if (ps_spawn_counts[type] > hottest_spawn_count)
		{
			hottest_spawn = type;
			hottest_spawn_count = ps_spawn_counts[type];
		}
		if (ps_mobjtype_times[type] > profiled_time)
		{
			profiled_object = type;
			profiled_time = ps_mobjtype_times[type];
			profiled_calls = ps_mobjtype_calls[type];
		}
	}
	if (!hottest_object_count && profiled_calls)
	{
		hottest_object = profiled_object;
		hottest_object_count = profiled_calls;
	}

	if (!buffer || !buffer_size)
		return;
	snprintf(buffer, buffer_size,
		"hot_phase=%s phase_ms=%.3f hot_object=%s object_count=%u "
		"profiled_object=%s object_ms=%.3f object_calls=%u "
		"spawns_this_tic=%u hot_spawn=%s hot_spawn_count=%u "
		"hot_instance=%s#%u state=%d pos=(%d,%d,%d) flags=0x%08x instance_ms=%.3f "
		"slow_lua=\"%s\" slow_lua_ms=%.3f checkpositions=%d "
		"lua_mobj_hooks=%d lua_mobj_visits=%d adaptive_effects=%u",
		hottest_phase,
		(precision && hottest_phase_time <= precision * 10)
			? (double)hottest_phase_time * 1000.0 / (double)precision : 0.0,
		PS_MobjTypeName(hottest_object), hottest_object_count,
		PS_MobjTypeName(profiled_object),
		precision ? (double)profiled_time * 1000.0 / (double)precision : 0.0,
		profiled_calls, ps_spawn_total, PS_MobjTypeName(hottest_spawn),
		hottest_spawn_count, PS_MobjTypeName(instance->type), instance->mobjnum,
		instance->state, instance->x >> FRACBITS, instance->y >> FRACBITS,
		instance->z >> FRACBITS, (unsigned)instance->flags,
		precision ? (double)instance->time * 1000.0 / (double)precision : 0.0,
		ps_slowest_lua[0] ? ps_slowest_lua : "none",
		precision ? (double)ps_slowest_lua_time * 1000.0 / (double)precision : 0.0,
		ps_checkposition_calls.value.i, ps_lua_mobjhooks.value.i,
		ps_lua_mobjiterates.value.i,
		(unsigned)R_GetAdaptiveEffectsLevel());
	buffer[buffer_size - 1] = '\0';
}

void PS_DeepProfile_f(void)
{
	char context[768];
	PS_ArmDeepProfile();
	PS_FormatHitchContext(context, sizeof context);
	CONS_Printf("Deep performance profiling armed for 5 seconds.\nCurrent snapshot: %s\n",
		context);
}

static double PS_Milliseconds(precise_t value)
{
	const UINT64 precision = I_GetPrecisePrecision();
	/* A timing slot can retain a pre-map timestamp while the map is torn down.
	 * Do not turn that unsigned subtraction into a multi-hour "measurement". */
	if (!precision || value > precision * 10)
		return 0.0;
	return (double)value * 1000.0 / (double)precision;
}

void PS_PerformanceReport_f(void)
{
	char report[16384];
	char context[1024];
	char movie[256];
	char directory[512];
	char path[640];
	char causes[2048];
	char lua_totals[3072];
	char mobj_totals[2048];
	char *cause = causes;
	size_t causeleft = sizeof causes;
	time_t rawtime = time(NULL);
	struct tm *when = localtime(&rawtime);
	const double frame_ms = PS_Milliseconds(ps_frametime.value.p);
	const double logic_ms = PS_Milliseconds(ps_tictime.value.p);
	const double render_ms = PS_Milliseconds(ps_rendercalltime.value.p);
	const double lua_ms = PS_Milliseconds(ps_lua_prethinkframe_time.value.p
		+ ps_lua_thinkframe_time.value.p + ps_lua_postthinkframe_time.value.p);
	INT32 congested_node = -1;
	UINT16 reliable_retries = 0;
	UINT16 reliable_backlog = Net_GetWorstReliableBacklog(&congested_node,
		&reliable_retries);
	int length;

#define PS_ADD_CAUSE(...) do { \
	int added = snprintf(cause, causeleft, __VA_ARGS__); \
	if (added > 0) { \
		size_t used = min((size_t)added, causeleft - 1); \
		cause += used; causeleft -= used; \
	} \
} while (0)

	causes[0] = '\0';
	if (lostpercent >= 5.0f || gamelostpercent >= 5.0f)
		PS_ADD_CAUSE("- Network instability is likely: packet loss is above 5%%. This can cause resyncs and delayed joins.\r\n");
	if (reliable_backlog >= 16 || reliable_retries >= 3)
		PS_ADD_CAUSE("- Node %d has a congested reliable queue (%u pending, %u retries). Its downloads are being throttled so gameplay packets can recover.\r\n",
			congested_node, reliable_backlog, reliable_retries);
	if (logic_ms > (1000.0 / TICRATE))
		PS_ADD_CAUSE("- Game logic exceeded one 35 Hz tic. Too many thinkers, collision checks, or Lua work can stall every player.\r\n");
	if (lua_ms >= 5.0)
		PS_ADD_CAUSE("- Lua frame hooks are expensive (%.2f ms). Run perfprofile, reproduce the slowdown, then run perfreport again to identify the slow script.\r\n", lua_ms);
	if (ps_checkposition_calls.value.i >= 500)
		PS_ADD_CAUSE("- Collision broad-phase ran %d position queries this frame. The deep object sample below can identify which object types are driving them.\r\n",
			ps_checkposition_calls.value.i);
	if (ps_numsprites.value.i >= 750)
		PS_ADD_CAUSE("- The scene contains %d visible sprites; %d safe opaque sprite submissions were state-batched across all render passes. Remaining models, shadows, particles, and transparent sprites still require ordered work.\r\n",
			ps_numsprites.value.i, ps_hw_batchedsprites.value.i);
#ifdef HWRENDER
	if (rendermode == render_opengl && PS_Milliseconds(ps_hw_skyboxtime.value.p) >= 8.0)
		PS_ADD_CAUSE("- OpenGL skybox rendering is expensive (%.2f ms). A complex 3D skybox is the likely cause.\r\n", PS_Milliseconds(ps_hw_skyboxtime.value.p));
	if (rendermode == render_opengl && PS_Milliseconds(ps_hw_portaltime.value.p) >= 8.0)
		PS_ADD_CAUSE("- OpenGL visuportals cost %.2f ms across %d visible entrances. Portal depth and destination scene complexity are the likely causes.\r\n",
			PS_Milliseconds(ps_hw_portaltime.value.p), ps_hw_numportals.value.i);
	if (rendermode == render_opengl && ps_hw_textureuploads.value.i)
		PS_ADD_CAUSE("- OpenGL uploaded %d texture%s this frame, costing %.2f ms on the render thread. This explains a first-look hitch; subsequent views should reuse the GPU cache.\r\n",
			ps_hw_textureuploads.value.i,
			ps_hw_textureuploads.value.i == 1 ? "" : "s",
			PS_Milliseconds(ps_hw_textureuploadtime.value.p));
	if (rendermode == render_opengl && ps_hw_numcalls.value.i >= 2000)
		PS_ADD_CAUSE("- OpenGL submitted %d draw calls. Texture/state changes and many separate objects are limiting performance.\r\n", ps_hw_numcalls.value.i);
#endif
	if (rendermode == render_soft && PS_Milliseconds(ps_sw_maskedtime.value.p) >= 8.0)
		PS_ADD_CAUSE("- Software masked sprites/midtextures cost %.2f ms. High resolution and overlapping transparent columns are the current rendering limit.\r\n",
			PS_Milliseconds(ps_sw_maskedtime.value.p));
	if (frame_ms >= 33.34 && render_ms > logic_ms)
		PS_ADD_CAUSE("- Rendering is currently the largest measured cost. Lowering expensive visual effects should help more than network changes.\r\n");
	if (!causes[0])
		PS_ADD_CAUSE("- No clear bottleneck crossed the warning thresholds in this snapshot. Leave perfprofile active while reproducing the problem, then run perfreport again.\r\n");

	PS_FormatHitchContext(context, sizeof context);
	PS_FormatLuaTotals(lua_totals, sizeof lua_totals);
	PS_FormatMobjTotals(mobj_totals, sizeof mobj_totals);
	M_GetMovieDiagnostics(movie, sizeof movie);
	length = snprintf(report, sizeof report,
		"SRB2 V5.52 performance report\r\n"
		"timestamp=%04d-%02d-%02d %02d:%02d:%02d\r\n"
		"map=%d gametic=%u leveltime=%u\r\n"
		"renderer=%s resolution=%dx%d fullscreen=%d fps_cap=%u vsync=%s pacing=%s\r\n"
		"players=%d nodes=%d receive_bps=%d send_bps=%d packet_loss=%.2f%% game_loss=%.2f%%\r\n"
		"worst_reliable_node=%d reliable_backlog=%u reliable_max_retries=%u\r\n"
		"frame_ms=%.3f render_ms=%.3f logic_ms=%.3f lua_frame_hooks_ms=%.3f\r\n"
		"visible_sprites=%d batched_sprite_submissions=%d polyobjects=%d visible_visuportals=%d visuportal_ms=%.3f texture_uploads=%d texture_upload_ms=%.3f adaptive_effects=%u\r\n"
		"capture=%s\r\n"
		"\r\nLikely causes / next actions:\r\n%s"
		"\r\nDetailed snapshot:\r\n%s\r\n"
		"\r\nTop Lua hooks since the previous report/profile:\r\n%s"
		"\r\nTop object thinker types in the deep sample:\r\n%s"
		"\r\nDeep object/Lua profiling has been armed for five seconds. Reproduce the hitch and run perfreport once more for stronger attribution.\r\n",
		when ? when->tm_year + 1900 : 0, when ? when->tm_mon + 1 : 0,
		when ? when->tm_mday : 0, when ? when->tm_hour : 0,
		when ? when->tm_min : 0, when ? when->tm_sec : 0,
		gamemap, (unsigned)gametic, (unsigned)leveltime,
		rendermode == render_opengl ? "OpenGL" : "Software", vid.width, vid.height,
		cv_fullscreen.value, R_GetFramerateCap(), cv_vidwait.string,
		I_CompositorProvidesPacing() ? "windows-compositor" :
			(I_VSyncProvidesPacing() ? "swap" : (cv_vidwait.value ? "engine" : "off")),
		D_NumPlayers(), D_NumNodes(false), getbps, sendbps,
		lostpercent, gamelostpercent, congested_node, reliable_backlog,
		reliable_retries, frame_ms, render_ms, logic_ms, lua_ms,
		ps_numsprites.value.i, ps_hw_batchedsprites.value.i, ps_numpolyobjects.value.i,
		ps_hw_numportals.value.i, PS_Milliseconds(ps_hw_portaltime.value.p),
		ps_hw_textureuploads.value.i, PS_Milliseconds(ps_hw_textureuploadtime.value.p),
		(unsigned)R_GetAdaptiveEffectsLevel(), movie, causes, context, lua_totals,
		mobj_totals);

	if (length < 0)
		return;
	if ((size_t)length >= sizeof report)
		length = sizeof report - 1;

	snprintf(directory, sizeof directory, "%s" PATHSEP "diagnostics", srb2home);
	directory[sizeof directory - 1] = '\0';
	I_mkdir(directory, 0755);
	snprintf(path, sizeof path, "%s" PATHSEP "performance-report.txt", directory);
	path[sizeof path - 1] = '\0';
	if (FIL_WriteFile(path, report, (size_t)length))
		CONS_Printf("Performance report saved to %s\n", path);
	else
		CONS_Alert(CONS_WARNING, "Could not save performance report to %s\n", path);
	CONS_Printf("Snapshot: %.2f ms frame, %.2f ms render, %.2f ms logic, "
		"%d sprites, %.2f%% packet loss.\n",
		frame_ms, render_ms, logic_ms, ps_numsprites.value.i, lostpercent);
	CONS_Printf("Likely causes:\n%s", causes);
	CONS_Printf("Full details are in performance-report.txt. Deep profiling is active for 5 seconds.\n");
	memset(ps_lua_totals, 0, sizeof ps_lua_totals);
	memset(ps_mobjtype_totals, 0, sizeof ps_mobjtype_totals);
	memset(ps_mobjtype_total_calls, 0, sizeof ps_mobjtype_total_calls);
	PS_ArmDeepProfile();

#undef PS_ADD_CAUSE
}

// Columns for perfstats pages.

// Position on screen is determined separately in the drawing functions.

// New columns must also be added to the drawing and update functions.
// Drawing functions: PS_DrawRenderStats, PS_DrawGameLogicStats, etc.
// Update functions:
//  - PS_UpdateFrameStats for frame-dependent values
//  - PS_UpdateTickStats for tick-dependent values

// Rendering stats columns

perfstatrow_t rendertime_rows[] = {
	{"frmtime", "Frame time:    ", &ps_frametime, PS_TIME},
	{"drwtime", "3d rendering:  ", &ps_rendercalltime, PS_TIME|PS_LEVEL},

#ifdef HWRENDER
	{" skybox ", " Skybox render: ", &ps_hw_skyboxtime, PS_TIME|PS_LEVEL|PS_HW},
	{" portal ", " Visuportals:    ", &ps_hw_portaltime, PS_TIME|PS_LEVEL|PS_HW},
	{" texupld ", " Texture upload: ", &ps_hw_textureuploadtime, PS_TIME|PS_LEVEL|PS_HW|PS_HIDE_ZERO},
	{" bsptime", " RenderBSPNode: ", &ps_bsptime, PS_TIME|PS_LEVEL|PS_HW},
	{" batsort", " Batch sort:    ", &ps_hw_batchsorttime, PS_TIME|PS_LEVEL|PS_HW|PS_BATCHING},
	{" batdraw", " Batch render:  ", &ps_hw_batchdrawtime, PS_TIME|PS_LEVEL|PS_HW|PS_BATCHING},
	{" sprsort", " Sprite sort:   ", &ps_hw_spritesorttime, PS_TIME|PS_LEVEL|PS_HW},
	{" sprdraw", " Sprite render: ", &ps_hw_spritedrawtime, PS_TIME|PS_LEVEL|PS_HW},
	{" nodesrt", " Drwnode sort:  ", &ps_hw_nodesorttime, PS_TIME|PS_LEVEL|PS_HW},
	{" nodedrw", " Drwnode render:", &ps_hw_nodedrawtime, PS_TIME|PS_LEVEL|PS_HW},
	{" other  ", " Other:         ", &ps_otherrendertime, PS_TIME|PS_LEVEL|PS_HW},
#endif

	{" bsptime", " RenderBSPNode: ", &ps_bsptime, PS_TIME|PS_LEVEL|PS_SW},
	{" sprclip", " R_ClipSprites: ", &ps_sw_spritecliptime, PS_TIME|PS_LEVEL|PS_SW},
	{" portals", " Portals+Skybox:", &ps_sw_portaltime, PS_TIME|PS_LEVEL|PS_SW},
	{" planes ", " R_DrawPlanes:  ", &ps_sw_planetime, PS_TIME|PS_LEVEL|PS_SW},
	{" masked ", " R_DrawMasked:  ", &ps_sw_maskedtime, PS_TIME|PS_LEVEL|PS_SW},
	{" other  ", " Other:         ", &ps_otherrendertime, PS_TIME|PS_LEVEL|PS_SW},

	{"ui     ", "UI render:     ", &ps_uitime, PS_TIME},
	{"finupdt", "I_FinishUpdate:", &ps_swaptime, PS_TIME},
	{0}
};

perfstatrow_t gamelogicbrief_row[] = {
	{"logic  ", "Game logic:    ", &ps_tictime, PS_TIME},
	{0}
};

perfstatrow_t commoncounter_rows[] = {
	{"bspcall", "BSP calls:   ", &ps_numbspcalls, 0},
	{"sprites", "Sprites:     ", &ps_numsprites, 0},
	{"drwnode", "Drawnodes:   ", &ps_numdrawnodes, 0},
	{"plyobjs", "Polyobjects: ", &ps_numpolyobjects, 0},
#ifdef HWRENDER
	{"portals", "Visuportals: ", &ps_hw_numportals, PS_HW},
#endif
	{0}
};

perfstatrow_t interpolation_rows[] = {
	{"intpfrc", "Interp frac: ", &ps_interp_frac, PS_TIME},
	{"intplag", "Interp lag:  ", &ps_interp_lag, PS_TIME},
	{0}
};

#ifdef HWRENDER
perfstatrow_t batchcount_rows[] = {
	{"polygon", "Polygons:  ", &ps_hw_numpolys, 0},
	{"vertex ", "Vertices:  ", &ps_hw_numverts, 0},
	{"sprbatch", "Sprite submits:", &ps_hw_batchedsprites, 0},
	{0}
};

perfstatrow_t batchcalls_rows[] = {
	{"drwcall", "Draw calls:", &ps_hw_numcalls, 0},
	{"shaders", "Shaders:   ", &ps_hw_numshaders, 0},
	{"texture", "Textures:  ", &ps_hw_numtextures, 0},
	{"polyflg", "Polyflags: ", &ps_hw_numpolyflags, 0},
	{"colors ", "Colors:    ", &ps_hw_numcolors, 0},
	{0}
};
#endif

// Game logic stats columns

perfstatrow_t gamelogic_rows[] = {
	{"logic  ", "Game logic:     ", &ps_tictime, PS_TIME},
	{" plrthnk", " P_PlayerThink:  ", &ps_playerthink_time, PS_TIME|PS_LEVEL},
	{" thnkers", " P_RunThinkers:  ", &ps_thinkertime, PS_TIME|PS_LEVEL},
	{"  plyobjs", "  Polyobjects:    ", &ps_thlist_times[THINK_POLYOBJ], PS_TIME|PS_LEVEL},
	{"  main   ", "  Main:           ", &ps_thlist_times[THINK_MAIN], PS_TIME|PS_LEVEL},
	{"  mobjs  ", "  Mobjs:          ", &ps_thlist_times[THINK_MOBJ], PS_TIME|PS_LEVEL},
	{"  dynslop", "  Dynamic slopes: ", &ps_thlist_times[THINK_DYNSLOPE], PS_TIME|PS_LEVEL},
	{"  precip ", "  Precipitation:  ", &ps_thlist_times[THINK_PRECIP], PS_TIME|PS_LEVEL},
	{" lprethinkf", " LUAh_PreThinkFrame:", &ps_lua_prethinkframe_time, PS_TIME|PS_LEVEL},
	{" lthinkf", " LUAh_ThinkFrame:", &ps_lua_thinkframe_time, PS_TIME|PS_LEVEL},
	{" lpostthinkf", " LUAh_PostThinkFrame:", &ps_lua_postthinkframe_time, PS_TIME|PS_LEVEL},
	{" other  ", " Other:          ", &ps_otherlogictime, PS_TIME|PS_LEVEL},
	{0}
};

perfstatrow_t thinkercount_rows[] = {
	{"thnkers", "Thinkers:       ", &ps_thinkercount, PS_LEVEL},
	{" plyobjs", " Polyobjects:    ", &ps_polythcount, PS_LEVEL},
	{" main   ", " Main:           ", &ps_mainthcount, PS_LEVEL},
	{" mobjs  ", " Mobjs:          ", &ps_mobjcount, PS_LEVEL},
	{"  regular", "  Regular:        ", &ps_regularcount, PS_LEVEL},
	{"  scenery", "  Scenery:        ", &ps_scenerycount, PS_LEVEL},
	{"  nothink", "  Nothink:        ", &ps_nothinkcount, PS_HIDE_ZERO|PS_LEVEL},
	{" dynslop", " Dynamic slopes: ", &ps_dynslopethcount, PS_LEVEL},
	{" precip ", " Precipitation:  ", &ps_precipcount, PS_LEVEL},
	{" remove ", " Pending removal:", &ps_removecount, PS_LEVEL},
	{0}
};

perfstatrow_t misc_calls_rows[] = {
	{"lmhook", "Lua mobj hooks: ", &ps_lua_mobjhooks, PS_LEVEL},
	{"lmiter", "Lua mobj visits:", &ps_lua_mobjiterates, PS_LEVEL},
	{"chkpos", "P_CheckPosition:", &ps_checkposition_calls, PS_LEVEL},
	{0}
};

// Sample collection status for averaging.
// Maximum of these two is shown to user if nonzero to tell that
// the reported averages are not correct yet.
int ps_frame_samples_left = 0;
int ps_tick_samples_left = 0;
// History writing positions for frame and tick based metrics
int ps_frame_index = 0;
int ps_tick_index = 0;

// dynamically allocated resizeable array for thinkframe hook stats
ps_hookinfo_t *prethinkframe_hooks = NULL;
int prethinkframe_hooks_length = 0;
int prethinkframe_hooks_capacity = 16;

ps_hookinfo_t *thinkframe_hooks = NULL;
int thinkframe_hooks_length = 0;
int thinkframe_hooks_capacity = 16;

ps_hookinfo_t *postthinkframe_hooks = NULL;
int postthinkframe_hooks_length = 0;
int postthinkframe_hooks_capacity = 16;

void PS_SetPreThinkFrameHookInfo(int index, precise_t time_taken, char* short_src)
{
	if (!prethinkframe_hooks)
	{
		// array needs to be initialized
		prethinkframe_hooks = Z_Calloc(sizeof(ps_hookinfo_t) * prethinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	if (index >= prethinkframe_hooks_capacity)
	{
		// array needs more space, realloc with double size
		int new_capacity = prethinkframe_hooks_capacity * 2;
		prethinkframe_hooks = Z_Realloc(prethinkframe_hooks,
			sizeof(ps_hookinfo_t) * new_capacity, PU_STATIC, NULL);
		// initialize new memory with zeros so the pointers in the structs are null
		memset(&prethinkframe_hooks[prethinkframe_hooks_capacity], 0,
			sizeof(ps_hookinfo_t) * prethinkframe_hooks_capacity);
		prethinkframe_hooks_capacity = new_capacity;
	}
	prethinkframe_hooks[index].time_taken.value.p = time_taken;
	memcpy(prethinkframe_hooks[index].short_src, short_src, LUA_IDSIZE * sizeof(char));
	// since the values are set sequentially from begin to end, the last call should leave
	// the correct value to this variable
	prethinkframe_hooks_length = index + 1;
}

void PS_SetThinkFrameHookInfo(int index, precise_t time_taken, char* short_src)
{
	if (!thinkframe_hooks)
	{
		// array needs to be initialized
		thinkframe_hooks = Z_Calloc(sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	if (index >= thinkframe_hooks_capacity)
	{
		// array needs more space, realloc with double size
		int new_capacity = thinkframe_hooks_capacity * 2;
		thinkframe_hooks = Z_Realloc(thinkframe_hooks,
			sizeof(ps_hookinfo_t) * new_capacity, PU_STATIC, NULL);
		// initialize new memory with zeros so the pointers in the structs are null
		memset(&thinkframe_hooks[thinkframe_hooks_capacity], 0,
			sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity);
		thinkframe_hooks_capacity = new_capacity;
	}
	thinkframe_hooks[index].time_taken.value.p = time_taken;
	memcpy(thinkframe_hooks[index].short_src, short_src, LUA_IDSIZE * sizeof(char));
	// since the values are set sequentially from begin to end, the last call should leave
	// the correct value to this variable
	thinkframe_hooks_length = index + 1;
}

void PS_SetPostThinkFrameHookInfo(int index, precise_t time_taken, char* short_src)
{
	if (!postthinkframe_hooks)
	{
		// array needs to be initialized
		postthinkframe_hooks = Z_Calloc(sizeof(ps_hookinfo_t) * postthinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	if (index >= postthinkframe_hooks_capacity)
	{
		// array needs more space, realloc with double size
		int new_capacity = postthinkframe_hooks_capacity * 2;
		postthinkframe_hooks = Z_Realloc(postthinkframe_hooks,
			sizeof(ps_hookinfo_t) * new_capacity, PU_STATIC, NULL);
		// initialize new memory with zeros so the pointers in the structs are null
		memset(&postthinkframe_hooks[postthinkframe_hooks_capacity], 0,
			sizeof(ps_hookinfo_t) * postthinkframe_hooks_capacity);
		postthinkframe_hooks_capacity = new_capacity;
	}
	postthinkframe_hooks[index].time_taken.value.p = time_taken;
	memcpy(postthinkframe_hooks[index].short_src, short_src, LUA_IDSIZE * sizeof(char));
	// since the values are set sequentially from begin to end, the last call should leave
	// the correct value to this variable
	postthinkframe_hooks_length = index + 1;
}

static boolean PS_HighResolution(void)
{
	return (vid.width >= 640 && vid.height >= 400);
}

static boolean PS_IsLevelActive(void)
{
	return gamestate == GS_LEVEL ||
			(gamestate == GS_TITLESCREEN && titlemapinaction);
}

// Is the row valid in the current context?
static boolean PS_IsRowValid(perfstatrow_t *row)
{
	return !((row->flags & PS_LEVEL && !PS_IsLevelActive())
		|| (row->flags & PS_SW && rendermode != render_soft)
		|| (row->flags & PS_HW && rendermode != render_opengl)
#ifdef HWRENDER
		|| (row->flags & PS_BATCHING && !cv_glbatching.value)
#endif
		);
}

// Should the row be visible on the screen?
static boolean PS_IsRowVisible(perfstatrow_t *row)
{
	boolean value_is_zero;

	if (row->flags & PS_TIME)
		value_is_zero = row->metric->value.p == 0;
	else
		value_is_zero = row->metric->value.i == 0;

	return !(!PS_IsRowValid(row) ||
		(row->flags & PS_HIDE_ZERO && value_is_zero));
}

static INT32 PS_GetMetricAverage(ps_metric_t *metric, boolean time_metric)
{
	char* history_read_pos = metric->history; // char* used for pointer arithmetic
	INT64 sum = 0;
	int i;
	int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);

	for (i = 0; i < cv_ps_samplesize.value; i++)
	{
		if (time_metric)
			sum += (*((precise_t*)history_read_pos)) / (I_GetPrecisePrecision() / 1000000);
		else
			sum += *((INT32*)history_read_pos);
		history_read_pos += value_size;
	}

	return sum / cv_ps_samplesize.value;
}

static INT32 PS_GetMetricMinOrMax(ps_metric_t *metric, boolean time_metric, boolean get_max)
{
	char* history_read_pos = metric->history; // char* used for pointer arithmetic
	INT32 found_value = get_max ? INT32_MIN : INT32_MAX;
	int i;
	int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);

	for (i = 0; i < cv_ps_samplesize.value; i++)
	{
		INT32 value;
		if (time_metric)
			value = (*((precise_t*)history_read_pos)) / (I_GetPrecisePrecision() / 1000000);
		else
			value = *((INT32*)history_read_pos);

		if ((get_max && value > found_value) ||
			(!get_max && value < found_value))
		{
			found_value = value;
		}
		history_read_pos += value_size;
	}

	return found_value;
}

// Calculates the standard deviation for metric.
static INT32 PS_GetMetricSD(ps_metric_t *metric, boolean time_metric)
{
	char* history_read_pos = metric->history; // char* used for pointer arithmetic
	INT64 sum = 0;
	int i;
	int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);
	INT32 avg = PS_GetMetricAverage(metric, time_metric);

	for (i = 0; i < cv_ps_samplesize.value; i++)
	{
		INT64 value;
		if (time_metric)
			value = (*((precise_t*)history_read_pos)) / (I_GetPrecisePrecision() / 1000000);
		else
			value = *((INT32*)history_read_pos);

		value -= avg;
		sum += value * value;

		history_read_pos += value_size;
	}

	return round(sqrt(sum / cv_ps_samplesize.value));
}

// Returns the value to show on screen for metric.
static INT32 PS_GetMetricScreenValue(ps_metric_t *metric, boolean time_metric)
{
	if (cv_ps_samplesize.value > 1 && metric->history)
	{
		if (cv_ps_descriptor.value == 1)
			return PS_GetMetricAverage(metric, time_metric);
		else if (cv_ps_descriptor.value == 2)
			return PS_GetMetricSD(metric, time_metric);
		else if (cv_ps_descriptor.value == 3)
			return PS_GetMetricMinOrMax(metric, time_metric, false);
		else
			return PS_GetMetricMinOrMax(metric, time_metric, true);
	}
	else
	{
		if (time_metric)
			return (metric->value.p) / (I_GetPrecisePrecision() / 1000000);
		else
			return metric->value.i;
	}
}

static int PS_DrawPerfRows(int x, int y, int color, perfstatrow_t *rows)
{
	const boolean hires = PS_HighResolution();
	INT32 draw_flags = V_MONOSPACE | color;
	perfstatrow_t * row;
	int draw_y = y;

	if (hires)
		draw_flags |= V_ALLOWLOWERCASE;

	for (row = rows; row->lores_label; ++row)
	{
		const char *label;
		INT32 value;
		char *final_str;

		if (!PS_IsRowVisible(row))
			continue;

		label = hires ? row->hires_label : row->lores_label;
		value = PS_GetMetricScreenValue(row->metric, !!(row->flags & PS_TIME));
		final_str = va("%s %d", label, value);

		if (hires)
		{
			V_DrawSmallString(x, draw_y, draw_flags, final_str);
			draw_y += 5;
		}
		else
		{
			V_DrawThinString(x, draw_y, draw_flags, final_str);
			draw_y += 8;
		}
	}

	return draw_y;
}

static void PS_UpdateMetricHistory(ps_metric_t *metric, boolean time_metric, boolean frame_metric)
{
	int index = frame_metric ? ps_frame_index : ps_tick_index;

	if (!metric->history)
	{
		// allocate history table
		int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);
		void** memory_user = &metric->history;

		metric->history = Z_Calloc(value_size * cv_ps_samplesize.value, PU_PERFSTATS,
				memory_user);

		// reset "samples left" counter since this history table needs to be filled
		if (frame_metric)
			ps_frame_samples_left = cv_ps_samplesize.value;
		else
			ps_tick_samples_left = cv_ps_samplesize.value;
	}

	if (time_metric)
	{
		precise_t *history = (precise_t*)metric->history;
		history[index] = metric->value.p;
	}
	else
	{
		INT32 *history = (INT32*)metric->history;
		history[index] = metric->value.i;
	}
}

static void PS_UpdateRowHistories(perfstatrow_t *rows, boolean frame_metric)
{
	perfstatrow_t *row;
	for (row = rows; row->lores_label; row++)
	{
		if (PS_IsRowValid(row))
			PS_UpdateMetricHistory(row->metric, !!(row->flags & PS_TIME), frame_metric);
	}
}

// Update all metrics that are calculated on every frame.
void PS_UpdateFrameStats(void)
{
	// update frame time
	precise_t currenttime = I_GetPreciseTime();
	precise_t attributedrendertime;
	ps_frametime.value.p = ps_prevframetime ? currenttime - ps_prevframetime : 0;
	ps_prevframetime = currenttime;

	// update 3d rendering stats
	if (PS_IsLevelActive())
	{
		// Remember to update this calculation when adding more 3d rendering stats!
		attributedrendertime = ps_bsptime.value.p;

#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			attributedrendertime +=
				ps_hw_skyboxtime.value.p +
				ps_hw_portaltime.value.p +
				ps_hw_nodesorttime.value.p +
				ps_hw_nodedrawtime.value.p +
				ps_hw_spritesorttime.value.p +
				ps_hw_spritedrawtime.value.p;

			if (cv_glbatching.value)
			{
				attributedrendertime +=
					ps_hw_batchsorttime.value.p +
					ps_hw_batchdrawtime.value.p;
			}
		}
		else
#endif
		{
			attributedrendertime +=
				ps_sw_spritecliptime.value.p +
				ps_sw_portaltime.value.p +
				ps_sw_planetime.value.p +
				ps_sw_maskedtime.value.p;
		}

		/* Some renderer timers can overlap by a few ticks. precise_t is unsigned,
		 * so subtracting a slightly larger attributed total used to wrap the
		 * "Other" row into a multi-billion value and pollute its average. */
		ps_otherrendertime.value.p = ps_rendercalltime.value.p > attributedrendertime
			? ps_rendercalltime.value.p - attributedrendertime : 0;
	}

	if (cv_ps_samplesize.value > 1)
	{
		PS_UpdateRowHistories(rendertime_rows, true);
		if (PS_IsLevelActive())
			PS_UpdateRowHistories(commoncounter_rows, true);

		if (R_UsingFrameInterpolation())
			PS_UpdateRowHistories(interpolation_rows, true);

#ifdef HWRENDER
		if (rendermode == render_opengl && cv_glbatching.value)
		{
			PS_UpdateRowHistories(batchcount_rows, true);
			PS_UpdateRowHistories(batchcalls_rows, true);
		}
#endif

		ps_frame_index++;
		if (ps_frame_index >= cv_ps_samplesize.value)
			ps_frame_index = 0;
		if (ps_frame_samples_left)
			ps_frame_samples_left--;
	}
}

// Update thinker counters by iterating the thinker lists.
static void PS_CountThinkers(void)
{
	int i;
	thinker_t *thinker;

	ps_thinkercount.value.i = 0;
	ps_polythcount.value.i = 0;
	ps_mainthcount.value.i = 0;
	ps_mobjcount.value.i = 0;
	ps_regularcount.value.i = 0;
	ps_scenerycount.value.i = 0;
	ps_nothinkcount.value.i = 0;
	ps_dynslopethcount.value.i = 0;
	ps_precipcount.value.i = 0;
	ps_removecount.value.i = 0;

	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (thinker = thlist[i].next; thinker != &thlist[i]; thinker = thinker->next)
		{
			ps_thinkercount.value.i++;
			if (thinker->removing)
				ps_removecount.value.i++;
			else if (i == THINK_POLYOBJ)
				ps_polythcount.value.i++;
			else if (i == THINK_MAIN)
				ps_mainthcount.value.i++;
			else if (i == THINK_MOBJ)
			{
				if (thinker->function.acp1 == (actionf_p1)P_MobjThinker)
				{
					mobj_t *mobj = (mobj_t*)thinker;
					ps_mobjcount.value.i++;
					if (mobj->flags & MF_NOTHINK)
						ps_nothinkcount.value.i++;
					else if (mobj->flags & MF_SCENERY)
						ps_scenerycount.value.i++;
					else
						ps_regularcount.value.i++;
				}
			}
			else if (i == THINK_DYNSLOPE)
				ps_dynslopethcount.value.i++;
			else if (i == THINK_PRECIP)
				ps_precipcount.value.i++;
		}
	}
}

// Update all metrics that are calculated on every tick.
void PS_UpdateTickStats(void)
{
	if (PS_IsLevelActive())
	{
		const precise_t attributed = ps_playerthink_time.value.p
			+ ps_thinkertime.value.p + ps_lua_prethinkframe_time.value.p
			+ ps_lua_thinkframe_time.value.p + ps_lua_postthinkframe_time.value.p;
		/* Hitch and perfreport diagnostics need this even when the visual
		 * perfstats page is hidden. The old conditional left every normal user's
		 * report saying other_logic_ms=0, concealing first-tic/map-script stalls. */
		ps_otherlogictime.value.p = ps_tictime.value.p > attributed
			? ps_tictime.value.p - attributed : 0;
	}
	if (cv_perfstats.value == 1 && cv_ps_samplesize.value > 1)
	{
		PS_UpdateRowHistories(gamelogicbrief_row, false);
	}
	if (cv_perfstats.value == 2)
	{
		if (PS_IsLevelActive())
		{
			PS_CountThinkers();
		}

		if (cv_ps_samplesize.value > 1)
		{
			PS_UpdateRowHistories(gamelogic_rows, false);
			PS_UpdateRowHistories(thinkercount_rows, false);
			PS_UpdateRowHistories(misc_calls_rows, false);
		}
	}
	if (cv_ps_samplesize.value > 1)
	{
		if(cv_perfstats.value >= 3 && PS_IsLevelActive())
		{
			int i;
			if (cv_perfstats.value == 3)
			{
				for (i = 0; i < thinkframe_hooks_length; i++)
					PS_UpdateMetricHistory(&thinkframe_hooks[i].time_taken, true, false);
			}
			else if (cv_perfstats.value == 4)
			{
				for (i = 0; i < prethinkframe_hooks_length; i++)
					PS_UpdateMetricHistory(&prethinkframe_hooks[i].time_taken, true, false);
			}
			else if (cv_perfstats.value == 5)
			{
				for (i = 0; i < postthinkframe_hooks_length; i++)
					PS_UpdateMetricHistory(&postthinkframe_hooks[i].time_taken, true, false);
			}
		}
		if (cv_perfstats.value)
		{
			ps_tick_index++;
			if (ps_tick_index >= cv_ps_samplesize.value)
				ps_tick_index = 0;
			if (ps_tick_samples_left)
				ps_tick_samples_left--;
		}
	}
}

static void PS_DrawDescriptorHeader(void)
{
	if (cv_ps_samplesize.value > 1)
	{
		const char* descriptor_names[] = {
			"average",
			"standard deviation",
			"minimum",
			"maximum"
		};
		const boolean hires = PS_HighResolution();
		char* str;
		INT32 flags = V_MONOSPACE | V_ALLOWLOWERCASE;
		int samples_left = max(ps_frame_samples_left, ps_tick_samples_left);
		int x, y;

		if (cv_perfstats.value >= 3)
		{
			x = 2;
			y = 0;
		}
		else
		{
			x = 20;
			y = hires ? 5 : 2;
		}

		if (samples_left)
		{
			str = va("Samples needed for correct results: %d", samples_left);
			flags |= V_REDMAP;
		}
		else
		{
			str = va("Showing the %s of %d samples.",
					descriptor_names[cv_ps_descriptor.value - 1], cv_ps_samplesize.value);
			flags |= V_GREENMAP;
		}

		if (hires)
			V_DrawSmallString(x, y, flags, str);
		else
			V_DrawThinString(x, y, flags, str);
	}
}

static void PS_DrawRenderStats(void)
{
	const boolean hires = PS_HighResolution();
	const int half_row = hires ? 5 : 4;
	int x, y, cy = 10;

	PS_DrawDescriptorHeader();

	y = PS_DrawPerfRows(20, 10, V_YELLOWMAP, rendertime_rows);

	PS_DrawPerfRows(20, y + half_row, V_GRAYMAP, gamelogicbrief_row);

	if (PS_IsLevelActive())
	{
		x = hires ? 115 : 90;
		cy = PS_DrawPerfRows(x, 10, V_BLUEMAP, commoncounter_rows) + half_row;

#ifdef HWRENDER
		if (rendermode == render_opengl && cv_glbatching.value)
		{
			x = hires ? 200 : 155;
			y = PS_DrawPerfRows(x, 10, V_PURPLEMAP, batchcount_rows);

			x = hires ? 200 : 220;
			y = hires ? y + half_row : 10;
			PS_DrawPerfRows(x, y, V_PURPLEMAP, batchcalls_rows);
		}
#endif
	}

	if (R_UsingFrameInterpolation())
	{
		x = hires ? 115 : 90;
		PS_DrawPerfRows(x, cy, V_ROSYMAP, interpolation_rows);
	}
}

static void PS_DrawGameLogicStats(void)
{
	const boolean hires = PS_HighResolution();
	int x, y;

	PS_DrawDescriptorHeader();

	PS_DrawPerfRows(20, 10, V_YELLOWMAP, gamelogic_rows);

	x = hires ? 115 : 90;
	PS_DrawPerfRows(x, 10, V_BLUEMAP, thinkercount_rows);

	if (hires)
		V_DrawSmallString(212, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, "Calls:");

	x = hires ? 216 : 170;
	y = hires ? 15 : 10;
	PS_DrawPerfRows(x, y, V_PURPLEMAP, misc_calls_rows);
}

static void draw_think_frame_stats(int hook_length, ps_hookinfo_t *hook)
{
	char s[100];
	int i;
	// text writing position
	int x = 2;
	int y = 4;
	UINT32 text_color;
	char tempbuffer[LUA_IDSIZE];
	char last_mod_name[LUA_IDSIZE];
	last_mod_name[0] = '\0';

	PS_DrawDescriptorHeader();

	for (i = 0; i < hook_length; i++)
	{

#define NEXT_ROW() \
y += 4; \
if (y > 192) \
{ \
	y = 4; \
	x += 106; \
	if (x > 214) \
		break; \
}

		char* str = hook[i].short_src;
		char* tempstr = tempbuffer;
		int len = (int)strlen(str);
		char* str_ptr;
		if (strcmp(".lua", str + len - 4) == 0)
		{
			str[len-4] = '\0'; // remove .lua at end
			len -= 4;
		}
		// we locate the wad/pk3 name in the string and compare it to
		// what we found on the previous iteration.
		// if the name has changed, print it out on the screen
		strcpy(tempstr, str);
		str_ptr = strrchr(tempstr, '|');
		if (str_ptr)
		{
			*str_ptr = '\0';
			str = str_ptr + 1; // this is the name of the hook without the mod file name
			str_ptr = strrchr(tempstr, PATHSEP[0]);
			if (str_ptr)
				tempstr = str_ptr + 1;
			// tempstr should now point to the mod name, (wad/pk3) possibly truncated
			if (strcmp(tempstr, last_mod_name) != 0)
			{
				strcpy(last_mod_name, tempstr);
				len = (int)strlen(tempstr);
				if (len > 25)
					tempstr += len - 25;
				snprintf(s, sizeof s - 1, "%s", tempstr);
				V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
				NEXT_ROW()
			}
			text_color = V_YELLOWMAP;
		}
		else
		{
			// probably a standalone lua file
			// cut off the folder if it's there
			str_ptr = strrchr(tempstr, PATHSEP[0]);
			if (str_ptr)
				str = str_ptr + 1;
			text_color = 0; // white
		}
		len = (int)strlen(str);
		if (len > 20)
			str += len - 20;
		snprintf(s, sizeof s - 1, "%20s: %d", str,
				PS_GetMetricScreenValue(&hook[i].time_taken, true));
		V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | text_color, s);
		NEXT_ROW()

#undef NEXT_ROW

	}
}

static void PS_DrawPreThinkFrameStats(void)
{
	draw_think_frame_stats(prethinkframe_hooks_length, prethinkframe_hooks);
}

static void PS_DrawThinkFrameStats(void)
{
	draw_think_frame_stats(thinkframe_hooks_length, thinkframe_hooks);
}

static void PS_DrawPostThinkFrameStats(void)
{
	draw_think_frame_stats(postthinkframe_hooks_length, postthinkframe_hooks);
}

void M_DrawPerfStats(void)
{
	if (cv_perfstats.value == 1) // rendering
	{
		PS_DrawRenderStats();
	}
	else if (cv_perfstats.value == 2) // logic
	{
		// PS_UpdateTickStats is called in TryRunTics, since otherwise it would miss
		// tics when frame skips happen
		PS_DrawGameLogicStats();
	}
	else if (cv_perfstats.value >= 3) // lua thinkframe
	{
		if (!PS_IsLevelActive())
			return;
		if (!PS_HighResolution())
		{
			// Low resolutions can't really use V_DrawSmallString that is used by thinkframe stats.
			// A low-res version using V_DrawThinString could be implemented,
			// but it would have much less space for information.
			V_DrawThinString(80, 92, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "Lua Perfstats is not available");
			V_DrawThinString(80, 100, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "for resolutions below 640x400.");
			return;
		}
		if (cv_perfstats.value == 3)
		{
			PS_DrawThinkFrameStats();
		}
		else if (cv_perfstats.value == 4)
		{
			PS_DrawPreThinkFrameStats();
		}
		else if (cv_perfstats.value == 5)
		{
			PS_DrawPostThinkFrameStats();
		}
	}
}

// remove and unallocate history from all metrics
static void PS_ClearHistory(void)
{
	int i;

	Z_FreeTag(PU_PERFSTATS);
	// thinkframe hook metric history pointers need to be cleared manually
	for (i = 0; i < thinkframe_hooks_length; i++)
	{
		thinkframe_hooks[i].time_taken.history = NULL;
	}

	ps_frame_index = ps_tick_index = 0;
	// PS_UpdateMetricHistory will set these correctly when it runs
	ps_frame_samples_left = ps_tick_samples_left = 0;
}

void PS_PerfStats_OnChange(void)
{
	if (cv_perfstats.value && cv_ps_samplesize.value > 1)
		PS_ClearHistory();
}

void PS_SampleSize_OnChange(void)
{
	if (cv_ps_samplesize.value > 1)
		PS_ClearHistory();
}
