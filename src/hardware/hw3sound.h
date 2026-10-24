// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2001 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __HW3_SOUND_H__
#define __HW3_SOUND_H__

#ifdef HW3SOUND
#include "hws_data.h"




enum
{
	HWS_DEFAULT_MODE = 0,
	HWS_DS3D,
	HWS_OTHER,
	HWS_FMOD3D,
	HWS_OPENAL,
};

typedef enum
{
	CT_NORMAL = 0,
	CT_ATTACK,
	CT_SCREAM,
	CT_AMBIENT,
} channel_type_t;

extern INT32  hws_mode;            

#if defined (HW3DS) || defined (DOXYGEN)
INT32 HW3S_Init(I_Error_t FatalErrorFunction, snddev_t *snd_dev);
#endif
void HW3S_Shutdown(void);
INT32 HW3S_GetVersion(void);


void HW3S_StartSound(const void *origin, sfxenum_t sfx_id);


void S_StartAmbientSound(sfxenum_t sfx_id, INT32 volume);
void S_StartAttackSound(const void *origin, sfxenum_t sfx_id);
void S_StartScreamSound(const void *origin, sfxenum_t sfx_id);










INT32 HW3S_I_StartSound(const void *origin, source3D_data_t *source_parm, channel_type_t channel, sfxenum_t sfx_id, INT32 vol, INT32 pitch, INT32 sep);
void HW3S_StopSoundByID(void *origin, sfxenum_t sfx_id);
void HW3S_StopSoundByNum(sfxenum_t sfxnum);
void HW3S_StopSound(void *origin);
void HW3S_StopSounds(void);

void HW3S_BeginFrameUpdate(void);
void HW3S_EndFrameUpdate(void);

void HW3S_UpdateSources(void);

void HW3S_SetSfxVolume(INT32 volume);


INT32  HW3S_SoundIsPlaying(INT32 handle);
void HW3S_SetSourcesNum(void);

INT32  HW3S_OriginPlaying(void *origin);
INT32  HW3S_IdPlaying(sfxenum_t id);
INT32  HW3S_SoundPlaying(void *origin, sfxenum_t id);

void *HW3S_GetSfx(sfxinfo_t *sfx);
void HW3S_FreeSfx(sfxinfo_t *sfx);

#else  


#define S_StartAttackSound S_StartSound
#define S_StartScreamSound S_StartSound

#endif  

INT32 S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source,
	INT32 *vol, INT32 *sep, INT32 *pitch, sfxinfo_t *sfxinfo);

#endif  
