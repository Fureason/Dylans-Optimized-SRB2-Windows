// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2001 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------





#ifdef _WINDOWS

#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <stdio.h>
FILE* logstream = NULL;
#else
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#ifndef HAVE_SDL  
FILE* logstream = NULL;
#endif
#endif

#ifdef __APPLE__
#include <al.h>  
#include <alc.h>  
#else
#include <AL/al.h>  
#include <AL/alc.h>  
#endif

#define  _CREATE_DLL_
#include "../../doomdef.h"
#include "../hw3dsdrv.h"



#define DEBUG_TO_FILE



typedef struct stack_snd_s
{
	ALuint ALsource; 

	ALint sfx_id; 

	ALint LRU; 

	ALboolean permanent; 
} stack_snd_t;

#ifndef AL_INVALID
#define AL_INVALID (-1)
#endif

static I_Error_t    I_ErrorOpenAl    = NULL;
static stack_snd_t *ALstack          = NULL;  
static ALsizei      allocated_sounds = 0;     
static ALsizei      allocate_delta   = 16;
static ALCdevice   *ALCDevice        = NULL;
static ALCcontext  *ALContext        = NULL;
static ALenum       ALo_Lasterror    = AL_NO_ERROR;
static ALenum       ALCo_Lasterror   = ALC_NO_ERROR;

static ALenum ALo_GetError(ALvoid)
{
	ALo_Lasterror = alGetError();
	return ALo_Lasterror;
}

static ALenum ALCo_GetError(ALvoid)
{
	ALCo_Lasterror = alcGetError(ALCDevice);
	return ALCo_Lasterror;
}

static ALfloat ALvol(ALint vol, ALsizei step)
{
	return ((ALfloat)vol)/(ALfloat)step;
}

  
                                   
 
     
                      
      
                              
                                                    
                                      
 
  

static const ALchar *GetALErrorString(ALenum err)
{
	switch (err)
	{
		case AL_NO_ERROR:
			return "There no error";
			break;

		case AL_INVALID_NAME:
			return "Invalid name";
			break;

		case AL_INVALID_ENUM:
			return "Invalid enum";
			break;

		case AL_INVALID_VALUE:
			return "Invalid value";
			break;

		case AL_INVALID_OPERATION:
			return "Invalid operation";
			break;

		case AL_OUT_OF_MEMORY:
			return "Out Of Memory";
			break;

		default:
			return alGetString(err);
			break;
	}
}

static const ALchar *GetALCErrorString(ALenum err)
{
	switch (err)
	{
		case ALC_NO_ERROR:
			return "There no error";
			break;

		case ALC_INVALID_DEVICE:
			return "Invalid Device";
			break;

		case ALC_INVALID_CONTEXT:
			return "Invalid Context";
			break;

		case ALC_INVALID_ENUM:
			return "Invalid enum";
			break;

		case ALC_INVALID_VALUE:
			return "Invalid value";
			break;

		case ALC_OUT_OF_MEMORY:
			return "Out Of Memory";
			break;

		default:
			return alcGetString(ALCDevice, err);
			break;
	}
}

                                                                
  
             
                                                                  
                  
  
                                                               
   





FUNCPRINTF void DBG_Printf(const char *lpFmt, ... )
{
#ifdef DEBUG_TO_FILE
	char    str[4096] = "";
	va_list arglist;

	va_start(arglist, lpFmt);
	vsnprintf(str, 4096, lpFmt, arglist);
	va_end(arglist);
	if (logstream)
		fwrite(str, strlen(str), 1 , logstream);
#else
	lpFmt = NULL;
#endif
}

                                                                
  
                                                     
  
                                                                
   
