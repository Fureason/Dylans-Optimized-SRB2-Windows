// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2025 by Sonic Team Junior.
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
//
// Changes by Graue <graue@oceanbase.org> are in the public domain.
//




#include <signal.h>

#ifdef _WIN32
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include "../doomtype.h"
typedef BOOL (WINAPI *p_GetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *p_IsProcessorFeaturePresent) (DWORD);
typedef DWORD (WINAPI *p_timeGetTime) (void);
typedef UINT (WINAPI *p_timeEndPeriod) (UINT);
typedef HANDLE (WINAPI *p_OpenFileMappingA) (DWORD, BOOL, LPCSTR);
typedef LPVOID (WINAPI *p_MapViewOfFile) (HANDLE, DWORD, DWORD, DWORD, SIZE_T);


#define SystemFunction036 NTAPI SystemFunction036
#include <ntsecapi.h>
#undef SystemFunction036

#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#elif defined (_MSC_VER)
#include <direct.h>
#endif
#if defined (__unix__) || defined (UNIXCOMMON)
#include <fcntl.h>
#endif

#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#if defined (__linux__) || defined (__HAIKU__)
#include <sys/statvfs.h>
#else
#include <sys/statvfs.h>
#include <sys/param.h>
#include <sys/mount.h>
 
#include <sys/types.h>
#ifdef FREEBSD
#include <kvm.h>
#endif
#include <nlist.h>
#include <sys/sysctl.h>
#endif
#endif

#if defined (__linux__) || defined (UNIXCOMMON)
#ifndef NOTERMIOS
#include <termios.h>
#include <sys/ioctl.h>  
#define HAVE_TERMIOS
#endif
#endif

#if defined(UNIXCOMMON)
#include <poll.h>
#endif

#if defined (__unix__) || (defined (UNIXCOMMON) && !defined (__APPLE__))
#include <errno.h>
#include <sys/wait.h>
#ifndef __HAIKU__  
#define NEWSIGNALHANDLER
#endif
#endif

#ifndef NOMUMBLE
#ifdef __linux__  
#include <sys/mman.h>
#ifdef MAP_FAILED
#define HAVE_SHM
#endif
#include <wchar.h>
#endif

#ifdef _WIN32
#define HAVE_MUMBLE
#define WINMUMBLE
#elif defined (HAVE_SHM)
#define HAVE_MUMBLE
#endif
#endif  

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __APPLE__
#include "macosx/mac_resources.h"
#endif

#ifndef errno
#include <errno.h>
#endif

#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#ifndef NOEXECINFO
#include <execinfo.h>
#endif
#include <time.h>
#define UNIXBACKTRACE
#endif


const char *wadDefaultPaths[] = {
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	"/usr/local/share/games/SRB2",
	"/usr/local/games/SRB2",
	"/usr/share/games/SRB2",
	"/usr/games/SRB2",
#elif defined (_WIN32)
	"c:\\games\\srb2",
	"\\games\\srb2",
#endif
	NULL
};


const char *wadSearchPaths[] = {
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	"/usr/local/games",
	"/usr/games",
	"/usr/local",
#elif defined (_WIN32)
	"c:\\games",
	"\\games",
#endif
	NULL
};

                               
  
#define WADKEYWORD1 "srb2.pk3"
                         
  
static char returnWadPath[256];



#include "../doomdef.h"
#include "../m_misc.h"
#include "../i_time.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../i_threads.h"
#include "../screen.h"  
#include "../netcode/d_net.h"
#include "../netcode/commands.h"
#include "../g_game.h"
#include "../filesrch.h"

#include "../i_joy.h"

#include "../m_argv.h"

#include "../r_main.h"  
#include "../r_fps.h"

#ifdef MAC_ALERT
#include "macosx/mac_alert.h"
#endif

#include "../d_main.h"

#if !defined(NOMUMBLE) && defined(HAVE_MUMBLE)

#include "../netcode/d_clisrv.h"
#include "../byteptr.h"
#endif

#define MAX_EXIT_FUNCS 32





#define MIN_SLEEP_DURATION_MS 2.1

UINT8 graphics_started = 0;

UINT8 keyboard_started = 0;

static boolean consolevent = false;
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
static boolean framebuffer = false;
#endif

static size_t num_exit_funcs;
static void (*exit_funcs[MAX_EXIT_FUNCS])(void);

static boolean is_quitting = false;

