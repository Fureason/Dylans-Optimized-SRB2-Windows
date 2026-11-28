#ifndef __M_INPUTLATENCY__
#define __M_INPUTLATENCY__

#include "doomtype.h"

extern boolean inputlatency_recording;

void M_RegisterInputLatency(void);
void M_InputLatencyKeyDown(INT32 key, precise_t posted);
void M_InputLatencyBuilt(void);
void M_InputLatencySent(void);
void M_InputLatencyServerTics(tic_t first, tic_t end);
void M_InputLatencyApplied(precise_t tick_started);
void M_InputLatencyPresented(void);
void M_InputLatencyCancel(void);

#endif