static ALboolean reallocate_stack(void)
{
	stack_snd_t *new_stack;
	if (ALstack)
		new_stack = realloc(ALstack, sizeof (stack_snd_t) * (allocated_sounds + allocate_delta));
	else
		new_stack = malloc(sizeof (stack_snd_t) * (allocate_delta));
	if (new_stack)
	{
		ALsizei clean_stack;
		ALstack = new_stack;
		memset(&ALstack[allocated_sounds], 0, allocate_delta * sizeof (stack_snd_t));
		for (clean_stack = allocated_sounds; clean_stack < allocated_sounds; clean_stack++)
			ALstack[clean_stack].ALsource = (ALuint)AL_INVALID;
		allocated_sounds += allocate_delta;
		return AL_TRUE;
	}
	return AL_FALSE;
}


                                                                
  
                           
  
                                                                
   
static ALvoid kill_sound(stack_snd_t *snd)
{
	if (alIsSource(snd->ALsource))
		alDeleteSources(1,&snd->ALsource);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alDeleteSources, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
	memset(snd,0, sizeof (stack_snd_t));
	snd->ALsource = (ALuint)AL_INVALID;
}

                                                                
  
                                          
                                                  
  
                                                                
   
static ALsizei find_handle(void)
{
	ALsizei free_sfx = 0;
	stack_snd_t *snd;

	for (snd = ALstack; free_sfx < allocated_sounds; snd++, free_sfx++)
	{
		if (snd->permanent)
			continue;
		if (snd->ALsource==(ALuint)AL_INVALID)
			break;
		if (!alIsSource(snd->ALsource))
			break;
		ALo_GetError();
		if (snd->sfx_id == 0)
			break;
	}



	if (free_sfx == allocated_sounds)
	{
		DBG_Printf("No free or same sfx found so increase stack (currently %d srcs)\n", allocated_sounds);
		free_sfx = reallocate_stack() ? free_sfx : (ALsizei)AL_INVALID;
	}
	return free_sfx;
}

