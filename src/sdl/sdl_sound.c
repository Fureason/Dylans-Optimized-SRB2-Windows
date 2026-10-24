// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2014-2023 by Sonic Team Junior.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------



#include <math.h>
#include "../doomdef.h"

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#if defined(HAVE_SDL) && SOUND==SOUND_SDL

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#ifdef HAVE_MIXER
#include <SDL_mixer.h>
 
#ifndef SDL_MIXER_COMPILEDVERSION
#define SDL_MIXER_COMPILEDVERSION \
	SDL_VERSIONNUM(MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL)
#endif

 
#ifndef SDL_MIXER_VERSION_ATLEAST
#define SDL_MIXER_VERSION_ATLEAST(X, Y, Z) \
	(SDL_MIXER_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))
#endif

#else
#define MIX_CHANNELS 8
#endif

#ifdef _WIN32
#include <direct.h>
#elif defined (__GNUC__)
#include <unistd.h>
#endif
#include "../z_zone.h"

#include "../m_swap.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../screen.h"  
#include "../doomdef.h"
#include "../doomstat.h"
#include "../s_sound.h"

#include "../d_main.h"

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"
#include "../hardware/hw3sound.h"
#include "hwsym_sdl.h"
#endif

#ifdef HAVE_LIBGME
#include "gme/gme.h"
#endif







#define NUM_CHANNELS            MIX_CHANNELS*4

#define INDEXOFSFX(x) ((sfxinfo_t *)x - S_sfx)

static Uint16 samplecount = 1024;  

typedef struct chan_struct
{

	Uint8 *data;  
	Uint8 *end;  


	Uint32 realstep;  
	Uint32 step;           
	Uint32 stepremainder;  
	Uint32 samplerate;  




	tic_t starttic;  



	INT32 handle;  


	void *id;  
	sfxenum_t sfxid;  
	INT32 vol;  
	INT32 sep;  


	Sint16* leftvol_lookup;  
	Sint16* rightvol_lookup;  
} chan_t;

static chan_t channels[NUM_CHANNELS];


static INT32 steptable[256];


static Sint16 vol_lookup[128 * 256];

UINT8 sound_started = false;
static SDL_mutex *Snd_Mutex = NULL;


static SDL_AudioSpec audio;

static SDL_bool musicStarted = SDL_FALSE;
#ifdef HAVE_MIXER
static SDL_mutex *Msc_Mutex = NULL;
 
#define MIDI_PATH     srb2home
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#define MIDI_PATH2    "/tmp"
#endif
#define MIDI_TMPFILE  "srb2music"
#define MIDI_TMPFILE2 "srb2wav"
static INT32 musicvol = 62;

#if SDL_MIXER_VERSION_ATLEAST(1,2,2)
#define MIXER_POS  
static void SDLCALL I_FinishMusic(void);
static double loopstartDig = 0.0l;
static SDL_bool loopingDig = SDL_FALSE;
static SDL_bool canlooping = SDL_TRUE;
#endif

#if SDL_MIXER_VERSION_ATLEAST(1,2,7)
#define USE_RWOPS  
#if 0  
#undef USE_RWOPS
#endif
#endif

#if SDL_MIXER_VERSION_ATLEAST(1,2,10)

#endif

#ifdef USE_RWOPS
static void * Smidi[2] = { NULL, NULL };
static SDL_bool canuseRW = SDL_TRUE;
#endif
static const char *fmidi[2] = { MIDI_TMPFILE, MIDI_TMPFILE2};

static const INT32 MIDIfade = 500;
static const INT32 Digfade = 0;

static Mix_Music *music[2] = { NULL, NULL };
#endif

typedef struct srb2audio_s {
	void *userdata;
#ifdef HAVE_LIBGME
	Music_Emu *gme_emu;
	UINT8 gme_pause;
	UINT8 gme_loop;
#endif
} srb2audio_t;

static srb2audio_t localdata;

                                                                               
                                                                                  
boolean I_StartMovieAudioCapture(const char *filename)
{
	(void)filename;
	return false;
}

void I_UpdateMovieAudioCapture(void) {}
void I_StopMovieAudioCapture(void) {}

static void Snd_LockAudio(void)  
{
	if (Snd_Mutex) SDL_LockMutex(Snd_Mutex);
	else if (sound_disabled) return;
	else if (midi_disabled && digital_disabled
#ifdef HW3SOUND
	         && hws_mode == HWS_DEFAULT_MODE
#endif
	        ) SDL_LockAudio();
#ifdef HAVE_MIXER
	else if (musicStarted) Mix_SetPostMix(NULL, NULL);
#endif
}

