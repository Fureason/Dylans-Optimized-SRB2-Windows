
#define SFX_MIX_CHANNELS 256
#define AUDIO_GAIN_ONE 65536u
static UINT32 mix_headroom_gain = AUDIO_GAIN_ONE;
static struct {Mix_Chunk *chunk; INT32 peak, volume, pan, applied; boolean active;} mix_voices[SFX_MIX_CHANNELS];
static struct {Mix_Chunk *chunk; INT32 peak;} mix_peaks[NUMSFX];
static INT32 mix_voice_limit;
static tic_t mix_headroom_tic;
static INT32 mix_music_applied = -1;
static UINT32 get_real_volume(UINT8 volume);

static UINT32 I_NominalMusicVolume(UINT8 volume)
{
#ifdef _WIN32
	if (I_SongType() == MU_MID) return 128;
#endif
	return ((UINT32)volume*128/31) * (UINT32)internal_volume / 100;
}

static INT32 I_MusicPeakBound(void)
{
	if (!I_SongPlaying() || songpaused) return 0;
#ifdef _WIN32
	if (I_SongType() == MU_MID) return 0;
#endif


#ifdef HAVE_GME
	if (gme) return 32768*min(music_volume,18)/20 * max(100,internal_volume)/100;
#endif
#ifdef HAVE_OPENMPT
	if (openmpt_mhandle) return 32768*min(music_volume,18)/20 * max(100,internal_volume)/100;
#endif
	return 32768*(INT32)music_volume/31 * max(100,internal_volume)/100;
}

static INT32 I_SfxPeak(sfxenum_t id, Mix_Chunk *chunk)
{
	INT32 peak = 0;
	Uint32 i;
	if (mix_peaks[id].chunk == chunk && mix_peaks[id].peak) return mix_peaks[id].peak;
	for (i = 0; i + 1 < chunk->alen; i += 2)
	{
		Sint16 sample;
		memcpy(&sample, chunk->abuf+i, sizeof sample);
		peak = max(peak, abs((INT32)sample));
	}
	mix_peaks[id].chunk = chunk;
	mix_peaks[id].peak = peak;
	return peak;
}

static void I_RebalanceSfx(INT32 pending, boolean recover)
{
	INT32 i, chunkvolume;
	INT64 left = 0, right = 0, total;
	UINT32 target = AUDIO_GAIN_ONE;
	if (!sound_started) return;
	if (recover && mix_headroom_tic == I_GetTime()+1) return;
	if (recover) mix_headroom_tic = I_GetTime()+1;
	for (i = 0; i < mix_voice_limit; i++)
	{
		INT32 volume;
		if (!mix_voices[i].active) continue;
		if (i != pending && !Mix_Playing(i)) { mix_voices[i].active = false; continue; }
		volume = (((UINT16)mix_voices[i].volume + 1) * (UINT16)sfx_volume) / 62;
		left += (INT64)mix_voices[i].peak*volume*min((255-mix_voices[i].pan)*2,255)/(128*255);
		right += (INT64)mix_voices[i].peak*volume*min(mix_voices[i].pan*2,255)/(128*255);
	}
	total = max(left,right);
	if (total && cv_soundclipping.value)
	{
		total += I_MusicPeakBound();
		if (total > 30000) target = (UINT32)(30000LL*AUDIO_GAIN_ONE/total);
	}
	if (!cv_soundclipping.value) mix_headroom_gain = AUDIO_GAIN_ONE;
	else if (target < mix_headroom_gain) mix_headroom_gain = target;
	else if (recover) mix_headroom_gain += min(target-mix_headroom_gain,2048u);




	chunkvolume = max(1,(INT32)sqrt((double)mix_headroom_gain*16384.0/AUDIO_GAIN_ONE));
	for (i = 0; i < mix_voice_limit; i++)
	{
		INT32 nominal, volume;
		if (!mix_voices[i].active) continue;
		nominal = (((UINT16)mix_voices[i].volume + 1) * (UINT16)sfx_volume) / 62;
		volume = (INT32)((INT64)nominal*mix_headroom_gain*128/(chunkvolume*AUDIO_GAIN_ONE));
		if (mix_voices[i].chunk->volume != chunkvolume) Mix_VolumeChunk(mix_voices[i].chunk,chunkvolume);
		volume = min(volume,128);
		if (volume != mix_voices[i].applied)
		{
			Mix_Volume(i,volume);
			mix_voices[i].applied = volume;
		}
	}
	i = get_real_volume(music_volume);
	if (i != mix_music_applied) { Mix_VolumeMusic(i); mix_music_applied = i; }
}
