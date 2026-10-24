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




#ifdef __GNUC__

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)


#pragma GCC diagnostic ignored "-Wclobbered"
#endif

#include <unistd.h>
#endif

#include <errno.h>


#include <ctype.h>

#include "doomdef.h"
#include "g_game.h"
#include "m_misc.h"
#include "m_tokenizer.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_time.h"
#include "i_video.h"
#include "i_sound.h"
#include "d_main.h"
#include "m_argv.h"
#include "i_system.h"
#ifdef HAVE_THREADS
#include "i_threads.h"
#endif
#include "command.h"  

#include "m_anigif.h"


#include "m_menu.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef HAVE_SDL
#include "sdl/hwsym_sdl.h"
#ifdef __linux__
#ifndef _LARGEFILE64_SOURCE
typedef off_t off64_t;
#endif
#endif
#endif

#if defined(__MINGW32__) && ((__GNUC__ > 7) || (__GNUC__ == 6 && __GNUC_MINOR__ >= 3)) && (__GNUC__ < 8)
#define PRIdS "u"
#elif defined(_WIN32) && !defined(__MINGW64__)
#define PRIdS "Iu"
#else
#define PRIdS "zu"
#endif

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "zlib.h"
 #include "png.h"
 #if (PNG_LIBPNG_VER_MAJOR > 1) || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4)
  #define NO_PNG_DEBUG  
 #endif
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG  
  #if defined (PNG_WRITE_APNG_SUPPORTED)  
    #include "apng.h"
    #define USE_APNG
  #endif

 #endif
#endif

static CV_PossibleValue_t screenshot_cons_t[] = {{0, "Default"}, {1, "HOME"}, {2, "SRB2"}, {3, "CUSTOM"}, {0, NULL}};
consvar_t cv_screenshot_option = CVAR_INIT ("screenshot_option", "Default", CV_SAVE|CV_CALL, screenshot_cons_t, Screenshot_option_Onchange);
consvar_t cv_screenshot_folder = CVAR_INIT ("screenshot_folder", "", CV_SAVE, NULL, NULL);

consvar_t cv_screenshot_colorprofile = CVAR_INIT ("screenshot_colorprofile", "Yes", CV_SAVE, CV_YesNo, NULL);

static CV_PossibleValue_t moviemode_cons_t[] = {{MM_VIDEO, "Video + Sound"}, {MM_GIF, "GIF"}, {MM_APNG, "aPNG"}, {MM_SCREENSHOT, "Screenshots"}, {0, NULL}};
consvar_t cv_moviemode = CVAR_INIT ("moviemode_mode", "GIF", CV_SAVE|CV_CALL, moviemode_cons_t, Moviemode_mode_Onchange);

consvar_t cv_movie_option = CVAR_INIT ("movie_option", "Default", CV_SAVE|CV_CALL, screenshot_cons_t, Moviemode_option_Onchange);
consvar_t cv_movie_folder = CVAR_INIT ("movie_folder", "", CV_SAVE, NULL, NULL);
static CV_PossibleValue_t movie_resolution_cons_t[] = {
	{0, "Native"}, {360, "360p"}, {540, "540p"}, {720, "720p"},
	{1080, "1080p"}, {1440, "1440p"}, {0, NULL}};
consvar_t cv_movie_resolution = CVAR_INIT ("movie_resolution", "720p", CV_SAVE,
	movie_resolution_cons_t, NULL);

static CV_PossibleValue_t zlib_mem_level_t[] = {
	{1, "(Min Memory) 1"},
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"},
	{8, "(Optimal) 8"},  
	{9, "(Max Memory) 9"}, {0, NULL}};

static CV_PossibleValue_t zlib_level_t[] = {
	{0, "No Compression"},   
	{1, "(Fastest) 1"},  
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"},
	{6, "(Optimal) 6"},  
	{7, "7"}, {8, "8"},
	{9, "(Maximum) 9"},  
	{0, NULL}};

static CV_PossibleValue_t zlib_strategy_t[] = {
	{0, "Normal"},  
	{1, "Filtered"},  
	{2, "Huffman Only"},  
	{3, "RLE"},  
	{4, "Fixed"},  
	{0, NULL}};

static CV_PossibleValue_t zlib_window_bits_t[] = {
#ifdef WBITS_8_OK
	{8, "256"},
#endif
	{9, "512"}, {10, "1k"}, {11, "2k"}, {12, "4k"}, {13, "8k"},
	{14, "16k"}, {15, "32k"},
	{0, NULL}};

static CV_PossibleValue_t apng_delay_t[] = {
	{1, "1x"},
	{2, "1/2x"},
	{3, "1/3x"},
	{4, "1/4x"},
	{0, NULL}};



consvar_t cv_zlib_memory = CVAR_INIT ("png_memory_level", "7", CV_SAVE, zlib_mem_level_t, NULL);
consvar_t cv_zlib_level = CVAR_INIT ("png_compress_level", "(Optimal) 6", CV_SAVE, zlib_level_t, NULL);
consvar_t cv_zlib_strategy = CVAR_INIT ("png_strategy", "Normal", CV_SAVE, zlib_strategy_t, NULL);
consvar_t cv_zlib_window_bits = CVAR_INIT ("png_window_size", "32k", CV_SAVE, zlib_window_bits_t, NULL);

consvar_t cv_zlib_memorya = CVAR_INIT ("apng_memory_level", "(Max Memory) 9", CV_SAVE, zlib_mem_level_t, NULL);
consvar_t cv_zlib_levela = CVAR_INIT ("apng_compress_level", "4", CV_SAVE, zlib_level_t, NULL);
consvar_t cv_zlib_strategya = CVAR_INIT ("apng_strategy", "RLE", CV_SAVE, zlib_strategy_t, NULL);
consvar_t cv_zlib_window_bitsa = CVAR_INIT ("apng_window_size", "32k", CV_SAVE, zlib_window_bits_t, NULL);
consvar_t cv_apng_delay = CVAR_INIT ("apng_speed", "1x", CV_SAVE, apng_delay_t, NULL);
consvar_t cv_apng_downscale = CVAR_INIT ("apng_downscale", "On", CV_SAVE, CV_OnOff, NULL);

#ifdef USE_APNG
static boolean apng_downscale = false;  
#endif

boolean takescreenshot = false;  

moviemode_t moviemode = MM_OFF;
static precise_t movie_prevcapturetime = 0;
static boolean movie_video_recording = false;
static UINT16 movie_video_framedelay = 1;
static char movie_video_output[MAX_WADPATH];
static char movie_video_frames[MAX_WADPATH];
static char movie_video_audio[MAX_WADPATH];
#ifdef _WIN32
static char movie_ffmpeg_path[MAX_PATH];
static HANDLE movie_ffmpeg_process = NULL;
static HANDLE movie_ffmpeg_input = NULL;
static boolean movie_ffmpeg_recording = false;
static boolean movie_ffmpeg_gif = false;
static boolean movie_ffmpeg_failed = false;
#define MOVIE_FRAME_QUEUE_SIZE 6
typedef struct
{
	UINT8 *pixels;
	UINT32 repeats;
} movie_frame_slot_t;
static movie_frame_slot_t movie_frame_queue[MOVIE_FRAME_QUEUE_SIZE];
static CRITICAL_SECTION movie_frame_queue_lock;
static HANDLE movie_frame_queue_event = NULL;
static HANDLE movie_frame_writer_thread = NULL;
static volatile boolean movie_frame_writer_stop = false;
static volatile boolean movie_frame_writer_failed = false;
static UINT8 movie_frame_queue_head = 0;
static UINT8 movie_frame_queue_tail = 0;
static UINT8 movie_frame_queue_count = 0;
static UINT64 movie_frames_scheduled = 0;
static UINT32 movie_capture_skips = 0;
static UINT32 movie_source_frames = 0;
static UINT64 movie_repeated_frames = 0;
static precise_t movie_recording_start = 0;
static UINT8 *movie_rgb_frame = NULL;
static size_t movie_rgb_frame_size = 0;
static INT32 movie_capture_width = 0;
static INT32 movie_capture_height = 0;
static INT32 movie_capture_fps = 30;
#endif

                                                                             
             
   
                                                
                                                 
                                                                           
                      
    
INT32 M_MapNumber(char first, char second)
{
	if (isdigit(first))
	{
		if (isdigit(second))
			return ((INT32)first - '0') * 10 + ((INT32)second - '0');
		return 0;
	}

	if (!isalpha(first))
		return 0;
	if (!isalnum(second))
		return 0;

	return 100 + ((INT32)tolower(first) - 'a') * 36 + (isdigit(second) ? ((INT32)second - '0') :
		((INT32)tolower(second) - 'a') + 10);
}






#if 0
int access(const char *path, int amode)
{
	int accesshandle = -1;
	FILE *handle = NULL;
	if (amode == 6)  
		handle = fopen(path, "r+");
	else if (amode == 4)  
		handle = fopen(path, "r");
	else if (amode == 2)  
		handle = fopen(path, "a+");
	else if (amode == 0)  
		handle = fopen(path, "rb");
	if (handle)
	{
		accesshandle = 0;
		fclose(handle);
	}
	return accesshandle;
}
#endif





#ifndef O_BINARY
#define O_BINARY 0
#endif

                      
   
                                            
                                                
                                          
                                              
    
boolean FIL_WriteFile(char const *name, const void *source, size_t length)
{
	FILE *handle = NULL;
	size_t count;


		handle = fopen(name, "w+b");

	if (!handle)
		return false;

	count = fwrite(source, 1, length, handle);
	fclose(handle);

	if (count < length)
		return false;

	return true;
}

                                                      
   
                                   
                                                                              
                                                                     
                                                                              
                          
    
