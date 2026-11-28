



#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifdef boolean
#pragma push_macro("boolean")
#undef boolean
#define MOVIE_MF_RESTORE_BOOLEAN
#endif
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#ifdef MOVIE_MF_RESTORE_BOOLEAN
#pragma pop_macro("boolean")
#undef MOVIE_MF_RESTORE_BOOLEAN
#endif

#define MOVIE_MF_RELEASE(p) do { if (p) { (p)->lpVtbl->Release(p); (p) = NULL; } } while (0)
#define MOVIE_MF_TRY(call) do { hr = (call); if (FAILED(hr)) goto done; } while (0)

typedef struct
{
	HMODULE platform, readwrite;
	boolean com_started, media_started;
	HRESULT (WINAPI *startup)(ULONG, DWORD);
	HRESULT (WINAPI *shutdown)(void);
	HRESULT (WINAPI *attributes)(IMFAttributes **, UINT32);
	HRESULT (WINAPI *media_type)(IMFMediaType **);
	HRESULT (WINAPI *sample)(IMFSample **);
	HRESULT (WINAPI *buffer)(DWORD, IMFMediaBuffer **);
	HRESULT (WINAPI *sink)(LPCWSTR, IMFByteStream *, IMFAttributes *, IMFSinkWriter **);
	HRESULT (WINAPI *reader)(LPCWSTR, IMFAttributes *, IMFSourceReader **);
} movie_mf_api_t;

typedef struct
{
	movie_mf_api_t api;
	IMFSinkWriter *writer;
	DWORD stream;
	UINT32 width, height, fps;
	UINT64 frames;
	boolean writing;
} movie_mf_writer_t;

static HRESULT M_MFPath(const char *path, WCHAR *wide, int count)
{
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count)
		&& !MultiByteToWideChar(CP_ACP, 0, path, -1, wide, count))
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

static HMODULE M_MFSystemLibrary(const WCHAR *name)
{
	WCHAR path[MAX_PATH];
	UINT count = GetSystemDirectoryW(path, MAX_PATH);
	if (!count || count + 1 + wcslen(name) >= MAX_PATH)
		return NULL;
	path[count++] = L'\\';
	wcscpy(path + count, name);
	return LoadLibraryW(path);
}

static void M_MFUnload(movie_mf_api_t *api)
{
	if (api->media_started) api->shutdown();
	if (api->readwrite) FreeLibrary(api->readwrite);
	if (api->platform) FreeLibrary(api->platform);
	if (api->com_started) CoUninitialize();
	memset(api, 0, sizeof *api);
}

static HRESULT M_MFLoad(movie_mf_api_t *api)
{
	HRESULT hr;
	memset(api, 0, sizeof *api);
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) return hr;
	api->com_started = true;
	api->platform = M_MFSystemLibrary(L"mfplat.dll");
	api->readwrite = M_MFSystemLibrary(L"mfreadwrite.dll");
	if (!api->platform || !api->readwrite)
	{
		hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
		goto done;
	}
#define MOVIE_MF_PROC(member, module, name) do { \
	FARPROC proc = GetProcAddress(api->module, name); \
	if (!proc) { hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND); goto done; } \
	memcpy(&api->member, &proc, sizeof api->member); \
} while (0)
	MOVIE_MF_PROC(startup, platform, "MFStartup");
	MOVIE_MF_PROC(shutdown, platform, "MFShutdown");
	MOVIE_MF_PROC(attributes, platform, "MFCreateAttributes");
	MOVIE_MF_PROC(media_type, platform, "MFCreateMediaType");
	MOVIE_MF_PROC(sample, platform, "MFCreateSample");
	MOVIE_MF_PROC(buffer, platform, "MFCreateMemoryBuffer");
	MOVIE_MF_PROC(sink, readwrite, "MFCreateSinkWriterFromURL");
	MOVIE_MF_PROC(reader, readwrite, "MFCreateSourceReaderFromURL");
#undef MOVIE_MF_PROC
	hr = api->startup(MF_VERSION, MFSTARTUP_FULL);
	if (SUCCEEDED(hr)) api->media_started = true;
done:
	if (FAILED(hr)) M_MFUnload(api);
	return hr;
}

