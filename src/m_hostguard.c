

#include "m_hostguard.h"
#include "command.h"
#include "console.h"
#include "d_main.h"
#include "doomstat.h"
#include "g_game.h"
#include "i_system.h"
#include "i_time.h"
#include "p_local.h"
#include "p_setup.h"
#include "netcode/d_clisrv.h"

#include <time.h>

static CV_PossibleValue_t added_range[] = {{100, "MIN"}, {100000, "MAX"}, {0, NULL}};
static CV_PossibleValue_t burst_range[] = {{50, "MIN"}, {100000, "MAX"}, {0, NULL}};
static consvar_t cv_hostguard = CVAR_INIT("hostoverloadguard", "On", CV_SAVE, CV_OnOff, NULL);
static consvar_t cv_hostguard_added = CVAR_INIT("hostoverloadadded", "5000", CV_SAVE, added_range, NULL);
static consvar_t cv_hostguard_burst = CVAR_INIT("hostoverloadburst", "2000", CV_SAVE, burst_range, NULL);
static consvar_t cv_hostguard_fallback = CVAR_INIT("hostoverloadfallback", "On", CV_SAVE, CV_OnOff, NULL);

static UINT32 live_objects, baseline_objects, spawned_this_tic;
static tic_t spawn_tic, last_recovery_time;
static UINT8 slow_tics, slow_frames, recovery_attempts;
static boolean baseline_set, recovery_pending, cooldown_notified;
static INT32 recovery_map;
static precise_t last_logic_time, last_frame_time;

static boolean HG_CanRecover(void)
{
	return server && (netgame || multiplayer) && gamestate == GS_LEVEL
		&& !levelloading && !paused && !demoplayback && !modeattacking
		&& cv_hostguard.value && baseline_set && leveltime >= 2*TICRATE;
}

static void HG_WriteIncident(const char *action, const char *reason)
{
	char directory[MAX_WADPATH], filename[MAX_WADPATH * 2];
	FILE *report;
	const precise_t precision = I_GetPrecisePrecision();
	snprintf(directory, sizeof directory, "%s" PATHSEP "diagnostics", srb2home);
	I_mkdir(directory, 0755);
	snprintf(filename, sizeof filename, "%s" PATHSEP "host-overload-v1.txt", directory);
	report = fopen(filename, "a");
	if (!report)
		return;
	fprintf(report,
		"time=%llu action=%s reason=%s map=%s gametic=%u leveltime=%u "
		"objects=%u baseline=%u added=%u burst=%u attempt=%u players=%d "
		"logic_ms=%.3f frame_ms=%.3f\n",
		(unsigned long long)time(NULL), action, reason, G_BuildMapName(gamemap),
		(unsigned)gametic, (unsigned)leveltime, (unsigned)live_objects,
		(unsigned)baseline_objects, (unsigned)(live_objects - baseline_objects),
		(unsigned)spawned_this_tic, (unsigned)recovery_attempts, D_NumPlayers(),
		precision ? (double)last_logic_time * 1000.0 / precision : 0.0,
		precision ? (double)last_frame_time * 1000.0 / precision : 0.0);
	fclose(report);
}

static void HG_RequestRecovery(const char *reason)
{
	const tic_t now = I_GetTime();
	const boolean recent = recovery_attempts && (tic_t)(now - last_recovery_time) < 120*TICRATE;
	char command[80];

	if (recovery_pending)
		return;
	if (!recent)
		recovery_attempts = 0;
	if (recent && !cv_hostguard_fallback.value)
	{
		if (!cooldown_notified)
		{
			CONS_Alert(CONS_WARNING, "Host protection: overload returned; automatic reload cooldown is active. Clear spawned objects or choose a smaller map.\n");
			HG_WriteIncident("cooldown", reason);
			cooldown_notified = true;
		}
		return;
	}

	recovery_pending = true;
	recovery_map = gamemap;
	last_recovery_time = now;
	cooldown_notified = false;
	recovery_attempts++;
	if (recovery_attempts >= 3)
	{
		CONS_Alert(CONS_WARNING, "Host protection: repeated overload after recovery. Pausing to stop the reload loop; choose a map or remove the heavy addon before resuming.\n");
		HG_WriteIncident("pause", reason);
		COM_BufAddText("pause 1\n");
	}
	else if (recovery_attempts == 2 && cv_hostguard_fallback.value)
	{
		recovery_map = 1;
		CONS_Alert(CONS_WARNING, "Host protection: overload returned; switching to GFZ1 in Co-op (%u added objects).\n", (unsigned)(live_objects - baseline_objects));
		HG_WriteIncident("fallback", reason);
		snprintf(command, sizeof command, "map MAP01 -gametype %d -force\n", GT_COOP);
		COM_BufAddText(command);
	}
	else
	{
		CONS_Alert(CONS_WARNING, "Host protection: reloading %s after object overload (%u added objects).\n", G_BuildMapName(gamemap), (unsigned)(live_objects - baseline_objects));
		HG_WriteIncident("reload", reason);
		snprintf(command, sizeof command, "map %s -force\n", G_BuildMapName(gamemap));
		COM_BufAddText(command);
	}
}

