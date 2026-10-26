// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Optional Discord Social SDK rich-presence bridge for the custom Windows
// build. This module is local-only and never touches SRB2's network protocol.
//-----------------------------------------------------------------------------

#ifndef __DISCORD_PRESENCE_H__
#define __DISCORD_PRESENCE_H__

void DRPC_RegisterVariables(void);
void DRPC_Init(void);
void DRPC_Update(void);
void DRPC_Shutdown(void);

#endif // __DISCORD_PRESENCE_H__