size_t FIL_ReadFileTag(char const *name, UINT8 **buffer, INT32 tag)
{
	FILE *handle = NULL;
	size_t count, length;
	UINT8 *buf;


		handle = fopenfile(name, "rb");

	if (!handle)
		return 0;

	fseek(handle,0,SEEK_END);
	length = ftell(handle);
	fseek(handle,0,SEEK_SET);

	buf = Z_Malloc(length + 1, tag, NULL);
	count = fread(buf, 1, length, handle);
	fclose(handle);

	if (count < length)
	{
		Z_Free(buf);
		return 0;
	}


	buf[length] = 0;

	*buffer = buf;
	return length;
}

                                                                             
   
                                                   
                                                       
    
boolean FIL_ConvertTextFileToBinary(const char *textfilename, const char *binfilename)
{
	FILE *textfile;
	FILE *binfile;
	UINT8 buffer[1024];
	size_t count;
	boolean success;

	textfile = fopen(textfilename, "r");
	if (!textfile)
		return false;

	binfile = fopen(binfilename, "wb");
	if (!binfile)
	{
		fclose(textfile);
		return false;
	}

	do
	{
		count = fread(buffer, 1, sizeof(buffer), textfile);
		fwrite(buffer, 1, count, binfile);
	} while (count);

	success = !(ferror(textfile) || ferror(binfile));

	fclose(textfile);
	fclose(binfile);

	return success;
}

                                
   
                                    
                                                     
    
boolean FIL_FileExists(char const *name)
{
	return access(name,0)+1;  
}


                                     
   
                                    
                                                         
    
boolean FIL_WriteFileOK(char const *name)
{
	return access(name,2)+1;  
}


                                    
   
                                    
                                                        
    
boolean FIL_ReadFileOK(char const *name)
{
	return access(name,4)+1;  
}

                                          
   
                                    
                                                                
    
boolean FIL_FileOK(char const *name)
{
	return access(name,6)+1;  
}


                                                                             
           
   
                                       
                                                               
    
void FIL_DefaultExtension(char *path, const char *extension)
{
	char *src;


	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;  
		src--;
	}

	strcat(path, extension);
}

void FIL_ForceExtension(char *path, const char *extension)
{
	char *src;


	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
		{
			*src = '\0';
			break;  
		}
		src--;
	}

	strcat(path, extension);
}

                                                              
                                              
   
                              
                                                                 
    
const char *FIL_GetExtension(const char *in)
{
	const char *out = NULL;
	while (*in++) 
		if (*in == '.')
			out = in;
	return out;
}

                                            
                                   
   
                              
                                                           
    
boolean FIL_CheckExtension(const char *in)
{
	while (*in++)
		if (*in == '.')
			return true;

	return false;
}









char configfile[MAX_WADPATH];




static boolean gameconfig_loaded = false;  

                                                           
   
                            
    
void Command_SaveConfig_f(void)
{
	char tmpstr[MAX_WADPATH];

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("saveconfig <filename[.cfg]> [-silent] : save config to a file\n"));
		return;
	}

	strcpy(tmpstr, COM_Argv(1));
	FIL_ForceExtension(tmpstr, ".cfg");

	M_SaveConfig(tmpstr);
	if (stricmp(COM_Argv(2), "-silent"))
		CONS_Printf(M_GetText("config saved as %s\n"), configfile);
}

                                                         
   
                                                    
    
void Command_LoadConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("loadconfig <filename[.cfg]> : load config from a file\n"));
		return;
	}

	strcpy(configfile, COM_Argv(1));
	FIL_ForceExtension(configfile, ".cfg");


	G_ClearAllControlKeys();
	G_CopyControls(gamecontrol, gamecontroldefault[gcs_fps], NULL, 0);
	G_CopyControls(gamecontrolbis, gamecontrolbisdefault[gcs_fps], NULL, 0);


	CV_ToggleExecVersion(true);
	COM_BufInsertText(va("%s \"%s\"\n", cv_execversion.name, cv_execversion.defaultvalue));
	CV_InitFilterVar();


	COM_BufInsertText(va("exec \"%s\"\n", configfile));


	COM_BufInsertText(va("%s \"%d\"\n", cv_execversion.name, EXECVERSION));
	CV_ToggleExecVersion(false);
}

                                                      
   
                                                  
    
void Command_ChangeConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("changeconfig <filename[.cfg]> : save current config and load another\n"));
		return;
	}

	COM_BufAddText(va("saveconfig \"%s\"\n", configfile));
	COM_BufAddText(va("loadconfig \"%s\"\n", COM_Argv(1)));
}

                                  
   
                            
    
void M_FirstLoadConfig(void)
{



	if (M_CheckParm("-config") && M_IsNextParm())
	{
		strcpy(configfile, M_GetNextParm());
		CONS_Printf(M_GetText("config file: %s\n"), configfile);
	}


	G_DefineDefaultControls();
	G_CopyControls(gamecontrol, gamecontroldefault[gcs_fps], NULL, 0);
	G_CopyControls(gamecontrolbis, gamecontrolbisdefault[gcs_fps], NULL, 0);


	CV_RegisterVar(&cv_execversion);



	CV_ToggleExecVersion(true);
	COM_BufInsertText(va("%s \"%s\"\n", cv_execversion.name, cv_execversion.defaultvalue));
	CV_InitFilterVar();


	COM_BufInsertText(va("exec \"%s\"\n", configfile));



	COM_BufInsertText(va("%s \"%d\"\n", cv_execversion.name, EXECVERSION));
	CV_ToggleExecVersion(false);



	gameconfig_loaded = true;


	if (!dedicated)
	{
		COM_BufAddText (va("%s \"%s\"\n",cv_skin.name,cv_defaultskin.string));
		COM_BufAddText (va("%s \"%s\"\n",cv_playercolor.name,cv_defaultplayercolor.string));
		COM_BufAddText (va("%s \"%s\"\n",cv_skin2.name,cv_defaultskin2.string));
		COM_BufAddText (va("%s \"%s\"\n",cv_playercolor2.name,cv_defaultplayercolor2.string));
	}
}

                                 
   
                            
    
void M_SaveConfig(const char *filename)
{
	FILE *f;
	char *filepath;


	if (!gameconfig_loaded)
		return;


	if (filename)
	{
		if (!strstr(filename, ".cfg"))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Config filename must be .cfg\n"));
			return;
		}



		if (!strstr(filename, srb2home))
			filepath = va(pandf,srb2home, filename);
		else
			filepath = Z_StrDup(filename);

		f = fopen(filepath, "w");

		if (f)
			strcpy(configfile, filepath);
		else
		{
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't save game config file %s\n"), filepath);
			return;
		}
	}
	else
	{
		if (!strstr(configfile, ".cfg"))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Config filename must be .cfg\n"));
			return;
		}

		f = fopen(configfile, "w");
		if (!f)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't save game config file %s\n"), configfile);
			return;
		}
	}


	fprintf(f, "// SRB2 configuration file.\n");



	fprintf(f, "%s \"%d\"\n", cv_execversion.name, EXECVERSION);



	if (tutorialmode && tutorialgcs)
	{
		CV_SetValue(&cv_usemouse, tutorialusemouse);
		CV_SetValue(&cv_alwaysfreelook, tutorialfreelook);
		CV_SetValue(&cv_mousemove, tutorialmousemove);
		CV_SetValue(&cv_analog[0], tutorialanalog);
		CV_SaveVariables(f);
		CV_Set(&cv_usemouse, cv_usemouse.defaultvalue);
		CV_Set(&cv_alwaysfreelook, cv_alwaysfreelook.defaultvalue);
		CV_Set(&cv_mousemove, cv_mousemove.defaultvalue);
		CV_Set(&cv_analog[0], cv_analog[0].defaultvalue);
	}
	else
		CV_SaveVariables(f);

	if (!dedicated)
	{
		if (tutorialmode && tutorialgcs)
			G_SaveKeySetting(f, gamecontroldefault[gcs_custom], gamecontrolbis);  
		else
			G_SaveKeySetting(f, gamecontrol, gamecontrolbis);
	}

	fclose(f);
}




static UINT8 screenshot_palette[768];
static void M_CreateScreenShotPalette(void)
{
	size_t i, j;
	for (i = 0, j = 0; i < 768; i += 3, j++)
	{
		RGBA_t locpal = ((cv_screenshot_colorprofile.value)
		? pLocalPalette[(max(st_palette,0)*256)+j]
		: pMasterPalette[(max(st_palette,0)*256)+j]);
		screenshot_palette[i] = locpal.s.red;
		screenshot_palette[i+1] = locpal.s.green;
		screenshot_palette[i+2] = locpal.s.blue;
	}
}

#if NUMSCREENS > 2
static const char *Newsnapshotfile(const char *pathname, const char *ext)
{
	static char freename[13] = "srb2XXXX.ext";
	int i = 5000;  
	int add = i;  
	int result;  


	strcpy(freename+9,ext);

	for (;;)
	{
		freename[4] = (char)('0' + (char)(i/1000));
		freename[5] = (char)('0' + (char)((i/100)%10));
		freename[6] = (char)('0' + (char)((i/10)%10));
		freename[7] = (char)('0' + (char)(i%10));

		if (FIL_WriteFileOK(va(pandf,pathname,freename)))  
			result = 1;  
		else  
		{
			if (!i)
				break;  

			freename[4] = (char)('0' + (char)((i-1)/1000));
			freename[5] = (char)('0' + (char)(((i-1)/100)%10));
			freename[6] = (char)('0' + (char)(((i-1)/10)%10));
			freename[7] = (char)('0' + (char)((i-1)%10));
			if (!FIL_WriteFileOK(va(pandf,pathname,freename)))  
				result = -1;  
			else
				break;  
		}

		add /= 2;

		if (!add)  
			add = 1;

		i += add * result;

		if (i < 0 || i > 9999)
			return NULL;
	}

	freename[4] = (char)('0' + (char)(i/1000));
	freename[5] = (char)('0' + (char)((i/100)%10));
	freename[6] = (char)('0' + (char)((i/10)%10));
	freename[7] = (char)('0' + (char)(i%10));

	return freename;
}
#endif