static void HG_Check(boolean from_spawn)
{
	UINT32 added;
	if (!HG_CanRecover() || recovery_pending || live_objects < baseline_objects)
		return;
	added = live_objects - baseline_objects;
	if (added < (UINT32)cv_hostguard_added.value)
		return;
	if (spawn_tic == gametic && spawned_this_tic >= (UINT32)cv_hostguard_burst.value)
		HG_RequestRecovery("rapid-spawning");
	else if (!from_spawn && added >= 2U * (UINT32)cv_hostguard_added.value)
		HG_RequestRecovery("object-accumulation");
	else if (!from_spawn && (slow_tics >= 3 || slow_frames >= 3))
		HG_RequestRecovery("sustained-slowdown");
}

void HG_ResetMap(void)
{


	if (!recovery_pending || recovery_map != gamemap)
	{
		recovery_attempts = 0;
		last_recovery_time = 0;
	}
	live_objects = baseline_objects = spawned_this_tic = 0;
	spawn_tic = 0;
	slow_tics = slow_frames = 0;
	baseline_set = recovery_pending = cooldown_notified = false;
	last_logic_time = last_frame_time = 0;
}

void HG_RecordSpawn(void)
{
	if (!server)
		return;
	if (live_objects < UINT32_MAX)
		live_objects++;
	if (spawn_tic != gametic)
	{
		spawn_tic = gametic;
		spawned_this_tic = 0;
	}
	if (spawned_this_tic < UINT32_MAX)
		spawned_this_tic++;



	HG_Check(true);
}

void HG_RecordRemoval(void)
{
	if (server && live_objects)
		live_objects--;
}

void HG_RecordTick(precise_t elapsed)
{
	const precise_t precision = I_GetPrecisePrecision();
	if (!server || gamestate != GS_LEVEL || levelloading)
		return;
	last_logic_time = elapsed;


	if (!baseline_set || leveltime < 2*TICRATE)
	{
		baseline_objects = max(baseline_objects, live_objects);
		baseline_set = true;
		slow_tics = slow_frames = 0;
		return;
	}
	if (!HG_CanRecover())
	{
		slow_tics = slow_frames = 0;
		return;
	}
	if (precision && elapsed >= precision/4)
	{
		if (slow_tics < UINT8_MAX)
			slow_tics++;
	}
	else
		slow_tics = 0;
	HG_Check(false);
}

void HG_RecordFrame(precise_t elapsed)
{
	const precise_t precision = I_GetPrecisePrecision();
	last_frame_time = elapsed;
	if (!HG_CanRecover())
	{
		slow_frames = 0;
		return;
	}
	if (precision && elapsed >= precision/4)
	{
		if (slow_frames < UINT8_MAX)
			slow_frames++;
	}
	else
		slow_frames = 0;
}

static void HG_Status(void)
{
	CONS_Printf("Host protection: %s, hosting=%d, objects=%u, baseline=%u, added threshold=%d, burst threshold=%d, recoveries=%u, pending=%d.\n",
		cv_hostguard.value ? "On" : "Off", server && (netgame || multiplayer),
		(unsigned)live_objects, (unsigned)baseline_objects, cv_hostguard_added.value,
		cv_hostguard_burst.value, (unsigned)recovery_attempts, recovery_pending);
}

void HG_Register(void)
{
	static boolean registered;
	if (registered)
		return;
	registered = true;
	CV_RegisterVar(&cv_hostguard);
	CV_RegisterVar(&cv_hostguard_added);
	CV_RegisterVar(&cv_hostguard_burst);
	CV_RegisterVar(&cv_hostguard_fallback);
	COM_AddCommand("hostoverloadstatus", HG_Status, COM_LUA);
}