static HRESULT M_MFVideoType(movie_mf_api_t *api, IMFMediaType **out,
	REFGUID subtype, UINT32 width, UINT32 height, UINT32 fps)
{
	HRESULT hr;
	IMFMediaType *type = NULL;
	MOVIE_MF_TRY(api->media_type(&type));
	MOVIE_MF_TRY(IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video));
	MOVIE_MF_TRY(IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, subtype));
	MOVIE_MF_TRY(IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, ((UINT64)width << 32) | height));
	MOVIE_MF_TRY(IMFMediaType_SetUINT64(type, &MF_MT_FRAME_RATE, ((UINT64)fps << 32) | 1));
	MOVIE_MF_TRY(IMFMediaType_SetUINT64(type, &MF_MT_PIXEL_ASPECT_RATIO, ((UINT64)1 << 32) | 1));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(type, &MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(type, &MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_BT709));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(type, &MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_709));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(type, &MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(type, &MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235));
	*out = type;
	type = NULL;
done:
	MOVIE_MF_RELEASE(type);
	return hr;
}

static HRESULT M_MFMovieOpen(movie_mf_writer_t *movie, const char *path,
	UINT32 width, UINT32 height, UINT32 fps)
{
	IMFAttributes *attributes = NULL;
	IMFMediaType *encoded = NULL, *input = NULL;
	WCHAR wide[MAX_WADPATH];
	HRESULT hr;
	memset(movie, 0, sizeof *movie);
	if (!width || !height || (width & 1) || (height & 1) || !fps
		|| (UINT64)width * height * 3 / 2 > MAXDWORD)
		return E_INVALIDARG;
	MOVIE_MF_TRY(M_MFLoad(&movie->api));
	MOVIE_MF_TRY(M_MFPath(path, wide, MAX_WADPATH));
	MOVIE_MF_TRY(movie->api.attributes(&attributes, 1));
	MOVIE_MF_TRY(IMFAttributes_SetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));

	MOVIE_MF_TRY(movie->api.sink(wide, NULL, attributes, &movie->writer));
	MOVIE_MF_TRY(M_MFVideoType(&movie->api, &encoded, &MFVideoFormat_H264, width, height, fps));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(encoded, &MF_MT_AVG_BITRATE,
		(UINT32)min(80000000ULL, max(4000000ULL, (UINT64)width * height * fps / 3))));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(encoded, &MF_MT_MPEG2_PROFILE, 77));
	MOVIE_MF_TRY(IMFSinkWriter_AddStream(movie->writer, encoded, &movie->stream));
	MOVIE_MF_TRY(M_MFVideoType(&movie->api, &input, &MFVideoFormat_NV12, width, height, fps));
	MOVIE_MF_TRY(IMFSinkWriter_SetInputMediaType(movie->writer, movie->stream, input, NULL));
	MOVIE_MF_TRY(IMFSinkWriter_BeginWriting(movie->writer));
	movie->writing = true;
	movie->width = width;
	movie->height = height;
	movie->fps = fps;
done:
	MOVIE_MF_RELEASE(attributes);
	MOVIE_MF_RELEASE(encoded);
	MOVIE_MF_RELEASE(input);
	if (FAILED(hr))
	{
		MOVIE_MF_RELEASE(movie->writer);
		M_MFUnload(&movie->api);
	}
	return hr;
}



static void M_MFRGBToNV12(const UINT8 *rgb, UINT8 *nv12, UINT32 width, UINT32 height)
{
	UINT8 *uv = nv12 + (size_t)width * height;
	for (UINT32 y = 0; y < height; y += 2)
		for (UINT32 x = 0; x < width; x += 2)
		{
			INT32 r = 0, g = 0, b = 0;
			for (UINT32 dy = 0; dy < 2; dy++)
				for (UINT32 dx = 0; dx < 2; dx++)
				{
					size_t pos = (size_t)(y + dy) * width + x + dx;
					const UINT8 *pixel = rgb + pos * 3;
					nv12[pos] = (UINT8)(16 + ((11966*pixel[0] + 40254*pixel[1]
						+ 4064*pixel[2] + 32768) >> 16));
					r += pixel[0]; g += pixel[1]; b += pixel[2];
				}
			uv[(size_t)(y / 2) * width + x] = (UINT8)(128
				+ ((-6596*r - 22189*g + 28784*b + 131072) >> 18));
			uv[(size_t)(y / 2) * width + x + 1] = (UINT8)(128
				+ ((28784*r - 26145*g - 2639*b + 131072) >> 18));
		}
}

