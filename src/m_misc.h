// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------




#ifndef __M_MISC__
#define __M_MISC__

#include "doomtype.h"
#include "tables.h"

#include "d_event.h"  
#include "command.h"

typedef enum {
	MM_OFF = 0,
	MM_APNG,
	MM_GIF,
	MM_SCREENSHOT,
	MM_VIDEO
} moviemode_t;
extern moviemode_t moviemode;

extern consvar_t cv_screenshot_option, cv_screenshot_folder, cv_screenshot_colorprofile;
extern consvar_t cv_moviemode, cv_movie_folder, cv_movie_option, cv_movie_resolution;
extern consvar_t cv_zlib_memory, cv_zlib_level, cv_zlib_strategy, cv_zlib_window_bits;
extern consvar_t cv_zlib_memorya, cv_zlib_levela, cv_zlib_strategya, cv_zlib_window_bitsa;
extern consvar_t cv_apng_delay, cv_apng_downscale;

void M_StartMovie(void);
void M_SaveFrame(void);
void M_StopMovie(void);
void M_GetMovieDiagnostics(char *buffer, size_t buffer_size);


#define CONFIGFILENAME "config.cfg"

INT32 M_MapNumber(char first, char second);

boolean FIL_WriteFile(char const *name, const void *source, size_t length);
size_t FIL_ReadFileTag(char const *name, UINT8 **buffer, INT32 tag);
#define FIL_ReadFile(n, b) FIL_ReadFileTag(n, b, PU_STATIC)

boolean FIL_ConvertTextFileToBinary(const char *textfilename, const char *binfilename);

boolean FIL_FileExists(const char *name);
boolean FIL_WriteFileOK(char const *name);
boolean FIL_ReadFileOK(char const *name);
boolean FIL_FileOK(char const *name);

void FIL_DefaultExtension (char *path, const char *extension);
void FIL_ForceExtension(char *path, const char *extension);
const char *FIL_GetExtension(const char *in);
boolean FIL_CheckExtension(const char *in);

#ifdef HAVE_PNG
boolean M_SavePNG(const char *filename, void *data, int width, int height, const UINT8 *palette);
#endif

extern boolean takescreenshot;
void M_ScreenShot(void);
void M_DoScreenShot(void);
void M_PollAsyncScreenshots(void);
boolean M_ScreenshotResponder(event_t *ev);

void Command_SaveConfig_f(void);
void Command_LoadConfig_f(void);
void Command_ChangeConfig_f(void);

void M_FirstLoadConfig(void);

void M_SaveConfig(const char *filename);

INT32 axtoi(const char *hexStg);

const char *GetRevisionString(void);


void strcatbf(char *s1, const char *s2, const char *s3);

void M_SetupMemcpy(void);

const char *M_FileError(FILE *handle);

int     M_PathParts      (const char *path);
boolean M_IsPathAbsolute (const char *path);
void    M_MkdirEach      (const char *path, int start, int mode);
void    M_MkdirEachUntil (const char *path, int start, int end, int mode);

 
 
int M_JumpWord (const char *s);

 
 
int M_JumpWordReverse (const char *line, int offset);

  
                                                           
                                                     
  
const char * M_Ftrim (double);


boolean M_IsStringEmpty(const char *s);


boolean M_StringToNumber(const char *input, int *out);


boolean M_StringToDecimal(const char *input, double *out);


FUNCMATH UINT8 M_CountBits(UINT32 num, UINT8 size);


int M_RoundUp(double number);

#include "w_wad.h"
extern char configfile[MAX_WADPATH];

#endif