static ALvoid ALSetPan(ALuint sid, INT32 sep)
{
	ALfloat facing[3] ={0.0f, 0.0f, 0.0f};  
	const ALfloat fsep = (ALfloat)sep;
	const ALfloat nsep = (ALfloat)NORMAL_SEP;
	if (sep)
	{
		facing[0] = (ALfloat)(fsep/nsep);
		facing[2] = 1.0f;
	}
	alSourcefv(sid, AL_POSITION, facing);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alSourcefv, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
}


                                                                               
  
                                 
  
                                                                               
EXPORT INT32 HWRAPI( Startup ) (I_Error_t FatalErrorFunction, snddev_t *snd_dev)
{
	ALenum          model      = AL_INVERSE_DISTANCE_CLAMPED;
	ALCboolean      inited     = ALC_FALSE;
	ALCint          AlSetup[8] = {ALC_FREQUENCY,22050,ALC_REFRESH,35,ALC_SYNC,AL_FALSE,ALC_INVALID,ALC_INVALID};
#if (defined (_WIN32) || defined (_WIN64)) && 0
	const ALCubyte *ALCdriver  = alcGetString(NULL,ALC_DEFAULT_DEVICE_SPECIFIER);
	const ALCubyte *DSdriver = "DirectSound";

	if (!strcmp((const ALCbyte *)ALCdriver,"DirectSound3D"))  
		ALCDevice = alcOpenDevice(DSdriver);  
	else  
		ALCDevice = alcOpenDevice(ALCdriver);  
#else
	ALCDevice = alcOpenDevice(alcGetString(NULL,ALC_DEFAULT_DEVICE_SPECIFIER));
#endif
	if (ALCo_GetError() != ALC_NO_ERROR || !ALCDevice)
	{
		DBG_Printf("S_OpenAl: Error %s when opening device!\n", GetALCErrorString(ALCo_Lasterror));
		return inited;
	}
	else
	{
		DBG_Printf("Driver Name : %s\n", alcGetString(ALCDevice,ALC_DEVICE_SPECIFIER));
		DBG_Printf("Extensions  : %s\n", alcGetString(ALCDevice,ALC_EXTENSIONS));
		DBG_Printf("S_OpenAl: OpenAl Device %s opened\n",alcGetString(ALCDevice, ALC_DEVICE_SPECIFIER));
	}

	I_ErrorOpenAl = FatalErrorFunction;

	AlSetup[1] = snd_dev->sample_rate;  
	AlSetup[1] = 44100;
	AlSetup[3] = 35;                    


	ALContext = alcCreateContext(ALCDevice,AlSetup);
	if (ALCo_GetError() != ALC_NO_ERROR || !ALContext)
	{
		DBG_Printf("S_OpenAl: Error %s when making the environment!\n",GetALCErrorString(ALCo_Lasterror));
		return inited;
	}
	else
	{
		DBG_Printf("S_OpenAl: OpenAl environment made\n");
	}

	alcMakeContextCurrent(ALContext);
	if (ALCo_GetError() != ALC_NO_ERROR)
	{
		DBG_Printf("S_OpenAl: Error %s when setting the environment!\n",GetALCErrorString(ALCo_Lasterror));
		return inited;
	}
	else
	{
		DBG_Printf("S_OpenAl: OpenAl environment setted up\n");
	}

	DBG_Printf("Vendor      : %s\n", alGetString(AL_VENDOR));
	DBG_Printf("Renderer    : %s\n", alGetString(AL_RENDERER));
	DBG_Printf("Version     : %s\n", alGetString(AL_VERSION));
	DBG_Printf("Extensions  : %s\n", alGetString(AL_EXTENSIONS));

	alDopplerFactor(0.0f);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("S_OpenAl: Error %s when setting Doppler Factor!\n",GetALErrorString(ALo_Lasterror));
		return inited;
	}
	else
	{
		DBG_Printf("S_OpenAl: Doppler Factor of %f setted\n", alGetDouble(AL_DOPPLER_FACTOR));
	}

	alSpeedOfSound(600.0f);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("S_OpenAl: Error %s when setting Speed of Sound!\n",GetALErrorString(ALo_Lasterror));
		return inited;
	}
	else
	{
		DBG_Printf("S_OpenAl: Speed of Sound set to %f\n", alGetDouble(AL_SPEED_OF_SOUND));
	}

	alDistanceModel(model);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("S_OpenAl: Error %s when setting Distance Model!\n",GetALErrorString(ALo_Lasterror));
		return inited;
	}
	else
	{

	}

	inited = reallocate_stack();

	return inited;
}

                                                                               
  
                    
  
                                                                                
EXPORT void HWRAPI( Shutdown ) ( void )
{
	ALsizei i;

	DBG_Printf ("S_OpenAL Shutdown()\n");

	for (i = 0; i < allocated_sounds; i++)
	{
		StopSource(i);
		kill_sound(ALstack + i);
	}

#if defined(linux) || defined(__linux) || defined(__linux__)
	alcMakeContextCurrent(NULL);

#endif

	if (ALContext)
	{
		alcDestroyContext(ALContext);
		ALContext = NULL;
	}

	if (ALstack)
	{
		free(ALstack);
		ALstack = NULL;
	}


	if (ALCDevice)
	{
		alcCloseDevice(ALCDevice);
		ALCDevice = NULL;
	}

}

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#pragma pack(1)
#endif

typedef struct
{
	ALushort header;      
	ALushort samplerate;  
	ALushort samples;     
	ALushort dummy;      
	ALubyte  data[0];     
} ATTRPACK dssfx_t;

#ifdef _MSC_VER
#pragma pack()
#pragma warning(default : 4200)
#endif

static ALsizei getfreq(ALvoid *sfxdata, size_t len)
{
	dssfx_t *dsfx = sfxdata;
	const ALsizei freq = SHORT(dsfx->samplerate);

	if (len <= 8)
		return 0;
	else
		return freq;
}