static HRESULT M_MFMovieFrame(movie_mf_writer_t *movie, const UINT8 *rgb, UINT32 repeats)
{
	IMFSample *sample = NULL;
	IMFMediaBuffer *buffer = NULL;
	BYTE *pixels = NULL;
	HRESULT hr;
	const DWORD size = movie->width * movie->height * 3 / 2;
	const UINT64 first = movie->frames, last = first + max(repeats, 1);
	MOVIE_MF_TRY(movie->api.sample(&sample));
	MOVIE_MF_TRY(movie->api.buffer(size, &buffer));
	MOVIE_MF_TRY(IMFMediaBuffer_Lock(buffer, &pixels, NULL, NULL));
	M_MFRGBToNV12(rgb, pixels, movie->width, movie->height);
	MOVIE_MF_TRY(IMFMediaBuffer_Unlock(buffer));
	MOVIE_MF_TRY(IMFMediaBuffer_SetCurrentLength(buffer, size));
	MOVIE_MF_TRY(IMFSample_AddBuffer(sample, buffer));
	MOVIE_MF_TRY(IMFSample_SetSampleTime(sample, (LONGLONG)(first * 10000000 / movie->fps)));
	MOVIE_MF_TRY(IMFSample_SetSampleDuration(sample, (LONGLONG)(last * 10000000 / movie->fps - first * 10000000 / movie->fps)));
	MOVIE_MF_TRY(IMFSinkWriter_WriteSample(movie->writer, movie->stream, sample));
	movie->frames = last;
done:
	MOVIE_MF_RELEASE(buffer);
	MOVIE_MF_RELEASE(sample);
	return hr;
}

static HRESULT M_MFMovieClose(movie_mf_writer_t *movie)
{
	HRESULT hr = movie->writing ? IMFSinkWriter_Finalize(movie->writer) : E_FAIL;
	MOVIE_MF_RELEASE(movie->writer);
	M_MFUnload(&movie->api);
	movie->writing = false;
	return hr;
}

static HRESULT M_MFReadSample(IMFSourceReader *reader, DWORD stream, IMFSample **sample, LONGLONG *time)
{
	HRESULT hr;
	DWORD flags = 0;
	*sample = NULL;
	do
	{
		hr = IMFSourceReader_ReadSample(reader, stream, 0, NULL, &flags, time, sample);
		if (FAILED(hr)) return hr;
		if (flags & MF_SOURCE_READERF_ERROR) return E_FAIL;
	} while (!*sample && !(flags & MF_SOURCE_READERF_ENDOFSTREAM));
	return S_OK;
}