static void Snd_UnlockAudio(void)  
{
	if (Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
	else if (sound_disabled) return;
	else if (midi_disabled && digital_disabled
#ifdef HW3SOUND
	         && hws_mode == HWS_DEFAULT_MODE
#endif
	        ) SDL_UnlockAudio();
#ifdef HAVE_MIXER
	else if (musicStarted) Mix_SetPostMix(audio.callback, audio.userdata);
#endif
}

static inline Uint16 Snd_LowerRate(Uint16 sr)
{
	if (sr <= audio.freq)  
		return sr;  
	for (;sr > audio.freq;)  
	{  
		if (sr % 2)  
			return sr;  
		sr /= 2;  
	}  
	if (sr == audio.freq)  
		return sr;  
	return sr*2;  
}

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#pragma pack(1)
#endif

typedef struct
{
	Uint16 header;      
	Uint16 samplerate;  
	Uint16 samples;     
	Uint16 dummy;       
	Uint8  data[0];     
} ATTRPACK dssfx_t;

#ifdef _MSC_VER
#pragma pack()
#pragma warning(default : 4200)
#endif





static void *getsfx(lumpnum_t sfxlump, size_t *len)
{
	dssfx_t *sfx, *paddedsfx;
	Uint16 sr , csr;
	size_t size = *len;
	SDL_AudioCVT sfxcvt;

	sfx = (dssfx_t *)malloc(size);
	if (sfx) W_ReadLump(sfxlump, (void *)sfx);
	else return NULL;
	sr = SHORT(sfx->samplerate);
	csr = Snd_LowerRate(sr);

	if (sr > csr && SDL_BuildAudioCVT(&sfxcvt, AUDIO_U8, 1, sr, AUDIO_U8, 1, csr))
	{ 

		sfxcvt.len = (INT32)size-8;  
		sfxcvt.buf = malloc(sfxcvt.len * sfxcvt.len_mult);  
		if (sfxcvt.buf) M_Memcpy(sfxcvt.buf, &(sfx->data), sfxcvt.len);  

		if (sfxcvt.buf && SDL_ConvertAudio(&sfxcvt) == 0)  
		{
				size = sfxcvt.len_cvt + 8;
				*len = sfxcvt.len_cvt;


				paddedsfx = (dssfx_t *) Z_Malloc(size, PU_SOUND, NULL);


				M_Memcpy(paddedsfx->data, sfxcvt.buf, sfxcvt.len_cvt);
				free(sfxcvt.buf);
				M_Memcpy(paddedsfx,sfx,8);
				paddedsfx->samplerate = SHORT(csr);  
		}
		else  
		{
			if (sfxcvt.buf) free(sfxcvt.buf);
			*len = size - 8;


			paddedsfx = (dssfx_t *)M_Memcpy(Z_Malloc(size, PU_SOUND, NULL), sfx, size);
		}
	}
	else
	{


		*len = size - 8;


		paddedsfx = (dssfx_t *)M_Memcpy(Z_Malloc(size, PU_SOUND, NULL), sfx, size);
	}


	free(sfx);


	return paddedsfx;
}


static void I_SetChannelParams(chan_t *c, INT32 vol, INT32 sep, INT32 step)
{
	INT32 leftvol;
	INT32 rightvol;
	c->vol = vol;
	c->sep = sep;
	c->step = c->realstep = step;

	if (step != steptable[128])
		c->step += (((c->samplerate<<16)/audio.freq)-65536);
	else if (c->samplerate != (unsigned)audio.freq)
		c->step = ((c->samplerate<<16)/audio.freq);




	vol = (vol * cv_soundvolume.value) >> 7;



	leftvol  = vol - ((vol*sep*sep) >> 16);  
	sep = 255 - sep;
	rightvol = vol - ((vol*sep*sep) >> 16);


	if (rightvol < 0)
		rightvol = 0;
	else if (rightvol > 127)
		rightvol = 127;
	if (leftvol < 0)
		leftvol = 0;
	else if (leftvol > 127)
		leftvol = 127;



	c->leftvol_lookup = &vol_lookup[leftvol*256];
	c->rightvol_lookup = &vol_lookup[rightvol*256];
}

static INT32 FindChannel(INT32 handle)
{
	INT32 i;

	for (i = 0; i < NUM_CHANNELS; i++)
		if (channels[i].handle == handle)
			return i;


	return -1;
}








static INT32 addsfx(sfxenum_t sfxid, INT32 volume, INT32 step, INT32 seperation)
{
	static UINT16 handlenums = 0;
	INT32 i, slot, oldestnum = 0;
	tic_t oldest = gametic;


#if 1
	if (
#if 0
	sfxid == sfx_stnmov || sfxid == sfx_sawup || sfxid == sfx_sawidl || sfxid == sfx_sawful || sfxid == sfx_sawhit || sfxid == sfx_pistol
#else
	    ( sfx_litng1 <= sfxid && sfxid >= sfx_litng4 )
	    || sfxid == sfx_trfire || sfxid == sfx_alarm || sfxid == sfx_spin
	    || sfxid == sfx_athun1 || sfxid == sfx_athun2 || sfxid == sfx_rainin
#endif
	   )
	{

		for (i = 0; i < NUM_CHANNELS; i++)
		{

			if ((channels[i].end) && (channels[i].sfxid == sfxid))
			{

				channels[i].end = NULL;


				break;
			}
		}
	}
#endif


	for (i = 0; (i < NUM_CHANNELS) && (channels[i].end); i++)
	{
		if (channels[i].starttic < oldest)
		{
			oldestnum = i;
			oldest = channels[i].starttic;
		}
	}





	if (i == NUM_CHANNELS)
		slot = oldestnum;
	else
		slot = i;

	channels[slot].end = NULL;



	channels[slot].data = (Uint8 *)S_sfx[sfxid].data;
	channels[slot].samplerate = (channels[slot].data[3]<<8)+channels[slot].data[2];
	channels[slot].data += 8;  

	while (FindChannel(handlenums)!=-1)
	{
		handlenums++;

		if (handlenums == UINT16_MAX)
			handlenums = 0;
	}



	channels[slot].handle = handlenums;


	channels[slot].stepremainder = 0;

	channels[slot].starttic = gametic;

	I_SetChannelParams(&channels[slot], volume, seperation, step);



	channels[slot].id = S_sfx[sfxid].data;

	channels[slot].sfxid = sfxid;


	channels[slot].end = channels[slot].data + S_sfx[sfxid].length;



	return handlenums;
}













static inline void I_SetChannels(void)
{



	INT32 i;
	INT32 j;

	INT32 *steptablemid = steptable + 128;

	if (sound_disabled)
		return;


	for (i = -128; i < 128; i++)
	{
		const double po = pow((double)(2.0l), (double)(i / 64.0l));
		steptablemid[i] = (INT32)(po * 65536.0l);
	}




	for (i = 0; i < 128; i++)
		for (j = 0; j < 256; j++)
		{



			vol_lookup[i * 256 + j] = (Sint16)((i * (j - 128) * 256) / 127);
		}
}

void I_SetSfxVolume(UINT8 volume)
{
	INT32 i;

	(void)volume;


	for (i = 0; i < NUM_CHANNELS; i++)
		if (channels[i].end) I_SetChannelParams(&channels[i], channels[i].vol, channels[i].sep, channels[i].realstep);


}

void *I_GetSfx(sfxinfo_t *sfx)
{
	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum(sfx);



#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_GetSfx(sfx);
#endif

	if (sfx->data)
		return sfx->data;  

	sfx->length = W_LumpLength(sfx->lumpnum);

	return getsfx(sfx->lumpnum, &sfx->length);

}

void I_FreeSfx(sfxinfo_t * sfx)
{



#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_FreeSfx(sfx);
	}
	else
#endif
	{
		size_t i;

		for (i = 1; i < NUMSFX; i++)
		{

			if (S_sfx[i].data == sfx->data)
			{
				if (S_sfx+i != sfx) S_sfx[i].data = NULL;
				S_sfx[i].lumpnum = LUMPERROR;
				S_sfx[i].length = 0;
			}
		}


		for (i = 0; i < NUM_CHANNELS; i++)
		{

			if (channels[i].end && channels[i].id == sfx->data)
			{
				channels[i].end = NULL;  
			}
		}

		Z_Free(sfx->data);
	}
	sfx->data = NULL;
	sfx->lumpnum = LUMPERROR;
}













INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority, INT32 channel)
{
	(void)priority;
	(void)pitch;
	(void)channel;

	if (sound_disabled)
		return 0;

	if (S_sfx[id].data == NULL) return -1;

	Snd_LockAudio();
	id = addsfx(id, vol, steptable[pitch], sep);
	Snd_UnlockAudio();

	return id;  
}

void I_StopSound(INT32 handle)
{




	INT32 i;

	i = FindChannel(handle);

	if (i != -1)
	{

		channels[i].end = NULL;

		channels[i].handle = -1;
		channels[i].starttic = 0;
	}

}

boolean I_SoundIsPlaying(INT32 handle)
{
	boolean isplaying = false;
	int chan = FindChannel(handle);
	if (chan != -1)
		isplaying = (channels[chan].end != NULL);
	return isplaying;
}

FUNCINLINE static ATTRINLINE void I_UpdateStream8S(Uint8 *stream, int len)
{


	register Sint16 dr;  
	register Uint8 sample;  
	register Sint16 dl;  


	Sint8 *rightout = (Sint8 *)stream;  
	Sint8 *leftout = rightout + 1; 
	const Uint8 step = 2;  

	INT32 chan;  


	len /= 2;  

	if (Snd_Mutex) SDL_LockMutex(Snd_Mutex);



	while (len--)
	{

		dl = *leftout;
		dr = *rightout;




		for (chan = 0; chan < NUM_CHANNELS; chan++)
		{

			if (channels[chan].end)
			{
#if 1

				sample = channels[chan].data[0];
#else

				sample = (((Uint32)channels[chan].data[0] *(0x10000 - channels[chan].stepremainder))
					+ ((Uint32)channels[chan].data[1]) * (channels[chan].stepremainder))) >> 16;
#endif




				dl = (Sint16)(dl+(channels[chan].leftvol_lookup[sample]>>8));
				dr = (Sint16)(dr+(channels[chan].rightvol_lookup[sample]>>8));

				channels[chan].stepremainder += channels[chan].step;

				if (channels[chan].data + (channels[chan].stepremainder >> 16) >= channels[chan].end)
					channels[chan].end = NULL;
				else
				{

					channels[chan].data += (channels[chan].stepremainder >> 16);

					channels[chan].stepremainder &= 0xffff;
				}
			}
		}




		if (dl > 0x7f)
			*leftout = 0x7f;
		else if (dl < -0x80)
			*leftout = -0x80;
		else
			*leftout = (Sint8)dl;


		if (dr > 0x7f)
			*rightout = 0x7f;
		else if (dr < -0x80)
			*rightout = -0x80;
		else
			*rightout = (Sint8)dr;


		leftout += step;
		rightout += step;

	}
	if (Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
}