static ALsizei makechan(ALuint sfxhandle, ALboolean perm)
{
	ALsizei chan = find_handle();

	if (chan == (ALsizei)AL_INVALID) return chan;

	alGenSources(1, &ALstack[chan].ALsource);
	if (ALo_GetError() != AL_NO_ERROR || !alIsSource(ALstack[chan].ALsource))
	{
		DBG_Printf("S_OpenAl: Error %s when make an ALSource!\n",GetALErrorString(ALo_Lasterror));
		ALstack[chan].ALsource = (ALuint)AL_INVALID;
		return (ALsizei)AL_INVALID;
	}

	ALstack[chan].permanent = perm;

	alSourceQueueBuffers(ALstack[chan].ALsource, 1, &sfxhandle);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("S_OpenAl: Error %s when queue up an alSource!\n",GetALErrorString(ALo_Lasterror));
	}

	return chan;
}

                                                                               
  
                    
  
                                                                                
EXPORT INT32 HWRAPI ( AddSource ) (source3D_data_t *src, u_int sfxhandle)
{
	ALsizei chan = makechan(sfxhandle, (ALboolean)(src?src->permanent:AL_FALSE));
	ALuint sid = (ALuint)AL_INVALID;
	if (chan == (ALsizei)AL_INVALID)
		return AL_INVALID;
	else
		sid = ALstack[chan].ALsource;

	if (src)  
	{
#if 0
		alSourcef(sid, AL_REFERENCE_DISTANCE, src->min_distance);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_REFERENCE_DISTANCE, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}

		alSourcef(sid, AL_MAX_DISTANCE, src->max_distance);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_MAX_DISTANCE, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
#endif

		if (src->head_relative)
			alSourcei(sid,  AL_SOURCE_RELATIVE, AL_TRUE);
		else
			alSourcei(sid, AL_SOURCE_RELATIVE, AL_FALSE);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_SOURCE_RELATIVE, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}

		alSource3f(sid, AL_POSITION, src->pos.x, src->pos.z, src->pos.y);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_POSITION, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
		alSource3f(sid, AL_VELOCITY, src->pos.momx, src->pos.momz, src->pos.momy);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_VELOCITY, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
	}
	else  
	{
		alSourcei(sid, AL_SOURCE_RELATIVE, AL_TRUE);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_SOURCE_RELATIVE, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
	}

	alSourcef(sid, AL_ROLLOFF_FACTOR, 0.0f);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_ROLLOFF_FACTOR, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	alSourcef(sid, AL_REFERENCE_DISTANCE, 0.0f);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_REFERENCE_DISTANC, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
	return chan;
}

EXPORT INT32 HWRAPI ( StartSource ) (INT32 chan)
{
	ALint state = AL_NONE;
	ALuint sid = (ALuint)AL_INVALID;

	if (chan <= AL_INVALID || !alIsSource(ALstack[chan].ALsource))
		return AL_FALSE;
	else
		sid = ALstack[chan].ALsource;

	ALo_GetError();

	alSourcePlay(sid);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alSourcePlay, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	alGetSourcei(sid, AL_SOURCE_STATE, &state);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alGetSourcei, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	return (state == AL_PLAYING);
}

EXPORT void HWRAPI ( StopSource) (INT32 chan)
{
	ALuint sfxhandle = AL_NONE;
	ALuint sid = (ALuint)AL_INVALID;
	if (chan <= AL_INVALID || !alIsSource(ALstack[chan].ALsource))
		return;
	else
		sid = ALstack[chan].ALsource;

	ALo_GetError();

#if 1
	alSourceQueueBuffers(sid, 1, &sfxhandle);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alSourceQueueBuffers, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
#else
	alSourceStop(sid);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alSourceStop, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
#endif
}

EXPORT INT32 HWRAPI ( GetHW3DSVersion) (void)
{
	return VERSION;
}

EXPORT void HWRAPI (BeginFrameUpdate) (void)
{
	alcSuspendContext(ALContext);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alcSuspendContext, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
}

EXPORT void HWRAPI (EndFrameUpdate) (void)
{
	alcProcessContext(ALContext);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alcProcessContext, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
}

EXPORT INT32 HWRAPI (IsPlaying) (INT32 chan)
{
	ALint state = AL_NONE;
	ALuint sid = (ALuint)AL_INVALID;

	if (chan <= AL_INVALID || !alIsSource(ALstack[chan].ALsource))
		return AL_FALSE;
	else
		sid = ALstack[chan].ALsource;

	ALo_GetError();

	alGetSourcei(sid, AL_SOURCE_STATE, &state);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: alGetSourcei, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	return (state == AL_PLAYING);
}

                                                                               
                 
  
                                   
             
                
             
                                                                               