#ifdef __linux__
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMAVAILABLE "MemAvailable:"
#define MEMFREE "MemFree:"
#define CACHED "Cached:"
#define BUFFERS "Buffers:"
#define SHMEM "Shmem:"

                                                                       
                         
static long get_entry(const char* name, const char* buf)
{
	long val;
	char* hit = strstr(buf, name);
	if (hit == NULL) {
		return -1;
	}

	errno = 0;
	val = strtol(hit + strlen(name), NULL, 10);
	if (errno != 0) {
		CONS_Alert(CONS_ERROR, M_GetText("get_entry: strtol() failed: %s\n"), strerror(errno));
		return -1;
	}
	return val;
}
#endif

size_t I_GetFreeMem(size_t *total)
{
#ifdef FREEBSD
	u_int v_free_count, v_page_size, v_page_count;
	size_t size = sizeof(v_free_count);
	sysctlbyname("vm.stats.vm.v_free_count", &v_free_count, &size, NULL, 0);
	size = sizeof(v_page_size);
	sysctlbyname("vm.stats.vm.v_page_size", &v_page_size, &size, NULL, 0);
	size = sizeof(v_page_count);
	sysctlbyname("vm.stats.vm.v_page_count", &v_page_count, &size, NULL, 0);

	if (total)
		*total = v_page_count * v_page_size;
	return v_free_count * v_page_size;
#elif defined (SOLARIS)
	 
	if (total)
		*total = 32 << 20;
	return 32 << 20;
#elif defined (_WIN32)
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if (total)
		*total = (size_t)info.dwTotalPhys;
	return (size_t)info.dwAvailPhys;
#elif defined (__linux__)
	 
	char buf[1024];
	char *memTag;
	size_t freeKBytes;
	size_t totalKBytes;
	INT32 n;
	INT32 meminfo_fd = -1;
	long Cached;
	long MemFree;
	long Buffers;
	long Shmem;
	long MemAvailable = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if (n < 0)
	{

		if (total)
			*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if ((memTag = strstr(buf, MEMTOTAL)) == NULL)
	{

		if (total)
			*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMTOTAL);
	totalKBytes = (size_t)atoi(memTag);

	if ((memTag = strstr(buf, MEMAVAILABLE)) == NULL)
	{
		Cached = get_entry(CACHED, buf);
		MemFree = get_entry(MEMFREE, buf);
		Buffers = get_entry(BUFFERS, buf);
		Shmem = get_entry(SHMEM, buf);
		MemAvailable = Cached + MemFree + Buffers - Shmem;

		if (MemAvailable == -1)
		{

			if (total)
				*total = 0L;
			return 0;
		}
		freeKBytes = MemAvailable;
	}
	else
	{
		memTag += sizeof (MEMAVAILABLE);
		freeKBytes = atoi(memTag);
	}

	if (total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#else

	if (total)
		*total = 48<<20;
	return 48<<20;
#endif
}

void I_Sleep(UINT32 ms)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	struct timespec ts = {
		.tv_sec = ms / 1000,
		.tv_nsec = ms % 1000 * 1000000,
	};
	int status;
	do status = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts);
	while (status == EINTR);
#elif defined (_WIN32)
	Sleep(ms);
#else
	(void)ms;
#warning No sleep function for this system!
#endif
}

void I_SleepDuration(precise_t duration)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__HAIKU__)
	UINT64 precision = I_GetPrecisePrecision();
	precise_t dest = I_GetPreciseTime() + duration;
	precise_t slack = (precision / 5000);  
	if (duration > slack)
	{
		duration -= slack;
		struct timespec ts = {
			.tv_sec = duration / precision,
			.tv_nsec = duration * 1000000000 / precision % 1000000000,
		};
		int status;
		do status = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts);
		while (status == EINTR);
	}


	while (((INT64)dest - (INT64)I_GetPreciseTime()) > 0);
#else
	UINT64 precision = I_GetPrecisePrecision();
	INT32 sleepvalue = cv_sleep.value;
	UINT64 delaygranularity;
	precise_t cur;
	precise_t dest;

	{
		double gran = round(((double)(precision / 1000) * sleepvalue * MIN_SLEEP_DURATION_MS));
		delaygranularity = (UINT64)gran;
	}

	cur = I_GetPreciseTime();
	dest = cur + duration;





	while ((INT64)(dest - cur) > 0)
	{


		if (sleepvalue > 0 && (dest - cur) > delaygranularity)
		{
			I_Sleep(sleepvalue);
		}



		cur = I_GetPreciseTime();
	}