FUNCINLINE static ATTRINLINE void I_UpdateStream8M(Uint8 *stream, int len)
{


	register Sint16 d;  
	register Uint8 sample;  


	Sint8 *monoout = (Sint8 *)stream;  
	const Uint8 step = 1;  

	INT32 chan;  




	if (Snd_Mutex) SDL_LockMutex(Snd_Mutex);



	while (len--)
	{

		d = *monoout;




		for (chan = 0; chan < NUM_CHANNELS; chan++)
		{

			if (channels[chan].end)
			{
#if 1

				sample = channels[chan].data[0];
#else

				sample = (((Uint32)channels[chan].data[0] *(0x10000 - channels[chan].stepremainder))
					+ ((Uint32)channels[chan].data[1]) * (channels[chan].stepremainder))) >> 16;
#endif




				d = (Sint16)(d+((channels[chan].leftvol_lookup[sample] + channels[chan].rightvol_lookup[sample])>>9));

				channels[chan].stepremainder += channels[chan].step;

				if (channels[chan].data + (channels[chan].stepremainder >> 16) >= channels[chan].end)
					channels[chan].end = NULL;
				else
				{

					channels[chan].data += (channels[chan].stepremainder >> 16);

					channels[chan].stepremainder &= 0xffff;
				}
			}
		}




		if (d > 0x7f)
			*monoout = 0x7f;
		else if (d < -0x80)
			*monoout = -0x80;
		else
			*monoout = (Sint8)d;


		monoout += step;
	}
	if (Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
}