EXPORT void HWRAPI (UpdateListener) (listener_data_t *data, INT32 num)
{
	if (num != 1) return;

	if (data)
	{
		ALfloat facing[6];
		ALdouble f_angle = 0.0;
		if (data->f_angle) f_angle = (ALdouble)((data->f_angle) / 180.0l * M_PIl);
		facing[0] = (ALfloat)cos(f_angle);
		facing[1] = 0.0f;
		facing[2] = (ALfloat)sin(f_angle);
		facing[3] = 0.0f;
		facing[4] = 1.0f;
		facing[5] = 0.0f;

		alListener3f(AL_POSITION,(ALfloat)-data->x,(ALfloat)data->z,(ALfloat)data->y);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_POSITIO, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}

		alListener3f(AL_VELOCITY,(ALfloat)data->momx,(ALfloat)data->momz,(ALfloat)data->momy);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_VELOCITY, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}

		alListenerfv(AL_ORIENTATION, facing);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: AL_ORIENTATION, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
	}
	else
	{
		DBG_Printf("Error: 1st listener data is missing\n");
	}
}

                                                                               
  
                                                                    
  
                                                                               
EXPORT void HWRAPI (UpdateSourceParms) (INT32 chan, INT32 vol, INT32 sep)
{
	ALuint sid = AL_NONE;
	ALint is2d = AL_FALSE;
	if (chan > AL_INVALID && !alIsSource(ALstack[chan].ALsource))
		return;
	else
		sid = ALstack[chan].ALsource;

	alGetSourcei(sid, AL_SOURCE_RELATIVE, &is2d);

	if (!is2d)
		return;

	if (vol != -1)
	{
		alSourcef(sid,AL_GAIN,ALvol(vol,256));
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: alSourcef, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
	}

	if (sep != -1)
	{
		ALSetPan(sid,sep-NORMAL_SEP);
		if (ALo_GetError() != AL_NO_ERROR)
		{
			DBG_Printf("%s: ALSetPan, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
		}
	}
}




EXPORT void HWRAPI (SetGlobalSfxVolume) (INT32 vol)
{
	alListenerf(AL_GAIN, ALvol(vol,32));
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_GAIN, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
}


EXPORT INT32 HWRAPI (SetCone) (INT32 chan, cone_def_t *cone_def)
{
	ALuint sid = AL_NONE;
	ALint is2d = AL_FALSE;
	if (chan > AL_INVALID && !alIsSource(ALstack[chan].ALsource))
		return AL_FALSE;
	else
		sid = ALstack[chan].ALsource;

	alGetSourcei(sid, AL_SOURCE_RELATIVE, &is2d);

	if(is2d || !cone_def)
		return AL_FALSE;

	alSourcef(sid,AL_CONE_INNER_ANGLE,cone_def->inner);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_CONE_INNER_ANGLE, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	alSourcef(sid,AL_CONE_OUTER_ANGLE,cone_def->outer);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_CONE_OUTER_ANGL, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	alSourcei(sid,AL_CONE_OUTER_GAIN, cone_def->outer_gain);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_CONE_OUTER_GAIN, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	return AL_TRUE;
}

EXPORT void HWRAPI (Update3DSource) (INT32 chan, source3D_pos_t *sfx)
{
	ALuint sid = AL_NONE;
	ALint is2d = AL_FALSE;
	if (chan > AL_INVALID && !alIsSource(ALstack[chan].ALsource))
		return;
	else
		sid = ALstack[chan].ALsource;

	alGetSourcei(sid, AL_SOURCE_RELATIVE, &is2d);

	if (is2d)
		return;

	alSource3f(sid, AL_POSITION, sfx->x, sfx->z, sfx->y);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_POSITION, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	alSource3f(sid, AL_VELOCITY, sfx->momx, sfx->momz, sfx->momy);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("%s: AL_VELOCITY, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
}




EXPORT INT32 HWRAPI (ReloadSource) (INT32 chan, u_int sfxhandle)
{
	ALuint sid;

	if (chan <= AL_INVALID || !alIsSource(ALstack[chan].ALsource) || !alIsBuffer(sfxhandle))
		return AL_INVALID;
	else
		sid = ALstack[chan].ALsource;

	ALo_GetError();

	alSourceQueueBuffers(sid, 1, &sfxhandle);
	if (ALo_GetError() == AL_INVALID_OPERATION)  
	{
		alSourceStop(sid);
		alSourcei(sid, AL_BUFFER, sfxhandle);
		ALo_GetError();
	}
	if (ALo_Lasterror != AL_NO_ERROR)
	{
		DBG_Printf("%s: alSourcei, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}

	return chan;
}

                                                                               
  
                                                                
                                  
  
                                                                               
EXPORT void HWRAPI (KillSource) (INT32 chan)
{
	if (chan > AL_INVALID && (ALsizei)chan <= allocated_sounds)
		kill_sound(ALstack + chan);
}

EXPORT u_int HWRAPI (AddSfx) (sfx_data_t *sfx)
{
	ALuint chan = (ALuint)AL_INVALID;
	ALsizei freq = 11025;
	ALubyte *data = NULL;
	ALsizei size = 0;

	if (!sfx)
		return 0;
	else
	{
		data = ((ALubyte *)sfx->data)+8;
		size = (ALsizei)(sfx->length-8);
	}

	alGenBuffers(1, &chan);
	if (ALo_GetError() != AL_NO_ERROR || !alIsBuffer(chan))
	{
		DBG_Printf("S_OpenAl: Error %s when make an ALBuffer!\n",GetALErrorString(ALo_Lasterror));
		return (u_int)AL_INVALID;
	}

	freq = getfreq(sfx->data, sfx->length);

	alBufferData(chan, AL_FORMAT_MONO8, data, size, freq);
	if (ALo_GetError() != AL_NO_ERROR)
	{
		DBG_Printf("S_OpenAl: Error %s when setting up a alBuffer!\n",GetALErrorString(ALo_Lasterror));
	}

	return chan;
}

EXPORT void HWRAPI (KillSfx) (u_int sfxhandle)
{
	ALuint ALsfx = sfxhandle;
	if (!alIsBuffer(ALsfx))
		return;
	alDeleteBuffers(1, &ALsfx);
	if (ALo_GetError() == AL_INVALID_OPERATION)
	{

	}
	else if (ALo_Lasterror != AL_NO_ERROR)
	{
		DBG_Printf("%s: alDeleteBuffers, %s", __FUNCTION__, GetALErrorString(ALo_Lasterror));
	}
}

EXPORT void HWRAPI (GetHW3DSTitle) (char *buf, size_t size)
{
	strncpy(buf, "OpenAL", size);
}


#ifdef _WINDOWS
BOOL WINAPI DllMain(HINSTANCE hinstDLL,  
                    DWORD fdwReason,     
                    LPVOID lpvReserved)  
{

	UNREFERENCED_PARAMETER(lpvReserved);
	switch ( fdwReason )
	{
		case DLL_PROCESS_ATTACH:


#ifdef DEBUG_TO_FILE
			logstream = fopen("s_openal.log", "wt");
			if (logstream == NULL)
				return FALSE;
#endif
		DisableThreadLibraryCalls(hinstDLL);
		break;

		case DLL_THREAD_ATTACH:

			break;

		case DLL_THREAD_DETACH:

			break;

		case DLL_PROCESS_DETACH:

#ifdef DEBUG_TO_FILE
			if ( logstream)
			{
				fclose(logstream);
				logstream  = NULL;
			}
#endif
			break;
	}
	return TRUE;   
}
#elif !defined (_WINDOWS)





EXPORT void _init()
{
#ifdef DEBUG_TO_FILE
	logstream = fopen("s_openal.log", "w+");
#endif
}

EXPORT void _fini()
{
#ifdef DEBUG_TO_FILE
	if (logstream)
		fclose(logstream);
	logstream = NULL;
#endif
}
#endif