#endif
}

precise_t I_GetPreciseTime(void)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (precise_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
#elif defined (_WIN32)
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (precise_t)counter.QuadPart;
#else
	return 0;
#endif
}

UINT64 I_GetPrecisePrecision(void)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	return 1000000000;
#elif defined (_WIN32)
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (UINT64)frequency.QuadPart;
#else
	return 1000000;
#endif
}

void I_GetEvent(void){}

static ticcmd_t emptycmd;

ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

ticcmd_t *I_BaseTiccmd2(void)
{

	return NULL;
}

FUNCNORETURN static void I_QuitStatus(int status)
{
	if (is_quitting)
		abort();

	is_quitting = true;
	M_SaveConfig(NULL);  
	D_SaveBan();  
	G_SaveGameData(clientGamedata);  




	if (demorecording)
		G_CheckDemoStatus();
	if (metalrecording)
		G_StopMetalRecording(false);

	D_QuitNetGame();
	CL_AbortDownloadResume();
	M_FreePlayerSetupColors();
	I_ShutdownSystem();
	W_Shutdown();
	exit(status);
}

void I_Quit(void)
{
	I_QuitStatus(0);
}

void I_Error(const char *error, ...)
{
	va_list argptr;
	char *buffer;
	size_t buflen;


	va_start(argptr, error);
	buflen = vsnprintf(NULL, 0, error, argptr);
	va_end(argptr);



	buffer = malloc(buflen+1);
	va_start(argptr, error);
	vsprintf(buffer, error, argptr);
	va_end(argptr);
	I_OutputMsg("\nI_Error(): %s\n", buffer);
	free(buffer);


	I_QuitStatus(-1);
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void){}

void I_InitJoystick2(void){}

INT32 I_NumJoys(void)
{
	return 0;
}

const char *I_GetJoyName(INT32 joyindex)
{
	(void)joyindex;
	return NULL;
}

#ifndef NOMUMBLE
#ifdef HAVE_MUMBLE





#define DEG2RAD (0.017453292519943295769236907684883l)  
#define MUMBLEUNIT (64.0f)  

static struct {
	UINT32 uiVersion;
#ifdef WINMUMBLE
	DWORD uiTick;
#else
	UINT32 uiTick;
#endif
	float fAvatarPosition[3];
	float fAvatarFront[3];
	float fAvatarTop[3];  
	wchar_t name[256];  
	float fCameraPosition[3];
	float fCameraFront[3];
	float fCameraTop[3];  
	wchar_t identity[256];  
	UINT32 context_len;
	unsigned char context[256];  
	wchar_t description[2048];  
} *mumble = NULL;
#endif  

static void I_SetupMumble(void)
{
#ifdef WINMUMBLE
	HANDLE hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (!hMap)
		return;

	mumble = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*mumble));
	if (!mumble)
		CloseHandle(hMap);
#elif defined (HAVE_SHM)
	int shmfd;
	char memname[256];

	snprintf(memname, 256, "/MumbleLink.%d", getuid());
	shmfd = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);

	if(shmfd < 0)
		return;

	mumble = mmap(NULL, sizeof(*mumble), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (mumble == MAP_FAILED)
		mumble = NULL;
#endif
}
#undef WINMUMBLE
#endif  