FUNCINLINE static ATTRINLINE void I_UpdateStream16S(Uint8 *stream, int len)
{


	register Sint32 dr;  
	register Uint8 sample;  
	register Sint32 dl;  


	Sint16 *rightout = (Sint16 *)(void *)stream;  
	Sint16 *leftout = rightout + 1; 
	const Uint8 step = 2;  

	INT32 chan;  


	len /= 4;  

	if (Snd_Mutex) SDL_LockMutex(Snd_Mutex);




	while (len--)
	{

		dl = *leftout;
		dr = *rightout;




		for (chan = 0; chan < NUM_CHANNELS; chan++)
		{

			if (channels[chan].end)
			{
#if 1

				sample = channels[chan].data[0];
#else

				sample = (((Uint32)channels[chan].data[0] *(0x10000 - channels[chan].stepremainder))
					+ ((Uint32)channels[chan].data[1]) * (channels[chan].stepremainder))) >> 16;
#endif




				dl += channels[chan].leftvol_lookup[sample];
				dr += channels[chan].rightvol_lookup[sample];

				channels[chan].stepremainder += channels[chan].step;

				if (channels[chan].data + (channels[chan].stepremainder >> 16) >= channels[chan].end)
					channels[chan].end = NULL;
				else
				{

					channels[chan].data += (channels[chan].stepremainder >> 16);

					channels[chan].stepremainder &= 0xffff;
				}
			}
		}




		if (dl > 0x7fff)
			*leftout = 0x7fff;
		else if (dl < -0x8000)
			*leftout = -0x8000;
		else
			*leftout = (Sint16)dl;


		if (dr > 0x7fff)
			*rightout = 0x7fff;
		else if (dr < -0x8000)
			*rightout = -0x8000;
		else
			*rightout = (Sint16)dr;


		leftout += step;
		rightout += step;

	}
	if (Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
}

FUNCINLINE static ATTRINLINE void I_UpdateStream16M(Uint8 *stream, int len)
{


	register Sint32 d;  
	register Uint8 sample;  


	Sint16 *monoout = (Sint16 *)(void *)stream;  
	const Uint8 step = 1;  

	INT32 chan;  


	len /= 2;  

	if (Snd_Mutex) SDL_LockMutex(Snd_Mutex);




	while (len--)
	{

		d = *monoout;




		for (chan = 0; chan < NUM_CHANNELS; chan++)
		{

			if (channels[chan].end)
			{
#if 1

				sample = channels[chan].data[0];
#else

				sample = (((Uint32)channels[chan].data[0] *(0x10000 - channels[chan].stepremainder))
					+ ((Uint32)channels[chan].data[1]) * (channels[chan].stepremainder))) >> 16;
#endif




				d += (channels[chan].leftvol_lookup[sample] + channels[chan].rightvol_lookup[sample])>>1;

				channels[chan].stepremainder += channels[chan].step;

				if (channels[chan].data + (channels[chan].stepremainder >> 16) >= channels[chan].end)
					channels[chan].end = NULL;
				else
				{

					channels[chan].data += (channels[chan].stepremainder >> 16);

					channels[chan].stepremainder &= 0xffff;
				}
			}
		}




		if (d > 0x7fff)
			*monoout = 0x7fff;
		else if (d < -0x8000)
			*monoout = -0x8000;
		else
			*monoout = (Sint16)d;


		monoout += step;
	}
	if (Snd_Mutex) SDL_UnlockMutex(Snd_Mutex);
}

#if 0  
static void I_UpdateSteamGME(Music_Emu *emu, INT16 *stream, int len, UINT8 looping)
{
	#define GME_BUFFER_LEN 44100*2048


	register Sint32 da;

	static short gme_buffer[GME_BUFFER_LEN];  
	Sint16 *in = gme_buffer;

	do
	{
		int out = min(GME_BUFFER_LEN, len);
		if ( gme_play( emu, len, gme_buffer ) ) { }  
		len -= out;
		while (out--)
		{

			da = *in;
			in++;
			da += *stream;
			stream++;

			da = *in;
			in++;
			da += *stream;
			stream++;
		}
		if (gme_track_ended( emu ))
		{
			if (looping)
				gme_seek( emu, 0);
			else
				break;
		}
	} while ( len );
	#undef GME_BUFFER_LEN
}
#endif

static void SDLCALL I_UpdateStream(void *userdata, Uint8 *stream, int len)
{
	if (!sound_started || !userdata)
		return;

	memset(stream, 0x00, len);  

	if ((audio.channels != 1 && audio.channels != 2) ||
	    (audio.format != AUDIO_S8 && audio.format != AUDIO_S16SYS))
		;  
	else if (audio.channels == 1 && audio.format == AUDIO_S8)
		I_UpdateStream8M(stream, len);
	else if (audio.channels == 2 && audio.format == AUDIO_S8)
		I_UpdateStream8S(stream, len);
	else if (audio.channels == 1 && audio.format == AUDIO_S16SYS)
		I_UpdateStream16M(stream, len);
	else if (audio.channels == 2 && audio.format == AUDIO_S16SYS)
	{
		I_UpdateStream16S(stream, len);











	}
}

