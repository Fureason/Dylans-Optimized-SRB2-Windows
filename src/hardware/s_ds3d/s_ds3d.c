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





#define INITGUID

#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <cguid.h>
#define DIRECTSOUND_VERSION  0x0700        

#ifdef __MINGW32__
#define NONAMELESSUNION
#endif
#include <dsound.h>

#if (defined (DIRECTSOUND_VERSION) && (DIRECTSOUND_VERSION >= 0x0700))
#define DX7  
#undef DUMMYUNIONNAMEN
#endif

#ifdef __MINGW32__

#endif

#define  _CREATE_DLL_
#include "../hw3dsdrv.h"
#include "../../m_fixed.h"
#include "../../doomdef.h"


#undef DEBUG_TO_FILE
#define DEBUG_TO_FILE

#if defined ( HAVE_SDL ) && !defined ( LOGMESSAGES )
#undef DEBUG_TO_FILE
#endif



typedef struct stack_snd_s
{

	LPDIRECTSOUNDBUFFER     dsbuffer;


	LPDIRECTSOUND3DBUFFER   dsbuffer3D;      




	DS3DBUFFER              parameters;


	INT32                     sfx_id;


	INT32                     LRU;



	INT32                     permanent;

} stack_t;


#define SOUND_ALLOCATE_DELTA    16        
#define MAX_LRU                 16        

static stack_t  *_stack;                  
static INT32      allocated_sounds;         

static INT32      srate;                    



#if defined (DEBUG_TO_FILE)
#include <stdio.h>
FILE *logstream = NULL;
#endif

static LPDIRECTSOUND            DSnd            = NULL;   
static LPDIRECTSOUNDBUFFER      PrimaryBuffer   = NULL;   
static LPDIRECTSOUND3DLISTENER  Listener        = NULL;   

static DS3DLISTENER             listener_parms;           

#ifdef DX7
static BOOL                     virtualization;           
#endif

static DWORD                    update_mode;              


static I_Error_t I_ErrorDS3D = NULL;




#define RELEASE_BUFFER(buf)     {if (buf) { IDirectSoundBuffer_Release(buf); (buf) = NULL; }}
#define RELEASE_3DBUFFER(buf)   {if (buf) { IDirectSound3DBuffer_Release(buf); (buf) = NULL; }}


#define _2DSOURCE_FLAGS (DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_STICKYFOCUS | DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY)
#define _3DSOURCE_FLAGS (DSBCAPS_CTRL3D | DSBCAPS_STATIC | DSBCAPS_MUTE3DATMAXDISTANCE | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY)


enum {IS_2DSOURCE = 0, IS_3DSOURCE = 1};

#define NEW_HANDLE  -1

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
			logstream = fopen("s_ds3d.log", "wt");
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
			if (logstream)
			{
				fclose(logstream);
				logstream = NULL;
			}
#endif
			break;
	}
	return TRUE;   
}
#endif





                                                                
  
             
                                                                  
                  
  
                                                                
   





void DBG_Printf(const char *lpFmt, ... )
{
#ifdef DEBUG_TO_FILE
	char    str[4096] = "";
	va_list arglist;

	va_start(arglist, lpFmt);
	vsnprintf(str, 4096, lpFmt, arglist);
	va_end(arglist);

	if (logstream)
		fwrite(str, strlen(str), 1, logstream);
#endif
}


                                                                
  
                                                           
  
                                                                
   
static BOOL reallocate_stack(void)
{
	stack_t *new_stack;

	if (_stack)
		new_stack = realloc(_stack, sizeof (stack_t) * (allocated_sounds + SOUND_ALLOCATE_DELTA));
	else
		new_stack = malloc(sizeof (stack_t) * (allocated_sounds + SOUND_ALLOCATE_DELTA));
	if (new_stack)
	{
		_stack = new_stack;
		ZeroMemory(&_stack[allocated_sounds], SOUND_ALLOCATE_DELTA * sizeof (stack_t));
		allocated_sounds += SOUND_ALLOCATE_DELTA;
	}
	return (new_stack != NULL);
}


                                                                
  
                           
  
                                                                
   