void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
#ifdef HAVE_MUMBLE



	double angle;
	fixed_t anglef;

	if (!mumble)
		return;

	if(mumble->uiVersion != 2) {
		wcsncpy(mumble->name, L"SRB2 "VERSIONSTRINGW, 256);
		wcsncpy(mumble->description, L"Sonic Robo Blast 2 with integrated Mumble Link support.", 2048);
		mumble->uiVersion = 2;
	}
	mumble->uiTick++;

	if (!netgame || gamestate != GS_LEVEL) {  
		mumble->fAvatarPosition[0] = mumble->fAvatarPosition[1] = mumble->fAvatarPosition[2] = 0.0f;
		mumble->fAvatarFront[0] = 1.0f;
		mumble->fAvatarFront[1] = mumble->fAvatarFront[2] = 0.0f;
		mumble->fCameraPosition[0] = mumble->fCameraPosition[1] = mumble->fCameraPosition[2] = 0.0f;
		mumble->fCameraFront[0] = 1.0f;
		mumble->fCameraFront[1] = mumble->fCameraFront[2] = 0.0f;
		return;
	}

	{
		UINT8 *p = mumble->context;
		WRITEMEM(p, server_context, 8);
		WRITEINT16(p, gamemap);
		mumble->context_len = (UINT32)(p - mumble->context);
	}

	if (mobj) {
		mumble->fAvatarPosition[0] = FIXED_TO_FLOAT(mobj->x) / MUMBLEUNIT;
		mumble->fAvatarPosition[1] = FIXED_TO_FLOAT(mobj->z) / MUMBLEUNIT;
		mumble->fAvatarPosition[2] = FIXED_TO_FLOAT(mobj->y) / MUMBLEUNIT;

		anglef = AngleFixed(mobj->angle);
		angle = FIXED_TO_FLOAT(anglef)*DEG2RAD;
		mumble->fAvatarFront[0] = (float)cos(angle);
		mumble->fAvatarFront[1] = 0.0f;
		mumble->fAvatarFront[2] = (float)sin(angle);
	} else {
		mumble->fAvatarPosition[0] = mumble->fAvatarPosition[1] = mumble->fAvatarPosition[2] = 0.0f;
		mumble->fAvatarFront[0] = 1.0f;
		mumble->fAvatarFront[1] = mumble->fAvatarFront[2] = 0.0f;
	}

	mumble->fCameraPosition[0] = FIXED_TO_FLOAT(listener.x) / MUMBLEUNIT;
	mumble->fCameraPosition[1] = FIXED_TO_FLOAT(listener.z) / MUMBLEUNIT;
	mumble->fCameraPosition[2] = FIXED_TO_FLOAT(listener.y) / MUMBLEUNIT;

	anglef = AngleFixed(listener.angle);
	angle = FIXED_TO_FLOAT(anglef)*DEG2RAD;
	mumble->fCameraFront[0] = (float)cos(angle);
	mumble->fCameraFront[1] = 0.0f;
	mumble->fCameraFront[2] = (float)sin(angle);
#else
	(void)mobj;
	(void)listener;
#endif  
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

INT32 I_GetKey(void)
{
	return 0;
}

void I_StartupTimer(void){}

void I_AddExitFunc(void (*func)())
{
	I_Assert(num_exit_funcs < sizeof(exit_funcs) / sizeof(exit_funcs[0]));
	exit_funcs[num_exit_funcs++] = func;
}

void I_RemoveExitFunc(void (*func)())
{

	(void)func;
}

#ifdef HAVE_TERMIOS
typedef struct
{
	size_t cursor;
	char buffer[256];
} feild_t;

static feild_t tty_con;



static boolean ttycon_ateol = true;


static INT32 tty_erase;
static INT32 tty_eof;

static struct termios tty_tc;









#if 0
static inline void tty_FlushIn(void)
{
	char key;
	while (read(STDIN_FILENO, &key, 1)!=-1);
}
#endif








static void tty_Back(void)
{
	write(STDOUT_FILENO, "\r", 1);
	if (tty_con.cursor>0)
	{
		write(STDOUT_FILENO, tty_con.buffer, tty_con.cursor);
	}
	write(STDOUT_FILENO, " \b", 2);
}

static void tty_Clear(void)
{
	size_t i;
	write(STDOUT_FILENO, "\r", 1);
	if (tty_con.cursor>0)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			write(STDOUT_FILENO, " ", 1);
		}
		write(STDOUT_FILENO, "\r", 1);
	}
}


static void I_ShutdownConsole(void)
{
	if (consolevent)
	{
		I_OutputMsg("Shutdown tty console\n");
		consolevent = false;
		tcsetattr (STDIN_FILENO, TCSADRAIN, &tty_tc);
	}
}

static void I_StartupConsole(void)
{
	struct termios tc;




	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	consolevent = !M_CheckParm("-noconsole");
	framebuffer = M_CheckParm("-framebuffer");

	if (framebuffer)
		consolevent = false;

	if (!consolevent) return;

	if (isatty(STDIN_FILENO)!=1)
	{
		I_OutputMsg("stdin is not a tty, tty console mode failed\n");
		consolevent = false;
		return;
	}
	memset(&tty_con, 0x00, sizeof(tty_con));
	tcgetattr (0, &tty_tc);
	tty_erase = tty_tc.c_cc[VERASE];
	tty_eof = tty_tc.c_cc[VEOF];
	tc = tty_tc;
	  
                                   
                                                             
                                                      
                                            
                                                           
                                                        
   
	tc.c_lflag &= ~(ECHO | ICANON);
	  
                        
                                    
    
	tc.c_iflag &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN] = 0;  
	tc.c_cc[VTIME] = 0;
	tcsetattr (0, TCSADRAIN, &tc);
}

