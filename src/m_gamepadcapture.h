// SONIC ROBO BLAST 2 -- shared controller setup capture, GPLv2.
#ifndef __M_GAMEPADCAPTURE_H__
#define __M_GAMEPADCAPTURE_H__

#define GP_CAPTURE_AXES (JOYAXISSET * 2)
#define GP_CAPTURE_THRESHOLD 20000
#define GP_CAPTURE_NEUTRAL 6500
#define GP_CAPTURE_STABLE 4

typedef enum
{
	GP_RELEASE_START, GP_MOVE_UP, GP_RELEASE_MIDDLE,
	GP_MOVE_RIGHT, GP_RELEASE_END, GP_CAPTURE_DONE
} gp_capture_phase_t;

typedef struct
{
	gp_capture_phase_t phase;
	boolean paired;
	UINT32 mask, peers[GP_CAPTURE_AXES];
	INT32 center[GP_CAPTURE_AXES];
	INT32 vertical, horizontal, binding, stable;
} gp_capture_t;

static void M_GPCaptureInit(gp_capture_t *capture, boolean paired, UINT32 mask,
	const INT32 *center, const UINT32 *peers)
{
	INT32 i;
	memset(capture, 0, sizeof *capture);
	capture->paired = paired;
	capture->mask = mask;
	for (i = 0; i < GP_CAPTURE_AXES; i++)
	{
		capture->center[i] = center[i];
		capture->peers[i] = peers[i];
	}
}

/* Poll actual device values, rather than letting menu arrow-key translation
 * turn a stick into an unrelated button binding. Save only after release. */
static void M_GPCaptureSample(gp_capture_t *capture, const INT32 *values)
{
	INT32 i, best = -1, peak = 0, second = 0;
	UINT32 mask = capture->mask;
	if (capture->phase == GP_CAPTURE_DONE) return;
	if (capture->phase == GP_RELEASE_START || capture->phase == GP_RELEASE_MIDDLE
		|| capture->phase == GP_RELEASE_END)
	{
		for (i = 0; i < GP_CAPTURE_AXES; i++)
			if ((mask & (1u << i)) && abs(values[i] - capture->center[i]) > GP_CAPTURE_NEUTRAL)
				break;
		if (i != GP_CAPTURE_AXES) capture->stable = 0;
		else if (++capture->stable >= GP_CAPTURE_STABLE)
		{
			capture->stable = 0;
			capture->phase = capture->phase == GP_RELEASE_START ? GP_MOVE_UP
				: capture->phase == GP_RELEASE_MIDDLE ? GP_MOVE_RIGHT : GP_CAPTURE_DONE;
		}
		return;
	}
	if (capture->phase == GP_MOVE_RIGHT)
	{
		INT32 vertical = abs(capture->vertical) - 1;
		mask &= ~(1u << vertical);
		if (capture->peers[vertical]) mask &= capture->peers[vertical];
	}
	for (i = 0; i < GP_CAPTURE_AXES; i++)
	{
		INT32 strength;
		if (!(mask & (1u << i))) continue;
		strength = abs(values[i] - capture->center[i]);
		if (strength > peak) { second = peak; peak = strength; best = i; }
		else if (strength > second) second = strength;
	}
	/* Ignore drift and ambiguous diagonal motion. */
	if (best < 0 || peak < GP_CAPTURE_THRESHOLD || peak * 4 < second * 5) return;
	i = values[best] > capture->center[best] ? best + 1 : -best - 1;
	if (!capture->paired)
	{
		capture->binding = i;
		capture->phase = GP_RELEASE_END;
	}
	else if (capture->phase == GP_MOVE_UP)
	{
		capture->vertical = i;
		capture->phase = GP_RELEASE_MIDDLE;
	}
	else
	{
		capture->horizontal = i;
		capture->phase = GP_RELEASE_END;
	}
}
#endif