static void kill_sound(stack_t *snd)
{


	if (snd->dsbuffer3D)
		RELEASE_3DBUFFER(snd->dsbuffer3D);

	if (snd->dsbuffer)
		RELEASE_BUFFER(snd->dsbuffer);

	ZeroMemory(snd, sizeof (stack_t));
}


                                                                
  
                                           
  
                                                                
   
static BOOL is_playing(stack_t *snd)
{
	DWORD               status;
	LPDIRECTSOUNDBUFFER dsbuffer = snd->dsbuffer;

	if (dsbuffer == NULL)
		return 0;

	IDirectSoundBuffer_GetStatus(dsbuffer, &status);

	return (status & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING));
}


                                                                
  
                                                       
                                                  
                   
  
                                                                
   
static LPDIRECTSOUNDBUFFER create_buffer (void *data, INT32 length, BOOL as3d)
{
	LPDIRECTSOUNDBUFFER dsbuffer;
	HRESULT             hr;
	WAVEFORMATEX        wfm;
	DSBUFFERDESC        dsbdesc;
	LPVOID              lpvAudio1;  
	DWORD               dwBytes1;   
	LPVOID              lpvAudio2;  
	DWORD               dwBytes2;   

	ZeroMemory(&wfm, sizeof (WAVEFORMATEX));
	wfm.wFormatTag = WAVE_FORMAT_PCM;
	wfm.nChannels = 1;
	wfm.nSamplesPerSec = data?*((UINT16 *)data+1):srate;       
	wfm.wBitsPerSample = 8;
	wfm.nBlockAlign = (WORD)(wfm.wBitsPerSample / 8 * wfm.nChannels);
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;


	ZeroMemory (&dsbdesc, sizeof (DSBUFFERDESC) );
	dsbdesc.dwSize = sizeof (DSBUFFERDESC);
	dsbdesc.dwFlags = as3d?_3DSOURCE_FLAGS:_2DSOURCE_FLAGS;
	dsbdesc.dwBufferBytes = length;
	dsbdesc.lpwfxFormat = &wfm;




#ifdef DX7
	if (as3d)
		dsbdesc.guid3DAlgorithm = (virtualization?DS3DALG_HRTF_FULL:DS3DALG_DEFAULT);
#endif

	hr = IDirectSound_CreateSoundBuffer (DSnd, &dsbdesc, &dsbuffer, NULL);



	if (FAILED(hr))
	{
		DBG_Printf("CreateSoundBuffer FAILED. Code %d\n", hr);
		return NULL;
	}

	if (data)
	{
		hr = IDirectSoundBuffer_Lock(dsbuffer, 0, length, &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0);


		if (hr == DSERR_BUFFERLOST)
		{
			hr = IDirectSoundBuffer_Restore(dsbuffer);
			if (FAILED(hr))
				I_ErrorDS3D("Restore fail on %x, code %d\n",dsbuffer, hr);
			hr = IDirectSoundBuffer_Lock(dsbuffer, 0, length, &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0);
			if (FAILED(hr))
				I_ErrorDS3D("Lock fail(2) on %x, code %d\n",dsbuffer, hr);
		}
		else
			if (FAILED(hr))
				I_ErrorDS3D("Lock fail(1) on %x, code %d\n",dsbuffer, hr);


		CopyMemory (lpvAudio1, (LPBYTE)data+8, dwBytes1);

		if ( dwBytes2 && lpvAudio2)
			CopyMemory(lpvAudio2, ((LPBYTE)data + 8) + dwBytes1, dwBytes2);



		hr = IDirectSoundBuffer_Unlock(dsbuffer, lpvAudio1, dwBytes1, lpvAudio2, dwBytes2);

		if (FAILED(hr))
			I_ErrorDS3D("Unlock fail on %x, code %d\n",dsbuffer, hr);
	}

	return dsbuffer;
}


                                                                
  
                                
  
                                                                
   