static void I_GetConsoleEvents(void)
{

	event_t ev = {0};
	char key = 0;
	struct pollfd pfd =
	{
		.fd = STDIN_FILENO,
		.events = POLLIN,
		.revents = 0,
	};

	if (!consolevent)
		return;

	for (;;)
	{
		if (poll(&pfd, 1, 0) < 1 || !(pfd.revents & POLLIN))
			return;

		ev.type = ev_console;
		ev.key = 0;
		if (read(STDIN_FILENO, &key, 1) == -1 || !key)
			return;




		if ((key == tty_erase) || (key == 127) || (key == 8))
		{
			if (tty_con.cursor > 0)
			{
				tty_con.cursor--;
				tty_con.buffer[tty_con.cursor] = '\0';
				tty_Back();
			}
			ev.key = KEY_BACKSPACE;
		}
		else if (key < ' ')  
		{
			if (key == '\n')
			{
				tty_Clear();
				tty_con.cursor = 0;
				ev.key = KEY_ENTER;
			}
			else if (key == 0x4)  
			{

				I_Quit();
			}
			else continue;
		}
		else if (tty_con.cursor < sizeof(tty_con.buffer))
		{

			ev.key = tty_con.buffer[tty_con.cursor] = key;
			tty_con.cursor++;

			write(STDOUT_FILENO, &key, 1);
		}
		if (ev.key) D_PostEvent(&ev);

	}
}

#elif defined (_WIN32)
static BOOL I_ReadyConsole(HANDLE ci)
{
	DWORD gotinput;
	if (ci == INVALID_HANDLE_VALUE) return FALSE;
	if (WaitForSingleObject(ci,0) != WAIT_OBJECT_0) return FALSE;
	if (GetFileType(ci) != FILE_TYPE_CHAR) return FALSE;
	if (!GetConsoleMode(ci, &gotinput)) return FALSE;
	return (GetNumberOfConsoleInputEvents(ci, &gotinput) && gotinput);
}

static boolean entering_con_command = false;

static void Impl_HandleKeyboardConsoleEvent(KEY_EVENT_RECORD evt, HANDLE co)
{
	event_t event;
	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	DWORD t;

	memset(&event,0x00,sizeof (event));

	if (evt.bKeyDown)
	{
		event.type = ev_console;
		entering_con_command = true;
		switch (evt.wVirtualKeyCode)
		{
			case VK_ESCAPE:
			case VK_TAB:
				event.key = KEY_NULL;
				break;
			case VK_RETURN:
				entering_con_command = false;
				 
			default:

				event.key = evt.uChar.AsciiChar;
		}
		if (co != INVALID_HANDLE_VALUE && GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &t))
		{
			if (event.key && event.key != KEY_LSHIFT && event.key != KEY_RSHIFT)
			{
#ifdef _UNICODE
				WriteConsole(co, &evt.uChar.UnicodeChar, 1, &t, NULL);
#else
				WriteConsole(co, &evt.uChar.AsciiChar, 1 , &t, NULL);
#endif
			}
			if (evt.wVirtualKeyCode == VK_BACK
				&& GetConsoleScreenBufferInfo(co,&CSBI))
			{
				WriteConsoleOutputCharacterA(co, " ",1, CSBI.dwCursorPosition, &t);
			}
		}
	}
	if (event.key) D_PostEvent(&event);
}

static void I_GetConsoleEvents(void)
{
	HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	INPUT_RECORD input;
	DWORD t;

	while (I_ReadyConsole(ci) && ReadConsoleInput(ci, &input, 1, &t) && t)
	{
		switch (input.EventType)
		{
			case KEY_EVENT:
				Impl_HandleKeyboardConsoleEvent(input.Event.KeyEvent, co);
				break;
			case MOUSE_EVENT:
			case WINDOW_BUFFER_SIZE_EVENT:
			case MENU_EVENT:
			case FOCUS_EVENT:
				break;
		}
	}
}

