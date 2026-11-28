
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

#ifndef __DISCORD_PRESENCE_H__
#define __DISCORD_PRESENCE_H__

#ifdef _WIN32
#include "command.h"

extern consvar_t cv_discordpresence;
#endif

void DRPC_RegisterVariables(void);
void DRPC_Init(void);
void DRPC_Update(void);
void DRPC_Shutdown(void);

#endif