#ifdef HAVE_PNG
FUNCNORETURN static void PNG_error(png_structp PNG, png_const_charp pngtext)
{

	I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static void M_PNGhdr(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 width, PNG_CONST png_uint_32 height, PNG_CONST png_byte *palette)
{
	const png_byte png_interlace = PNG_INTERLACE_NONE;  
	if (palette)
	{
		png_colorp png_PLTE = png_malloc(png_ptr, sizeof(png_color)*256);  
		const png_byte *pal = palette;
		png_uint_16 i;
		for (i = 0; i < 256; i++)
		{
			png_PLTE[i].red   = *pal; pal++;
			png_PLTE[i].green = *pal; pal++;
			png_PLTE[i].blue  = *pal; pal++;
		}
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_PLTE(png_ptr, png_info_ptr, png_PLTE, 256);
		png_free(png_ptr, (png_voidp)png_PLTE);  
		png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
		png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
	}
	else
	{
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_compression_strategy(png_ptr, Z_FILTERED);
	}
}

#define SRB2PNGTXT 11  
typedef struct
{
	char text[SRB2PNGTXT][96];
} pngtextsnapshot_t;

#ifdef PNG_TEXT_SUPPORTED
static char pngtextkeys[SRB2PNGTXT][12] = {
	"Title", "Description", "Playername", "Mapnum", "Mapname",
	"Location", "Interface", "Render Mode", "Revision", "Build Date", "Build Time"};

static void M_CapturePNGText(pngtextsnapshot_t *snapshot, PNG_CONST png_byte movie)
{
	snprintf(snapshot->text[0], sizeof snapshot->text[0], "Sonic Robo Blast 2 %s", VERSIONSTRING);
	snprintf(snapshot->text[1], sizeof snapshot->text[1], "%s", movie ? "SRB2 Movie" : "SRB2 Screenshot");
	snprintf(snapshot->text[2], sizeof snapshot->text[2], "%s", cv_playername.zstring);

	if (gamestate == GS_LEVEL)
		snprintf(snapshot->text[3], sizeof snapshot->text[3], "%s", G_BuildMapName(gamemap));
	else
		snprintf(snapshot->text[3], sizeof snapshot->text[3], "Unknown");

	if (gamestate == GS_LEVEL && mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->lvlttl[0] != '\0')
		snprintf(snapshot->text[4], sizeof snapshot->text[4], "%s%s%s",
			mapheaderinfo[gamemap-1]->lvlttl,
			(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE) ? "" : " Zone",
			(mapheaderinfo[gamemap-1]->actnum > 0) ? va(" %d",mapheaderinfo[gamemap-1]->actnum) : "");
	else
		snprintf(snapshot->text[4], sizeof snapshot->text[4], "Unknown");

	if (gamestate == GS_LEVEL && players[displayplayer].mo)
		snprintf(snapshot->text[5], sizeof snapshot->text[5], "X:%d Y:%d Z:%d A:%d",
			players[displayplayer].mo->x>>FRACBITS,
			players[displayplayer].mo->y>>FRACBITS,
			players[displayplayer].mo->z>>FRACBITS,
			FixedInt(AngleFixed(players[displayplayer].mo->angle)));
	else
		snprintf(snapshot->text[5], sizeof snapshot->text[5], "Unknown");

	snprintf(snapshot->text[6], sizeof snapshot->text[6], "%s",
#ifdef HAVE_SDL
		"SDL");
#elif defined (_WINDOWS)
		"DirectX");
#else
		"Unknown");
#endif

	switch (rendermode)
	{
		case render_soft:
			snprintf(snapshot->text[7], sizeof snapshot->text[7], "Software");
			break;
		case render_opengl:
			snprintf(snapshot->text[7], sizeof snapshot->text[7], "OpenGL");
			break;
		default:  
			snprintf(snapshot->text[7], sizeof snapshot->text[7], "None");
			break;
	}
	snprintf(snapshot->text[8], sizeof snapshot->text[8], "%s", comprevision);
	snprintf(snapshot->text[9], sizeof snapshot->text[9], "%s", compdate);
	snprintf(snapshot->text[10], sizeof snapshot->text[10], "%s", comptime);
}

static void M_SetPNGText(png_structp png_ptr, png_infop png_info_ptr, pngtextsnapshot_t *snapshot)
{
	png_text png_infotext[SRB2PNGTXT];
	size_t i;
	memset(png_infotext,0x00,sizeof (png_infotext));

	for (i = 0; i < SRB2PNGTXT; i++)
	{
		png_infotext[i].key = pngtextkeys[i];
		png_infotext[i].text = snapshot->text[i];
	}

	png_set_text(png_ptr, png_info_ptr, png_infotext, SRB2PNGTXT);
}
#else
static void M_CapturePNGText(pngtextsnapshot_t *snapshot, PNG_CONST png_byte movie)
{
	(void)movie;
	memset(snapshot, 0, sizeof *snapshot);
}
#endif

static void M_PNGText(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_byte movie)
{
#ifdef PNG_TEXT_SUPPORTED
	pngtextsnapshot_t snapshot;
	M_CapturePNGText(&snapshot, movie);
	M_SetPNGText(png_ptr, png_info_ptr, &snapshot);
#else
	(void)png_ptr;
	(void)png_info_ptr;
	(void)movie;
#endif
}

static inline void M_PNGImage(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 height, png_bytep png_buf)
{
	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	png_bytepp row_pointers = png_malloc(png_ptr, height* sizeof (png_bytep));
	png_uint_32 y;
	for (y = 0; y < height; y++)
	{
		row_pointers[y] = png_buf;
		png_buf += pitch;
	}
	png_write_image(png_ptr, row_pointers);
	png_free(png_ptr, (png_voidp)row_pointers);
}

#ifdef USE_APNG
static png_structp apng_ptr = NULL;
static png_infop   apng_info_ptr = NULL;
static apng_infop  apng_ainfo_ptr = NULL;
static png_FILE_p  apng_FILE = NULL;
static png_uint_32 apng_frames = 0;
#ifdef PNG_STATIC  
#define aPNG_set_acTL png_set_acTL
#define aPNG_write_frame_head png_write_frame_head
#define aPNG_write_frame_tail png_write_frame_tail
#else  

#ifndef PNG_WRITE_APNG_SUPPORTED  

#ifndef PNG_INFO_acTL
#define PNG_INFO_acTL 0x10000L
#endif
#ifndef PNG_INFO_fcTL
#define PNG_INFO_fcTL 0x20000L
#endif
#ifndef PNG_FIRST_FRAME_HIDDEN
#define PNG_FIRST_FRAME_HIDDEN       0x0001
#endif
#ifndef PNG_DISPOSE_OP_NONE
#define PNG_DISPOSE_OP_NONE        0x00
#endif
#ifndef PNG_DISPOSE_OP_BACKGROUND
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#endif
#ifndef PNG_DISPOSE_OP_PREVIOUS
#define PNG_DISPOSE_OP_PREVIOUS    0x02
#endif
#ifndef PNG_BLEND_OP_SOURCE
#define PNG_BLEND_OP_SOURCE        0x00
#endif
#ifndef PNG_BLEND_OP_OVER
#define PNG_BLEND_OP_OVER          0x01
#endif
#ifndef PNG_HAVE_acTL
#define PNG_HAVE_acTL             0x4000
#endif
#ifndef PNG_HAVE_fcTL
#define PNG_HAVE_fcTL             0x8000L
#endif

#endif
typedef png_uint_32 (*P_png_set_acTL) (png_structp png_ptr,
   png_infop info_ptr, png_uint_32 num_frames, png_uint_32 num_plays);
typedef void (*P_png_write_frame_head) (png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op);

typedef void (*P_png_write_frame_tail) (png_structp png_ptr,
   png_infop info_ptr);
static P_png_set_acTL aPNG_set_acTL = NULL;
static P_png_write_frame_head aPNG_write_frame_head = NULL;
static P_png_write_frame_tail aPNG_write_frame_tail = NULL;
#endif