static HRESULT M_MFMovieMux(const char *video, const char *audio, const char *output,
	UINT64 frames, UINT32 fps)
{
	movie_mf_api_t api;
	IMFSourceReader *video_reader = NULL, *audio_reader = NULL;
	IMFSinkWriter *writer = NULL;
	IMFAttributes *attributes = NULL;
	IMFMediaType *video_type = NULL, *pcm = NULL, *aac = NULL;
	IMFSample *vsample = NULL, *asample = NULL;
	DWORD video_stream = 0, audio_stream = 0;
	UINT32 rate = 0, channels = 0;
	LONGLONG vtime = 0, atime = 0, end = fps ? (LONGLONG)(frames * 10000000 / fps) : 0;
	WCHAR vpath[MAX_WADPATH], apath[MAX_WADPATH], opath[MAX_WADPATH];
	HRESULT hr;
	memset(&api, 0, sizeof api);
	MOVIE_MF_TRY(M_MFPath(video, vpath, MAX_WADPATH));
	MOVIE_MF_TRY(M_MFPath(output, opath, MAX_WADPATH));
	if (!audio)
		return MoveFileExW(vpath, opath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
			? S_OK : HRESULT_FROM_WIN32(GetLastError());
	MOVIE_MF_TRY(M_MFPath(audio, apath, MAX_WADPATH));
	MOVIE_MF_TRY(M_MFLoad(&api));
	MOVIE_MF_TRY(api.reader(vpath, NULL, &video_reader));
	MOVIE_MF_TRY(IMFSourceReader_SetStreamSelection(video_reader, MF_SOURCE_READER_ALL_STREAMS, FALSE));
	MOVIE_MF_TRY(IMFSourceReader_SetStreamSelection(video_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE));
	MOVIE_MF_TRY(IMFSourceReader_GetNativeMediaType(video_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &video_type));
	MOVIE_MF_TRY(IMFSourceReader_SetCurrentMediaType(video_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, video_type));
	MOVIE_MF_TRY(api.reader(apath, NULL, &audio_reader));
	MOVIE_MF_TRY(IMFSourceReader_GetNativeMediaType(audio_reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &pcm));
	MOVIE_MF_TRY(IMFMediaType_GetUINT32(pcm, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate));
	MOVIE_MF_TRY(IMFMediaType_GetUINT32(pcm, &MF_MT_AUDIO_NUM_CHANNELS, &channels));
	MOVIE_MF_TRY(api.attributes(&attributes, 1));
	MOVIE_MF_TRY(IMFAttributes_SetUINT32(attributes, &MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
	MOVIE_MF_TRY(api.sink(opath, NULL, attributes, &writer));
	MOVIE_MF_TRY(IMFSinkWriter_AddStream(writer, video_type, &video_stream));
	MOVIE_MF_TRY(IMFSinkWriter_SetInputMediaType(writer, video_stream, video_type, NULL));
	MOVIE_MF_TRY(api.media_type(&aac));
	MOVIE_MF_TRY(IMFMediaType_SetGUID(aac, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio));
	MOVIE_MF_TRY(IMFMediaType_SetGUID(aac, &MF_MT_SUBTYPE, &MFAudioFormat_AAC));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(aac, &MF_MT_AUDIO_NUM_CHANNELS, channels));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(aac, &MF_MT_AUDIO_SAMPLES_PER_SECOND, rate));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(aac, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
	MOVIE_MF_TRY(IMFMediaType_SetUINT32(aac, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000));
	MOVIE_MF_TRY(IMFSinkWriter_AddStream(writer, aac, &audio_stream));
	MOVIE_MF_TRY(IMFSinkWriter_SetInputMediaType(writer, audio_stream, pcm, NULL));
	MOVIE_MF_TRY(IMFSinkWriter_BeginWriting(writer));
	MOVIE_MF_TRY(M_MFReadSample(video_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &vsample, &vtime));
	MOVIE_MF_TRY(M_MFReadSample(audio_reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &asample, &atime));
	while (vsample || asample)
	{
		if (asample && atime >= end) MOVIE_MF_RELEASE(asample);
		if (vsample && (!asample || vtime <= atime))
		{
			MOVIE_MF_TRY(IMFSinkWriter_WriteSample(writer, video_stream, vsample));
			MOVIE_MF_RELEASE(vsample);
			MOVIE_MF_TRY(M_MFReadSample(video_reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &vsample, &vtime));
		}
		else if (asample)
		{
			MOVIE_MF_TRY(IMFSinkWriter_WriteSample(writer, audio_stream, asample));
			MOVIE_MF_RELEASE(asample);
			MOVIE_MF_TRY(M_MFReadSample(audio_reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, &asample, &atime));
		}
	}
	MOVIE_MF_TRY(IMFSinkWriter_Finalize(writer));
done:
	MOVIE_MF_RELEASE(vsample);
	MOVIE_MF_RELEASE(asample);
	MOVIE_MF_RELEASE(video_type);
	MOVIE_MF_RELEASE(pcm);
	MOVIE_MF_RELEASE(aac);
	MOVIE_MF_RELEASE(video_reader);
	MOVIE_MF_RELEASE(audio_reader);
	MOVIE_MF_RELEASE(writer);
	MOVIE_MF_RELEASE(attributes);
	M_MFUnload(&api);
	return hr;
}

#undef MOVIE_MF_TRY
#undef MOVIE_MF_RELEASE
