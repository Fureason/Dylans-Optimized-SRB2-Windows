
#ifndef __M_HOSTGUARD_H__
#define __M_HOSTGUARD_H__

#include "doomtype.h"

void HG_Register(void);
void HG_ResetMap(void);
void HG_RecordSpawn(void);
void HG_RecordRemoval(void);
void HG_RecordTick(precise_t elapsed);
void HG_RecordFrame(precise_t elapsed);

#endif