static inline boolean M_PNGLib(void)
{
#ifdef PNG_STATIC  
	return true;
#else
	static void *pnglib = NULL;
	if (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail)
		return true;
	if (pnglib)
		return false;
#ifdef _WIN32
	pnglib = GetModuleHandleA("libpng.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng12.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng13.dll");
#elif defined (HAVE_SDL)
#ifdef __APPLE__
	pnglib = hwOpen("libpng.dylib");
#else
	pnglib = hwOpen("libpng.so");
#endif
#endif
	if (!pnglib)
		return false;
#ifdef HAVE_SDL
	aPNG_set_acTL = hwSym("png_set_acTL", pnglib);
	aPNG_write_frame_head = hwSym("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = hwSym("png_write_frame_tail", pnglib);
#endif
#ifdef _WIN32
	aPNG_set_acTL = GetProcAddress("png_set_acTL", pnglib);
	aPNG_write_frame_head = GetProcAddress("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = GetProcAddress("png_write_frame_tail", pnglib);
#endif
	return (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail);
#endif
}

static void M_PNGFrame(png_structp png_ptr, png_infop png_info_ptr, png_bytep png_buf)
{
	png_uint_16 downscale = apng_downscale ? vid.dup : 1;

	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	PNG_CONST png_uint_32 width = vid.width / downscale;
	PNG_CONST png_uint_32 height = vid.height / downscale;
	png_bytepp row_pointers = png_malloc(png_ptr, height * sizeof (png_bytep));
	png_uint_32 x, y;
	png_uint_16 framedelay = movie_video_recording
		? movie_video_framedelay : (png_uint_16)cv_apng_delay.value;

	apng_frames++;

	for (y = 0; y < height; y++)
	{
		row_pointers[y] = malloc(pitch * sizeof(png_byte));
		for (x = 0; x < width; x++)
			row_pointers[y][x] = png_buf[x * downscale];
		png_buf += pitch * (downscale * downscale);
	}





	                                                  
                       
    

#ifndef PNG_STATIC
	if (aPNG_write_frame_head)
#endif
		aPNG_write_frame_head(apng_ptr, apng_info_ptr, row_pointers,
			width,      
			height,     
			0,          
			0,          
			framedelay, max(cv_gif_maxfps.value, 1), 
			PNG_DISPOSE_OP_BACKGROUND,  
			PNG_BLEND_OP_SOURCE         
		                     );

	png_write_image(png_ptr, row_pointers);

#ifndef PNG_STATIC
	if (aPNG_write_frame_tail)
#endif
		aPNG_write_frame_tail(apng_ptr, apng_info_ptr);

	png_free(png_ptr, (png_voidp)row_pointers);
}

static void M_PNGfix_acTL(png_structp png_ptr, png_infop png_info_ptr,
		apng_infop png_ainfo_ptr)
{
	apng_set_acTL(png_ptr, png_info_ptr, png_ainfo_ptr, apng_frames, 0);

#ifndef NO_PNG_DEBUG
	png_debug(1, "in png_write_acTL\n");
#endif
}

static boolean M_SetupaPNG(png_const_charp filename, png_bytep pal)
{
	png_uint_16 downscale;

	apng_downscale = (!!cv_apng_downscale.value);

	downscale = apng_downscale ? vid.dup : 1;

	apng_FILE = fopen(filename,"wb+");  
	if (!apng_FILE)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on opening %s for write\n", filename);
		return false;
	}

	apng_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	 PNG_error, PNG_warn);
	if (!apng_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on initialize libpng\n");
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_info_ptr = png_create_info_struct(apng_ptr);
	if (!apng_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on allocate for libpng\n");
		png_destroy_write_struct(&apng_ptr,  NULL);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_ainfo_ptr = apng_create_info_struct(apng_ptr);
	if (!apng_ainfo_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on allocate for apng\n");
		png_destroy_write_struct(&apng_ptr, &apng_info_ptr);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	png_init_io(apng_ptr, apng_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(apng_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif



	png_set_compression_level(apng_ptr, cv_zlib_levela.value);
	png_set_compression_mem_level(apng_ptr, cv_zlib_memorya.value);
	png_set_compression_strategy(apng_ptr, cv_zlib_strategya.value);
	png_set_compression_window_bits(apng_ptr, cv_zlib_window_bitsa.value);

	M_PNGhdr(apng_ptr, apng_info_ptr, vid.width / downscale, vid.height / downscale, pal);

	M_PNGText(apng_ptr, apng_info_ptr, true);

	apng_set_set_acTL_fn(apng_ptr, apng_ainfo_ptr, aPNG_set_acTL);

	apng_set_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr, PNG_UINT_31_MAX, 0);

	apng_write_info(apng_ptr, apng_info_ptr, apng_ainfo_ptr);

	apng_frames = 0;

	return true;
}
#endif
#endif




#if NUMSCREENS > 2
static inline moviemode_t M_StartMovieAPNG(const char *pathname)
{
#ifdef USE_APNG
	UINT8 *palette = NULL;
	const char *freename = NULL;
	boolean ret = false;

	if (!M_PNGLib())
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: libpng not found\n");
		return MM_OFF;
	}

	if (!(freename = Newsnapshotfile(pathname,"png")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (rendermode == render_soft)
	{
		M_CreateScreenShotPalette();
		palette = screenshot_palette;
	}

	ret = M_SetupaPNG(va(pandf,pathname,freename), palette);

	if (!ret)
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_APNG;
#else

	(void)pathname;
	CONS_Alert(CONS_ERROR, "Couldn't create aPNG: this build lacks aPNG support\n");
	return MM_OFF;
#endif
}

static inline moviemode_t M_StartMovieGIF(const char *pathname)
{
#ifdef HAVE_ANIGIF
	const char *freename;

	if (!(freename = Newsnapshotfile(pathname,"gif")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create GIF: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (!GIF_open(va(pandf,pathname,freename)))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create GIF: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_GIF;
#else

	(void)pathname;
	CONS_Alert(CONS_ERROR, "Couldn't create GIF: this build lacks GIF support\n");
	return MM_OFF;
#endif
}

#ifdef _WIN32
typedef struct
{
	HANDLE process;
	char frames[MAX_WADPATH];
	char audio[MAX_WADPATH];
	char output[MAX_WADPATH];
	boolean gif;
	boolean audio_enabled;
	UINT64 timeline_frames;
	UINT32 source_frames;
	UINT64 repeated_frames;
	UINT32 capture_skips;
} movie_encode_job_t;

static boolean M_StartHiddenProcessAndWait(char *commandline)
{
	STARTUPINFOA startup;
	PROCESS_INFORMATION process;
	DWORD exitcode = 1;
	memset(&startup, 0, sizeof startup);
	startup.cb = sizeof startup;
	memset(&process, 0, sizeof process);
	if (!CreateProcessA(movie_ffmpeg_path, commandline, NULL, NULL, FALSE,
		CREATE_NO_WINDOW, NULL, NULL, &startup, &process))
		return false;
	CloseHandle(process.hThread);
	WaitForSingleObject(process.hProcess, INFINITE);
	GetExitCodeProcess(process.hProcess, &exitcode);
	CloseHandle(process.hProcess);
	return exitcode == 0;
}

static void M_WaitForMovieEncode(void *userdata)
{
	movie_encode_job_t *job = (movie_encode_job_t *)userdata;
	DWORD capture_exit = 1;
	boolean finalized = false;
	char commandline[6*MAX_WADPATH];
	if (!job)
		return;
	WaitForSingleObject(job->process, INFINITE);
	GetExitCodeProcess(job->process, &capture_exit);
	CloseHandle(job->process);

	if (capture_exit == 0 && job->gif)
	{
		snprintf(commandline, sizeof commandline,
			"\"%s\" -y -hide_banner -loglevel error -i \"%s\" "
			"-filter_complex \"[0:v]split[a][b];[a]palettegen=stats_mode=diff[p];"
			"[b][p]paletteuse=dither=sierra2_4a:diff_mode=rectangle\" "
			"-loop 0 \"%s\"",
			movie_ffmpeg_path, job->frames, job->output);
		finalized = M_StartHiddenProcessAndWait(commandline);
	}
	else if (capture_exit == 0 && job->audio_enabled)
	{
		snprintf(commandline, sizeof commandline,
			"\"%s\" -y -hide_banner -loglevel error -i \"%s\" -i \"%s\" "
			"-filter_complex \"[1:a]aresample=async=1000:first_pts=0[a]\" "
			"-map 0:v:0 -map \"[a]\" -c:v copy -c:a aac -b:a 192k "
			"-shortest -movflags +faststart \"%s\"",
			movie_ffmpeg_path, job->frames, job->audio, job->output);
		finalized = M_StartHiddenProcessAndWait(commandline);
	}

	if (finalized)
	{
		remove(job->frames);
		if (job->audio_enabled)
			remove(job->audio);
		I_OutputMsg("Recording finished: %s (%u captures, %llu timeline frames, "
			"%llu repeated frames, %u queue skips)\n", job->output,
			job->source_frames, (unsigned long long)job->timeline_frames,
			(unsigned long long)job->repeated_frames, job->capture_skips);
	}
	else
		I_OutputMsg("Recording finalization failed. Recoverable source kept at %s\n",
			job->frames);
	free(job);
}

static void M_GetMovieCaptureSize(INT32 *width, INT32 *height)
{
	INT32 wanted = cv_movie_resolution.value;
	if (wanted <= 0 || wanted >= vid.height)
		wanted = vid.height;
	*height = max(2, wanted & ~1);
	*width = max(2, (INT32)(((INT64)vid.width * *height / vid.height) & ~1));
}

static boolean M_WriteMoviePipe(const UINT8 *data, size_t bytes)
{
	while (bytes)
	{
		DWORD written = 0;
		DWORD chunk = (DWORD)min(bytes, (size_t)0x40000000);
		if (!WriteFile(movie_ffmpeg_input, data, chunk, &written, NULL) || !written)
			return false;
		data += written;
		bytes -= written;
	}
	return true;
}

static DWORD WINAPI M_MovieFrameWriter(LPVOID userdata)
{
	(void)userdata;
	for (;;)
	{
		movie_frame_slot_t slot;
		UINT32 repeat;
		boolean have_frame = false;

		EnterCriticalSection(&movie_frame_queue_lock);
		if (movie_frame_queue_count)
		{
			slot = movie_frame_queue[movie_frame_queue_head];
			have_frame = true;
		}
		else if (movie_frame_writer_stop)
		{
			LeaveCriticalSection(&movie_frame_queue_lock);
			break;
		}
		LeaveCriticalSection(&movie_frame_queue_lock);

		if (!have_frame)
		{
			WaitForSingleObject(movie_frame_queue_event, INFINITE);
			continue;
		}

		if (!movie_frame_writer_failed)
		{
			for (repeat = 0; repeat < slot.repeats; repeat++)
			{
				if (!M_WriteMoviePipe(slot.pixels, movie_rgb_frame_size))
				{
					movie_frame_writer_failed = true;
					break;
				}
			}
		}

		EnterCriticalSection(&movie_frame_queue_lock);
		movie_frame_queue_head = (movie_frame_queue_head + 1) % MOVIE_FRAME_QUEUE_SIZE;
		movie_frame_queue_count--;
		LeaveCriticalSection(&movie_frame_queue_lock);
	}
	return 0;
}

static boolean M_CaptureMovieFrame(UINT8 *frame)
{
	UINT8 *source = NULL;
	INT32 x, y;
#ifdef HWRENDER
	if (rendermode == render_opengl)
		source = HWR_GetScreenshot();
	else
#endif
	{
		source = screens[2];
		I_ReadScreen(source);
		M_CreateScreenShotPalette();
	}
	if (!source)
		return false;
	for (y = 0; y < movie_capture_height; ++y)
	{
		const INT32 sy = (INT32)((INT64)y * vid.height / movie_capture_height);
		UINT8 *dest = frame + (size_t)y * movie_capture_width * 3;
		for (x = 0; x < movie_capture_width; ++x)
		{
			const INT32 sx = (INT32)((INT64)x * vid.width / movie_capture_width);
#ifdef HWRENDER
			if (rendermode == render_opengl)
			{
				const UINT8 *pixel = source + ((size_t)sy * vid.width + sx) * 3;
				*dest++ = pixel[0]; *dest++ = pixel[1]; *dest++ = pixel[2];
			}
			else
#endif
			{
				const UINT8 index = source[(size_t)sy * vid.width + sx];
				*dest++ = screenshot_palette[index*3];
				*dest++ = screenshot_palette[index*3+1];
				*dest++ = screenshot_palette[index*3+2];
			}
		}
	}
#ifdef HWRENDER
	if (rendermode == render_opengl)
		free(source);
#endif
	return true;
}

static boolean M_QueueMovieFrame(UINT32 repeats)
{
	movie_frame_slot_t *slot;
	EnterCriticalSection(&movie_frame_queue_lock);
	if (movie_frame_queue_count == MOVIE_FRAME_QUEUE_SIZE)
	{
		LeaveCriticalSection(&movie_frame_queue_lock);
		movie_capture_skips++;
		return true;
	}
	slot = &movie_frame_queue[movie_frame_queue_tail];
	LeaveCriticalSection(&movie_frame_queue_lock);

	if (!M_CaptureMovieFrame(slot->pixels))
		return false;
	slot->repeats = max(repeats, 1);
	EnterCriticalSection(&movie_frame_queue_lock);
	movie_frame_queue_tail = (movie_frame_queue_tail + 1) % MOVIE_FRAME_QUEUE_SIZE;
	movie_frame_queue_count++;
	movie_frames_scheduled += slot->repeats;
	movie_source_frames++;
	movie_repeated_frames += slot->repeats - 1;
	LeaveCriticalSection(&movie_frame_queue_lock);
	SetEvent(movie_frame_queue_event);
	return true;
}

static moviemode_t M_StartMovieFFmpeg(const char *pathname, boolean gif)
{
	SECURITY_ATTRIBUTES security;
	STARTUPINFOA startup;
	PROCESS_INFORMATION process;
	HANDLE pipe_read = NULL, null_output = INVALID_HANDLE_VALUE;
	char commandline[4*MAX_WADPATH];
	const char *freename;
	char basename[13];

	if (!SearchPathA(NULL, "ffmpeg.exe", NULL, sizeof movie_ffmpeg_path,
		movie_ffmpeg_path, NULL))
	{
		if (!gif)
			CONS_Alert(CONS_ERROR, "Video + Sound needs ffmpeg.exe in PATH. GIF and aPNG recording still work.\n");
		return MM_OFF;
	}
	if (!(freename = Newsnapshotfile(pathname, gif ? "gif" : "mp4")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't reserve a recording filename in %s\n", pathname);
		return MM_OFF;
	}
	strlcpy(basename, freename, sizeof basename);
	basename[8] = '\0';
	snprintf(movie_video_output, sizeof movie_video_output, pandf, pathname, freename);
	snprintf(movie_video_frames, sizeof movie_video_frames, "%s" PATHSEP "%s-capture.mkv", pathname, basename);
	snprintf(movie_video_audio, sizeof movie_video_audio, "%s" PATHSEP "%s-capture.wav", pathname, basename);
	M_GetMovieCaptureSize(&movie_capture_width, &movie_capture_height);
	movie_capture_fps = max(cv_gif_maxfps.value, 1);
	movie_rgb_frame_size = (size_t)movie_capture_width * movie_capture_height * 3;
	movie_rgb_frame = malloc(movie_rgb_frame_size * MOVIE_FRAME_QUEUE_SIZE);
	if (!movie_rgb_frame)
		return MM_OFF;
	for (INT32 slot = 0; slot < MOVIE_FRAME_QUEUE_SIZE; slot++)
		movie_frame_queue[slot].pixels = movie_rgb_frame + movie_rgb_frame_size * slot;
	memset(&security, 0, sizeof security);
	security.nLength = sizeof security;
	security.bInheritHandle = TRUE;
	if (!CreatePipe(&pipe_read, &movie_ffmpeg_input, &security, 1024*1024)
	 || !SetHandleInformation(movie_ffmpeg_input, HANDLE_FLAG_INHERIT, 0))
	{
		if (pipe_read) CloseHandle(pipe_read);
		if (movie_ffmpeg_input) CloseHandle(movie_ffmpeg_input);
		movie_ffmpeg_input = NULL;
		free(movie_rgb_frame); movie_rgb_frame = NULL;
		return MM_OFF;
	}
	null_output = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
		&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	memset(&startup, 0, sizeof startup);
	startup.cb = sizeof startup;
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = pipe_read;
	startup.hStdOutput = null_output;
	startup.hStdError = null_output;
	memset(&process, 0, sizeof process);
	snprintf(commandline, sizeof commandline,
		"\"%s\" -y -hide_banner -loglevel error -f rawvideo -pix_fmt rgb24 "
		"-video_size %dx%d -framerate %d -i pipe:0 -an "
		"-vf \"scale=in_range=pc:out_range=tv:out_color_matrix=bt709\" "
		"-c:v libx264 -preset ultrafast -tune zerolatency -crf 14 "
		"-x264-params colorprim=bt709:transfer=bt709:colormatrix=bt709:range=limited "
		"-pix_fmt yuv444p -color_range tv -colorspace bt709 "
		"-color_primaries bt709 -color_trc bt709 \"%s\"",
		movie_ffmpeg_path, movie_capture_width, movie_capture_height,
		movie_capture_fps, movie_video_frames);
	if (!CreateProcessA(movie_ffmpeg_path, commandline, NULL, NULL, TRUE,
		CREATE_NO_WINDOW|BELOW_NORMAL_PRIORITY_CLASS, NULL, NULL, &startup, &process))
	{
		CloseHandle(pipe_read);
		if (null_output != INVALID_HANDLE_VALUE) CloseHandle(null_output);
		CloseHandle(movie_ffmpeg_input); movie_ffmpeg_input = NULL;
		free(movie_rgb_frame); movie_rgb_frame = NULL;
		return MM_OFF;
	}
	CloseHandle(process.hThread);
	CloseHandle(pipe_read);
	if (null_output != INVALID_HANDLE_VALUE) CloseHandle(null_output);
	movie_ffmpeg_process = process.hProcess;
	InitializeCriticalSection(&movie_frame_queue_lock);
	movie_frame_queue_event = CreateEventA(NULL, FALSE, FALSE, NULL);
	movie_frame_writer_stop = movie_frame_writer_failed = false;
	movie_frame_queue_head = movie_frame_queue_tail = movie_frame_queue_count = 0;
	movie_frames_scheduled = 0;
	movie_capture_skips = 0;
	movie_source_frames = 0;
	movie_repeated_frames = 0;
	movie_frame_writer_thread = movie_frame_queue_event
		? CreateThread(NULL, 0, M_MovieFrameWriter, NULL, 0, NULL) : NULL;
	if (!movie_frame_writer_thread)
	{
		if (movie_frame_queue_event) CloseHandle(movie_frame_queue_event);
		movie_frame_queue_event = NULL;
		DeleteCriticalSection(&movie_frame_queue_lock);
		CloseHandle(movie_ffmpeg_input); movie_ffmpeg_input = NULL;
		WaitForSingleObject(movie_ffmpeg_process, INFINITE);
		CloseHandle(movie_ffmpeg_process); movie_ffmpeg_process = NULL;
		free(movie_rgb_frame); movie_rgb_frame = NULL;
		remove(movie_video_frames);
		return MM_OFF;
	}
	SetThreadPriority(movie_frame_writer_thread, THREAD_PRIORITY_BELOW_NORMAL);
	movie_ffmpeg_recording = true;
	movie_ffmpeg_gif = gif;
	movie_ffmpeg_failed = false;
	movie_video_recording = !gif;
	if (!gif && !I_StartMovieAudioCapture(movie_video_audio))
	{
		movie_frame_writer_stop = true;
		SetEvent(movie_frame_queue_event);
		WaitForSingleObject(movie_frame_writer_thread, INFINITE);
		CloseHandle(movie_frame_writer_thread); movie_frame_writer_thread = NULL;
		CloseHandle(movie_frame_queue_event); movie_frame_queue_event = NULL;
		DeleteCriticalSection(&movie_frame_queue_lock);
		CloseHandle(movie_ffmpeg_input); movie_ffmpeg_input = NULL;
		WaitForSingleObject(movie_ffmpeg_process, INFINITE);
		CloseHandle(movie_ffmpeg_process); movie_ffmpeg_process = NULL;
		movie_ffmpeg_recording = movie_video_recording = false;
		free(movie_rgb_frame); movie_rgb_frame = NULL;
		remove(movie_video_frames);
		CONS_Alert(CONS_ERROR, "Couldn't capture mixed game audio.\n");
		return MM_OFF;
	}
	movie_recording_start = I_GetPreciseTime();
	CONS_Printf("Recording %dx%d at %d FPS. Final conversion runs after you stop.\n",
		movie_capture_width, movie_capture_height, movie_capture_fps);
	return gif ? MM_GIF : MM_VIDEO;
}
#endif

static inline moviemode_t M_StartMovieVideo(const char *pathname)
{
#ifdef _WIN32
	return M_StartMovieFFmpeg(pathname, false);
#else
	(void)pathname;
	CONS_Alert(CONS_ERROR, "Video + Sound is not available in this build.\n");
	return MM_OFF;
#endif
}
#endif

void M_GetMovieDiagnostics(char *buffer, size_t buffer_size)
{
	if (!buffer || !buffer_size)
		return;
#ifdef _WIN32
	if (movie_ffmpeg_recording)
	{
		UINT8 queued;
		EnterCriticalSection(&movie_frame_queue_lock);
		queued = movie_frame_queue_count;
		LeaveCriticalSection(&movie_frame_queue_lock);
		snprintf(buffer, buffer_size,
			"recording=%s capture=%dx%d@%d queue=%u/%u captures=%u "
			"timeline_frames=%llu repeated_frames=%llu queue_skips=%u encoder_failed=%d",
			movie_ffmpeg_gif ? "gif" : "mp4", movie_capture_width,
			movie_capture_height, movie_capture_fps, queued,
			(unsigned)MOVIE_FRAME_QUEUE_SIZE, movie_source_frames,
			(unsigned long long)movie_frames_scheduled,
			(unsigned long long)movie_repeated_frames, movie_capture_skips,
			movie_frame_writer_failed || movie_ffmpeg_failed);
		return;
	}
#endif
	snprintf(buffer, buffer_size, "recording=%s",
		moviemode == MM_APNG ? "apng" : moviemode == MM_GIF ? "legacy_gif"
		: moviemode == MM_SCREENSHOT ? "screenshots" : "off");
}

void M_StartMovie(void)
{
#if NUMSCREENS > 2
	char pathname[MAX_WADPATH];

	if (moviemode)
		return;

	if (cv_movie_option.value == 0)
		strcpy(pathname, usehome ? srb2home : srb2path);
	else if (cv_movie_option.value == 1)
		strcpy(pathname, srb2home);
	else if (cv_movie_option.value == 2)
		strcpy(pathname, srb2path);
	else if (cv_movie_option.value == 3 && *cv_movie_folder.string != '\0')
		strcpy(pathname, cv_movie_folder.string);

	if (cv_movie_option.value != 3)
	{
		strcat(pathname, PATHSEP"movies"PATHSEP);
		I_mkdir(pathname, 0755);
	}

	if (rendermode == render_none)
		I_Error("Can't make a movie without a render system\n");

	switch (cv_moviemode.value)
	{
		case MM_VIDEO:
			moviemode = M_StartMovieVideo(pathname);
			break;
		case MM_GIF:
#ifdef _WIN32
			moviemode = M_StartMovieFFmpeg(pathname, true);
			if (moviemode != MM_OFF)
				break;
#endif
			moviemode = M_StartMovieGIF(pathname);
			break;
		case MM_APNG:
			moviemode = M_StartMovieAPNG(pathname);
			break;
		case MM_SCREENSHOT:
			moviemode = MM_SCREENSHOT;
			break;
		default:  
			return;
	}

	if (movie_video_recording)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "MP4 video + sound");
#ifdef _WIN32
	else if (movie_ffmpeg_recording && movie_ffmpeg_gif)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "GIF (fast capture)");
#endif
	else if (moviemode == MM_APNG)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "aPNG");
	else if (moviemode == MM_GIF)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "GIF");
	else if (moviemode == MM_SCREENSHOT)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "screenshots");
	movie_prevcapturetime = 0;


#endif
}

void M_SaveFrame(void)
{
#if NUMSCREENS > 2
	const precise_t now = I_GetPreciseTime();
	const precise_t precision = I_GetPrecisePrecision();
	const precise_t interval = precision / (UINT32)max(cv_gif_maxfps.value, 1);

	if (movie_video_recording)
		I_UpdateMovieAudioCapture();

	                                                                            
                                                                    
	if (!singletics && movie_prevcapturetime
	&& now - movie_prevcapturetime < interval)
		return;
	if (movie_video_recording && movie_prevcapturetime && precision)
	{
		const UINT64 elapsed = now - movie_prevcapturetime;
		const UINT64 delay = (elapsed * (UINT64)max(cv_gif_maxfps.value, 1)
			+ precision / 2) / precision;
		                                                                      
                                                                           
		movie_video_framedelay = (UINT16)min((UINT64)UINT16_MAX,
			max(1ULL, delay));
	}
	else
		movie_video_framedelay = 1;
	movie_prevcapturetime = now;

#ifdef _WIN32
	if (movie_ffmpeg_recording)
	{
		UINT64 desired_frames = 1;
		UINT64 pending_frames;
		if (precision && movie_recording_start && now > movie_recording_start)
			desired_frames += ((UINT64)(now - movie_recording_start)
				* (UINT64)movie_capture_fps) / precision;
		pending_frames = desired_frames > movie_frames_scheduled
			? desired_frames - movie_frames_scheduled : 1;
		if (movie_frame_writer_failed)
			movie_ffmpeg_failed = true;
		if (!movie_ffmpeg_failed && !M_QueueMovieFrame((UINT32)min(pending_frames, (UINT64)UINT32_MAX)))
		{
			movie_ffmpeg_failed = true;
			CONS_Alert(CONS_ERROR, "Recording stopped accepting frames; stop recording to keep the recoverable capture.\n");
		}
		return;
	}
#endif

	switch (moviemode)
	{
		case MM_SCREENSHOT:
			takescreenshot = true;
			return;
		case MM_GIF:
			GIF_frame();
			return;
		case MM_APNG:
#ifdef USE_APNG
			{
				UINT8 *linear = NULL;
				if (!apng_FILE)  
				{
					moviemode = MM_OFF;
					return;
				}

				if (rendermode == render_soft)
				{

					linear = screens[2];
					I_ReadScreen(linear);
				}
#ifdef HWRENDER
				else
					linear = HWR_GetScreenshot();
#endif
				M_PNGFrame(apng_ptr, apng_info_ptr, (png_bytep)linear);
#ifdef HWRENDER
				if (rendermode != render_soft && linear)
					free(linear);
#endif

				if (apng_frames == PNG_UINT_31_MAX)
				{
					CONS_Alert(CONS_NOTICE, M_GetText("Max movie size reached\n"));
					M_StopMovie();
				}
			}
#else
			moviemode = MM_OFF;
#endif
			return;
		default:
			return;
	}
#endif
}

void M_StopMovie(void)
{
#if NUMSCREENS > 2
#ifdef _WIN32
	if (movie_ffmpeg_recording)
	{
		movie_encode_job_t *job;
		if (movie_video_recording)
			I_StopMovieAudioCapture();
		movie_video_recording = false;
		movie_frame_writer_stop = true;
		SetEvent(movie_frame_queue_event);
		WaitForSingleObject(movie_frame_writer_thread, INFINITE);
		CloseHandle(movie_frame_writer_thread); movie_frame_writer_thread = NULL;
		CloseHandle(movie_frame_queue_event); movie_frame_queue_event = NULL;
		DeleteCriticalSection(&movie_frame_queue_lock);
		CloseHandle(movie_ffmpeg_input);
		movie_ffmpeg_input = NULL;
		free(movie_rgb_frame);
		movie_rgb_frame = NULL;
		job = malloc(sizeof *job);
		if (job)
		{
			job->process = movie_ffmpeg_process;
			strlcpy(job->frames, movie_video_frames, sizeof job->frames);
			strlcpy(job->audio, movie_video_audio, sizeof job->audio);
			strlcpy(job->output, movie_video_output, sizeof job->output);
			job->gif = movie_ffmpeg_gif;
			job->audio_enabled = !movie_ffmpeg_gif;
			job->timeline_frames = movie_frames_scheduled;
			job->source_frames = movie_source_frames;
			job->repeated_frames = movie_repeated_frames;
			job->capture_skips = movie_capture_skips;
#ifdef HAVE_THREADS
			I_spawn_thread("movie finalizer", M_WaitForMovieEncode, job);
#else
			M_WaitForMovieEncode(job);
#endif
			CONS_Printf("Finishing recording in the background: %s\n", movie_video_output);
		}
		else
		{
			CloseHandle(movie_ffmpeg_process);
			CONS_Alert(CONS_ERROR, "Could not start the recording finalizer; kept %s\n", movie_video_frames);
		}
		movie_ffmpeg_process = NULL;
		movie_ffmpeg_recording = movie_ffmpeg_gif = false;
		moviemode = MM_OFF;
		CONS_Printf(M_GetText("Movie mode disabled.\n"));
		return;
	}
#endif
	switch (moviemode)
	{
		case MM_GIF:
			if (!GIF_close())
				return;
			break;
		case MM_APNG:
#ifdef USE_APNG
			if (!apng_FILE)
				return;

			if (apng_frames)
			{
				M_PNGfix_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
				apng_write_end(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
			}

			png_destroy_write_struct(&apng_ptr, &apng_info_ptr);

			fclose(apng_FILE);
			apng_FILE = NULL;
			CONS_Printf("aPNG closed; wrote %u frames\n", (UINT32)apng_frames);
			apng_frames = 0;
			break;
#else
			return;
#endif
		case MM_SCREENSHOT:
			break;
		default:
			return;
	}
	moviemode = MM_OFF;
	CONS_Printf(M_GetText("Movie mode disabled.\n"));
#endif
}




#ifdef USE_PNG
                              
   
                                         
                                   
                                         
                                          
                                          
                                            
    
static void PNG_file_error(png_structp PNG, png_const_charp pngtext)
{
	(void)pngtext;
	png_longjmp(PNG, 1);
}

static void PNG_file_warn(png_structp PNG, png_const_charp pngtext)
{
	(void)PNG;
	(void)pngtext;
}

static boolean M_SavePNGConfigured(const char *filename, void *data, int width, int height,
	const UINT8 *palette, pngtextsnapshot_t *snapshot, INT32 compression_level,
	INT32 memory_level, INT32 strategy, INT32 window_bits)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	PNG_CONST png_byte *PLTE = (const png_byte *)palette;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;

	png_FILE = fopen(filename,"wb");
	if (!png_FILE)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on opening %s for write\n", filename);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_file_error, PNG_file_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on initialize libpng\n");
		fclose(png_FILE);
		remove(filename);
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on allocate for libpng\n");
		png_destroy_write_struct(&png_ptr,  NULL);
		fclose(png_FILE);
		remove(filename);
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{

		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		fclose(png_FILE);
		remove(filename);
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr),jmpbuf, sizeof (jmp_buf));
#endif
	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif



	png_set_compression_level(png_ptr, compression_level);
	png_set_compression_mem_level(png_ptr, memory_level);
	png_set_compression_strategy(png_ptr, strategy);
	png_set_compression_window_bits(png_ptr, window_bits);

	M_PNGhdr(png_ptr, png_info_ptr, width, height, PLTE);

#ifdef PNG_TEXT_SUPPORTED
	M_SetPNGText(png_ptr, png_info_ptr, snapshot);
#else
	(void)snapshot;
#endif

	png_write_info(png_ptr, png_info_ptr);

	M_PNGImage(png_ptr, png_info_ptr, height, data);

	png_write_end(png_ptr, png_info_ptr);
	png_destroy_write_struct(&png_ptr, &png_info_ptr);

	fclose(png_FILE);
	return true;
}

boolean M_SavePNG(const char *filename, void *data, int width, int height, const UINT8 *palette)
{
	pngtextsnapshot_t snapshot;
	M_CapturePNGText(&snapshot, false);
	return M_SavePNGConfigured(filename, data, width, height, palette, &snapshot,
		cv_zlib_level.value, cv_zlib_memory.value,
		cv_zlib_strategy.value, cv_zlib_window_bits.value);
}

#ifdef HAVE_THREADS
typedef struct
{
	char filename[MAX_WADPATH + 16];
	char pathname[MAX_WADPATH];
	char freename[16];
	UINT8 *pixels;
	UINT8 palette[768];
	boolean paletted;
	boolean done;
	boolean result;
	INT32 width, height;
	INT32 compression_level, memory_level, strategy, window_bits;
	pngtextsnapshot_t metadata;
} asyncscreenshot_t;

static I_mutex asyncscreenshot_mutex;
static asyncscreenshot_t *asyncscreenshot;

static void M_AsyncScreenshotWorker(void *userdata)
{
	asyncscreenshot_t *job = (asyncscreenshot_t *)userdata;
	boolean result = M_SavePNGConfigured(job->filename, job->pixels, job->width, job->height,
		job->paletted ? job->palette : NULL, &job->metadata,
		job->compression_level, job->memory_level, job->strategy, job->window_bits);

	I_lock_mutex(&asyncscreenshot_mutex);
	job->result = result;
	job->done = true;
	I_unlock_mutex(asyncscreenshot_mutex);
}

static boolean M_AsyncScreenshotBusy(void)
{
	boolean busy;
	I_lock_mutex(&asyncscreenshot_mutex);
	busy = (asyncscreenshot != NULL);
	I_unlock_mutex(asyncscreenshot_mutex);
	return busy;
}

static boolean M_StartAsyncScreenshot(const char *filename, const char *pathname,
	const char *freename, UINT8 *pixels, boolean paletted)
{
	asyncscreenshot_t *job = calloc(1, sizeof *job);
	if (!job)
		return false;

	snprintf(job->filename, sizeof job->filename, "%s", filename);
	snprintf(job->pathname, sizeof job->pathname, "%s", pathname);
	snprintf(job->freename, sizeof job->freename, "%s", freename);
	job->pixels = pixels;
	job->paletted = paletted;
	job->width = vid.width;
	job->height = vid.height;
	job->compression_level = cv_zlib_level.value;
	job->memory_level = cv_zlib_memory.value;
	job->strategy = cv_zlib_strategy.value;
	job->window_bits = cv_zlib_window_bits.value;
	if (paletted)
		M_Memcpy(job->palette, screenshot_palette, sizeof job->palette);
	M_CapturePNGText(&job->metadata, false);

	I_lock_mutex(&asyncscreenshot_mutex);
	if (asyncscreenshot)
	{
		I_unlock_mutex(asyncscreenshot_mutex);
		free(job);
		return false;
	}
	asyncscreenshot = job;
	I_unlock_mutex(asyncscreenshot_mutex);
	I_spawn_thread("png-screenshot", M_AsyncScreenshotWorker, job);
	return true;
}

void M_PollAsyncScreenshots(void)
{
	asyncscreenshot_t *finished = NULL;

	if (!asyncscreenshot)
		return;
	I_lock_mutex(&asyncscreenshot_mutex);
	if (asyncscreenshot && asyncscreenshot->done)
	{
		finished = asyncscreenshot;
		asyncscreenshot = NULL;
	}
	I_unlock_mutex(asyncscreenshot_mutex);

	if (!finished)
		return;
	if (finished->result)
		CONS_Printf(M_GetText("Screen shot %s saved in %s\n"), finished->freename, finished->pathname);
	else
		CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot %s in %s\n"), finished->freename, finished->pathname);
	free(finished->pixels);
	free(finished);
}
#endif
#else
                       
    
typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bits_per_pixel;

	UINT16 xmin, ymin;
	UINT16 xmax, ymax;
	UINT16 hres, vres;
	UINT8  palette[48];

	UINT8 reserved;
	UINT8 color_planes;
	UINT16 bytes_per_line;
	UINT16 palette_type;

	char filler[58];
	UINT8 data;  
} pcx_t;

                              
   
                                         
                                   
                                         
                                          
                                         
    