void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch)
{




	INT32 i = FindChannel(handle);

	if (i != -1 && channels[i].end)
	{

		I_SetChannelParams(&channels[i], vol, sep, steptable[pitch]);

	}

}

#ifdef HW3SOUND

static void *soundso = NULL;

static INT32 Init3DSDriver(const char *soName)
{
	if (soName) soundso = hwOpen(soName);
#if defined (_WIN32) && defined (_X86_) && !defined (STATIC3DS)
	HW3DS.pfnStartup            = hwSym("Startup@8",soundso);
	HW3DS.pfnShutdown           = hwSym("Shutdown@0",soundso);
	HW3DS.pfnAddSfx             = hwSym("AddSfx@4",soundso);
	HW3DS.pfnAddSource          = hwSym("AddSource@8",soundso);
	HW3DS.pfnStartSource        = hwSym("StartSource@4",soundso);
	HW3DS.pfnStopSource         = hwSym("StopSource@4",soundso);
	HW3DS.pfnGetHW3DSVersion    = hwSym("GetHW3DSVersion@0",soundso);
	HW3DS.pfnBeginFrameUpdate   = hwSym("BeginFrameUpdate@0",soundso);
	HW3DS.pfnEndFrameUpdate     = hwSym("EndFrameUpdate@0",soundso);
	HW3DS.pfnIsPlaying          = hwSym("IsPlaying@4",soundso);
	HW3DS.pfnUpdateListener     = hwSym("UpdateListener@8",soundso);
	HW3DS.pfnUpdateSourceParms  = hwSym("UpdateSourceParms@12",soundso);
	HW3DS.pfnSetCone            = hwSym("SetCone@8",soundso);
	HW3DS.pfnSetGlobalSfxVolume = hwSym("SetGlobalSfxVolume@4",soundso);
	HW3DS.pfnUpdate3DSource     = hwSym("Update3DSource@8",soundso);
	HW3DS.pfnReloadSource       = hwSym("ReloadSource@8",soundso);
	HW3DS.pfnKillSource         = hwSym("KillSource@4",soundso);
	HW3DS.pfnKillSfx            = hwSym("KillSfx@4",soundso);
	HW3DS.pfnGetHW3DSTitle      = hwSym("GetHW3DSTitle@8",soundso);
#else
	HW3DS.pfnStartup            = hwSym("Startup",soundso);
	HW3DS.pfnShutdown           = hwSym("Shutdown",soundso);
	HW3DS.pfnAddSfx             = hwSym("AddSfx",soundso);
	HW3DS.pfnAddSource          = hwSym("AddSource",soundso);
	HW3DS.pfnStartSource        = hwSym("StartSource",soundso);
	HW3DS.pfnStopSource         = hwSym("StopSource",soundso);
	HW3DS.pfnGetHW3DSVersion    = hwSym("GetHW3DSVersion",soundso);
	HW3DS.pfnBeginFrameUpdate   = hwSym("BeginFrameUpdate",soundso);
	HW3DS.pfnEndFrameUpdate     = hwSym("EndFrameUpdate",soundso);
	HW3DS.pfnIsPlaying          = hwSym("IsPlaying",soundso);
	HW3DS.pfnUpdateListener     = hwSym("UpdateListener",soundso);
	HW3DS.pfnUpdateSourceParms  = hwSym("UpdateSourceParms",soundso);
	HW3DS.pfnSetCone            = hwSym("SetCone",soundso);
	HW3DS.pfnSetGlobalSfxVolume = hwSym("SetGlobalSfxVolume",soundso);
	HW3DS.pfnUpdate3DSource     = hwSym("Update3DSource",soundso);
	HW3DS.pfnReloadSource       = hwSym("ReloadSource",soundso);
	HW3DS.pfnKillSource         = hwSym("KillSource",soundso);
	HW3DS.pfnKillSfx            = hwSym("KillSfx",soundso);
	HW3DS.pfnGetHW3DSTitle      = hwSym("GetHW3DSTitle",soundso);
#endif


		return true;


}
#endif

void I_ShutdownSound(void)
{
	if (sound_disabled || !sound_started)
		return;

	CONS_Printf("I_ShutdownSound: ");

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_Shutdown();
		hwClose(soundso);
		return;
	}