static void I_StartupConsole(void)
{
	HANDLE ci, co;
	BOOL gotConsole = AllocConsole();
	consolevent = !M_CheckParm("-noconsole");
	if (gotConsole)
	{
		SetConsoleTitleA("SRB2 Console");
		consolevent = true;
	}


	ci = CreateFile(TEXT("CONIN$") ,               GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	co = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ci != INVALID_HANDLE_VALUE)
	{
		const DWORD CM = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT;
		SetStdHandle(STD_INPUT_HANDLE, ci);
		if (GetFileType(ci) == FILE_TYPE_CHAR)
			SetConsoleMode(ci, CM);  
	}
	if (co != INVALID_HANDLE_VALUE)
	{
		SetStdHandle(STD_OUTPUT_HANDLE, co);
		SetStdHandle(STD_ERROR_HANDLE, co);
	}
}
static inline void I_ShutdownConsole(void){}
#else
static void I_GetConsoleEvents(void){}
static inline void I_StartupConsole(void)
{
	consolevent = !M_CheckParm("-noconsole");
	framebuffer = M_CheckParm("-framebuffer");

	if (framebuffer)
		consolevent = false;
}
static inline void I_ShutdownConsole(void){}
#endif

void I_OutputMsg(const char *fmt, ...)
{
	size_t len;
	char *txt;
	va_list argptr;

	va_start(argptr,fmt);
	len = vsnprintf(NULL, 0, fmt, argptr);
	va_end(argptr);
	if (len == 0)
		return;

	txt = malloc(len+1);
	va_start(argptr,fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

#if defined (_WIN32) && defined (_MSC_VER)
	OutputDebugStringA(txt);
#endif

#ifdef LOGMESSAGES
	if (logstream)
	{
		fwrite(txt, len, 1, logstream);
		fflush(logstream);
	}
#endif

#if defined (_WIN32)
#ifdef DEBUGFILE
	if (debugfile != stderr)
#endif
	{
		HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD bytesWritten;

		if (co == INVALID_HANDLE_VALUE)
		{
			free(txt);
			return;
		}

		if (GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &bytesWritten))
		{
			static COORD coordNextWrite = {0,0};
			LPVOID oldLines = NULL;
			INT oldLength;
			CONSOLE_SCREEN_BUFFER_INFO csbi;


			GetConsoleScreenBufferInfo(co, &csbi);
			oldLength = csbi.dwSize.X * (csbi.dwCursorPosition.Y - coordNextWrite.Y) + csbi.dwCursorPosition.X - coordNextWrite.X;

			if (oldLength > 0)
			{
				LPVOID blank = malloc(oldLength);
				if (!blank)
				{
					free(txt);
					return;
				}
				memset(blank, ' ', oldLength);  
				oldLines = malloc(oldLength*sizeof(TCHAR));
				if (!oldLines)
				{
					free(blank);
					free(txt);
					return;
				}

				ReadConsoleOutputCharacter(co, oldLines, oldLength, coordNextWrite, &bytesWritten);



				SetConsoleCursorPosition(co, coordNextWrite);

				WriteConsoleA(co, blank, oldLength, &bytesWritten, NULL);
				free(blank);


				SetConsoleCursorPosition(co, coordNextWrite);
			}


			WriteConsoleA(co, txt, (DWORD)len, &bytesWritten, NULL);


			GetConsoleScreenBufferInfo(co, &csbi);
			coordNextWrite = csbi.dwCursorPosition;


			if (oldLines && entering_con_command)
				WriteConsole(co, oldLines, oldLength, &bytesWritten, NULL);
			if (oldLines) free(oldLines);
		}
		else  
			WriteFile(co, txt, (DWORD)len, &bytesWritten, NULL);
	}
#else
#ifdef HAVE_TERMIOS
	if (consolevent && ttycon_ateol)
	{
		tty_Clear();
		ttycon_ateol = false;
	}
#endif

	if (!framebuffer)
		fprintf(stderr, "%s", txt);
#ifdef HAVE_TERMIOS
	if (consolevent && txt[len-1] == '\n')
	{
		write(STDOUT_FILENO, tty_con.buffer, tty_con.cursor);
		ttycon_ateol = true;
	}
#endif


	if (!framebuffer)
		fflush(stderr);

#endif
	free(txt);
}

void I_OsPolling(void)
{
	if (consolevent)
		I_GetConsoleEvents();
}

FUNCNORETURN static ATTRNORETURN void quit_handler(int num)
{
	(void)num;


	I_Quit();
}

static void I_RegisterSignals (void)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)

	struct sigaction sig;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;
	sig.sa_handler = quit_handler;
	sigaction(SIGINT, &sig, NULL);
	sig.sa_handler = quit_handler;
	sigaction(SIGTERM, &sig, NULL);
