
// Distributed under the GNU General Public License, version 2.


#include "doomdef.h"
#include "command.h"
#include "console.h"
#include "doomstat.h"
#include "g_game.h"
#include "g_input.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "m_inputlatency.h"
#include "m_menu.h"
#include "p_local.h"
#include "netcode/d_clisrv.h"
#include "netcode/tic_command.h"

enum
{
	IL_EVENT,
	IL_COMMAND,
	IL_SENT,
	IL_RECEIVED,
	IL_TICK_START,
	IL_APPLIED,
	IL_PRESENTED,
	IL_PHASES
};

#define INPUT_LATENCY_SAMPLES 32

typedef struct
{
	INT32 control;
	UINT16 button;
	const char *name;
} input_latency_action_t;

static const input_latency_action_t actions[] = {
	{GC_JUMP, BT_JUMP, "Jump"},
	{GC_SPIN, BT_SPIN, "Spin"},
	{GC_FIRE, BT_ATTACK, "Fire"},
	{GC_FIRENORMAL, BT_FIRENORMAL, "Normal Fire"}
};

typedef struct
{
	const input_latency_action_t *action;
	precise_t at[IL_PHASES];
	boolean remote;
	UINT32 ping;
	INT32 buffer_mode;
} input_latency_sample_t;

boolean inputlatency_recording;
static input_latency_sample_t probe;
static tic_t probe_tic;
static INT32 probe_player;
static input_latency_sample_t samples[INPUT_LATENCY_SAMPLES];
static UINT32 next_sample, sample_count, expired_samples;

void M_InputLatencyCancel(void)
{
	probe.action = NULL;
}

void M_InputLatencyKeyDown(INT32 key, precise_t posted)
{
	if (!posted || probe.action || key <= 0 || key >= NUMINPUTS || gamekeydown[key]
		|| gamestate != GS_LEVEL || demoplayback || dedicated || menuactive || CON_Ready()
		|| chat_on || paused || P_AutoPause() || displayplayer != consoleplayer
		|| consoleplayer < 0 || consoleplayer >= MAXPLAYERS
		|| !playeringame[consoleplayer] || players[consoleplayer].spectator
		|| players[consoleplayer].playerstate != PST_LIVE || players[consoleplayer].exiting)
		return;

	const boolean remote = client && netgame;
	for (size_t i = 0; i < sizeof actions / sizeof *actions; i++)
	{
		const input_latency_action_t *action = &actions[i];
		const INT32 *bindings = gamecontrol[action->control];
		if (key != bindings[0] && key != bindings[1])
			continue;

		if ((localcmds.buttons & action->button)
			|| (remote && neededtic && (netcmds[(neededtic - 1) % BACKUPTICS][consoleplayer].buttons & action->button)))
			continue;

		memset(&probe, 0, sizeof probe);
		probe.action = action;
		probe.at[IL_EVENT] = posted;
		probe.remote = remote;
		probe.buffer_mode = remote ? cv_netlowlatency.value : 0;
		probe_player = consoleplayer;
		return;
	}
}

void M_InputLatencyBuilt(void)
{
	if (!probe.action || probe.at[IL_COMMAND] || !(localcmds.buttons & probe.action->button))
		return;

	probe.at[IL_COMMAND] = I_GetPreciseTime();
	if (!probe.remote)
	{
		probe.at[IL_SENT] = probe.at[IL_RECEIVED] = probe.at[IL_COMMAND];
		probe_tic = maketic;
	}
}

void M_InputLatencySent(void)
{
	if (!probe.action || !probe.remote || !probe.at[IL_COMMAND] || probe.at[IL_SENT]
		|| !(localcmds.buttons & probe.action->button))
		return;

	probe.at[IL_SENT] = I_GetPreciseTime();
	probe_tic = neededtic;
}

void M_InputLatencyServerTics(tic_t first, tic_t end)
{
	if (!probe.action || !probe.remote || !probe.at[IL_SENT] || probe.at[IL_RECEIVED])
		return;



	for (tic_t tic = max(first, probe_tic); tic < end; tic++)
	{
		if (!(netcmds[tic % BACKUPTICS][probe_player].buttons & probe.action->button))
			continue;
		probe.at[IL_RECEIVED] = I_GetPreciseTime();
		probe_tic = tic;
		return;
	}
}