#endif

	if (midi_disabled && digital_disabled)
		SDL_CloseAudio();
	CONS_Printf("%s", M_GetText("shut down\n"));
	sound_started = false;
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	if (Snd_Mutex)
		SDL_DestroyMutex(Snd_Mutex);
	Snd_Mutex = NULL;
}

void I_UpdateSound(void)
{
}

void I_StartupSound(void)
{
#ifdef HW3SOUND
	const char *sdrv_name = NULL;
#endif
#ifndef HAVE_MIXER
	midi_disabled = digital_disabled = true;
#endif

	memset(channels, 0, sizeof (channels));  

	audio.format = AUDIO_S16SYS;
	audio.channels = 2;
	audio.callback = I_UpdateStream;
	audio.userdata = &localdata;


	CONS_Printf("I_StartupSound:\n");

#ifdef _WIN32


	SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
#endif


	if (SDL_WasInit(SDL_INIT_AUDIO) == SDL_INIT_AUDIO)
		CONS_Printf("SDL Audio already started\n");
	else if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		CONS_Alert(CONS_ERROR, "Error initializing SDL Audio: %s\n", SDL_GetError());

		return;
	}


	if (M_CheckParm ("-freq") && M_IsNextParm())
	{
		audio.freq = atoi(M_GetNextParm());
		if (!audio.freq) audio.freq = cv_samplerate.value;
		audio.samples = (Uint16)((samplecount/2)*(INT32)(audio.freq/11025));  
		CONS_Printf (M_GetText(" requested frequency of %d hz\n"), audio.freq);
	}
	else
	{
		audio.samples = samplecount;
		audio.freq = cv_samplerate.value;
	}

	if (M_CheckParm ("-mono"))
	{
		audio.channels = 1;
		audio.samples /= 2;
	}

	if (sound_disabled)
		return;

#ifdef HW3SOUND
#ifdef STATIC3DS
	if (M_CheckParm("-3dsound") || M_CheckParm("-ds3d"))
	{
		hws_mode = HWS_OPENAL;
	}
#elif defined (_WIN32)
	if (M_CheckParm("-ds3d"))
	{
		hws_mode = HWS_DS3D;
		sdrv_name = "s_ds3d.dll";
	}
	else if (M_CheckParm("-fmod3d"))
	{
		hws_mode = HWS_FMOD3D;
		sdrv_name = "s_fmod.dll";
	}
	else if (M_CheckParm("-openal"))
	{
		hws_mode = HWS_OPENAL;
		sdrv_name = "s_openal.dll";
	}
#else
	if (M_CheckParm("-fmod3d"))
	{
		hws_mode = HWS_FMOD3D;
		sdrv_name = "./s_fmod.so";
	}
	else if (M_CheckParm("-openal"))
	{
		hws_mode = HWS_OPENAL;
		sdrv_name = "./s_openal.so";
	}
#endif
	else if (M_CheckParm("-sounddriver") &&  M_IsNextParm())
	{
		hws_mode = HWS_OTHER;
		sdrv_name = M_GetNextParm();
	}
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		if (Init3DSDriver(sdrv_name))
		{
			snddev_t            snddev;



			snddev.bps = 16;
			snddev.sample_rate = audio.freq;
			snddev.numsfxs = NUMSFX;
#if defined (_WIN32)
			snddev.cooplevel = 0x00000002;
			snddev.hWnd = vid.WndParent;
#endif
			if (HW3S_Init(I_Error, &snddev))
			{
				audio.userdata = NULL;
				CONS_Printf("%s", M_GetText(" Using 3D sound driver\n"));
				return;
			}
			CONS_Printf("%s", M_GetText(" Failed loading 3D sound driver\n"));

			HW3S_Shutdown();
			hwClose(soundso);
		}
		CONS_Printf("%s", M_GetText(" Failed loading 3D sound driver\n"));
		hws_mode = HWS_DEFAULT_MODE;
	}
#endif
	if (!musicStarted && SDL_OpenAudio(&audio, &audio) < 0)
	{
		CONS_Printf("%s", M_GetText(" couldn't open audio with desired format\n"));
		sound_disabled = true;
		return;
	}
	else
	{


	}
	samplecount = audio.samples;
	CV_SetValue(&cv_samplerate, audio.freq);
	CONS_Printf(M_GetText(" configured audio device with %d samples/slice at %ikhz(%dms buffer)\n"), samplecount, audio.freq/1000, (INT32) (((float)audio.samples * 1000.0f) / audio.freq));

	CONS_Printf("%s", M_GetText(" Sound module ready\n"));

	if (!musicStarted) SDL_PauseAudio(0);

	I_SetChannels();
	sound_started = true;
	Snd_Mutex = SDL_CreateMutex();
}