#else
	signal(SIGINT, quit_handler);
	signal(SIGBREAK, quit_handler);
	signal(SIGTERM, quit_handler);
#endif
}

INT32 I_StartupSystem(void)
{
#ifdef HAVE_THREADS
	I_start_threads();
	I_AddExitFunc(I_stop_threads);
#endif
	I_StartupConsole();
	I_RegisterSignals();
#ifndef NOMUMBLE
	I_SetupMumble();
#endif
	return 0;
}

void I_ShutdownSystem(void)
{
	INT32 c;

	I_ShutdownConsole();

	for (c = MAX_QUIT_FUNCS-1; c >= 0; c--)
		if (exit_funcs[c])
			(*exit_funcs[c])();
#ifdef LOGMESSAGES
	if (logstream)
	{
		I_OutputMsg("I_ShutdownSystem(): end of logstream.\n");
		fclose(logstream);
		logstream = NULL;
	}
#endif
}

void I_GetDiskFreeSpace(INT64* freespace)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	struct statvfs stfs;
	if (statvfs(srb2home, &stfs) == -1)
	{
		*freespace = INT32_MAX;
		return;
	}
	*freespace = stfs.f_bavail * stfs.f_bsize;
#elif defined (_WIN32)
	static p_GetDiskFreeSpaceExA pfnGetDiskFreeSpaceEx = NULL;
	static boolean testwin95 = false;
	ULARGE_INTEGER usedbytes, lfreespace;

	if (!testwin95)
	{
		pfnGetDiskFreeSpaceEx = (p_GetDiskFreeSpaceExA)(LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA");
		testwin95 = true;
	}
	if (pfnGetDiskFreeSpaceEx)
	{
		if (pfnGetDiskFreeSpaceEx(srb2home, &lfreespace, &usedbytes, NULL))
			*freespace = lfreespace.QuadPart;
		else
			*freespace = INT32_MAX;
	}
	else
	{
		DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
		GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
						 &NumberOfFreeClusters, &TotalNumberOfClusters);
		*freespace = BytesPerSector*SectorsPerCluster*NumberOfFreeClusters;
	}
#else  
	*freespace = 1024*1024*1024;
#endif
}

char *I_GetUserName(void)
{
	static char username[MAXPLAYERNAME+1];
	char *p;
#ifdef _WIN32
	DWORD i = MAXPLAYERNAME;

	if (!GetUserNameA(username, &i))
#endif
	{
		p = I_GetEnv("USER");
		if (!p)
		{
			p = I_GetEnv("user");
			if (!p)
			{
				p = I_GetEnv("USERNAME");
				if (!p)
				{
					p = I_GetEnv("username");
					if (!p)
					{
						return NULL;
					}
				}
			}
		}
		strncpy(username, p, MAXPLAYERNAME);
	}


	if (strcmp(username, "") != 0)
		return username;
	return NULL;  
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{

#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (__CYGWIN__)
	return mkdir(dirname, unixright);
#elif defined (_WIN32)
	UNREFERENCED_PARAMETER(unixright);  
	return CreateDirectoryA(dirname, NULL);
#else
	(void)dirname;
	(void)unixright;
	return false;
#endif
}

const CPUInfoFlags *I_CPUInfo(void)
{
	return NULL;
}

static boolean isWadPathOk(const char *path)
{
	char *wad3path = malloc(256);

	if (!wad3path)
		return false;

	sprintf(wad3path, pandf, path, WADKEYWORD1);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	free(wad3path);
	return false;
}

static void pathonly(char *s)
{
	size_t j;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			if (s[j] == ':') s[j+1] = 0;
			else s[j] = 0;
			return;
		}
}

static const char *searchWad(const char *searchDir)
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, WADKEYWORD1);
	fstemp = filesearch(tempsw,searchDir,NULL,true,20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}

	return NULL;
}

#define CHECKWADPATH(ret) \
do { \
	I_OutputMsg(",%s", ret); \
	if (isWadPathOk(ret)) \
		return ret; \
} while (0)

#define SEARCHWAD(str) \
do { \
	WadPath = searchWad(str); \
	if (WadPath) \
		return WadPath; \
} while (0)