#if NUMSCREENS > 2
static boolean WritePCXfile(const char *filename, const UINT8 *data, int width, int height, const UINT8 *pal)
{
	int i;
	size_t length;
	pcx_t *pcx;
	UINT8 *pack;

	pcx = Z_Malloc(width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a;  
	pcx->version = 5;  
	pcx->encoding = 1;  
	pcx->bits_per_pixel = 8;  
	pcx->xmin = pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset(pcx->palette, 0, sizeof (pcx->palette));
	pcx->reserved = 0;
	pcx->color_planes = 1;  
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(1);  
	memset(pcx->filler, 0, sizeof (pcx->filler));


	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}


	*pack++ = 0x0c;  


	{
		for (i = 0; i < 256; i++)
		{
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
		}
	}


	length = pack - (UINT8 *)pcx;
	i = FIL_WriteFile(filename, pcx, length);

	Z_Free(pcx);
	return i;
}
#endif
#endif

#if !(defined(USE_PNG) && defined(HAVE_THREADS))
void M_PollAsyncScreenshots(void)
{
}
#endif

void M_ScreenShot(void)
{
	takescreenshot = true;
}

                       
                                                                      
                                                              
   
                      
    
void M_DoScreenShot(void)
{
#if NUMSCREENS > 2
	const char *freename = NULL;
	char pathname[MAX_WADPATH];
	boolean ret = false;
	UINT8 *linear = NULL;


	takescreenshot = false;
	M_PollAsyncScreenshots();


	if (rendermode == render_none)
		return;

#if defined(USE_PNG) && defined(HAVE_THREADS)
	if (moviemode != MM_SCREENSHOT && M_AsyncScreenshotBusy())
	{
		CONS_Alert(CONS_NOTICE, M_GetText("A screen shot is still being saved; try again in a moment.\n"));
		return;
	}
#endif

	if (cv_screenshot_option.value == 0)
		strcpy(pathname, usehome ? srb2home : srb2path);
	else if (cv_screenshot_option.value == 1)
		strcpy(pathname, srb2home);
	else if (cv_screenshot_option.value == 2)
		strcpy(pathname, srb2path);
	else if (cv_screenshot_option.value == 3 && *cv_screenshot_folder.string != '\0')
		strcpy(pathname, cv_screenshot_folder.string);

	if (cv_screenshot_option.value != 3)
	{
		strcat(pathname, PATHSEP"screenshots"PATHSEP);
		I_mkdir(pathname, 0755);
	}

#ifdef USE_PNG
	freename = Newsnapshotfile(pathname,"png");
#else
	if (rendermode == render_soft)
		freename = Newsnapshotfile(pathname,"pcx");
	else if (rendermode == render_opengl)
		freename = Newsnapshotfile(pathname,"tga");
#endif

	if (!freename)
		goto failure;

#if defined(USE_PNG) && defined(HAVE_THREADS)



	if (moviemode != MM_SCREENSHOT)
	{
		boolean paletted = (rendermode == render_soft);
		if (paletted)
		{
			linear = malloc((size_t)vid.width * (size_t)vid.height);
			if (linear)
			{
				I_ReadScreen(linear);
				M_CreateScreenShotPalette();
			}
		}
#ifdef HWRENDER
		else if (rendermode == render_opengl)
			linear = HWR_GetScreenshot();
#endif
		if (!linear)
			goto failure;
		if (M_StartAsyncScreenshot(va(pandf,pathname,freename), pathname, freename, linear, paletted))
			return;
		free(linear);
		linear = NULL;
		goto failure;
	}
#endif

	if (rendermode == render_soft)
	{

		linear = screens[2];
		I_ReadScreen(linear);
	}


#ifdef HWRENDER
	if (rendermode == render_opengl)
		ret = HWR_Screenshot(va(pandf,pathname,freename));
	else
#endif
	{
		M_CreateScreenShotPalette();
#ifdef USE_PNG
		ret = M_SavePNG(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#else
		ret = WritePCXfile(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#endif
	}

failure:
	if (ret)
	{
		if (moviemode != MM_SCREENSHOT)
			CONS_Printf(M_GetText("Screen shot %s saved in %s\n"), freename, pathname);
	}
	else
	{
		if (freename)
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot %s in %s\n"), freename, pathname);
		else
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot in %s (all 10000 slots used!)\n"), pathname);

		if (moviemode == MM_SCREENSHOT)
			M_StopMovie();
	}
#endif
}

boolean M_ScreenshotResponder(event_t *ev)
{
	INT32 ch = -1;
	if (dedicated || ev->type != ev_keydown)
		return false;

	ch = ev->key;

	if (ch >= KEY_MOUSE1 && menuactive)  
		return false;

	if (ch == KEY_F8 || ch == gamecontrol[GC_SCREENSHOT][0] || ch == gamecontrol[GC_SCREENSHOT][1])  
		M_ScreenShot();
	else if (ch == KEY_F9 || ch == gamecontrol[GC_RECORDGIF][0] || ch == gamecontrol[GC_RECORDGIF][1])  
		((moviemode) ? M_StopMovie : M_StartMovie)();
	else
		return false;
	return true;
}







#ifdef GETTEXT
	#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
		#define GETTEXTDOMAIN1 "/usr/share/locale"
		#define GETTEXTDOMAIN2 "/usr/local/share/locale"
	#elif defined (_WIN32)
		#define GETTEXTDOMAIN1 "."
	#endif
#endif  

void M_StartupLocale(void)
{
#ifdef GETTEXT
	char *textdomhandle = NULL;
#endif  

	CONS_Printf("M_StartupLocale...\n");

	setlocale(LC_ALL, "");


	setlocale(LC_NUMERIC, "C");

#ifdef GETTEXT

#ifdef GETTEXTDOMAIN1
	textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN1);
#endif
#ifdef GETTEXTDOMAIN2
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN2);
#endif
#ifdef GETTEXTDOMAIN3
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN3);
#endif
#ifdef GETTEXTDOMAIN4
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN4);
#endif
	if (textdomhandle)
		textdomain("srb2");
	else
		CONS_Printf("Could not find locale text domain!\n");