void M_InputLatencyApplied(precise_t tick_started)
{
	if (!probe.action || !probe.at[IL_RECEIVED] || probe.at[IL_APPLIED])
		return;
	if (gametic < probe_tic || (probe.remote && gametic != probe_tic)
		|| !(netcmds[gametic % BACKUPTICS][probe_player].buttons & probe.action->button))
		return;

	probe.at[IL_TICK_START] = max(tick_started, probe.at[IL_RECEIVED]);
	probe.at[IL_APPLIED] = I_GetPreciseTime();
}

void M_InputLatencyPresented(void)
{
	if (!probe.action)
		return;

	const precise_t now = I_GetPreciseTime();
	if (gamestate != GS_LEVEL || probe_player != consoleplayer
		|| probe.remote != (client && netgame)
		|| (probe.remote && probe.buffer_mode != cv_netlowlatency.value)
		|| now - probe.at[IL_EVENT] >= 2 * I_GetPrecisePrecision())
	{
		expired_samples++;
		M_InputLatencyCancel();
		return;
	}
	if (!probe.at[IL_APPLIED])
		return;

	probe.at[IL_PRESENTED] = now;
	probe.ping = probe.remote ? playerpingtable[probe_player] : 0;
	if (sample_count)
	{
		const input_latency_sample_t *last = &samples[(next_sample + INPUT_LATENCY_SAMPLES - 1) % INPUT_LATENCY_SAMPLES];
		if (last->remote != probe.remote || last->buffer_mode != probe.buffer_mode)
			sample_count = next_sample = 0;
	}
	samples[next_sample] = probe;
	next_sample = (next_sample + 1) % INPUT_LATENCY_SAMPLES;
	sample_count = min(sample_count + 1, INPUT_LATENCY_SAMPLES);
	M_InputLatencyCancel();
}

static void M_PrintInputLatency(void)
{
	CONS_Printf("Input latency recording %s. %u unmatched or expired presses.\n",
		inputlatency_recording ? "on" : "off", expired_samples);
	if (!sample_count)
	{
		CONS_Printf("No completed samples. Use inputlatency on, close the console, then tap Jump, Spin, or Fire in a level.\n");
		return;
	}

	const input_latency_sample_t *last = &samples[(next_sample + INPUT_LATENCY_SAMPLES - 1) % INPUT_LATENCY_SAMPLES];
	const double ms_per_count = 1000.0 / I_GetPrecisePrecision();
	const char *const labels[] = {
		"Input event to command", "Command to send", "Send to server update",
		"Waiting in tick queue", "Processing game tick", "Game tick to present", "Total"
	};
	CONS_Printf("Last press %s. %u recent %s samples.\n",
		last->action->name, sample_count, last->remote ? "online" : "local");
	if (last->remote)
		CONS_Printf("Server-reported ping at last press %u ms.\n", last->ping);
	CONS_Printf("%-25s %7s %7s %7s\n", "Timing in ms", "Last", "Average", "Peak");

	for (size_t i = 0; i < IL_PHASES; i++)
	{
		if (!last->remote && (i == IL_COMMAND || i == IL_SENT))
			continue;

		const size_t start = i == IL_PRESENTED ? IL_EVENT : i;
		const size_t end = i == IL_PRESENTED ? IL_PRESENTED : i + 1;
		precise_t sum = 0, peak = 0;
		for (UINT32 j = 0; j < sample_count; j++)
		{
			const precise_t elapsed = samples[j].at[end] - samples[j].at[start];
			sum += elapsed;
			peak = max(peak, elapsed);
		}
		const char *label = !last->remote && i == IL_RECEIVED ? "Command to tick start" : labels[i];
		CONS_Printf("%-25s %7.1f %7.1f %7.1f\n", label,
			(last->at[end] - last->at[start]) * ms_per_count,
			sum * ms_per_count / sample_count, peak * ms_per_count);
	}
}

static void Command_InputLatency(void)
{
	const char *arg = COM_Argv(1);
	if (COM_Argc() == 1)
		M_PrintInputLatency();
	else if (COM_Argc() == 2 && !stricmp(arg, "on"))
	{
		M_InputLatencyCancel();
		sample_count = next_sample = expired_samples = 0;
		inputlatency_recording = true;
		CONS_Printf("Input latency recording on. Close the console and tap Jump, Spin, or Fire a few times in a level. Run inputlatency to read the results.\n");
	}
	else if (COM_Argc() == 2 && !stricmp(arg, "off"))
	{
		inputlatency_recording = false;
		M_InputLatencyCancel();
		CONS_Printf("Input latency recording off.\n");
	}
	else
		CONS_Printf("inputlatency [on|off]\n");
}

void M_RegisterInputLatency(void)
{
	COM_AddCommand("inputlatency", Command_InputLatency, 0);
}