static const char *locateWad(void)
{
	const char *envstr;
	const char *WadPath;
	int i;

	I_OutputMsg("SRB2WADDIR");

	if (((envstr = I_GetEnv("SRB2WADDIR")) != NULL) && isWadPathOk(envstr))
		return envstr;

#ifndef NOCWD

	strcpy(returnWadPath, ".");
	I_OutputMsg(",%s", returnWadPath);
	if (isWadPathOk(returnWadPath))
		return NULL;
#endif

#ifdef __APPLE__
	OSX_GetResourcesPath(returnWadPath);
	CHECKWADPATH(returnWadPath);
#endif


	for (i = 0; wadDefaultPaths[i]; i++)
	{
		strcpy(returnWadPath, wadDefaultPaths[i]);
		CHECKWADPATH(returnWadPath);
	}

#ifndef NOHOME

	if ((envstr = I_GetEnv("HOME")) != NULL)
	{
		char *tmp = malloc(strlen(envstr) + 1 + sizeof(DEFAULTDIR));
		strcpy(tmp, envstr);
		strcat(tmp, "/");
		strcat(tmp, DEFAULTDIR);
		CHECKWADPATH(tmp);
		free(tmp);
	}
#endif


	for (i = 0; wadSearchPaths[i]; i++)
	{
		I_OutputMsg(", in:%s", wadSearchPaths[i]);
		SEARCHWAD(wadSearchPaths[i]);
	}


	return NULL;
}

const char *I_LocateWad(void)
{
	const char *waddir;

	I_OutputMsg("Looking for WADs in: ");

	waddir = locateWad();
	I_OutputMsg("\n");

	if (waddir)
	{

#if defined (_WIN32)
		waddir = _fullpath(NULL, waddir, MAX_PATH);
		SetCurrentDirectoryA(waddir);
#else
		waddir = realpath(waddir, NULL);
		if (chdir(waddir) == -1)
			I_OutputMsg("Couldn't change working directory\n");
#endif
	}
	return waddir;
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

void I_UpdateMouseGrab(void){}

char *I_GetEnv(const char *name)
{
	return getenv(name);
}

INT32 I_PutEnv(char *name)
{
	return putenv(name);
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	return NULL;
}

size_t I_GetRandomBytes(char *destination, size_t count)
{
#if defined (__unix__) || defined (UNIXCOMMON) || defined(__APPLE__)
	FILE *rndsource;
	size_t actual_bytes;

	if (!(rndsource = fopen("/dev/urandom", "r")))
		if (!(rndsource = fopen("/dev/random", "r")))
			actual_bytes = 0;

	if (rndsource)
	{
		actual_bytes = fread(destination, 1, count, rndsource);
		fclose(rndsource);
	}

	if (actual_bytes == 0)
		I_OutputMsg("I_GetRandomBytes(): couldn't get any random bytes");

	return actual_bytes;
#elif defined (_WIN32)
	if (RtlGenRandom(destination, count))
		return count;

	I_OutputMsg("I_GetRandomBytes(): couldn't get any random bytes");
	return 0;
#else
	#warning SDL I_GetRandomBytes is not implemented on this platform.
	return 0;
#endif
}

void I_RegisterSysCommands(void){}

char const *I_GetSysName(void)
{

#if defined(_WIN32) || defined(__CYGWIN__)
	return "Windows";
#elif defined(__APPLE__)
	return "Mac OS";
#elif defined(__linux__)
	return "Linux";
#elif defined(__FreeBSD__)
	return "FreeBSD";
#elif defined(__OpenBSD__)
	return "OpenBSD";
#elif defined(__NetBSD__)
	return "NetBSD";
#elif defined(__DragonFly__)
	return "DragonFly BSD";
#elif defined(__gnu_hurd__)
	return "GNU Hurd";  
#elif defined(__hpux)
	return "HP-UX";
#elif defined(EPLAN9)
	return "Plan 9";
#elif defined(__HAIKU__)
	return "Haiku";
#elif defined(__BEOS__)
	return "BeOS";
#elif defined(__minix)
	return "Minix";
#elif defined(__sun)
#if defined(__SVR4) || defined(__srv4__)
	return "Solaris";  
#else
	return "SunOS";
#endif
#elif defined(_AIX)
	return "AIX";
#elif defined(__SYLLABLE__)
	return "SyllableOS";  
#else
	return "Unknown";
#endif
}

void I_GetCursorPosition(INT32 *x, INT32 *y)
{
	(void)x;
	(void)y;
}

void I_SetTextInputMode(boolean active)
{
	(void)active;
}

boolean I_GetTextInputMode(void)
{
	return false;
}

#include "../sdl/dosstr.c"