#endif  
}





                                                   
                               
   
                                
                                                                         
                             
    
char *va(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

                                                                               
                                                         
                                                             
   
                                                    
                                                         
                                                         
    
void strcatbf(char *s1, const char *s2, const char *s3)
{
	char tmp[1024];

	strcpy(tmp, s1);
	strcpy(s1, s2);
	strcat(s1, s3);
	strcat(s1, tmp);
}

                                                                  
                                                                          
                                                          
   
                                     
                                                            
    
INT32 axtoi(const char *hexStg)
{
	INT32 n = 0;
	INT32 m = 0;
	INT32 count;
	INT32 intValue = 0;
	INT32 digit[8];
	while (n < 8)
	{
		if (hexStg[n] == '\0')
			break;
		if (hexStg[n] >= '0' && hexStg[n] <= '9')  
			digit[n] = (hexStg[n] & 0x0f);
		else if (hexStg[n] >= 'a' && hexStg[n] <= 'f')  
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else if (hexStg[n] >= 'A' && hexStg[n] <= 'F')  
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else
			break;
		n++;
	}
	count = n;
	m = n - 1;
	n = 0;
	while (n < count)
	{
		intValue = intValue | (digit[n] << (m << 2));
		m--;
		n++;
	}
	return intValue;
}



static UINT32 oldendPos = 0;  
static UINT32 endPos = 0;  

                                                                                         
                                                                                                                              
                                                                                                                         
                                                                                 
               
   
                                                                                                                                        
                                                                                                                                               
                                                                                                                                               
  
char *M_GetToken(const char *inputString)
{
	static const char *stringToUse = NULL;  
	static UINT32 startPos = 0;

	static UINT32 stringLength = 0;
	static UINT8 inComment = 0;  
	char *texturesToken = NULL;
	UINT32 texturesTokenLength = 0;

	if (inputString != NULL)
	{
		stringToUse = inputString;
		startPos = 0;
		oldendPos = endPos = 0;
		stringLength = strlen(inputString);
	}
	else
	{
		startPos = oldendPos = endPos;
	}
	if (stringToUse == NULL)
		return NULL;


	if (startPos < stringLength - 1
		&& inComment == 0)
	{
		if (stringToUse[startPos] == '/'
			&& stringToUse[startPos+1] == '/')
		{

			inComment = 1;
		}
		else if (stringToUse[startPos] == '/'
			&& stringToUse[startPos+1] == '*')
		{

			inComment = 2;
		}
	}


	while ((stringToUse[startPos] == ' '
			|| stringToUse[startPos] == '\t'
			|| stringToUse[startPos] == '\r'
			|| stringToUse[startPos] == '\n'
			|| stringToUse[startPos] == '\0'
			|| stringToUse[startPos] == '=' || stringToUse[startPos] == ';'  
			|| inComment != 0)
			&& startPos < stringLength)
	{

		if (inComment == 1
			&& stringToUse[startPos] == '\n')
		{

			inComment = 0;
		}
		else if (inComment == 2
			&& startPos < stringLength - 1
			&& stringToUse[startPos] == '*'
			&& stringToUse[startPos+1] == '/')
		{

			inComment = 0;
			startPos++;  
		}

		startPos++;


		if (startPos < stringLength - 1
			&& inComment == 0)
		{
			if (stringToUse[startPos] == '/'
				&& stringToUse[startPos+1] == '/')
			{

				inComment = 1;
			}
			else if (stringToUse[startPos] == '/'
				&& stringToUse[startPos+1] == '*')
			{

				inComment = 2;
			}
		}
	}


	if (startPos == stringLength) {
		endPos = stringLength;
		return NULL;
	}

	else if (stringToUse[startPos] == ','
			|| stringToUse[startPos] == '{'
			|| stringToUse[startPos] == '}')
	{
		endPos = startPos + 1;
		texturesToken = (char *)Z_Malloc(2*sizeof(char),PU_STATIC,NULL);
		texturesToken[0] = stringToUse[startPos];
		texturesToken[1] = '\0';
		return texturesToken;
	}

	else if (stringToUse[startPos] == '"')
	{
		endPos = ++startPos;
		while (stringToUse[endPos] != '"' && endPos < stringLength)
			endPos++;

		texturesTokenLength = endPos++ - startPos;

		texturesToken = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);

		M_Memcpy(texturesToken, stringToUse+startPos, (size_t)texturesTokenLength);

		texturesToken[texturesTokenLength] = '\0';

		return texturesToken;
	}


	endPos = startPos + 1;
	while ((stringToUse[endPos] != ' '
			&& stringToUse[endPos] != '\t'
			&& stringToUse[endPos] != '\r'
			&& stringToUse[endPos] != '\n'
			&& stringToUse[endPos] != ','
			&& stringToUse[endPos] != '{'
			&& stringToUse[endPos] != '}'
			&& stringToUse[endPos] != '=' && stringToUse[endPos] != ';'  
			&& inComment == 0)
			&& endPos < stringLength)
	{
		endPos++;

		if (endPos < stringLength - 1
			&& inComment == 0)
		{
			if (stringToUse[endPos] == '/'
				&& stringToUse[endPos+1] == '/')
			{

				inComment = 1;
			}
			else if (stringToUse[endPos] == '/'
				&& stringToUse[endPos+1] == '*')
			{

				inComment = 2;
			}
		}
	}
	texturesTokenLength = endPos - startPos;


	texturesToken = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);

	M_Memcpy(texturesToken, stringToUse+startPos, (size_t)texturesTokenLength);

	texturesToken[texturesTokenLength] = '\0';
	return texturesToken;
}

                                   
                                                                                           
                                                                                                      
                              
   
void M_UnGetToken(void)
{
	endPos = oldendPos;
}

