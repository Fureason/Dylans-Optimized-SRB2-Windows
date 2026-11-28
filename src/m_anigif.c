// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2013-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2013      by "Ninji".
// Copyright (C) 2013-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_anigif.c
/// \brief Animated GIF creation movie mode.
///        Uses an implementation of Lempel–Ziv–Welch (LZW) compression,
///        which by-the-way: the patents have expired for over ten years ago.

#include "m_anigif.h"
#include "d_main.h"
#if defined(HAVE_SDL) && defined(HAVE_THREADS)
#include "SDL.h"
#define GIF_BACKGROUND 1
#endif
#include "v_video.h"
#include "i_video.h"
#include "i_system.h" // I_GetPreciseTime
#include "m_misc.h"
#include "st_stuff.h" // st_palette
#include "doomstat.h" // singletics

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// GIFs are always little-endian
#include "byteptr.h"

CV_PossibleValue_t gif_dynamicdelay_cons_t[] = {
	{0, "Off"},
	{1, "On"},
	{2, "Accurate, experimental"},
{0, NULL}};

consvar_t cv_gif_optimize = CVAR_INIT ("gif_optimize", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_gif_downscale =  CVAR_INIT ("gif_downscale", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_gif_dynamicdelay = CVAR_INIT ("gif_dynamicdelay", "On", CV_SAVE, gif_dynamicdelay_cons_t, NULL);
consvar_t cv_gif_localcolortable =  CVAR_INIT ("gif_localcolortable", "On", CV_SAVE, CV_OnOff, NULL);
static CV_PossibleValue_t gif_maxfps_cons_t[] = {{5, "MIN"}, {60, "MAX"}, {0, NULL}};
consvar_t cv_gif_maxfps = CVAR_INIT ("gif_maxfps", "30", CV_SAVE, gif_maxfps_cons_t, NULL);

#ifdef HAVE_ANIGIF
#define GIF_QUEUE_SIZE 3

typedef struct
{
	UINT8 *pixels;
	size_t capacity;
	RGBA_t palette[256];
	UINT16 delay;
	boolean rgb;
} gif_capture_t;

static FILE *gif_out;
static boolean gif_optimize, gif_localcolortable, gif_colorprofile;
static UINT8 gif_dynamicdelay;
static INT32 gif_width, gif_height, gif_source_width, gif_source_height, gif_step;
static RGBA_t gif_headerpalette[256], gif_previouspalette[256];
static UINT8 *gif_pixels, *gif_previous;
static gif_capture_t gif_queue[GIF_QUEUE_SIZE];
static INT32 gif_frames;
static unsigned gif_dropped, gif_captures;
static boolean gif_write_failed;
static precise_t gif_nextcapturetime, gif_lastcapturetime;
static UINT64 gif_delayremainder;
static UINT8 gif_writeover;

#ifdef GIF_BACKGROUND
static SDL_Thread *gif_thread;
static SDL_mutex *gif_mutex;
static SDL_cond *gif_ready;
static unsigned gif_head, gif_tail, gif_count;
static boolean gif_finishing;
#endif
// OPTIMIZE gif output
// ---

//
// GIF_optimizecmprow
// checks a row for modification, and if any is detected, what parts
// modified input 'last': returns current row if modification detected
// modified input 'left': returns leftmost known changed pixel
// modified input 'right': returns rightmost known changed pixel
//
static void GIF_optimizeregion(const UINT8 *current, const UINT8 *previous,
	INT32 *x, INT32 *y, INT32 *w, INT32 *h)
{
	INT32 left = gif_width, right = -1, top = gif_height, bottom = -1;
	for (INT32 row = 0; row < gif_height; row++)
	{
		const size_t offset = (size_t)row * gif_width;
		const UINT8 *a = current + offset, *b = previous + offset;
		if (!memcmp(a, b, gif_width))
			continue;
		top = min(top, row);
		bottom = row;
		for (INT32 col = 0; col < left; col++)
		{
			if (a[col] != b[col])
			{
				left = col;
				break;
			}
		}
		for (INT32 col = gif_width - 1; col > right; col--)
		{
			if (a[col] != b[col])
			{
				right = col;
				break;
			}
		}
	}

	if (right < 0)
	{

		*x = *y = 0;
		*w = *h = 1;
		return;
	}
	*x = left;
	*y = top;
	*w = right + 1 - left;
	*h = bottom + 1 - top;
}
// GIF Bit WRiter
// ---
static UINT8 gifbwr_buf[256];
static UINT8 *gifbwr_cur;
static UINT8 gifbwr_bufsize = 0;

static UINT32 gifbwr_bits_buf = 0;
static INT32 gifbwr_bits_num = 0;
static UINT8 gifbwr_bits_min = 9;

//
// GIF_bwr_flush
// flushes any bits remaining in the buffer.
//
static void GIF_bwrflush(void)
{
	if (gifbwr_bits_num > 0) // will be between 1 and 7
	{
		WRITEUINT8(gifbwr_cur, (UINT8)(gifbwr_bits_buf&0xFF));
		++gifbwr_bufsize;
	}
	gifbwr_bits_buf = gifbwr_bits_num = 0;
}

//
// GIF_bwr_write
// writes bits into bit buffer,
// writes into buffer when whole bytes obtained
//
static void GIF_bwrwrite(UINT32 idata)
{
	gifbwr_bits_buf |= (idata << gifbwr_bits_num);
	gifbwr_bits_num += gifbwr_bits_min;
	while (gifbwr_bits_num >= 8)
	{
		WRITEUINT8(gifbwr_cur, (UINT8)(gifbwr_bits_buf&0xFF));
		gifbwr_bits_buf >>= 8;
		gifbwr_bits_num -= 8;
		++gifbwr_bufsize;
	}
}



// SCReen BUFfer (obviously)
// ---
static const UINT8 *gif_input;
static size_t scrbuf_pos, scrbuf_linebegin, scrbuf_lineend, scrbuf_writeend;
// GIF LZW algorithm
// ---
#define GIFLZW_TABLECLR  0x100
#define GIFLZW_DATAEND   0x101
#define GIFLZW_DICTSTART 0x102
#define GIFLZW_MAXCODE 4096

static UINT16 giflzw_workingCode;
static UINT16 giflzw_nextCodeToAssign;
static UINT32 giflzw_hashTable[16384];

//
// GIF_prepareLZW
// prepatres the LZW hash table for use
//
static void GIF_prepareLZW(void)
{
	gifbwr_bits_min = 9;
	giflzw_nextCodeToAssign = GIFLZW_DICTSTART;

	memset(giflzw_hashTable, 0, sizeof giflzw_hashTable);
}

//
// GIF_searchHash
// searches the LZW hash table for a match
//
static char GIF_searchHash(UINT32 key, UINT32 *pOutput)
{
	UINT32 entry, position = (key >> 6) & 0x3FFF;

	while (giflzw_hashTable[position] != 0)
	{
		entry = giflzw_hashTable[position];
		if ((entry >> 12) == key)
		{
			*pOutput = (entry & 0xFFF);
			return 1;
		}

		position = (position + 1) & 0x3FFF;
	}

	return 0;
}

//
// GIF_addHash
// stores a hash in the hash table
//
static void GIF_addHash(UINT32 key, UINT32 value)
{
	UINT32 position = (key >> 6) & 0x3FFF;

	for (;;)
	{
		if (giflzw_hashTable[position] == 0)
		{
			giflzw_hashTable[position] = (key << 12) | (value & 0xFFF);
			return;
		}

		position = (position + 1) & 0x3FFF;
	}
}

//
// GIF_feedByte
// feeds bytes into the working code,
// and to the hash table or output from there.
//
static void GIF_feedByte(UINT8 pbyte)
{
	UINT32 key, hashOutput = 0;

	// Prepare a code with this byte if we have none
	if (giflzw_workingCode == UINT16_MAX)
	{
		giflzw_workingCode = pbyte;
		return;
	}

	// If we're here, this means we have a code in progress
	// Is this string already in the dictionary?
	key = (giflzw_workingCode << 8) | pbyte;

	if (0 == GIF_searchHash(key, &hashOutput))
	{
		// It wasn't found.
		// That means we can output what we already had, and
		// create a new dictionary entry containing that
		// plus our new byte.
		if (giflzw_nextCodeToAssign > (1 << gifbwr_bits_min))
			++gifbwr_bits_min; // out of room, extend minbits

		GIF_bwrwrite(giflzw_workingCode);
		GIF_addHash(key, giflzw_nextCodeToAssign);
		++giflzw_nextCodeToAssign;

		// Seed the working code with this byte, for the next
		// round
		giflzw_workingCode = pbyte;
		return;
	}

	// This string is in there, so update our working code!
	giflzw_workingCode = hashOutput;
}

//
// GIF_lzw
// polls the hashtable, does writing, etc
//
static void GIF_lzw(void)
{
	while (scrbuf_pos <= scrbuf_writeend)
	{
		GIF_feedByte(gif_input[scrbuf_pos]);
		if (giflzw_nextCodeToAssign >= GIFLZW_MAXCODE)
		{
			GIF_bwrwrite(GIFLZW_TABLECLR);
			GIF_prepareLZW();
		}
		if (++scrbuf_pos == scrbuf_lineend)
		{
			scrbuf_lineend += gif_width;
			scrbuf_linebegin += gif_width;
			scrbuf_pos = scrbuf_linebegin;
		}
		// Just a bit of overflow prevention
		if (gifbwr_bufsize >= 248)
			break;
	}
	if (scrbuf_pos > scrbuf_writeend)
	{
		// 4.15.14 - I failed to account for the possibility that
		// these two writes could possibly cause minbits increases.
		// Luckily, we have a guarantee that the first byte CANNOT exceed
		// the maximum possible code.  So, we do a minbits check here...
		if (giflzw_nextCodeToAssign++ > (1 << gifbwr_bits_min))
			++gifbwr_bits_min; // out of room, extend minbits
		GIF_bwrwrite(giflzw_workingCode);

		// And luckily once more, if the data marker somehow IS at
		// MAXCODE it doesn't matter, because it still marks the
		// end of the stream and thus no extending will happen!
		// But still, we need to check minbits again...
		if (giflzw_nextCodeToAssign++ > (1 << gifbwr_bits_min))
			++gifbwr_bits_min; // out of room, extend minbits
		GIF_bwrwrite(GIFLZW_DATAEND);

		// Okay, the flush is safe at least.
		GIF_bwrflush();
		gif_writeover = 1;
	}
}



// GIF HEADer (okay yeah)
// ---
const UINT8 gifhead_base[6] = {0x47,0x49,0x46,0x38,0x39,0x61}; // GIF89a
const UINT8 gifhead_nsid[19] = {0x21,0xFF,0x0B, // extension block + size
	0x4E,0x45,0x54,0x53,0x43,0x41,0x50,0x45,0x32,0x2E,0x30, // NETSCAPE2.0
	0x03,0x01,0xFF,0xFF,0x00}; // sub-block, repetitions


//
// GIF_getpalette
// determine the palette for the current frame.
//
static const RGBA_t *GIF_getpalette(size_t palnum)
{
	// In hardware mode, always returns the local palette
#ifdef HWRENDER
	if (rendermode == render_opengl)
		return pLocalPalette;
	else
#endif
		return (gif_colorprofile ? &pLocalPalette[palnum*256] : &pMasterPalette[palnum*256]);
}

//
// GIF_palwrite
// writes the gif palette.
// used both for the header and local color tables.
//
static UINT8 *GIF_palwrite(UINT8 *p, const RGBA_t *pal)
{
	INT32 i;
	for (i = 0; i < 256; i++)
	{
		WRITEUINT8(p, pal[i].s.red);
		WRITEUINT8(p, pal[i].s.green);
		WRITEUINT8(p, pal[i].s.blue);
	}
	return p;
}

//
// GIF_headwrite
// writes the gif header to the currently open output file.
//
static boolean GIF_headwrite(void)
{
	UINT8 header[800];
	UINT8 *p = header;
	WRITEMEM(p, gifhead_base, sizeof gifhead_base);
	WRITEUINT16(p, gif_width);
	WRITEUINT16(p, gif_height);
	WRITEUINT8(p, 0xF7);
	WRITEUINT8(p, 0);
	WRITEUINT8(p, 0);
	p = GIF_palwrite(p, gif_headerpalette);
	WRITEMEM(p, gifhead_nsid, sizeof gifhead_nsid);
	return fwrite(header, 1, p - header, gif_out) == (size_t)(p - header);
}
// GIF FRAME (surprise!)
// ---
const UINT8 gifframe_gchead[4] = {0x21,0xF9,0x04,0x04}; // GCE, bytes, packed byte (no trans = 0 | no input = 0 | don't remove = 4)

static UINT8 *gifframe_data;

#ifdef HWRENDER
static colorlookup_t gif_colorlookup;
#endif

static boolean GIF_framewrite(gif_capture_t *capture)
{
	const size_t pixels = (size_t)gif_width * gif_height;
#ifdef HWRENDER
	if (capture->rgb)
	{

		InitColorLUT(&gif_colorlookup, capture->palette, false);
		for (INT32 y = 0; y < gif_height; y++)
		{
			const UINT8 *rgb = capture->pixels + (size_t)y * gif_step * gif_source_width * 3;
			UINT8 *out = gif_pixels + (size_t)y * gif_width;
			for (INT32 x = 0; x < gif_width; x++, rgb += gif_step * 3)
				out[x] = GetColorLUT(&gif_colorlookup, rgb[0], rgb[1], rgb[2]);
		}
	}
	else
#endif
		memcpy(gif_pixels, capture->pixels, pixels);

	INT32 blitx = 0, blity = 0, blitw = gif_width, blith = gif_height;
	if (gif_optimize && gif_frames
		&& !memcmp(gif_previouspalette, capture->palette, sizeof gif_previouspalette))
		GIF_optimizeregion(gif_pixels, gif_previous, &blitx, &blity, &blitw, &blith);

	UINT8 *p = gifframe_data;
	WRITEMEM(p, gifframe_gchead, sizeof gifframe_gchead);
	WRITEUINT16(p, capture->delay);
	WRITEUINT8(p, 0);
	WRITEUINT8(p, 0);
	WRITEUINT8(p, 0x2C);
	WRITEUINT16(p, blitx);
	WRITEUINT16(p, blity);
	WRITEUINT16(p, blitw);
	WRITEUINT16(p, blith);

	if (gif_localcolortable && memcmp(gif_headerpalette, capture->palette, sizeof gif_headerpalette))
	{
		WRITEUINT8(p, 0x87);
		p = GIF_palwrite(p, capture->palette);
	}
	else
		WRITEUINT8(p, 0);

	gif_input = gif_pixels;
	scrbuf_pos = scrbuf_linebegin = (size_t)blity * gif_width + blitx;
	scrbuf_lineend = scrbuf_linebegin + blitw;
	scrbuf_writeend = scrbuf_pos + (size_t)(blith - 1) * gif_width + blitw - 1;
	gifbwr_cur = gifbwr_buf;
	GIF_prepareLZW();
	giflzw_workingCode = UINT16_MAX;
	WRITEUINT8(p, gifbwr_bits_min - 1);
	GIF_bwrwrite(GIFLZW_TABLECLR);

	gif_writeover = 0;
	while (!gif_writeover)
	{
		GIF_lzw();
		WRITEUINT8(p, gifbwr_bufsize);
		WRITEMEM(p, gifbwr_buf, gifbwr_bufsize);
		gifbwr_bufsize = 0;
		gifbwr_cur = gifbwr_buf;
	}
	WRITEUINT8(p, 0);

	if (fwrite(gifframe_data, 1, p - gifframe_data, gif_out) != (size_t)(p - gifframe_data))
		return false;

	UINT8 *next = gif_previous;
	gif_previous = gif_pixels;
	gif_pixels = next;
	memcpy(gif_previouspalette, capture->palette, sizeof gif_previouspalette);
	gif_frames++;
	return true;
}

#ifdef GIF_BACKGROUND
static int SDLCALL GIF_writer(void *unused)
{
	(void)unused;
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);

	SDL_LockMutex(gif_mutex);
	while (!gif_finishing || gif_count)
	{
		if (!gif_count)
		{
			SDL_CondWait(gif_ready, gif_mutex);
			continue;
		}

		SDL_UnlockMutex(gif_mutex);
		if (!gif_write_failed)
			gif_write_failed = !GIF_framewrite(&gif_queue[gif_head]);
		SDL_LockMutex(gif_mutex);
		gif_head = (gif_head + 1) % GIF_QUEUE_SIZE;
		gif_count--;
	}
	SDL_UnlockMutex(gif_mutex);
	return 0;
}
#endif

static void GIF_freeBuffers(void)
{
#ifdef GIF_BACKGROUND
	if (gif_ready)
		SDL_DestroyCond(gif_ready);
	if (gif_mutex)
		SDL_DestroyMutex(gif_mutex);
	gif_ready = NULL;
	gif_mutex = NULL;
#endif
	free(gifframe_data);
	free(gif_pixels);
	free(gif_previous);
	for (unsigned i = 0; i < GIF_QUEUE_SIZE; i++)
	{
		free(gif_queue[i].pixels);
		gif_queue[i].pixels = NULL;
		gif_queue[i].capacity = 0;
	}
	gifframe_data = gif_pixels = gif_previous = NULL;
}

INT32 GIF_open(const char *filename)
{
	gif_optimize = cv_gif_optimize.value != 0;
	gif_dynamicdelay = (UINT8)cv_gif_dynamicdelay.value;
	gif_localcolortable = cv_gif_localcolortable.value != 0;
	gif_colorprofile = cv_screenshot_colorprofile.value != 0;
	gif_source_width = vid.width;
	gif_source_height = vid.height;
	gif_step = cv_gif_downscale.value ? max(1, vid.dup) : 1;
	gif_width = vid.width / gif_step;
	gif_height = vid.height / gif_step;

	const size_t pixels = (size_t)gif_width * gif_height;
	const size_t slot_size = rendermode == render_opengl
		? (size_t)gif_source_width * gif_source_height * 3 : pixels;
	gif_pixels = malloc(pixels);
	gif_previous = malloc(pixels);

	gifframe_data = malloc(pixels * 2 + 2048);
	if (!gif_pixels || !gif_previous || !gifframe_data)
		goto fail;
	for (unsigned i = 0; i < GIF_QUEUE_SIZE; i++)
	{
		gif_queue[i].pixels = malloc(slot_size);
		if (!gif_queue[i].pixels)
			goto fail;
		gif_queue[i].capacity = slot_size;
	}
#ifdef GIF_BACKGROUND
	gif_mutex = SDL_CreateMutex();
	gif_ready = SDL_CreateCond();
	if (!gif_mutex || !gif_ready)
		goto fail;
	gif_head = gif_tail = gif_count = 0;
	gif_finishing = false;
#endif

	gif_out = fopen(filename, "wb");
	if (!gif_out)
		goto fail;
	memcpy(gif_headerpalette, GIF_getpalette(0), sizeof gif_headerpalette);
	if (!GIF_headwrite())
		goto fail;

	gif_frames = gif_dropped = gif_captures = 0;
	gif_delayremainder = gif_nextcapturetime = gif_lastcapturetime = 0;
	gif_write_failed = false;
#ifdef GIF_BACKGROUND
	gif_thread = SDL_CreateThread(GIF_writer, "GIF encoder", NULL);
	if (!gif_thread)
		goto fail;
#endif
	return 1;

fail:
	if (gif_out)
	{
		fclose(gif_out);
		gif_out = NULL;
		remove(filename);
	}
	GIF_freeBuffers();
	return 0;
}

void GIF_frame(void)
{
	const precise_t now = I_GetPreciseTime();
	const UINT64 precision = I_GetPrecisePrecision();
	const UINT32 fps = max(cv_gif_maxfps.value, 1);
	const UINT64 interval = max(1, precision / fps);
	if (!gif_out || now < gif_nextcapturetime)
		return;
	if (vid.width != gif_source_width || vid.height != gif_source_height)
	{
		CONS_Printf("The recording was saved before the video mode changed.\n");
		M_StopMovie();
		return;
	}
	if (!gif_nextcapturetime)
		gif_nextcapturetime = now;
	gif_nextcapturetime += ((now - gif_nextcapturetime) / interval + 1) * interval;

	gif_capture_t *capture = &gif_queue[0];
#ifdef GIF_BACKGROUND
	SDL_LockMutex(gif_mutex);
	if (gif_count == GIF_QUEUE_SIZE)
	{
		SDL_UnlockMutex(gif_mutex);
		gif_dropped++;
		return;
	}
	capture = &gif_queue[gif_tail];
	SDL_UnlockMutex(gif_mutex);
#endif


	capture->rgb = rendermode == render_opengl;
#ifdef HWRENDER
	if (capture->rgb)
	{
		const size_t size = (size_t)gif_source_width * gif_source_height * 3;
		if (capture->capacity < size)
		{
			UINT8 *pixels = realloc(capture->pixels, size);
			if (!pixels)
			{
				CONS_Alert(CONS_ERROR, "Couldn't allocate the OpenGL GIF frame buffer. Saving the recorded frames.\n");
				M_StopMovie();
				return;
			}
			capture->pixels = pixels;
			capture->capacity = size;
		}
		if (!HWR_ReadScreenshot(capture->pixels))
			return;
	}
	else
#endif
	{
		for (INT32 y = 0; y < gif_height; y++)
		{
			const UINT8 *source = screens[0] + (size_t)y * gif_step * vid.rowbytes;
			UINT8 *target = capture->pixels + (size_t)y * gif_width;
			if (gif_step == 1)
				memcpy(target, source, gif_width);
			else
				for (INT32 x = 0; x < gif_width; x++)
					target[x] = source[x * gif_step];
		}
	}
	const RGBA_t *palette = gif_localcolortable ? GIF_getpalette(max(st_palette, 0)) : gif_headerpalette;
	memcpy(capture->palette, palette, sizeof capture->palette);

	if (gif_dynamicdelay && !singletics && precision && gif_captures)
	{
		const UINT64 scaled = (now - gif_lastcapturetime) * 100 + gif_delayremainder;
		capture->delay = (UINT16)min(UINT16_MAX, max(1ULL, scaled / precision));
		gif_delayremainder = scaled % precision;
	}
	else
	{
		const UINT64 scaled = (UINT64)gif_captures * 100;
		capture->delay = (UINT16)((scaled + 100) / fps - scaled / fps);
	}
	gif_captures++;
	gif_lastcapturetime = now;
#ifdef GIF_BACKGROUND
	SDL_LockMutex(gif_mutex);
	gif_tail = (gif_tail + 1) % GIF_QUEUE_SIZE;
	gif_count++;
	SDL_CondSignal(gif_ready);
	SDL_UnlockMutex(gif_mutex);
#else
	if (!gif_write_failed)
		gif_write_failed = !GIF_framewrite(capture);
#endif
}

INT32 GIF_close(void)
{
	if (!gif_out)
		return 0;
#ifdef GIF_BACKGROUND
	SDL_LockMutex(gif_mutex);
	gif_finishing = true;
	SDL_CondSignal(gif_ready);
	SDL_UnlockMutex(gif_mutex);
	SDL_WaitThread(gif_thread, NULL);
	gif_thread = NULL;
#endif
	if (fputc(';', gif_out) == EOF)
		gif_write_failed = true;
	if (fclose(gif_out) != 0)
		gif_write_failed = true;
	gif_out = NULL;
	GIF_freeBuffers();

	if (gif_write_failed)
		CONS_Alert(CONS_ERROR, "The GIF recording could not finish writing to disk.\n");
	else
		CONS_Printf("Animated GIF closed; wrote %d frames (%u skipped while the encoder was busy).\n",
			gif_frames, gif_dropped);
	return 1;
}
#endif