static LPDIRECTSOUND3DBUFFER create_3dbuffer(LPDIRECTSOUNDBUFFER dsbuffer, LPDIRECTSOUND3DBUFFER source3d)
{
	HRESULT hr;

	hr = IDirectSoundBuffer_QueryInterface(dsbuffer,
	 &IID_IDirectSound3DBuffer, (void **)&source3d);

	if (FAILED(hr))
	{
		DBG_Printf("Couldn't obtain 3D Buffer interface. Code %d\n", hr);
		return NULL;
	}
#ifdef DX7
	if (hr == DS_NO_VIRTUALIZATION)
	{
		DBG_Printf("The 3D virtualization is not supported under this OS.\n");
			virtualization = FALSE;
	}
#endif
	return source3d;
}


                                                                
  
                                          
                                                  
  
                                                                
   
static INT32 find_handle(INT32 new_sfx_id, INT32 new_is3d)
{
	INT32      free_sfx;
	stack_t *snd;


	new_sfx_id = new_is3d = 0;
	for (free_sfx = 0, snd = _stack; free_sfx < allocated_sounds; snd++, free_sfx++)
	{

		if (snd->permanent)
			continue;
  
                       
   
                                                                   
    
                 
             
    

                                            
                    
   
  
		if (snd->dsbuffer == 0)
			break;

	}


	if (free_sfx == allocated_sounds)
	{
		DBG_Printf("No free or same sfx found so increase stack (currently %d srcs)\n", allocated_sounds);
		free_sfx = reallocate_stack() ? free_sfx : -1;
	}

	return free_sfx;
}


                                                                
  
                                                           
  
                                                                
   
static INT32 recalc_volume(INT32 base_vol, INT32 steps)
{
	return (base_vol * ((DSBVOLUME_MAX-DSBVOLUME_MIN)/4)) / steps
	       + (DSBVOLUME_MAX - ((DSBVOLUME_MAX-DSBVOLUME_MIN)/4));
}


                                                                
  
                                                 
  
                                                                
   
static void UpdateSoundVolume (LPDIRECTSOUNDBUFFER lpSnd, INT32 volume)
{
	                                                               
                                                              

                                                
	IDirectSoundBuffer_SetVolume (lpSnd, recalc_volume(volume, 256));
}






#define DSBPAN_RANGE    (DSBPAN_RIGHT-(DSBPAN_LEFT))