static tokenizer_t *globalTokenizer = NULL;

void M_TokenizerOpen(const char *inputString, size_t len)
{
	globalTokenizer = Tokenizer_Open(inputString, len, 2);
}

void M_TokenizerClose(void)
{
	Tokenizer_Close(globalTokenizer);
	globalTokenizer = NULL;
}

const char *M_TokenizerRead(UINT32 i)
{
	if (!globalTokenizer)
		return NULL;

	return Tokenizer_SRB2Read(globalTokenizer, i);
}

UINT32 M_TokenizerGetEndPos(void)
{
	if (!globalTokenizer)
		return 0;

	return Tokenizer_GetEndPos(globalTokenizer);
}

void M_TokenizerSetEndPos(UINT32 newPos)
{
	if (globalTokenizer)
		Tokenizer_SetEndPos(globalTokenizer, newPos);
}

                           
    
UINT8 M_CountBits(UINT32 num, UINT8 size)
{
	UINT8 i, sum = 0;

	for (i = 0; i < size; ++i)
		if (num & (1 << i))
			++sum;
	return sum;
}

const char *GetRevisionString(void)
{
	static char rev[9] = {0};
	if (rev[0])
		return rev;

	if (comprevision[0] == 'r')
		strncpy(rev, comprevision, 7);
	else
		snprintf(rev, 7, "r%s", comprevision);
	rev[7] = '\0';

	return rev;
}

                                                        
                                                          
                           
                                                            
                                 
    