#if 0  
static void I_ShutdownGMEMusic(void)
{
	Snd_LockAudio();
	if (localdata.gme_emu)
		gme_delete(localdata.gme_emu);
	localdata.gme_emu = NULL;
	Snd_UnlockAudio();
}
#endif

void I_InitMusic(void)
{
#if 0  
	I_AddExitFunc(I_ShutdownGMEMusic);
#endif
}

void I_ShutdownMusic(void) { }





musictype_t I_SongType(void)
{
	return MU_NONE;
}

boolean I_SongPlaying(void)
{
	return false;
}

boolean I_SongPaused(void)
{
	return false;
}





boolean I_SetSongSpeed(float speed)
{
	(void)speed;
	return false;
}





UINT32 I_GetSongLength(void)
{
	return 0;
}

boolean I_SetSongLoopPoint(UINT32 looppoint)
{
	(void)looppoint;
	return false;
}

UINT32 I_GetSongLoopPoint(void)
{
	return 0;
}

boolean I_SetSongPosition(UINT32 position)
{
	(void)position;
	return false;
}

UINT32 I_GetSongPosition(void)
{
	return 0;
}





#if 0  
static void I_StopGME(void)
{
	Snd_LockAudio();
	gme_seek(localdata.gme_emu, 0);
	Snd_UnlockAudio();
}

static void I_PauseGME(void)
{
	localdata.gme_pause = true;
}

static void I_ResumeGME(void)
{
	localdata.gme_pause = false;
}
#endif

boolean I_LoadSong(char *data, size_t len)
{
	(void)data;
	(void)len;
	return false;
}

void I_UnloadSong(void) { }

boolean I_PlaySong(boolean looping)
{
	(void)looping;
	return false;
}

void I_StopSong(void)
{
#if 0  
	I_StopGME();
#endif
}

void I_PauseSong(void)
{
#if 0  
	I_PauseGME();
#endif
}

void I_ResumeSong(void)
{
#if 0
	I_ResumeGME();
#endif
}

void I_SetMusicVolume(UINT8 volume)
{
	(void)volume;
}

boolean I_SetSongTrack(INT32 track)
{
	(void)track;
	return false;
}





void I_SetInternalMusicVolume(UINT8 volume)
{
	(void)volume;
}

void I_StopFadingSong(void)
{
}

boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void))
{
	(void)target_volume;
	(void)source_volume;
	(void)ms;
	(void)callback;
	return false;
}

boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void))
{
	(void)target_volume;
	(void)ms;
	(void)callback;
	return false;
}

boolean I_FadeOutStopSong(UINT32 ms)
{
	(void)ms;
	return false;
}

boolean I_FadeInPlaySong(UINT32 ms, boolean looping)
{
        (void)ms;
        (void)looping;
        return false;
}







#if 0  
static void I_CleanupGME(void *userdata)
{
	Z_Free(userdata);
}

static boolean I_StartGMESong(const char *musicname, boolean looping)
{
	char filename[9];
	void *data;
	lumpnum_t lumpnum;
	size_t lumplength;
	Music_Emu *emu;
	const char* gme_err;

	Snd_LockAudio();
	if (localdata.gme_emu)
		gme_delete(localdata.gme_emu);
	localdata.gme_emu = NULL;
	Snd_UnlockAudio();

	snprintf(filename, sizeof filename, "o_%s", musicname);

	lumpnum = W_CheckNumForName(filename);

	if (lumpnum == LUMPERROR)
	{
		return false;  
	}
	else
		lumplength = W_LumpLength(lumpnum);

	data = W_CacheLumpNum(lumpnum, PU_MUSIC);

	gme_err = gme_open_data(data, (long)lumplength, &emu, audio.freq);
	if (gme_err != NULL) {

		return false;
	}
	gme_set_user_data(emu, data);
	gme_set_user_cleanup(emu, I_CleanupGME);
	gme_start_track(emu, 0);
#ifdef HAVE_MIXER
	gme_set_fade(emu, Digfade);
#endif

	Snd_LockAudio();
	localdata.gme_emu = emu;
	localdata.gme_pause = false;
	localdata.gme_loop = (UINT8)looping;
	Snd_UnlockAudio();

	return true;
}
#endif

#endif  