#define SEP_RANGE       256
static void Update2DSoundPanning (LPDIRECTSOUNDBUFFER lpSnd, INT32 sep)
{
	HRESULT hr;
	hr = IDirectSoundBuffer_SetPan (lpSnd, (sep * DSBPAN_RANGE)/SEP_RANGE - DSBPAN_RIGHT);


}



                                                                               
  
                                 
  
                                                                               
EXPORT BOOL HWRAPI( Startup ) (I_Error_t FatalErrorFunction, snddev_t *snd_dev)
{
	HRESULT      hr;
	DSBUFFERDESC desc;
	WAVEFORMATEX wfm;
	DSCAPS       dscaps;
	DWORD        speakers;
	DWORD        speaker_config;
	DWORD        speaker_geometry;

	I_ErrorDS3D = FatalErrorFunction;
	DBG_Printf ("S_DS3D Init(): DirectSound3D driver for SRB2 v1.09");  

	DBG_Printf("Initialising DirectSound3D...\n");
	hr = DirectSoundCreate( NULL, &DSnd, NULL);
	if (FAILED(hr))
	{
		DBG_Printf("Failed to obtain DirectSound\n");
		return FALSE;
	}

	hr = IDirectSound_SetCooperativeLevel(DSnd, snd_dev->hWnd, snd_dev->cooplevel);
	if (FAILED(hr))
	{
		DBG_Printf("Couldn't set coopertive level\n");
		return FALSE;
	}

	dscaps.dwSize = sizeof (DSCAPS);

	IDirectSound_GetCaps(DSnd, &dscaps);
	IDirectSound_GetSpeakerConfig(DSnd, &speakers);

	DBG_Printf("Sound hardware capabilities:\n");
	DBG_Printf("   Driver is %scertified.\n", (dscaps.dwFlags & DSCAPS_CERTIFIED) == DSCAPS_CERTIFIED?"":"not ");
	DBG_Printf("   Maximum hardware mixing buffers %d\n", dscaps.dwMaxHwMixingAllBuffers);
	DBG_Printf("   Maximum hardware 3D buffers %d\n", dscaps.dwFreeHw3DAllBuffers);

	speaker_config = DSSPEAKER_CONFIG(speakers);
	speaker_geometry = DSSPEAKER_GEOMETRY(speakers);

	DBG_Printf("Current speaker configuration: ");

	switch (speaker_config)
	{
#ifdef DX7
		case DSSPEAKER_5POINT1:
			DBG_Printf("5.1 (5 speakers with subwoofer).\n");
			break;
#endif
		case DSSPEAKER_HEADPHONE:
			DBG_Printf("headphone.\n");
			break;

		case DSSPEAKER_MONO:
			DBG_Printf("single speaker (mono).\n");
			break;

		case DSSPEAKER_QUAD:
			DBG_Printf("quadrophonic\n");
			break;

		case DSSPEAKER_SURROUND:
			DBG_Printf("surround.\n");
			break;

		case DSSPEAKER_STEREO:
			DBG_Printf("stereo with %s geometry ",
				speaker_geometry == DSSPEAKER_GEOMETRY_WIDE
					? "wide (arc of 20 deg.)"
					: speaker_geometry == DSSPEAKER_GEOMETRY_NARROW
						? "narrow (arc of 10 deg.)"
						: speaker_geometry == DSSPEAKER_GEOMETRY_MIN
							? "min (arc of 5 deg.)"
							: speaker_geometry == DSSPEAKER_GEOMETRY_MAX
								? "max (arc of 180 deg.)"
								: "unknown");
			break;
		default:
			DBG_Printf("undetectable.\n");

	}

	update_mode = DS3D_IMMEDIATE;


	ZeroMemory(&desc, sizeof (DSBUFFERDESC));
	desc.dwSize = sizeof (DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	desc.dwBufferBytes = 0;
	desc.lpwfxFormat = NULL;

	hr = IDirectSound_CreateSoundBuffer(DSnd, &desc, &PrimaryBuffer, NULL);
	if (FAILED(hr))
	{
		DBG_Printf("CreateSoundBuffer FAILED (ErrNo %d)\n", hr);
		return FALSE;
	}


	hr = IDirectSoundBuffer_QueryInterface(PrimaryBuffer,
	 &IID_IDirectSound3DListener, (void **)&Listener);

	if (FAILED(hr))
	{
		DBG_Printf("Couldn't obtain 3D Listener interface (ErrNo %d)\n", hr);
		return FALSE;
	}


	IDirectSound3DListener_SetDistanceFactor(Listener, 1.0f/72.0f, DS3D_IMMEDIATE);
	IDirectSound3DListener_SetRolloffFactor(Listener, 1.6f, DS3D_IMMEDIATE);

	listener_parms.dwSize = sizeof (DS3DLISTENER);
	IDirectSound3DListener_GetAllParameters(Listener, &listener_parms);

	ZeroMemory (&wfm, sizeof (WAVEFORMATEX));
	wfm.wFormatTag = WAVE_FORMAT_PCM;
	wfm.nChannels = 2;
	wfm.nSamplesPerSec = srate = snd_dev->sample_rate;
	wfm.wBitsPerSample = (WORD)snd_dev->bps;
	wfm.nBlockAlign = (WORD)(wfm.wBitsPerSample / 8 * wfm.nChannels);
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

	if (snd_dev->cooplevel >= DSSCL_PRIORITY)
	{
		hr = IDirectSoundBuffer_SetFormat(PrimaryBuffer, &wfm);
		if (FAILED(hr))
			DBG_Printf("Couldn't set primary buffer format.\n");

		DBG_Printf(" Compacting onboard sound-memory...");
		hr = IDirectSound_Compact(DSnd);
		DBG_Printf(" %s\n", SUCCEEDED(hr) ? M_GetText("Done\n") : M_GetText("Failed\n"));
	}

#ifdef DX7

	virtualization = TRUE;
#endif

	_stack = NULL;
	allocated_sounds = 0;
	IDirectSoundBuffer_Play(PrimaryBuffer, 0, 0, DSBPLAY_LOOPING);
	return reallocate_stack();
}



                                                                
  
                  
  
                                                                
   
EXPORT void HWRAPI( Shutdown ) (void)
{
	INT32 i;

	DBG_Printf ("S_DS3D Shutdown()\n");

	for (i = 0; i < allocated_sounds; i++)
	{
		StopSource(i);
		kill_sound(_stack + i);
	}

	if (_stack)
		free(_stack);

	if (Listener)
	{
		IDirectSound3DListener_Release(Listener);
		Listener = NULL;
	}

	if (PrimaryBuffer)
		RELEASE_BUFFER(PrimaryBuffer);

	if (DSnd)
	{
		IDirectSound_Release(DSnd);
		DSnd = NULL;
	}

}



EXPORT INT32 HWRAPI (IsPlaying) (INT32 handle)
{
	if (handle < 0 || handle >= allocated_sounds)
		return FALSE;

	return is_playing(_stack + handle);
}



static float recalc_pitch(INT32 doom_pitch)
{
	return doom_pitch < NORMAL_PITCH ?
	 (float)(doom_pitch + NORMAL_PITCH) / (NORMAL_PITCH * 2)
	 :(float)doom_pitch / (float)NORMAL_PITCH;
}


static stack_t *setup_source(INT32 handle, sfx_data_t *sfx, BOOL is_3dsource)
{
	stack_t               *snd;

	INT32                    data_length;
	LPDIRECTSOUNDBUFFER    dsbuffer = NULL;
	LPDIRECTSOUND3DBUFFER  ds3dbuffer = NULL;

	if (handle == NEW_HANDLE)
		handle = find_handle(sfx?sfx->id:0, is_3dsource);

	snd = _stack + handle;


	if (snd->dsbuffer)
		return snd;

	data_length = sfx?sfx->length - 8:DSBSIZE_MIN;

	dsbuffer = create_buffer(sfx ? sfx->data : NULL, data_length, is_3dsource);
	if (dsbuffer)
	{

		if (is_3dsource)
		{
			ds3dbuffer = create_3dbuffer(dsbuffer, ds3dbuffer);
			if (!ds3dbuffer)
			{
				RELEASE_BUFFER(dsbuffer);
				return NULL;
			}
			snd->parameters.dwSize = sizeof (DS3DBUFFER);
			IDirectSound3DBuffer_GetAllParameters(ds3dbuffer, &snd->parameters);
		}


		if (sfx)
		{
			DWORD freq;

			freq = (DWORD)((float)(*((UINT16 *)sfx->data+1)) * recalc_pitch(sfx->pitch));
			IDirectSoundBuffer_SetFrequency(dsbuffer, freq);
		}

		snd->dsbuffer = dsbuffer;
		snd->dsbuffer3D = ds3dbuffer;
		snd->LRU = 0;
		snd->sfx_id = sfx ? sfx->id : 0;
		if (!is_3dsource)
			snd->permanent = 0;
		return snd;
	}
	return NULL;

}

                                                                               
  
                             
  
                                                                                
EXPORT INT32 HWRAPI ( Add2DSource ) (sfx_data_t *sfx)
{
	stack_t *snd;

	if (!sfx)
		return -1;

	snd = setup_source(NEW_HANDLE, sfx, IS_2DSOURCE);
	if (snd)
	{
		UpdateSoundVolume(snd->dsbuffer, sfx->volume);
		Update2DSoundPanning(snd->dsbuffer, sfx->sep);
	}
	return snd - _stack;
}

                                                                               
  
                    
  
                                                                                

EXPORT INT32 HWRAPI ( Add3DSource ) (source3D_data_t *src, sfx_data_t *sfx)
{
	stack_t *snd;

	snd = setup_source(NEW_HANDLE, sfx, IS_3DSOURCE);

	if (snd)
	{

		                 
                 
                 

                                                                 
                                                                                                     
                                                                                                                

#ifdef DUMMYUNIONNAMEN
		snd->parameters.vPosition.DUMMYUNIONNAMEN(9).x = src->pos.x;
		snd->parameters.vPosition.DUMMYUNIONNAMEN(10).y = src->pos.z;
		snd->parameters.vPosition.DUMMYUNIONNAMEN(11).z = src->pos.y;

		snd->parameters.vVelocity.DUMMYUNIONNAMEN(9).x = src->pos.momx;
		snd->parameters.vVelocity.DUMMYUNIONNAMEN(10).y = src->pos.momz;
		snd->parameters.vVelocity.DUMMYUNIONNAMEN(11).z = src->pos.momy;
#else
		snd->parameters.vPosition.x = src->pos.x;
		snd->parameters.vPosition.y = src->pos.z;
		snd->parameters.vPosition.z = src->pos.y;

		snd->parameters.vVelocity.x = src->pos.momx;
		snd->parameters.vVelocity.y = src->pos.momz;
		snd->parameters.vVelocity.z = src->pos.momy;
#endif
		snd->parameters.dwMode = src->head_relative ? DS3DMODE_HEADRELATIVE : DS3DMODE_NORMAL;

		snd->parameters.flMinDistance = src->min_distance;
		snd->parameters.flMaxDistance = src->max_distance;


		snd->permanent = src->permanent;

		UpdateSoundVolume(snd->dsbuffer, sfx?sfx->volume:255);
		IDirectSound3DBuffer_SetAllParameters(snd->dsbuffer3D, &snd->parameters, DS3D_IMMEDIATE);
		IDirectSoundBuffer_SetCurrentPosition(snd->dsbuffer, 0);
	}

	return (snd - _stack);
}

                                                                               
  
                                                                
                                  
  
                                                                               
EXPORT void HWRAPI (KillSource) (INT32 handle)
{

	if (handle < 0 || handle >= allocated_sounds)
		return;
  
                                
  
                                                        
                         
  
     
     

		kill_sound(_stack + handle);

}


                                                                               
  
                                                      
  
                                                                               
EXPORT void HWRAPI (Update2DSoundParms) (INT32 handle, INT32 vol, INT32 sep)
{
	LPDIRECTSOUNDBUFFER dsbuffer;

	if (handle < 0 || handle >= allocated_sounds)
		return;

	if ((_stack+handle)->dsbuffer3D)
		return;
	dsbuffer = (_stack + handle)->dsbuffer;

	UpdateSoundVolume(dsbuffer, vol);
	Update2DSoundPanning(dsbuffer, sep);
}






EXPORT void HWRAPI (SetGlobalSfxVolume) (INT32 volume)
{
	INT32     vol;
	HRESULT hr;


	if (volume)
		vol = recalc_volume(volume, 31);
	else
		vol = DSBVOLUME_MIN;

	hr = IDirectSoundBuffer_SetVolume (PrimaryBuffer, vol);
}

EXPORT void HWRAPI ( StopSource) (INT32 handle)
{
	LPDIRECTSOUNDBUFFER dsbuffer;

	if (handle < 0 || handle >= allocated_sounds)
		return;

	dsbuffer = (_stack + handle)->dsbuffer;

	if (dsbuffer)
	{
		IDirectSoundBuffer_Stop(dsbuffer);
		IDirectSoundBuffer_SetCurrentPosition(dsbuffer, 0);
	}
}


EXPORT INT32 HWRAPI ( GetHW3DSVersion) (void)
{
	return VERSION;
}


EXPORT void HWRAPI (BeginFrameUpdate) (void)
{
	update_mode = DS3D_DEFERRED;
}


EXPORT void HWRAPI (EndFrameUpdate) (void)
{
	if (update_mode == DS3D_DEFERRED)
		IDirectSound3DListener_CommitDeferredSettings(Listener);

	update_mode = DS3D_IMMEDIATE;
}


                                                                               
                 
  
                                   
             
                
             
                                                                               
EXPORT void HWRAPI (UpdateListener) (listener_data_t *data)
{
	D3DVECTOR pos;
	D3DVECTOR face;
	D3DVECTOR head;
	D3DVECTOR velocity;
	double      f_angle;



#ifdef DUMMYUNIONNAMEN
	pos.DUMMYUNIONNAMEN(9).x = (D3DVALUE)data->x;
	pos.DUMMYUNIONNAMEN(10).y = (D3DVALUE)data->z;
	pos.DUMMYUNIONNAMEN(11).z = (D3DVALUE)data->y;

	velocity.DUMMYUNIONNAMEN(9).x = (D3DVALUE)data->momx;
	velocity.DUMMYUNIONNAMEN(10).y = (D3DVALUE)data->momz;
	velocity.DUMMYUNIONNAMEN(11).z = (D3DVALUE)data->momy;
#else
	pos.x = (D3DVALUE)data->x;
	pos.y = (D3DVALUE)data->z;
	pos.z = (D3DVALUE)data->y;

	velocity.x = (D3DVALUE)data->momx;
	velocity.y = (D3DVALUE)data->momz;
	velocity.z = (D3DVALUE)data->momy;
#endif
	f_angle = (data->f_angle) / 180 * M_PI;















#ifdef DUMMYUNIONNAMEN
	face.DUMMYUNIONNAMEN(9).x = (D3DVALUE)cos(f_angle);
	face.DUMMYUNIONNAMEN(11).z = (D3DVALUE)sin(f_angle);
	face.DUMMYUNIONNAMEN(10).y = 0.0f;

	head.DUMMYUNIONNAMEN(9).x = 0.0f;
	head.DUMMYUNIONNAMEN(10).y = 1.0f;
	head.DUMMYUNIONNAMEN(11).z = 0.0f;
#else
	face.x = (D3DVALUE)cos(f_angle);
	face.z = (D3DVALUE)sin(f_angle);
	face.y = 0.0f;

	head.x = 0.0f;
	head.y = 1.0f;
	head.z = 0.0f;
#endif



  
                    

                                         
                        
                       
  


	CopyMemory(&listener_parms.vPosition, &pos, sizeof (D3DVECTOR));
	CopyMemory(&listener_parms.vOrientFront, &face, sizeof (D3DVECTOR));
	CopyMemory(&listener_parms.vOrientTop, &head, sizeof (D3DVECTOR));
	CopyMemory(&listener_parms.vVelocity, &velocity, sizeof (D3DVECTOR));

	IDirectSound3DListener_SetAllParameters(Listener, &listener_parms, update_mode);
}

EXPORT void HWRAPI (UpdateListener2) (listener_data_t *data)
{
	D3DVECTOR pos;
	D3DVECTOR face;
	D3DVECTOR head;
	D3DVECTOR velocity;
	double      f_angle;



	if (!data) return;

#ifdef DUMMYUNIONNAMEN
	pos.DUMMYUNIONNAMEN(9).x = (D3DVALUE)data->x;
	pos.DUMMYUNIONNAMEN(10).y = (D3DVALUE)data->z;
	pos.DUMMYUNIONNAMEN(11).z = (D3DVALUE)data->y;

	velocity.DUMMYUNIONNAMEN(9).x = (D3DVALUE)data->momx;
	velocity.DUMMYUNIONNAMEN(10).y = (D3DVALUE)data->momz;
	velocity.DUMMYUNIONNAMEN(11).z = (D3DVALUE)data->momy;
#else
	pos.x = (D3DVALUE)data->x;
	pos.y = (D3DVALUE)data->z;
	pos.z = (D3DVALUE)data->y;

	velocity.x = (D3DVALUE)data->momx;
	velocity.y = (D3DVALUE)data->momz;
	velocity.z = (D3DVALUE)data->momy;
#endif
	f_angle = (data->f_angle) / 180 * M_PI;















#ifdef DUMMYUNIONNAMEN
	face.DUMMYUNIONNAMEN(9).x = cos(f_angle);
	face.DUMMYUNIONNAMEN(11).z = sin(f_angle);
	face.DUMMYUNIONNAMEN(10).y = 0.0f;

	head.DUMMYUNIONNAMEN(9).x = 0.0f;
	head.DUMMYUNIONNAMEN(10).y = 1.0f;
	head.DUMMYUNIONNAMEN(11).z = 0.0f;
#else
	face.x = (D3DVALUE)cos(f_angle);
	face.z = (D3DVALUE)sin(f_angle);
	face.y = 0.0f;

	head.x = 0.0f;
	head.y = 1.0f;
	head.z = 0.0f;
#endif




  
                    

                                         
                        
                       
  


	CopyMemory(&listener_parms.vPosition, &pos, sizeof (D3DVECTOR));
	CopyMemory(&listener_parms.vOrientFront, &face, sizeof (D3DVECTOR));
	CopyMemory(&listener_parms.vOrientTop, &head, sizeof (D3DVECTOR));
	CopyMemory(&listener_parms.vVelocity, &velocity, sizeof (D3DVECTOR));



}

EXPORT INT32 HWRAPI (SetCone) (INT32 handle, cone_def_t *cone_def)
{
	stack_t     *snd;


	if (handle < 0 || handle >= allocated_sounds)
		return -1;

	snd = _stack + handle;

	if (snd->dsbuffer3D)
	{

		                                           
                                                  
                                                         
                                                         
                                            

		snd->parameters.dwInsideConeAngle = (DWORD)cone_def->inner;
		snd->parameters.dwOutsideConeAngle = (DWORD)cone_def->outer;
		snd->parameters.lConeOutsideVolume = recalc_volume(cone_def->outer_gain, 256);

		return IDirectSound3DBuffer_SetAllParameters(snd->dsbuffer3D, &snd->parameters, update_mode);
	}

	return -1;
}

EXPORT void HWRAPI (Update3DSource) (INT32 handle, source3D_pos_t *data)
{
	stack_t     *snd;

	if (handle < 0 || handle >= allocated_sounds)
		return;

	snd = _stack + handle;
	if (snd->dsbuffer3D)
	{

		                                     
                                                       



#ifdef DUMMYUNIONNAMEN
		snd->parameters.vPosition.DUMMYUNIONNAMEN(9).x = data->x;
		snd->parameters.vPosition.DUMMYUNIONNAMEN(10).y = data->z;
		snd->parameters.vPosition.DUMMYUNIONNAMEN(11).z = data->y;
#else
		snd->parameters.vPosition.x = data->x;
		snd->parameters.vPosition.y = data->z;
		snd->parameters.vPosition.z = data->y;
#endif

		                                        
                                        
                                 

#ifdef DUMMYUNIONNAMEN
		snd->parameters.vVelocity.x = data->momx;
		snd->parameters.vVelocity.y = data->momz;
		snd->parameters.vVelocity.z = data->momy;
#else
		snd->parameters.vVelocity.x = data->momx;
		snd->parameters.vVelocity.y = data->momz;
		snd->parameters.vVelocity.z = data->momy;
#endif





		IDirectSound3DBuffer_SetAllParameters(snd->dsbuffer3D, &snd->parameters, update_mode);
	}
}


EXPORT INT32 HWRAPI (StartSource) (INT32 handle)
{
	LPDIRECTSOUNDBUFFER snd;

	if (handle < 0 || handle >= allocated_sounds)
		return -1;

	snd = (_stack + handle)->dsbuffer;
	IDirectSoundBuffer_SetCurrentPosition(snd, 0);
	return IDirectSoundBuffer_Play(snd, 0, 0, 0);

}




EXPORT INT32 HWRAPI (Reload3DSource) (INT32 handle, sfx_data_t *data)
{
	DS3DBUFFER  temp;
	stack_t    *snd;
	INT32         perm;



	if (handle < 0 || handle >= allocated_sounds)
		return -1;

	snd = _stack + handle;
	CopyMemory(&temp, &snd->parameters, sizeof (DS3DBUFFER));
	perm = snd->permanent;

	kill_sound(snd);

	snd = setup_source(handle, data, IS_3DSOURCE);
	                                                                
                           
            
                                                                   
                             
  
                                
            
  

                        
                
	if (snd)
	{
		snd->permanent = perm;
		IDirectSound3DBuffer_SetAllParameters(snd->dsbuffer3D, &temp, DS3D_IMMEDIATE);
		CopyMemory(&snd->parameters, &temp, sizeof (DS3DBUFFER));
	}

	return(snd - _stack);
}

EXPORT void HWRAPI (UpdateSourceVolume) (INT32 handle, INT32 volume)
{
	if (handle < 0 || handle >= allocated_sounds)
		return;
	UpdateSoundVolume((_stack + handle)->dsbuffer, volume);
}