char *sizeu1(size_t num)
{
	static char sizeu1_buf[28];
	sprintf(sizeu1_buf, "%"PRIdS, num);
	return sizeu1_buf;
}

char *sizeu2(size_t num)
{
	static char sizeu2_buf[28];
	sprintf(sizeu2_buf, "%"PRIdS, num);
	return sizeu2_buf;
}

char *sizeu3(size_t num)
{
	static char sizeu3_buf[28];
	sprintf(sizeu3_buf, "%"PRIdS, num);
	return sizeu3_buf;
}

char *sizeu4(size_t num)
{
	static char sizeu4_buf[28];
	sprintf(sizeu4_buf, "%"PRIdS, num);
	return sizeu4_buf;
}

char *sizeu5(size_t num)
{
	static char sizeu5_buf[28];
	sprintf(sizeu5_buf, "%"PRIdS, num);
	return sizeu5_buf;
}

void *M_Memcpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

                                                                   
  
const char *M_FileError(FILE *fp)
{
	if (ferror(fp))
		return strerror(errno);
	else
		return "end-of-file";
}

                                            
  
int M_PathParts(const char *path)
{
	int n;
	const char *p;
	const char *t;
	if (path == NULL)
		return 0;
	for (n = 0, p = path ;; ++n)
	{
		t = p;
		if (( p = strchr(p, PATHSEP[0]) ))
			p += strspn(p, PATHSEP);
		else
		{
			if (*t) 
				n++;
			break;
		}
	}
	return n;
}

                                             
  
boolean M_IsPathAbsolute(const char *path)
{
#ifdef _WIN32
	return ( strncmp(&path[1], ":\\", 2) == 0 );
#else
	return ( path[0] == '/' );
#endif
}

                                      
  
void M_MkdirEachUntil(const char *cpath, int start, int end, int mode)
{
	char path[MAX_WADPATH];
	char *p;
	char *t;

	if (end > 0 && end <= start)
		return;

	strlcpy(path, cpath, sizeof path);
#ifdef _WIN32
	if (strncmp(&path[1], ":\\", 2) == 0)
		p = &path[3];
	else
#endif
		p = path;

	if (end > 0)
		end -= start;

	for (; start > 0; --start)
	{
		p += strspn(p, PATHSEP);
		if (!( p = strchr(p, PATHSEP[0]) ))
			return;
	}
	p += strspn(p, PATHSEP);
	for (;;)
	{
		if (end > 0 && !--end)
			break;

		t = p;
		if (( p = strchr(p, PATHSEP[0]) ))
		{
			*p = '\0';
			I_mkdir(path, mode);
			*p = PATHSEP[0];
			p += strspn(p, PATHSEP);
		}
		else
		{
			if (*t)
				I_mkdir(path, mode);
			break;
		}
	}
}

void M_MkdirEach(const char *path, int start, int mode)
{
	M_MkdirEachUntil(path, start, -1, mode);
}

int M_JumpWord(const char *line)
{
	int c;

	c = line[0];

	if (isspace(c))
		return strspn(line, " ");
	else if (ispunct(c))
		return strspn(line, PUNCTUATION);
	else
	{
		if (isspace(line[1]))
			return 1 + strspn(&line[1], " ");
		else
			return strcspn(line, " "PUNCTUATION);
	}
}

int M_JumpWordReverse(const char *line, int offset)
{
	int (*is)(int);
	int c;
	if (offset == 0)  
		return 0;
	c = line[--offset];
	if (isspace(c))
		is = isspace;
	else if (ispunct(c))
		is = ispunct;
	else
		is = isalnum;
	c = (*is)(line[offset]);
	while (offset > 0 &&
			(*is)(line[offset - 1]) == c)
		offset--;
	return offset;
}

const char * M_Ftrim (double f)
{
	static char dig[9]; 
	int i;
	 
	sprintf(dig, "%.6f", fabs(modf(f, &f)));
	 
	for (i = strlen(dig)-1; dig[i] == '0'; --i)
		;
	if (dig[i] == '.') 
		return "";
	else
	{
		dig[i + 1] = '\0';
		return &dig[1]; 
	}
}


boolean M_IsStringEmpty(const char *s)
{
	const char *ch = s;

	if (s == NULL || s[0] == '\0')
		return true;

	for (;;ch++)
	{
		if (!(*ch))
			break;
		if (!isspace((*ch)))
			return false;
	}

	return true;
}


boolean M_StringToNumber(const char *input, int *out)
{
	char *end_position = NULL;

	errno = 0;

	int result = strtol(input, &end_position, 10);
	if (end_position == input || *end_position != '\0')
		return false;

	if (errno == ERANGE)
		return false;

	*out = result;

	return true;
}


boolean M_StringToDecimal(const char *input, double *out)
{
	char *end_position = NULL;

	errno = 0;

	double result = strtod(input, &end_position);
	if (end_position == input || *end_position != '\0')
		return false;

	if (errno == ERANGE)
		return false;

	*out = result;

	return true;
}


int M_RoundUp(double number)
{
	if (number > 255.0l)
		return 255;
	if (number < 0.0l)
		return 0;

	if ((int)number <= (int)(number - 0.5f))
		return (int)number + 1;

	return (int)number;
}
