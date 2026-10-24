// Emacs style mode select   -*- C++ -*-
//
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
#ifdef _WIN64
#include <dbghelp.h>
#endif
#include "../doomtype.h"
typedef BOOL (WINAPI *p_GetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *p_IsProcessorFeaturePresent) (DWORD);
typedef DWORD (WINAPI *p_timeGetTime) (void);
typedef UINT (WINAPI *p_timeEndPeriod) (UINT);
typedef HANDLE (WINAPI *p_OpenFileMappingA) (DWORD, BOOL, LPCSTR);
typedef LPVOID (WINAPI *p_MapViewOfFile) (HANDLE, DWORD, DWORD, DWORD, SIZE_T);
typedef HANDLE (WINAPI *p_CreateWaitableTimerExW)(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif


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

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED
#include "SDL.h"

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "SDL_cpuinfo.h"
#define HAVE_SDLCPUINFO

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
#include "../netcode/d_clisrv.h"
#include "../netcode/commands.h"
#include "../g_game.h"
#include "../filesrch.h"
#include "endtxt.h"
#include "sdlmain.h"

#include "../i_joy.h"

#include "../m_argv.h"

#include "../r_main.h"  
#include "../r_fps.h"
#include "../m_perfstats.h"

#ifdef MAC_ALERT
#include "macosx/mac_alert.h"
#endif

#include "../d_main.h"

#ifdef _WIN32
#define COMPANION_CONTROL_MAGIC 0x32425253UL

typedef struct
{
	DWORD magic;
	DWORD parentpid;
	volatile LONG stop_requested;
} companion_control_t;

static HANDLE companion_process;
static HANDLE companion_control_mapping;
static companion_control_t *companion_control;
static HANDLE companion_child_mapping;
static const companion_control_t *companion_child_control;
static HANDLE companion_parent_process;
static boolean companion_child_checked;
static boolean companion_parent_missing;
static boolean companion_exit_registered;

static void I_CloseCompanionControl(void)
{
	if (companion_control)
	{
		UnmapViewOfFile(companion_control);
		companion_control = NULL;
	}

	if (companion_control_mapping)
	{
		CloseHandle(companion_control_mapping);
		companion_control_mapping = NULL;
	}
}

boolean I_CompanionServerRunning(void)
{
	DWORD exitcode;

	if (!companion_process)
		return false;

	if (!GetExitCodeProcess(companion_process, &exitcode) || exitcode != STILL_ACTIVE)
	{
		CloseHandle(companion_process);
		companion_process = NULL;
		I_CloseCompanionControl();
		return false;
	}

	return true;
}

void I_StopCompanionServer(void)
{
	if (companion_process)
	{
		DWORD waitresult;

		if (companion_control)
			InterlockedExchange(&companion_control->stop_requested, 1);

		waitresult = WaitForSingleObject(companion_process, 5000);
		if (waitresult == WAIT_TIMEOUT)
		{
			I_OutputMsg("Local dedicated companion did not stop in time; terminating it.\n");
			TerminateProcess(companion_process, 0);
			WaitForSingleObject(companion_process, 1000);
		}

		CloseHandle(companion_process);
		companion_process = NULL;
	}

	I_CloseCompanionControl();
}

boolean I_StartCompanionServer(const char *responsefile, const char *workingdir)
{
	SECURITY_ATTRIBUTES security;
	STARTUPINFOA startup;
	PROCESS_INFORMATION process;
	char executable[MAX_PATH];
	char commandline[2048];
	DWORD parentpid = GetCurrentProcessId();

	if (!responsefile || !workingdir)
		return false;

	if (I_CompanionServerRunning())
		I_StopCompanionServer();

	memset(&security, 0, sizeof security);
	security.nLength = sizeof security;
	security.bInheritHandle = TRUE;
	companion_control_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE,
		&security, PAGE_READWRITE, 0, sizeof (*companion_control), NULL);
	if (!companion_control_mapping)
	{
		I_OutputMsg("Could not create the local dedicated control block (%lu).\n",
			(unsigned long)GetLastError());
		return false;
	}
	companion_control = (companion_control_t *)MapViewOfFile(
		companion_control_mapping, FILE_MAP_ALL_ACCESS, 0, 0,
		sizeof (*companion_control));
	if (!companion_control)
	{
		I_OutputMsg("Could not map the local dedicated control block (%lu).\n",
			(unsigned long)GetLastError());
		I_CloseCompanionControl();
		return false;
	}
	companion_control->magic = COMPANION_CONTROL_MAGIC;
	companion_control->parentpid = parentpid;
	companion_control->stop_requested = 0;

	if (!GetModuleFileNameA(NULL, executable, sizeof executable))
	{
		I_OutputMsg("Could not locate the SRB2 executable for local dedicated hosting.\n");
		I_CloseCompanionControl();
		return false;
	}

	snprintf(commandline, sizeof commandline,
		"\"%s\" \"@%s\" -companioncontrol %p -companionparent %lu",
		executable, responsefile, (void *)companion_control_mapping,
		(unsigned long)parentpid);

	memset(&startup, 0, sizeof startup);
	startup.cb = sizeof startup;
	memset(&process, 0, sizeof process);

	if (!CreateProcessA(executable, commandline, NULL, NULL, TRUE,
		CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP, NULL, workingdir,
		&startup, &process))
	{
		I_OutputMsg("Could not start the local dedicated companion (%lu).\n",
			(unsigned long)GetLastError());
		I_CloseCompanionControl();
		return false;
	}

	CloseHandle(process.hThread);
	companion_process = process.hProcess;
	if (!companion_exit_registered)
	{
		I_AddExitFunc(I_StopCompanionServer);
		companion_exit_registered = true;
	}

	I_OutputMsg("Local dedicated companion started (PID %lu).\n",
		(unsigned long)process.dwProcessId);
	return true;
}

boolean I_CompanionShutdownRequested(void)
{
	if (!companion_child_checked)
	{
		INT32 parm;

		companion_child_checked = true;

		parm = M_CheckParm("-companioncontrol");
		if (parm && M_IsNextParm())
		{
			const unsigned long long handlevalue = _strtoui64(M_GetNextParm(), NULL, 16);
			companion_child_mapping = (HANDLE)(ULONG_PTR)handlevalue;
			companion_child_control = (const companion_control_t *)MapViewOfFile(
				companion_child_mapping, FILE_MAP_READ, 0, 0,
				sizeof (*companion_child_control));
		}

		parm = M_CheckParm("-companionparent");
		if (parm && M_IsNextParm())
		{
			DWORD parentpid = (DWORD)strtoul(M_GetNextParm(), NULL, 10);
			if (parentpid)
			{
				companion_parent_process = OpenProcess(SYNCHRONIZE, FALSE, parentpid);
				if (!companion_parent_process)
					companion_parent_missing = true;
			}
		}
	}

	if (companion_parent_missing)
	{
		I_OutputMsg("Companion controller process could not be opened.\n");
		return true;
	}

	if (companion_child_control
		&& companion_child_control->magic == COMPANION_CONTROL_MAGIC
		&& companion_child_control->parentpid != 0
		&& companion_child_control->stop_requested == 1)
	{
		I_OutputMsg("Companion controller requested a clean shutdown.\n");
		return true;
	}

	if (companion_parent_process
		&& WaitForSingleObject(companion_parent_process, 0) == WAIT_OBJECT_0)
	{
		I_OutputMsg("Companion controller process exited.\n");
		return true;
	}

	return false;
}
#else
boolean I_StartCompanionServer(const char *responsefile, const char *workingdir)
{
	(void)responsefile;
	(void)workingdir;
	I_OutputMsg("Local dedicated companion hosting is not available on this platform yet.\n");
	return false;
}

void I_StopCompanionServer(void) {}
boolean I_CompanionServerRunning(void) { return false; }
boolean I_CompanionShutdownRequested(void) { return false; }
#endif

#if defined(_WIN32) && !defined(DEDICATED)
static char crash_context[256] = "starting SRB2";
static char crash_last_addon[256] = "none loaded this session";
static volatile sig_atomic_t crash_context_writing;
#define CRASH_TIMELINE_SLOTS 8
static char crash_timeline[CRASH_TIMELINE_SLOTS][192];
static UINT8 crash_timeline_next;
static UINT8 crash_timeline_count;
static precise_t diagnostic_level_transition_start;
static boolean diagnostic_level_transition_active;
static void I_AppendDiagnostic(const char *filename, const char *text, DWORD length);

static void I_GetCrashDirectory(char *directory, size_t directory_size)
{
	const char *home = *srb2home ? srb2home : ".";
	DWORD attributes;
	DWORD create_error;

	if (!directory || !directory_size)
		return;
	snprintf(directory, directory_size, "%s" PATHSEP "crashes", home);
	directory[directory_size - 1] = '\0';
	if (!CreateDirectoryA(directory, NULL))
	{
		create_error = GetLastError();
		attributes = GetFileAttributesA(directory);
		if (create_error != ERROR_ALREADY_EXISTS
		|| attributes == INVALID_FILE_ATTRIBUTES
		|| !(attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			                                                                     
                                                                              
			strncpy(directory, home, directory_size - 1);
			directory[directory_size - 1] = '\0';
		}
	}
}

static const char *I_CrashProcessMode(void)
{
	if (dedicated)
		return "dedicated server";
	if (netgame)
		return server ? "listen server" : "network client";
	if (multiplayer)
		return "local multiplayer";
	return "single player or menu";
}

void I_MarkLevelTransitionStart(precise_t started)
{
	diagnostic_level_transition_start = started;
	diagnostic_level_transition_active = true;
}

void I_MarkLevelTransitionEnd(void)
{
	diagnostic_level_transition_active = false;
}

void I_RecordDiagnosticHeartbeat(void)
{
	static tic_t previous;
	const tic_t now = I_GetTime();
	char *entry;

	if (previous && now < previous + TICRATE)
		return;
	previous = now;
	entry = crash_timeline[crash_timeline_next];
	snprintf(entry, sizeof crash_timeline[0],
		"uptime_tics=%u map=%d gametic=%u leveltime=%u state=%d renderer=%d "
		"players=%d net=%d operation=%.80s",
		(unsigned)now, gamemap, (unsigned)gametic, (unsigned)leveltime,
		gamestate, rendermode, D_NumPlayers(), netgame, crash_context);
	entry[sizeof crash_timeline[0] - 1] = '\0';
	crash_timeline_next = (crash_timeline_next + 1) % CRASH_TIMELINE_SLOTS;
	if (crash_timeline_count < CRASH_TIMELINE_SLOTS)
		crash_timeline_count++;
}

static void I_FormatCrashTimeline(char *buffer, size_t buffer_size)
{
	size_t used = 0;
	UINT8 first;
	UINT8 i;

	if (!buffer || !buffer_size)
		return;
	buffer[0] = '\0';
	first = (crash_timeline_next + CRASH_TIMELINE_SLOTS
		- crash_timeline_count) % CRASH_TIMELINE_SLOTS;
	for (i = 0; i < crash_timeline_count && used < buffer_size - 1; i++)
	{
		const char *entry = crash_timeline[(first + i) % CRASH_TIMELINE_SLOTS];
		int added = snprintf(buffer + used, buffer_size - used,
			"timeline_%u=%s\r\n", (unsigned)i, entry);
		if (added <= 0)
			break;
		used += min((size_t)added, buffer_size - used - 1);
	}
}

void I_SetCrashContext(const char *context)
{
	if (!context)
		return;
	strncpy(crash_context, context, sizeof crash_context - 1);
	crash_context[sizeof crash_context - 1] = '\0';
}

void I_SetCrashAddonContext(const char *filename)
{
	if (!filename)
		return;
	strncpy(crash_last_addon, filename, sizeof crash_last_addon - 1);
	crash_last_addon[sizeof crash_last_addon - 1] = '\0';
}

static void I_WriteCrashContext(INT32 signalnum)
{
	char directory[512];
	char path[768];
	char text[4096];
	char timeline[1800];
	int length;
	DWORD written;
	HANDLE file;
	SYSTEMTIME now;

	if (crash_context_writing)
		return;
	crash_context_writing = 1;
	GetLocalTime(&now);
	I_FormatCrashTimeline(timeline, sizeof timeline);

	I_GetCrashDirectory(directory, sizeof directory);
	snprintf(path, sizeof path, "%s" PATHSEP "crash-context.txt", directory);
	path[sizeof path - 1] = '\0';
	length = snprintf(text, sizeof text,
		"SRB2 V5.52 crash context\r\n"
		"timestamp=%04u-%02u-%02u %02u:%02u:%02u.%03u\r\n"
		"build=%s branch=%s revision=%s compiled=%s %s\r\n"
		"signal=%d\r\nsigsegv_confirmed=%s\r\nlast_operation=%s\r\nlast_addon=%s\r\n"
		"map=%d\r\ngametic=%u\r\nleveltime=%u\r\ngamestate=%d\r\n"
		"renderer=%d\r\nresolution=%dx%d\r\n"
		"mode=%s\r\nmodifiedgame=%d\r\nnetgame=%d\r\nmultiplayer=%d\r\n"
		"server=%d\r\nplayers=%d\r\nnodes=%d\r\nprocess_bits=%u\r\n"
		"\r\nRecent one-second timeline (oldest to newest):\r\n%s"
		"\r\nKeep this file together with the matching .RPT, .dmp, and executable.\r\n"
		"A SIGSEGV/access violation cannot safely be ignored; prevention requires the failing stack and state.\r\n",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
		VERSIONSTRING, compbranch, comprevision, compdate, comptime,
		signalnum, signalnum == SIGSEGV ? "yes" : "no", crash_context, crash_last_addon,
		gamemap, (unsigned)gametic,
		(unsigned)leveltime, gamestate, rendermode, vid.width, vid.height,
		I_CrashProcessMode(), modifiedgame, netgame, multiplayer, server,
		D_NumPlayers(), D_NumNodes(false), (unsigned)(sizeof(void *) * 8), timeline);
	if (length < 0)
		return;
	if ((size_t)length >= sizeof text)
		length = sizeof text - 1;

	file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return;
	WriteFile(file, text, (DWORD)length, &written, NULL);
	FlushFileBuffers(file);
	CloseHandle(file);
}

#ifdef _WIN64
typedef BOOL (WINAPI *p_MiniDumpWriteDump)(HANDLE, DWORD, HANDLE,
	MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
	PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

static volatile LONG crash_exception_writing;

static const char *I_Win32ExceptionName(DWORD code)
{
	switch (code)
	{
		case EXCEPTION_ACCESS_VIOLATION: return "access violation (SIGSEGV)";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "misaligned data access";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "floating-point divide by zero";
		case EXCEPTION_FLT_INVALID_OPERATION: return "invalid floating-point operation";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
		case EXCEPTION_IN_PAGE_ERROR: return "in-page memory error";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
		case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
		default: return "unhandled Windows exception";
	}
}

static const char *I_Win32LikelyCause(const EXCEPTION_RECORD *record,
	ULONG_PTR modulebase)
{
	HMODULE mainmodule = GetModuleHandleA(NULL);
	ULONG_PTR access_type = 0;
	ULONG_PTR access_address = 0;

	if (!record)
		return "unknown; the Windows exception record was unavailable";

	switch (record->ExceptionCode)
	{
		case EXCEPTION_STACK_OVERFLOW:
			return "stack overflow, usually caused by runaway recursion or an excessively deep call chain";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "integer division by zero";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return "floating-point division by zero";
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return "invalid instruction, commonly corrupted code flow or unsupported generated code";
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return "misaligned memory access";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return "CPU-detected array bounds violation";
		case EXCEPTION_IN_PAGE_ERROR:
			return "Windows could not page required memory in; storage, mapped-file, or memory failure is possible";
		default:
			break;
	}

	if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
		return "unclassified native exception; use the exception code and minidump for the exact call stack";

	if (record->NumberParameters >= 2)
	{
		access_type = record->ExceptionInformation[0];
		access_address = record->ExceptionInformation[1];
	}
	if (access_type == 8)
		return "attempted to execute invalid memory, suggesting a bad function pointer or corrupted return address";
	if (access_address < 0x10000)
		return "near-null pointer access, suggesting a missing, freed, or not-yet-created object";
	if (modulebase && modulebase != (ULONG_PTR)mainmodule)
		return "faulting instruction is inside an external DLL or graphics/audio driver";
	if (strstr(crash_context, "renderer") || strstr(crash_context, "video mode"))
		return "invalid memory access while changing or using renderer state";
	if (strstr(crash_context, "add-on") || strstr(crash_context, "SOC"))
		return "invalid memory access during add-on or resource loading";
	if (strstr(crash_context, "Lua"))
		return "invalid memory access while running a Lua-related engine phase";
	if (access_type == 1)
		return "invalid write, commonly a stale pointer, freed object, or out-of-bounds destination";
	return "invalid read, commonly a stale pointer, freed object, or out-of-bounds source";
}




static LONG WINAPI I_Win32CrashException(PEXCEPTION_POINTERS exception)
{
	EXCEPTION_RECORD *record;
	CONTEXT *context;
	SYSTEMTIME now;
	MEMORY_BASIC_INFORMATION memory;
	MINIDUMP_EXCEPTION_INFORMATION dump_exception;
	p_MiniDumpWriteDump write_dump = NULL;
	FARPROC dump_proc = NULL;
	HMODULE dbghelp = NULL;
	HANDLE file;
	DWORD written;
	char directory[512];
	char reportpath[768];
	char dumppath[768];
	char modulepath[MAX_PATH] = "unknown";
	char report[8192];
	char history[1024];
	char timeline[1800];
	const char *access = "n/a";
	ULONG_PTR access_address = 0;
	ULONG_PTR modulebase = 0;
	BOOL dump_written = FALSE;
	DWORD dump_error = ERROR_SUCCESS;
	const char *fault_owner;
	const char *likely_cause;
	HMODULE mainmodule;
	int length;

	if (!exception || !exception->ExceptionRecord || !exception->ContextRecord)
		return EXCEPTION_CONTINUE_SEARCH;

	record = exception->ExceptionRecord;
	context = exception->ContextRecord;



	switch (record->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_STACK_OVERFLOW:
			break;
		default:
			return EXCEPTION_CONTINUE_SEARCH;
	}

	if (InterlockedCompareExchange(&crash_exception_writing, 1, 0) != 0)
		return EXCEPTION_CONTINUE_SEARCH;

	I_WriteCrashContext(record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ? SIGSEGV : SIGABRT);
	GetLocalTime(&now);
	I_FormatCrashTimeline(timeline, sizeof timeline);
	I_GetCrashDirectory(directory, sizeof directory);



	snprintf(reportpath, sizeof reportpath,
		"%s" PATHSEP "srb2win-crash.RPT", directory);
	snprintf(dumppath, sizeof dumppath,
		"%s" PATHSEP "srb2win-crash.dmp", directory);

	memset(&memory, 0, sizeof memory);
	if (VirtualQuery(record->ExceptionAddress, &memory, sizeof memory) == sizeof memory
	&& memory.Type == MEM_IMAGE)
	{
		modulebase = (ULONG_PTR)memory.AllocationBase;
		GetModuleFileNameA((HMODULE)memory.AllocationBase, modulepath, sizeof modulepath);
	}

	if ((record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
	|| record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
	&& record->NumberParameters >= 2)
	{
		access = record->ExceptionInformation[0] == 1 ? "write"
			: (record->ExceptionInformation[0] == 8 ? "execute" : "read");
		access_address = record->ExceptionInformation[1];
	}
	mainmodule = GetModuleHandleA(NULL);
	fault_owner = !modulebase ? "unknown or non-image memory"
		: (modulebase == (ULONG_PTR)mainmodule
			? "SRB2 executable" : "external DLL or driver");
	likely_cause = I_Win32LikelyCause(record, modulebase);

	dbghelp = LoadLibraryA("dbghelp.dll");
	if (dbghelp)
	{
		dump_proc = GetProcAddress(dbghelp, "MiniDumpWriteDump");
		memcpy(&write_dump, &dump_proc, sizeof write_dump);
	}
	else
		dump_error = GetLastError();
	if (dbghelp && !write_dump)
		dump_error = ERROR_PROC_NOT_FOUND;
	if (write_dump)
	{
		file = CreateFileA(dumppath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			dump_exception.ThreadId = GetCurrentThreadId();
			dump_exception.ExceptionPointers = exception;
			dump_exception.ClientPointers = FALSE;
			dump_written = write_dump(GetCurrentProcess(), GetCurrentProcessId(), file,
				(MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs
				| MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo
				| MiniDumpWithUnloadedModules), &dump_exception, NULL, NULL);
			if (!dump_written)
				dump_error = GetLastError();
			FlushFileBuffers(file);
			CloseHandle(file);
		}
		else
			dump_error = GetLastError();
	}

	length = snprintf(report, sizeof report,
		"SRB2 V5.52 native x64 crash report\r\n"
		"timestamp=%04u-%02u-%02u %02u:%02u:%02u.%03u\r\n"
		"build=%s branch=%s revision=%s compiled=%s %s\r\n"
		"process_id=%lu\r\nthread_id=%lu\r\nmode=%s\r\n"
		"exception=0x%08lX\r\nreason=%s\r\n"
		"likely_cause=%s\r\n"
		"cause_note=This category is best-effort context, not proof; the minidump and module offset provide the exact call stack.\r\n"
		"fault_address=%p\r\nfault_module=%s\r\nmodule_offset=0x%llX\r\n"
		"fault_owner=%s\r\nexception_flags=0x%08lX\r\n"
		"memory_operation=%s\r\nmemory_address=0x%llX\r\n"
		"rip=0x%llX\r\nrsp=0x%llX\r\nrbp=0x%llX\r\n"
		"rax=0x%llX\r\nrbx=0x%llX\r\nrcx=0x%llX\r\nrdx=0x%llX\r\n"
		"rsi=0x%llX\r\nrdi=0x%llX\r\nr8=0x%llX\r\nr9=0x%llX\r\n"
		"last_operation=%s\r\nlast_addon=%s\r\n"
		"map=%d\r\ngametic=%u\r\nleveltime=%u\r\nrenderer=%d\r\nresolution=%dx%d\r\n"
		"netgame=%d\r\nserver=%d\r\nplayers=%d\r\nnodes=%d\r\n"
		"packet_loss_percent=%.3f\r\ngame_tic_loss_percent=%.3f\r\n"
		"minidump=%s\r\nminidump_written=%s\r\nminidump_error=%lu\r\n"
		"last_addon_note=The last loaded add-on is context only and is not automatically the cause.\r\n"
		"\r\nRecent one-second timeline (oldest to newest):\r\n%s"
		"\r\nAll native crash files are kept together in the crashes folder.\r\n"
		"Keep this .RPT, its .dmp, and crash-context.txt together. Build symbols are retained separately.\r\n",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
		VERSIONSTRING, compbranch, comprevision, compdate, comptime,
		(unsigned long)GetCurrentProcessId(), (unsigned long)GetCurrentThreadId(),
		I_CrashProcessMode(),
		(unsigned long)record->ExceptionCode, I_Win32ExceptionName(record->ExceptionCode),
		likely_cause,
		record->ExceptionAddress, modulepath,
		(unsigned long long)(modulebase ? (ULONG_PTR)record->ExceptionAddress - modulebase : 0),
		fault_owner, (unsigned long)record->ExceptionFlags,
		access, (unsigned long long)access_address,
		(unsigned long long)context->Rip, (unsigned long long)context->Rsp,
		(unsigned long long)context->Rbp, (unsigned long long)context->Rax,
		(unsigned long long)context->Rbx, (unsigned long long)context->Rcx,
		(unsigned long long)context->Rdx, (unsigned long long)context->Rsi,
		(unsigned long long)context->Rdi, (unsigned long long)context->R8,
		(unsigned long long)context->R9, crash_context, crash_last_addon,
		gamemap, (unsigned)gametic, (unsigned)leveltime, rendermode,
		vid.width, vid.height, netgame, server, D_NumPlayers(), D_NumNodes(false),
		lostpercent, gamelostpercent, dumppath, dump_written ? "yes" : "no",
		(unsigned long)dump_error,
		timeline);
	if (length > 0)
	{
		file = CreateFileA(reportpath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			WriteFile(file, report, (DWORD)min((size_t)length, sizeof report - 1), &written, NULL);
			FlushFileBuffers(file);
			CloseHandle(file);
		}
	}

	                                                                           
                                                                               
                                                                              
	length = snprintf(history, sizeof history,
		"%04u-%02u-%02u %02u:%02u:%02u exception=0x%08lX module=%s "
		"offset=0x%llX map=%d gametic=%u operation=%s\r\n",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
		(unsigned long)record->ExceptionCode, modulepath,
		(unsigned long long)(modulebase ? (ULONG_PTR)record->ExceptionAddress - modulebase : 0),
		gamemap, (unsigned)gametic, crash_context);
	if (length > 0)
		I_AppendDiagnostic("crash-history-v5.52.txt", history,
			(DWORD)min((size_t)length, sizeof history - 1));

	if (dbghelp)
		FreeLibrary(dbghelp);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static void I_AppendDiagnostic(const char *filename, const char *text, DWORD length)
{
	#define DIAGNOSTIC_MAX_BYTES (8ULL * 1024ULL * 1024ULL)
	char directory[512];
	char path[1024];
	char previous[1034];
	DWORD written;
	HANDLE file;
	LARGE_INTEGER filesize;

	snprintf(directory, sizeof directory,
		"%s" PATHSEP "diagnostics", srb2home);
	directory[sizeof directory - 1] = '\0';
	CreateDirectoryA(directory, NULL);
	snprintf(path, sizeof path, "%s" PATHSEP "%s", directory, filename);
	path[sizeof path - 1] = '\0';
	file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return;

	                                                                      
                                                                            
                                                                               
	if (GetFileSizeEx(file, &filesize)
		&& (ULONGLONG)filesize.QuadPart >= DIAGNOSTIC_MAX_BYTES)
	{
		CloseHandle(file);
		snprintf(previous, sizeof previous, "%s.previous", path);
		previous[sizeof previous - 1] = '\0';
		MoveFileExA(path, previous, MOVEFILE_REPLACE_EXISTING);
		file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
			return;
	}
	WriteFile(file, text, length, &written, NULL);
	CloseHandle(file);
	#undef DIAGNOSTIC_MAX_BYTES
}

void I_ReportHitch(const char *operation, precise_t started, UINT32 threshold_ms)
{
	typedef struct
	{
		char operation[96];
		precise_t last_report;
	} hitch_rate_t;
	static hitch_rate_t rate[32];
	precise_t elapsed;
	precise_t finished;
	precise_t interval;
	const char *rate_operation = operation;
	UINT64 precision = I_GetPrecisePrecision();
	UINT64 elapsed_us;
	UINT32 i, slot = 0;
	boolean expected_transition;
	boolean window_manager_blocked;
	SYSTEMTIME now;
	char text[2048];
	char profile[768];
	char movie[256];
	int length;
	size_t total = 0;
	size_t freebytes;

	finished = I_GetPreciseTime();
	                                                                           
                                                                             
                                                                                 
	if (!precision || !started || finished < started)
		return;
	elapsed = finished - started;
	elapsed_us = (elapsed * 1000000ULL) / precision;
	if (elapsed_us < (UINT64)threshold_ms * 1000ULL)
		return;

	                                                                           
                                                                            
                                                                           
                                                                              
                                                                          
                                                                          
	if (diagnostic_level_transition_active && operation
	&& (!strncmp(operation, "OpenGL texture prepare ", 23)
		|| !strncmp(operation, "OpenGL texture upload ", 22)
		|| !strncmp(operation, "OpenGL driver upload ", 21)
		|| !strncmp(operation, "OpenGL flat upload ", 19)))
		return;

	                                                                        
                                                                          
                                                                               
	if (diagnostic_level_transition_start
	&& started <= diagnostic_level_transition_start
	&& finished >= diagnostic_level_transition_start
	&& operation
	&& (!strcmp(operation, "game tic")
		|| !strcmp(operation, "active frame (ticks/render/input/audio)")))
		return;

	expected_transition = diagnostic_level_transition_active
		|| (operation && (!strncmp(operation, "map ", 4)
			|| !strncmp(operation, "add-on ", 7)
			|| !strncmp(operation, "complete level transition", 25)
			|| !strcmp(operation, "initial map tics")
			|| !strcmp(operation, "Lua MapLoad hooks")
			|| !strcmp(operation, "post-hook sound precache")));
	window_manager_blocked = operation
		&& !strcmp(operation, "active frame (ticks/render/input/audio)")
		&& I_WindowManagerBlockedDuring(started, finished);

	                                                                         
                                                                           
                                                                              
	if (operation && !strncmp(operation, "sound decode ", 13))
		rate_operation = "sound decode";
	else if (operation && !strncmp(operation, "Lua ", 4))
		rate_operation = "Lua hook";

	if (!rate_operation || !strcmp(rate_operation, "game tic")
	|| !strcmp(rate_operation, "Lua hook"))
		interval = (precise_t)precision;
	else if (!strcmp(rate_operation, "sound decode")
	|| !strcmp(rate_operation, "active frame (ticks/render/input/audio)"))
		interval = (precise_t)(precision * 2);
	else
		interval = (precise_t)(precision / 4);
	for (i = 0; i < 32; i++)
	{
		if (!rate[i].operation[0] || (rate_operation
			&& !strcmp(rate[i].operation, rate_operation)))
		{
			slot = i;
			break;
		}
		if (rate[i].last_report < rate[slot].last_report)
			slot = i;
	}
	if (rate[slot].operation[0] && rate[slot].last_report
		&& finished - rate[slot].last_report < interval)
		return;
	strncpy(rate[slot].operation, rate_operation ? rate_operation : "unknown",
		sizeof rate[slot].operation - 1);
	rate[slot].operation[sizeof rate[slot].operation - 1] = '\0';
	rate[slot].last_report = finished;

	PS_ArmHitchProfile();
	PS_FormatHitchContext(profile, sizeof profile);
	M_GetMovieDiagnostics(movie, sizeof movie);

	GetLocalTime(&now);
	freebytes = I_GetFreeMem(&total);
	length = snprintf(text, sizeof text,
		"[%04u-%02u-%02u %02u:%02u:%02u.%03u] operation=%s elapsed_ms=%llu "
		"map=%d gametic=%u leveltime=%u renderer=%d resolution=%dx%d "
		"fullscreen=%d vsync=%d fpscap=%d effective_cap=%u refresh_hz=%u "
		"focused=%d netgame=%d server=%d players=%d nodes=%d "
		"network_get_bps=%d network_send_bps=%d packet_loss=%.2f game_loss=%.2f "
		"visible_sprites=%d polyobjects=%d "
		"logic_ms=%.3f playerthink_ms=%.3f thinkers_ms=%.3f other_logic_ms=%.3f "
		"lua_pre_ms=%.3f lua_think_ms=%.3f lua_post_ms=%.3f "
		"process_bits=%u free_mib=%llu total_mib=%llu classification=%s "
		"%s %s\r\n",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
		operation ? operation : "unknown", (unsigned long long)(elapsed_us / 1000ULL),
		gamemap, (unsigned)gametic, (unsigned)leveltime, rendermode, vid.width, vid.height,
		cv_fullscreen.value, cv_vidwait.value, cv_fpscap.value, R_GetFramerateCap(),
		I_GetRefreshRate(), !window_notinfocus, netgame, server,
		D_NumPlayers(), D_NumNodes(false), getbps, sendbps, lostpercent, gamelostpercent,
		ps_numsprites.value.i, ps_numpolyobjects.value.i,
		ps_tictime.value.p <= precision*10 ? ((double)ps_tictime.value.p * 1000.0 / (double)precision) : 0.0,
		ps_playerthink_time.value.p <= precision*10 ? ((double)ps_playerthink_time.value.p * 1000.0 / (double)precision) : 0.0,
		ps_thinkertime.value.p <= precision*10 ? ((double)ps_thinkertime.value.p * 1000.0 / (double)precision) : 0.0,
		ps_otherlogictime.value.p <= precision*10 ? ((double)ps_otherlogictime.value.p * 1000.0 / (double)precision) : 0.0,
		ps_lua_prethinkframe_time.value.p <= precision*10 ? ((double)ps_lua_prethinkframe_time.value.p * 1000.0 / (double)precision) : 0.0,
		ps_lua_thinkframe_time.value.p <= precision*10 ? ((double)ps_lua_thinkframe_time.value.p * 1000.0 / (double)precision) : 0.0,
		ps_lua_postthinkframe_time.value.p <= precision*10 ? ((double)ps_lua_postthinkframe_time.value.p * 1000.0 / (double)precision) : 0.0,
		(unsigned)(sizeof(void *) * 8), (unsigned long long)(freebytes >> 20),
		(unsigned long long)(total >> 20),
		expected_transition ? "expected-transition"
			: (window_manager_blocked ? "windows-move-resize" : "live-hitch"),
		profile, movie);
	if (length > 0)
	{
		DWORD bytes = (DWORD)min((size_t)length, sizeof text - 1);
	I_AppendDiagnostic("hitches-v5.52.txt", text, bytes);
		                                                                    
                                                                          
                                                                     
		if (elapsed_us >= 500000ULL && !expected_transition
			&& !window_manager_blocked)
		{
			I_AppendDiagnostic("freeze-incidents-v5.52.txt", text, bytes);
			CONS_Alert(CONS_WARNING,
				"Freeze detected (%llu ms during %s). Details were saved in diagnostics/freeze-incidents-v5.52.txt\n",
				(unsigned long long)(elapsed_us / 1000ULL),
				operation ? operation : "unknown operation");
		}
	}
}

void I_ReportFrameHitch(precise_t started, precise_t finished)
{
	static precise_t last_report;
	UINT64 precision = I_GetPrecisePrecision();
	UINT64 elapsed_us, threshold_us = 20000;
	UINT32 cap = R_GetFramerateCap();

	if (!precision || finished < started)
		return;
	elapsed_us = ((UINT64)(finished - started) * 1000000ULL) / precision;
	if (cap && threshold_us < 2000000ULL / cap)
		threshold_us = 2000000ULL / cap;
	if (elapsed_us < threshold_us)
		return;


	if (last_report && finished - last_report < (precise_t)(precision * 2))
		return;
	last_report = finished;
	I_ReportHitch("active frame (ticks/render/input/audio)", started,
		(UINT32)(threshold_us / 1000ULL));
}

void I_ReportRemoteExit(const char *playername, INT32 node, boolean timedout,
	UINT32 ping_tics)
{
	SYSTEMTIME now;
	char text[2048];
	char incident[128];
	const char *likely_reason;
	boolean valid_node = node >= 0 && node < MAXNETNODES;
	int length;

	GetLocalTime(&now);
	if (!timedout)
		likely_reason = "normal leave command received";
	else if (lostpercent >= 5.0f || gamelostpercent >= 5.0f)
		likely_reason = "high measured packet/tic loss; congestion or an unstable network path is likely";
	else if (ping_tics >= TICRATE)
		likely_reason = "extremely high latency or jitter; the client may also have frozen or gone to sleep";
	else
		likely_reason = "keep-alive packets stopped; possible client freeze/crash, PC sleep, Wi-Fi interruption, or route failure";

	length = snprintf(text, sizeof text,
		"[%04u-%02u-%02u %02u:%02u:%02u.%03u]\r\n"
		"player=%s\r\nnode=%d\r\nevent=%s\r\nremote_sigsegv=%s\r\n"
		"map=%d\r\ngametic=%u\r\nleveltime=%u\r\nserver=%d\r\nrenderer=%d\r\n"
		"ping_tics=%u\r\nping_ms_approx=%u\r\nnetwork_get_bps=%d\r\nnetwork_send_bps=%d\r\n"
		"system_packet_loss_percent=%.3f\r\ngame_tic_loss_percent=%.3f\r\n"
		"node_ingame=%d\r\nnode_last_tic=%u\r\nnode_supposed_tic=%u\r\n"
		"node_tics_behind=%d\r\nlikely_reason=%s\r\n"
		"explanation=SRB2 2.2.15 does not transmit a remote crash code, so the host can identify a timeout but cannot prove whether the remote process crashed.\r\n\r\n",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
		playername ? playername : "unknown", node, timedout ? "connection-timeout" : "graceful-quit",
		timedout ? "unknown" : "not-indicated", gamemap, (unsigned)gametic,
		(unsigned)leveltime, server, rendermode, (unsigned)ping_tics,
		(unsigned)((ping_tics * 1000U) / TICRATE), getbps, sendbps,
		lostpercent, gamelostpercent, valid_node ? netnodes[node].ingame : 0,
		valid_node ? (unsigned)netnodes[node].tic : 0,
		valid_node ? (unsigned)netnodes[node].supposedtic : 0,
		valid_node ? (INT32)(gametic - netnodes[node].tic) : 0, likely_reason);
	if (length > 0)
	{
		DWORD bytes = (DWORD)min((size_t)length, sizeof text - 1);
	I_AppendDiagnostic("network-exits-v5.52.txt", text, bytes);
		if (timedout)
		{
			snprintf(incident, sizeof incident,
				"connection-timeout-%04u%02u%02u-%02u%02u%02u-%03u-node%d.txt",
				now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
				now.wSecond, now.wMilliseconds, node);
			I_AppendDiagnostic(incident, text, bytes);
		}
	}
}
#else
void I_ReportHitch(const char *operation, precise_t started, UINT32 threshold_ms)
{
	(void)operation;
	(void)started;
	(void)threshold_ms;
}
#endif

#if !defined(NOMUMBLE) && defined(HAVE_MUMBLE)

#include "../netcode/d_clisrv.h"
#include "../byteptr.h"
#endif





#define MIN_SLEEP_DURATION_MS 2.1

                                

                                     

             
  
static void JoyReset(SDLJoyInfo_t *JoySet)
{
	if (JoySet->dev)
	{
		SDL_JoystickClose(JoySet->dev);
	}
	JoySet->dev = NULL;
	JoySet->oldjoy = -1;
	JoySet->axises = JoySet->buttons = JoySet->hats = JoySet->balls = 0;

}

                                        
  
static INT32 joystick_started  = 0;

                                    
  
SDLJoyInfo_t JoyInfo;


                                         
  
static INT32 joystick2_started = 0;

                                    
  
SDLJoyInfo_t JoyInfo2;

#ifdef HAVE_TERMIOS
static INT32 fdmouse2 = -1;
static INT32 mouse2_started = 0;
#endif

SDL_bool consolevent = SDL_FALSE;
SDL_bool framebuffer = SDL_FALSE;

UINT8 keyboard_started = false;

#ifdef UNIXBACKTRACE

static void bt_write_file(int fd, const char *string) {
	ssize_t written = 0;
	ssize_t sourcelen = strlen(string);

	while (fd != -1 && (written != -1 && errno != EINTR) && written < sourcelen)
		written = write(fd, string, sourcelen);
}

static void bt_write_stderr(const char *string) {
	bt_write_file(STDERR_FILENO, string);
}

static void bt_write_all(int fd, const char *string) {
	bt_write_file(fd, string);
	bt_write_file(STDERR_FILENO, string);
}

static void write_backtrace(INT32 signal)
{
	int fd = -1;
	time_t rawtime;
	struct tm timeinfo;

	enum { BT_SIZE = 1024, STR_SIZE = 32 };
#ifndef NOEXECINFO
	void *funcptrs[BT_SIZE];
	size_t bt_size;
#endif
	char timestr[STR_SIZE];

	const char *filename = va("%s" PATHSEP "%s", srb2home, "crash-log.txt");

	fd = open(filename, O_CREAT|O_APPEND|O_RDWR, S_IRUSR|S_IWUSR);

	if (fd == -1)  
		bt_write_stderr("\nWARNING: Couldn't open crash log for writing! Make sure your permissions are correct. Please save the below report!\n");


	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	strftime(timestr, STR_SIZE, "%a, %d %b %Y %T %z", &timeinfo);

	bt_write_file(fd, "------------------------\n");  

	bt_write_all(fd, "\n");  
	bt_write_all(fd, "An error occurred within SRB2! Send this stack trace to someone who can help!\n");

	if (fd != -1)  
		bt_write_stderr("(Or find crash-log.txt in your SRB2 directory.)\n");  


	bt_write_file(fd, "Time of crash: ");
	bt_write_file(fd, timestr);
	bt_write_file(fd, "\n");



	bt_write_file(fd, "Cause: ");
	bt_write_file(fd, strsignal(signal));
	bt_write_file(fd, "\n");  

#ifdef NOEXECINFO
	bt_write_all(fd, "\nNo Backtrace support\n");
#else
	bt_write_all(fd, "\nBacktrace:\n");


	bt_size = backtrace(funcptrs, BT_SIZE);
	backtrace_symbols_fd(funcptrs, bt_size, fd);
	backtrace_symbols_fd(funcptrs, bt_size, STDERR_FILENO);
#endif

	bt_write_file(fd, "\n");  
	if (fd != -1) {
		fsync(fd);  
		close(fd);
	}
}

#endif  

static void I_ReportSignal(int num, int coredumped)
{

	const char *sigmsg, *signame;
	char ttl[128];
	char sigttl[512] = "Process killed by signal: ";
	const char *reportmsg = "\n\nTo help us figure out the cause, you can visit our official Discord server\nwhere you will find more instructions on how to submit a crash report.\n\nSorry for the inconvenience!";

	switch (num)
	{




	case SIGILL:
		sigmsg = "SRB2 has attempted to execute an illegal instruction and needs to close.";
		signame = "SIGILL";  
		break;
	case SIGFPE:
		sigmsg = "SRB2 has encountered a mathematical exception and needs to close.";
		signame = "SIGFPE";  
		break;
	case SIGSEGV:
		sigmsg = "SRB2 has attempted to access a memory location that it shouldn't and needs to close.";
		signame = "SIGSEGV";  
		break;








	case SIGABRT:
		sigmsg = "SRB2 was terminated by an abort signal.";
		signame = "SIGABRT";  
		break;
	default:
		sigmsg = "SRB2 was terminated by an unknown signal.";

		sprintf(ttl, "number %d", num);
		if (coredumped)
			signame = 0;
		else
			signame = ttl;
	}

	if (coredumped)
	{
		if (signame)
			sprintf(ttl, "%s (core dumped)", signame);
		else
			strcat(ttl, " (core dumped)");

		signame = ttl;
	}

	strcat(sigttl, signame);
	I_OutputMsg("%s\n", sigttl);

	if (M_CheckParm("-dedicated"))
		return;

	const SDL_MessageBoxButtonData buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0,		"OK" },
		{ 										0, 1,  "Discord" },
	};

	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR,  
		NULL,  
		sigttl,  
		va("%s %s", sigmsg, reportmsg),  
		SDL_arraysize(buttons),  
		buttons,  
		NULL  
	};

	int buttonid;

	SDL_ShowMessageBox(&messageboxdata, &buttonid);

#if SDL_VERSION_ATLEAST(2,0,14)
	if (buttonid == 1)
		SDL_OpenURL("https://www.srb2.org/discord");
#endif
}

#ifndef NEWSIGNALHANDLER
FUNCNORETURN static ATTRNORETURN void signal_handler(INT32 num)
{
#if defined(_WIN32) && !defined(DEDICATED)
	I_WriteCrashContext(num);
#endif
	D_QuitNetGame();  
	CL_AbortDownloadResume();
#ifdef UNIXBACKTRACE
	write_backtrace(num);
#endif
	I_ReportSignal(num, 0);
	I_ShutdownSystem();
	signal(num, SIG_DFL);                
	raise(num);
	I_Quit();
}
#endif

FUNCNORETURN static ATTRNORETURN void quit_handler(int num)
{
	signal(num, SIG_DFL);  
	raise(num);
	I_Quit();
}

#ifdef HAVE_TERMIOS

SDL_bool stdin_active = SDL_TRUE;

typedef struct
{
	size_t cursor;
	char buffer[256];
} feild_t;

feild_t tty_con;



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
		consolevent = SDL_FALSE;
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
		consolevent = SDL_FALSE;

	if (!consolevent) return;

	if (isatty(STDIN_FILENO)!=1)
	{
		I_OutputMsg("stdin is not a tty, tty console mode failed\n");
		consolevent = SDL_FALSE;
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

void I_GetConsoleEvents(void)
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
		else if (tty_con.cursor < sizeof (tty_con.buffer))
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

void I_GetConsoleEvents(void)
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
	const INT32 ded = M_CheckParm("-dedicated");
	BOOL gotConsole = FALSE;
	if (M_CheckParm("-console") || ded)
		gotConsole = AllocConsole();
#ifdef _DEBUG
	else if (M_CheckParm("-noconsole") && !ded)
#else
	else if (!M_CheckParm("-console") && !ded)
#endif
	{
		FreeConsole();
		gotConsole = FALSE;
	}

	if (gotConsole)
	{
		SetConsoleTitleA("SRB2 Console");
		consolevent = SDL_TRUE;
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
void I_GetConsoleEvents(void){}
static inline void I_StartupConsole(void)
{
#ifdef _DEBUG
	consolevent = !M_CheckParm("-noconsole");
#else
	consolevent = M_CheckParm("-console");
#endif

	framebuffer = M_CheckParm("-framebuffer");

	if (framebuffer)
		consolevent = SDL_FALSE;
}
static inline void I_ShutdownConsole(void){}
#endif




static void I_RegisterSignals (void)
{
#ifdef SIGINT
	signal(SIGINT , quit_handler);
#endif
#ifdef SIGBREAK
	signal(SIGBREAK , quit_handler);
#endif
#ifdef SIGTERM
	signal(SIGTERM , quit_handler);
#endif



#ifndef NEWSIGNALHANDLER
	signal(SIGILL , signal_handler);
	signal(SIGSEGV , signal_handler);
	signal(SIGABRT , signal_handler);
	signal(SIGFPE , signal_handler);
#endif
}

#ifdef NEWSIGNALHANDLER
static void signal_handler_child(INT32 num)
{
#ifdef UNIXBACKTRACE
	write_backtrace(num);
#endif

	signal(num, SIG_DFL);                
	raise(num);
}

static void I_RegisterChildSignals(void)
{


	signal(SIGILL , signal_handler_child);
	signal(SIGSEGV , signal_handler_child);
	signal(SIGABRT , signal_handler_child);
	signal(SIGFPE , signal_handler_child);
}
#endif




void I_OutputMsg(const char *fmt, ...)
{
	int needed;
	size_t len;
	char *txt;
	va_list  argptr;

	va_start(argptr,fmt);
	needed = vsnprintf(NULL, 0, fmt, argptr);
	va_end(argptr);
	if (needed <= 0)
		return;
	len = (size_t)needed;

	txt = malloc(len+1);
	if (!txt)
		return;
	va_start(argptr,fmt);
	vsnprintf(txt, len+1, fmt, argptr);
	va_end(argptr);

#ifdef HAVE_TTF
	if (TTF_WasInit()) I_TTFDrawText(currentfont, solid, DEFAULTFONTFGR, DEFAULTFONTFGG, DEFAULTFONTFGB,  DEFAULTFONTFGA,
	DEFAULTFONTBGR, DEFAULTFONTBGG, DEFAULTFONTBGB, DEFAULTFONTBGA, txt);
#endif

#if defined (_WIN32) && defined (_MSC_VER)
	OutputDebugStringA(txt);
#endif

	len = strlen(txt);

#ifdef LOGMESSAGES
	if (logstream)
	{
		size_t d = fwrite(txt, len, 1, logstream);
		fflush(logstream);
		(void)d;
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
					free(txt);
					free(blank);
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




INT32 I_GetKey (void)
{

	event_t *ev;
	INT32 rc = 0;


	for (; eventtail != eventhead; eventtail = (eventtail+1)&(MAXEVENTS-1))
	{
		ev = &events[eventtail];
		if (ev->type == ev_keydown || ev->type == ev_console)
		{
			rc = ev->key;
			continue;
		}
	}

	return rc;
}




void I_JoyScale(void)
{
	Joystick.bGamepadStyle = cv_joyscale.value==0;
	JoyInfo.scale = Joystick.bGamepadStyle?1:cv_joyscale.value;
}

void I_JoyScale2(void)
{
	Joystick2.bGamepadStyle = cv_joyscale2.value==0;
	JoyInfo2.scale = Joystick2.bGamepadStyle?1:cv_joyscale2.value;
}


INT32 I_GetJoystickDeviceIndex(SDL_Joystick *dev)
{
	INT32 i, count = SDL_NumJoysticks();

	for (i = 0; dev && i < count; i++)
	{
		SDL_Joystick *test = SDL_JoystickOpen(i);
		if (test && test == dev)
			return i;
		else if (JoyInfo.dev != test && JoyInfo2.dev != test)
			SDL_JoystickClose(test);
	}

	return -1;
}

                                    
  
static UINT64 lastjoybuttons = 0;

                                
  
static UINT64 lastjoyhats = 0;

                                


             


  
void I_ShutdownJoystick(void)
{
	INT32 i;
	event_t event;
	event.type=ev_keyup;
	event.x = 0;
	event.y = 0;

	lastjoybuttons = lastjoyhats = 0;


	for (i=0;i<JOYBUTTONS;i++)
	{
		event.key=KEY_JOY1+i;
		D_PostEvent(&event);
	}


	for (i=0;i<JOYHATS*4;i++)
	{
		event.key=KEY_HAT1+i;
		D_PostEvent(&event);
	}


	event.type = ev_joystick;
	for (i=0;i<JOYAXISSET; i++)
	{
		event.key = i;
		D_PostEvent(&event);
	}

	joystick_started = 0;
	JoyReset(&JoyInfo);


}

void I_GetJoystickEvents(void)
{
	static event_t event = {0,0,0,0,false};
	INT32 i = 0;
	UINT64 joyhats = 0;
#if 0
	UINT64 joybuttons = 0;
	Sint16 axisx, axisy;
#endif

	if (!joystick_started) return;

	if (!JoyInfo.dev)  
		return;

#if 0


	for (i = JoyInfo.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo.dev,i))
			joybuttons |= 1;
	}

	if (joybuttons != lastjoybuttons)
	{
		INT64 j = 1;  
		INT64 newbuttons = joybuttons ^ lastjoybuttons;
		lastjoybuttons = joybuttons;

		for (i = 0; i < JOYBUTTONS; i++, j <<= 1)
		{
			if (newbuttons & j)  
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.key = KEY_JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}
#endif

	for (i = JoyInfo.hats - 1; i >= 0; i--)
	{
		Uint8 hat = SDL_JoystickGetHat(JoyInfo.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=(UINT64)0x1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=(UINT64)0x1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=(UINT64)0x1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=(UINT64)0x1<<(3 + 4*i);
	}

	if (joyhats != lastjoyhats)
	{
		INT64 j = 1;  
		INT64 newhats = joyhats ^ lastjoyhats;
		lastjoyhats = joyhats;

		for (i = 0; i < JOYHATS*4; i++, j <<= 1)
		{
			if (newhats & j)  
			{
				if (joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.key = KEY_HAT1 + i;
				D_PostEvent(&event);
			}
		}
	}

#if 0

	event.type = ev_joystick;

	for (i = JOYAXISSET - 1; i >= 0; i--)
	{
		event.key = i;
		if (i*2 + 1 <= JoyInfo.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 0);
		else axisx = 0;
		if (i*2 + 2 <= JoyInfo.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 1);
		else axisy = 0;



		axisx = axisx/32;
		axisy = axisy/32;


		if (Joystick.bGamepadStyle)
		{

			if (axisx < -(JOYAXISRANGE/2))
				event.x = -1;
			else if (axisx > (JOYAXISRANGE/2))
				event.x = 1;
			else event.x = 0;
			if (axisy < -(JOYAXISRANGE/2))
				event.y = -1;
			else if (axisy > (JOYAXISRANGE/2))
				event.y = 1;
			else event.y = 0;
		}
		else
		{

			axisx = JoyInfo.scale?((axisx/JoyInfo.scale)*JoyInfo.scale):axisx;
			axisy = JoyInfo.scale?((axisy/JoyInfo.scale)*JoyInfo.scale):axisy;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= axisx && axisx <= SDL_JDEADZONE) axisx = 0;
			if (-SDL_JDEADZONE <= axisy && axisy <= SDL_JDEADZONE) axisy = 0;
#endif


			event.x = axisx;  
			event.y = axisy;  
		}
		D_PostEvent(&event);
	}
#endif
}

                               

                              

               


  
static int joy_open(int joyindex)
{
	SDL_Joystick *newdev = NULL;
	int num_joy = 0;

	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		CONS_Printf(M_GetText("Joystick subsystem not started\n"));
		return -1;
	}

	if (joyindex <= 0)
		return -1;

	num_joy = SDL_NumJoysticks();

	if (num_joy == 0)
	{
		CONS_Printf("%s", M_GetText("Found no joysticks on this system\n"));
		return -1;
	}

	newdev = SDL_JoystickOpen(joyindex-1);











	if (JoyInfo.dev)
	{
		if (JoyInfo.dev == newdev  
			|| (newdev == NULL && SDL_JoystickGetAttached(JoyInfo.dev)))  
			return JoyInfo.axises;

		CONS_Debug(DBG_GAMELOGIC, "Joystick1 device is changing; resetting events...\n");
		I_ShutdownJoystick();
	}

	JoyInfo.dev = newdev;

	if (JoyInfo.dev == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Joystick1: Couldn't open device - %s\n"), SDL_GetError());
		return -1;
	}
	else
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Joystick1: %s\n"), SDL_JoystickName(JoyInfo.dev));
		JoyInfo.axises = SDL_JoystickNumAxes(JoyInfo.dev);
		if (JoyInfo.axises > JOYAXISSET*2)
			JoyInfo.axises = JOYAXISSET*2;
	                  
   
                                      
            
     

		JoyInfo.buttons = SDL_JoystickNumButtons(JoyInfo.dev);
		if (JoyInfo.buttons > JOYBUTTONS)
			JoyInfo.buttons = JOYBUTTONS;

		JoyInfo.hats = SDL_JoystickNumHats(JoyInfo.dev);
		if (JoyInfo.hats > JOYHATS)
			JoyInfo.hats = JOYHATS;

		JoyInfo.balls = SDL_JoystickNumBalls(JoyInfo.dev);



		return JoyInfo.axises;
	}
}



                                    
  
static UINT64 lastjoy2buttons = 0;

                                
  
static UINT64 lastjoy2hats = 0;

                                


             
  
void I_ShutdownJoystick2(void)
{
	INT32 i;
	event_t event;
	event.type = ev_keyup;
	event.x = 0;
	event.y = 0;

	lastjoy2buttons = lastjoy2hats = 0;


	for (i = 0; i < JOYBUTTONS; i++)
	{
		event.key = KEY_2JOY1 + i;
		D_PostEvent(&event);
	}


	for (i = 0; i < JOYHATS*4; i++)
	{
		event.key = KEY_2HAT1 + i;
		D_PostEvent(&event);
	}


	event.type = ev_joystick2;
	for (i = 0; i < JOYAXISSET; i++)
	{
		event.key = i;
		D_PostEvent(&event);
	}

	joystick2_started = 0;
	JoyReset(&JoyInfo2);


}

void I_GetJoystick2Events(void)
{
	static event_t event = {0,0,0,0,false};
	INT32 i = 0;
	UINT64 joyhats = 0;
#if 0
	INT64 joybuttons = 0;
	INT32 axisx, axisy;
#endif

	if (!joystick2_started)
		return;

	if (!JoyInfo2.dev)  
		return;


#if 0


	for (i = JoyInfo2.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo2.dev,i))
			joybuttons |= 1;
	}

	if (joybuttons != lastjoy2buttons)
	{
		INT64 j = 1;  
		INT64 newbuttons = joybuttons ^ lastjoy2buttons;
		lastjoy2buttons = joybuttons;

		for (i = 0; i < JOYBUTTONS; i++, j <<= 1)
		{
			if (newbuttons & j)  
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.key = KEY_2JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}
#endif

	for (i = JoyInfo2.hats - 1; i >= 0; i--)
	{
		Uint8 hat = SDL_JoystickGetHat(JoyInfo2.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=(UINT64)0x1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=(UINT64)0x1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=(UINT64)0x1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=(UINT64)0x1<<(3 + 4*i);
	}

	if (joyhats != lastjoy2hats)
	{
		INT64 j = 1;  
		INT64 newhats = joyhats ^ lastjoy2hats;
		lastjoy2hats = joyhats;

		for (i = 0; i < JOYHATS*4; i++, j <<= 1)
		{
			if (newhats & j)  
			{
				if (joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.key = KEY_2HAT1 + i;
				D_PostEvent(&event);
			}
		}
	}

#if 0

	event.type = ev_joystick2;

	for (i = JOYAXISSET - 1; i >= 0; i--)
	{
		event.key = i;
		if (i*2 + 1 <= JoyInfo2.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 0);
		else axisx = 0;
		if (i*2 + 2 <= JoyInfo2.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 1);
		else axisy = 0;


		axisx = axisx/32;
		axisy = axisy/32;

		if (Joystick2.bGamepadStyle)
		{

			if (axisx < -(JOYAXISRANGE/2))
				event.x = -1;
			else if (axisx > (JOYAXISRANGE/2))
				event.x = 1;
			else
				event.x = 0;
			if (axisy < -(JOYAXISRANGE/2))
				event.y = -1;
			else if (axisy > (JOYAXISRANGE/2))
				event.y = 1;
			else
				event.y = 0;
		}
		else
		{

			axisx = JoyInfo2.scale?((axisx/JoyInfo2.scale)*JoyInfo2.scale):axisx;
			axisy = JoyInfo2.scale?((axisy/JoyInfo2.scale)*JoyInfo2.scale):axisy;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= axisx && axisx <= SDL_JDEADZONE) axisx = 0;
			if (-SDL_JDEADZONE <= axisy && axisy <= SDL_JDEADZONE) axisy = 0;
#endif


			event.x = axisx;  
			event.y = axisy;  
		}
		D_PostEvent(&event);
	}
#endif
}

                               

                              

               


  
static int joy_open2(int joyindex)
{
	SDL_Joystick *newdev = NULL;
	int num_joy = 0;

	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		CONS_Printf(M_GetText("Joystick subsystem not started\n"));
		return -1;
	}

	if (joyindex <= 0)
		return -1;

	num_joy = SDL_NumJoysticks();

	if (num_joy == 0)
	{
		CONS_Printf("%s", M_GetText("Found no joysticks on this system\n"));
		return -1;
	}

	newdev = SDL_JoystickOpen(joyindex-1);











	if (JoyInfo2.dev)
	{
		if (JoyInfo2.dev == newdev  
			|| (newdev == NULL && SDL_JoystickGetAttached(JoyInfo2.dev)))  
			return JoyInfo.axises;

		CONS_Debug(DBG_GAMELOGIC, "Joystick2 device is changing; resetting events...\n");
		I_ShutdownJoystick2();
	}

	JoyInfo2.dev = newdev;

	if (JoyInfo2.dev == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Joystick2: couldn't open device - %s\n"), SDL_GetError());
		return -1;
	}
	else
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Joystick2: %s\n"), SDL_JoystickName(JoyInfo2.dev));
		JoyInfo2.axises = SDL_JoystickNumAxes(JoyInfo2.dev);
		if (JoyInfo2.axises > JOYAXISSET*2)
			JoyInfo2.axises = JOYAXISSET*2;
                  
   
                                      
            
     

		JoyInfo2.buttons = SDL_JoystickNumButtons(JoyInfo2.dev);
		if (JoyInfo2.buttons > JOYBUTTONS)
			JoyInfo2.buttons = JOYBUTTONS;

		JoyInfo2.hats = SDL_JoystickNumHats(JoyInfo2.dev);
		if (JoyInfo2.hats > JOYHATS)
			JoyInfo2.hats = JOYHATS;

		JoyInfo2.balls = SDL_JoystickNumBalls(JoyInfo2.dev);



		return JoyInfo2.axises;
	}
}




void I_InitJoystick(void)
{
	SDL_Joystick *newjoy = NULL;


	if (M_CheckParm("-nojoy"))
		return;

	if (M_CheckParm("-noxinput"))
		SDL_SetHintWithPriority("SDL_XINPUT_ENABLED", "0", SDL_HINT_OVERRIDE);

	if (M_CheckParm("-nohidapi"))
		SDL_SetHintWithPriority("SDL_JOYSTICK_HIDAPI", "0", SDL_HINT_OVERRIDE);

	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		CONS_Printf("I_InitJoystick()...\n");

		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf(M_GetText("Couldn't initialize joystick: %s\n"), SDL_GetError());
			return;
		}
	}

	if (cv_usejoystick.value)
		newjoy = SDL_JoystickOpen(cv_usejoystick.value-1);

	if (newjoy && JoyInfo2.dev == newjoy)  
		cv_usejoystick.value = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
	else if (newjoy && joy_open(cv_usejoystick.value) != -1)
	{


		JoyInfo.oldjoy = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
		joystick_started = 1;
	}
	else
	{
		if (JoyInfo.oldjoy)
			I_ShutdownJoystick();
		cv_usejoystick.value = 0;
		joystick_started = 0;
	}

	if (JoyInfo.dev != newjoy && JoyInfo2.dev != newjoy)
		SDL_JoystickClose(newjoy);
}

void I_InitJoystick2(void)
{
	SDL_Joystick *newjoy = NULL;


	if (M_CheckParm("-nojoy"))
		return;

	if (M_CheckParm("-noxinput"))
		SDL_SetHintWithPriority("SDL_XINPUT_ENABLED", "0", SDL_HINT_OVERRIDE);

	if (M_CheckParm("-nohidapi"))
		SDL_SetHintWithPriority("SDL_JOYSTICK_HIDAPI", "0", SDL_HINT_OVERRIDE);

	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		CONS_Printf("I_InitJoystick2()...\n");

		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf(M_GetText("Couldn't initialize joystick: %s\n"), SDL_GetError());
			return;
		}
	}

	if (cv_usejoystick2.value)
		newjoy = SDL_JoystickOpen(cv_usejoystick2.value-1);

	if (newjoy && JoyInfo.dev == newjoy)  
		cv_usejoystick2.value = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
	else if (newjoy && joy_open2(cv_usejoystick2.value) != -1)
	{


		JoyInfo2.oldjoy = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
		joystick2_started = 1;
	}
	else
	{
		if (JoyInfo2.oldjoy)
			I_ShutdownJoystick2();
		cv_usejoystick2.value = 0;
		joystick2_started = 0;
	}

	if (JoyInfo.dev != newjoy && JoyInfo2.dev != newjoy)
		SDL_JoystickClose(newjoy);
}

static void I_ShutdownInput(void)
{



	I_ShutdownJoystick();
	I_ShutdownJoystick2();

	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		CONS_Printf("Shutting down joy system\n");
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		I_OutputMsg("I_Joystick: SDL's Joystick system has been shutdown\n");
	}
}

INT32 I_NumJoys(void)
{
	INT32 numjoy = 0;
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
		numjoy = SDL_NumJoysticks();
	return numjoy;
}

static char joyname[255];  

const char *I_GetJoyName(INT32 joyindex)
{
	const char *tempname = NULL;
	joyname[0] = 0;
	joyindex--;  
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		tempname = SDL_JoystickNameForIndex(joyindex);
		if (tempname)
			strncpy(joyname, tempname, sizeof(joyname)-1);
	}
	return joyname;
}

#ifndef NOMUMBLE
#ifdef HAVE_MUMBLE





#define DEG2RAD (0.017453292519943295769236907684883l)  
#define MUMBLEUNIT (64.0f)  

static struct {
#ifdef WINMUMBLE
	UINT32 uiVersion;
	DWORD uiTick;
#else
	Uint32 uiVersion;
	Uint32 uiTick;
#endif
	float fAvatarPosition[3];
	float fAvatarFront[3];
	float fAvatarTop[3];  
	wchar_t name[256];  
	float fCameraPosition[3];
	float fCameraFront[3];
	float fCameraTop[3];  
	wchar_t identity[256];  
#ifdef WINMUMBLE
	UINT32 context_len;
#else
	Uint32 context_len;
#endif
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
#undef WINMUMBLE
#endif  

#ifdef HAVE_TERMIOS

void I_GetMouseEvents(void)
{
	static UINT8 mdata[5];
	static INT32 i = 0,om2b = 0;
	INT32 di, j, mlp, button;
	event_t event;
	const INT32 mswap[8] = {0, 4, 1, 5, 2, 6, 3, 7};

	if (!mouse2_started) return;
	for (mlp = 0; mlp < 20; mlp++)
	{
		for (; i < 5; i++)
		{
			di = read(fdmouse2, mdata+i, 1);
			if (di == -1) return;
		}
		if ((mdata[0] & 0xf8) != 0x80)
		{
			for (j = 1; j < 5; j++)
				if ((mdata[j] & 0xf8) == 0x80)
					for (i = 0; i < 5-j; i++)  
						mdata[i] = mdata[i+j];
			if (i < 5) continue;
		}
		else
		{
			button = mswap[~mdata[0] & 0x07];
			for (j = 0; j < MOUSEBUTTONS; j++)
			{
				if (om2b & (1<<j))
				{
					if (!(button & (1<<j)))  
					{
						event.type = ev_keyup;
						event.key = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
				else
				{
					if (button & (1<<j))
					{
						event.type = ev_keydown;
						event.key = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
			}
			event.x = ((SINT8)mdata[1])+((SINT8)mdata[3]);
			event.y = ((SINT8)mdata[2])+((SINT8)mdata[4]);
			if (event.x && event.y)
			{
				event.type = ev_mouse2;
				event.key = 0;
				D_PostEvent(&event);
			}
		}
		i = 0;
	}
}




static void I_ShutdownMouse2(void)
{
	if (fdmouse2 != -1) close(fdmouse2);
	mouse2_started = 0;
}
#elif defined (_WIN32)

static HANDLE mouse2filehandle = INVALID_HANDLE_VALUE;

static void I_ShutdownMouse2(void)
{
	event_t event;
	INT32 i;

	if (mouse2filehandle == INVALID_HANDLE_VALUE)
		return;

	SetCommMask(mouse2filehandle, 0);

	EscapeCommFunction(mouse2filehandle, CLRDTR);
	EscapeCommFunction(mouse2filehandle, CLRRTS);

	PurgeComm(mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT |
			  PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle(mouse2filehandle);


	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		event.type = ev_keyup;
		event.key = KEY_2MOUSE1+i;
		D_PostEvent(&event);
	}

	mouse2filehandle = INVALID_HANDLE_VALUE;
}

#define MOUSECOMBUFFERSIZE 256
static INT32 handlermouse2x,handlermouse2y,handlermouse2buttons;

static void I_PoolMouse2(void)
{
	UINT8 buffer[MOUSECOMBUFFERSIZE];
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	DWORD dwLength;
	char dx,dy;

	static INT32 bytenum;
	static UINT8 combytes[4];
	DWORD i;

	ClearCommError(mouse2filehandle, &dwErrorFlags, &ComStat);
	dwLength = min(MOUSECOMBUFFERSIZE, ComStat.cbInQue);

	if (dwLength <= 0)
		return;

	if (!ReadFile(mouse2filehandle, buffer, dwLength, &dwLength, NULL))
	{
		CONS_Alert(CONS_WARNING, "%s", M_GetText("Read Error on secondary mouse port\n"));
		return;
	}


	for (i = 0; i < dwLength; i++)
	{
		if ((buffer[i] & 64)== 64)
			bytenum = 0;

		if (bytenum < 4)
			combytes[bytenum] = buffer[i];
		bytenum++;

		if (bytenum == 1)
		{
			handlermouse2buttons &= ~3;
			handlermouse2buttons |= ((combytes[0] & (32+16)) >> 4);
		}
		else if (bytenum == 3)
		{
			dx = (char)((combytes[0] &  3) << 6);
			dy = (char)((combytes[0] & 12) << 4);
			dx = (char)(dx + combytes[1]);
			dy = (char)(dy + combytes[2]);
			handlermouse2x+= dx;
			handlermouse2y+= dy;
		}
		else if (bytenum == 4)  
		{
			if (buffer[i] & 32)
				handlermouse2buttons |= 4;
			else
				handlermouse2buttons &= ~4;
		}
	}
}

void I_GetMouseEvents(void)
{
	static UINT8 lastbuttons2 = 0;  
	event_t event;

	if (mouse2filehandle == INVALID_HANDLE_VALUE)
		return;

	I_PoolMouse2();

	if (handlermouse2buttons != lastbuttons2)
	{
		INT32 i, j = 1, k;
		k = (handlermouse2buttons ^ lastbuttons2);  
		lastbuttons2 = (UINT8)handlermouse2buttons;

		for (i = 0; i < MOUSEBUTTONS; i++, j <<= 1)
			if (k & j)
			{
				if (handlermouse2buttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.key = KEY_2MOUSE1+i;
				D_PostEvent(&event);
			}
	}

	if (handlermouse2x != 0 || handlermouse2y != 0)
	{
		event.type = ev_mouse2;
		event.key = 0;

		event.x = handlermouse2x << 1;
		event.y = handlermouse2y << 1;
		handlermouse2x = 0;
		handlermouse2y = 0;

		D_PostEvent(&event);
	}
}
#else
void I_GetMouseEvents(void){};
#endif




void I_StartupMouse2(void)
{
#ifdef HAVE_TERMIOS
	struct termios m2tio;
	size_t i;
	INT32 dtr = -1, rts = -1;;
	I_ShutdownMouse2();
	if (cv_usemouse2.value == 0) return;
	if ((fdmouse2 = open(cv_mouse2port.string, O_RDONLY|O_NONBLOCK|O_NOCTTY)) == -1)
	{
		CONS_Printf(M_GetText("Error opening %s!\n"), cv_mouse2port.string);
		return;
	}
	tcflush(fdmouse2, TCIOFLUSH);
	m2tio.c_iflag = IGNBRK;
	m2tio.c_oflag = 0;
	m2tio.c_cflag = CREAD|CLOCAL|HUPCL|CS8|CSTOPB|B1200;
	m2tio.c_lflag = 0;
	m2tio.c_cc[VTIME] = 0;
	m2tio.c_cc[VMIN] = 1;
	tcsetattr(fdmouse2, TCSANOW, &m2tio);
	for (i = 0; i < strlen(cv_mouse2opt.string); i++)
	{
		if (toupper(cv_mouse2opt.string[i]) == 'D')
		{
			if (cv_mouse2opt.string[i+1] == '-')
				dtr = 0;
			else
				dtr = 1;
		}
		if (toupper(cv_mouse2opt.string[i]) == 'R')
		{
			if (cv_mouse2opt.string[i+1] == '-')
				rts = 0;
			else
				rts = 1;
		}
		if (dtr != -1 || rts != -1)
		{
			INT32 c;
			if (!ioctl(fdmouse2, TIOCMGET, &c))
			{
				if (!dtr)
					c &= ~TIOCM_DTR;
				else if (dtr > 0)
					c |= TIOCM_DTR;
			}
			if (!rts)
				c &= ~TIOCM_RTS;
			else if (rts > 0)
				c |= TIOCM_RTS;
			ioctl(fdmouse2, TIOCMSET, &c);
		}
	}
	mouse2_started = 1;
	I_AddExitFunc(I_ShutdownMouse2);
#elif defined (_WIN32)
	DCB dcb;

	if (mouse2filehandle != INVALID_HANDLE_VALUE)
		I_ShutdownMouse2();

	if (cv_usemouse2.value == 0)
		return;

	if (mouse2filehandle == INVALID_HANDLE_VALUE)
	{

		mouse2filehandle = CreateFileA(cv_mouse2port.string, GENERIC_READ | GENERIC_WRITE,
									   0,                      
									   NULL,                   
									   OPEN_EXISTING,
									   FILE_ATTRIBUTE_NORMAL,
									   NULL);
		if (mouse2filehandle == INVALID_HANDLE_VALUE)
		{
			INT32 e = GetLastError();
			if (e == 5)
				CONS_Alert(CONS_ERROR, M_GetText("Can't open %s: Access denied\n"), cv_mouse2port.string);
			else
				CONS_Alert(CONS_ERROR, M_GetText("Can't open %s: error %d\n"), cv_mouse2port.string, e);
			return;
		}
	}





	SetupComm(mouse2filehandle, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);


	PurgeComm(mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT
			  | PURGE_TXCLEAR | PURGE_RXCLEAR);


	dcb.DCBlength = sizeof (DCB);

	GetCommState(mouse2filehandle, &dcb);

	dcb.BaudRate = CBR_1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	SetCommState(mouse2filehandle, &dcb);
	I_AddExitFunc(I_ShutdownMouse2);
#endif
}




void I_Tactile(FFType pFFType, const JoyFF_t *FFEffect)
{

	(void)pFFType;
	(void)FFEffect;
}

void I_Tactile2(FFType pFFType, const JoyFF_t *FFEffect)
{

	(void)pFFType;
	(void)FFEffect;
}

                                    
  
static ticcmd_t emptycmd;

ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

                                    
  
static ticcmd_t emptycmd2;

ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}






static Uint64 timer_frequency;

precise_t I_GetPreciseTime(void)
{
	return SDL_GetPerformanceCounter();
}

UINT64 I_GetPrecisePrecision(void)
{
	return SDL_GetPerformanceFrequency();
}

static UINT32 frame_rate;

static double frame_frequency;
static UINT64 frame_epoch;
static double elapsed_frames;

static void I_InitFrameTime(const UINT64 now, const UINT32 cap)
{
	frame_rate = cap;
	frame_epoch = now;



	if (frame_rate == 0)
	{

		frame_frequency = 1.0;
		return;
	}

	frame_frequency = timer_frequency / (double)frame_rate;
}

double I_GetFrameTime(void)
{
	const UINT64 now = SDL_GetPerformanceCounter();
	const UINT32 cap = R_GetFramerateCap();

	if (cap != frame_rate)
	{

		I_InitFrameTime(now, cap);
	}

	if (frame_rate == 0)
	{

		elapsed_frames += 1.0;
	}
	else
	{
		elapsed_frames += (now - frame_epoch) / frame_frequency;
	}

	frame_epoch = now;  
	return elapsed_frames;
}




void I_StartupTimer(void)
{
	timer_frequency = SDL_GetPerformanceFrequency();

	I_InitFrameTime(0, R_GetFramerateCap());
	elapsed_frames  = 0.0;
}

void I_Sleep(UINT32 ms)
{
	SDL_Delay(ms);
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
#elif defined(_WIN32)
	UINT64 precision = I_GetPrecisePrecision();
	precise_t dest = I_GetPreciseTime() + duration;
	precise_t slack = precision / 5000;  
	static HANDLE frame_timer = NULL;

	if (!frame_timer)
	{
		HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
		p_CreateWaitableTimerExW create_timer = NULL;
		if (kernel32)
		{
			FARPROC proc = GetProcAddress(kernel32, "CreateWaitableTimerExW");
			memcpy(&create_timer, &proc, sizeof(create_timer));
		}

		if (create_timer)
			frame_timer = create_timer(NULL, NULL,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		if (!frame_timer)
			frame_timer = CreateWaitableTimerW(NULL, TRUE, NULL);
	}

	if (duration > slack)
	{
		const UINT64 coarse = duration - slack;
		LARGE_INTEGER due;
		                                                                        
                                                                          
		due.QuadPart = -(LONGLONG)max((UINT64)1,
			(coarse * 10000000ULL + precision - 1) / precision);

		if (!frame_timer || !SetWaitableTimer(frame_timer, &due, 0, NULL, NULL, FALSE)
		|| WaitForSingleObject(frame_timer, INFINITE) != WAIT_OBJECT_0)
		{
			UINT32 fallback_ms = (UINT32)((coarse * 1000ULL) / precision);
			if (fallback_ms)
				SDL_Delay(fallback_ms);
		}
	}

	while (((INT64)dest - (INT64)I_GetPreciseTime()) > 0);
#elif defined (MIN_SLEEP_DURATION_MS)
	UINT64 precision = I_GetPrecisePrecision();
	INT32 sleepvalue = cv_sleep.value;
	UINT64 spin_slack;
	precise_t cur;
	precise_t dest;

	                                                                           
                                                                             
                                        
	spin_slack = precision / 1000;

	cur = I_GetPreciseTime();
	dest = cur + duration;





	while ((INT64)(dest - cur) > 0)
	{
		const UINT64 remaining = dest - cur;
		if (sleepvalue > 0
		&& remaining > spin_slack + ((UINT64)sleepvalue * precision) / 1000)
		{
			UINT32 sleep_ms = (UINT32)(((remaining - spin_slack) * 1000ULL) / precision);
			I_Sleep(sleep_ms);
		}



		cur = I_GetPreciseTime();
	}
#endif
}

#ifdef NEWSIGNALHANDLER
ATTRNORETURN static FUNCNORETURN void newsignalhandler_Warn(const char *pr)
{
	char text[128];

	snprintf(text, sizeof text,
			"Error while setting up signal reporting: %s: %s",
			pr,
			strerror(errno)
	);

	I_OutputMsg("%s\n", text);

	if (!M_CheckParm("-dedicated"))
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Startup error",
			text, NULL);

	I_ShutdownConsole();
	exit(-1);
}

static void I_Fork(void)
{
	int child;
	int status;
	int signum;
	int c;

	child = fork();

	switch (child)
	{
		case -1:
			newsignalhandler_Warn("fork()");
			break;
		case 0:
			I_RegisterChildSignals();
			break;
		default:
			if (logstream)
				fclose(logstream); 

			c = wait(&status);

#ifdef LOGMESSAGES
			 
			logstream = fopen(logfilename, "at");
#else
			logstream = 0;
#endif

			if (c == -1)
			{
				kill(child, SIGKILL);
				newsignalhandler_Warn("wait()");
			}
			else
			{
				if (WIFSIGNALED (status))
				{
					signum = WTERMSIG (status);
#ifdef WCOREDUMP
					I_ReportSignal(signum, WCOREDUMP (status));
#else
					I_ReportSignal(signum, 0);
#endif
					status = 128 + signum;
				}
				else if (WIFEXITED (status))
				{
					status = WEXITSTATUS (status);
				}

				I_ShutdownConsole();
				exit(status);
			}
	}
}
#endif 

INT32 I_StartupSystem(void)
{
	SDL_version SDLcompiled;
	SDL_version SDLlinked;
	SDL_VERSION(&SDLcompiled)
	SDL_GetVersion(&SDLlinked);
#ifdef HAVE_THREADS
	I_start_threads();
	I_AddExitFunc(I_stop_threads);
#endif
	I_StartupConsole();
#ifdef NEWSIGNALHANDLER


	if (!M_CheckParm("-nofork"))
		I_Fork();
#endif
	I_RegisterSignals();
	I_OutputMsg("Compiled for SDL version: %d.%d.%d\n",
	 SDLcompiled.major, SDLcompiled.minor, SDLcompiled.patch);
	I_OutputMsg("Linked with SDL version: %d.%d.%d\n",
	 SDLlinked.major, SDLlinked.minor, SDLlinked.patch);
	if (SDL_Init(0) < 0)
		I_Error("SRB2: SDL System Error: %s", SDL_GetError());  
#if defined(_WIN64) && !defined(DEDICATED)
	AddVectoredExceptionHandler(1, I_Win32CrashException);
#endif
#ifndef NOMUMBLE
	I_SetupMumble();
#endif
	return 0;
}




void I_Quit(void)
{
	static SDL_bool quiting = SDL_FALSE;

	 
	if (quiting) goto death;
	SDLforceUngrabMouse();
	quiting = SDL_FALSE;
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
	I_ShutdownMusic();
	I_ShutdownSound();

	I_ShutdownGraphics();
	I_ShutdownInput();
	I_ShutdownSystem();
	if (M_CheckParm("-nofork"))  
		I_ShutdownConsole();
	SDL_Quit();
	 
	if (!M_CheckParm("-noendtxt") && W_CheckNumForName("ENDOOM") != LUMPERROR)
	{
		printf("\r");
		ShowEndTxt();
	}
	if (myargmalloc)
		free(myargv);  
death:
	W_Shutdown();
	exit(0);
}

void I_WaitVBL(INT32 count)
{
	count = 1;
	SDL_Delay(count);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}




                                 
  
static INT32 errorcount = 0;

                                    
  
static boolean shutdowning = false;

void I_Error(const char *error, ...)
{
	va_list argptr;
	char buffer[8192];


	if (shutdowning)
	{
		errorcount++;
		if (errorcount == 1)
			SDLforceUngrabMouse();

		if (errorcount == 2)
			I_ShutdownMusic();
		if (errorcount == 3)
			I_ShutdownSound();
		if (errorcount == 4)
			I_ShutdownGraphics();
		if (errorcount == 5)
			I_ShutdownInput();
		if (errorcount == 6)
			I_ShutdownSystem();
		if (errorcount == 7)
			SDL_Quit();
		if (errorcount == 8)
		{
			M_SaveConfig(NULL);
			G_SaveGameData(clientGamedata);
		}
		if (errorcount > 20)
		{
			va_start(argptr, error);
			vsnprintf(buffer, sizeof buffer, error, argptr);
			va_end(argptr);



			if (!M_CheckParm("-dedicated"))
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					"SRB2 "VERSIONSTRING" Recursive Error",
					buffer, NULL);

			W_Shutdown();
			exit(-1);  
		}
	}

	shutdowning = true;


	va_start(argptr, error);
	vsnprintf(buffer, sizeof buffer, error, argptr);
	va_end(argptr);
	I_OutputMsg("\nI_Error(): %s\n", buffer);


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
	I_ShutdownMusic();
	I_ShutdownSound();

	I_ShutdownGraphics();
	I_ShutdownInput();
	I_ShutdownSystem();
	SDL_Quit();




	if (!M_CheckParm("-dedicated"))
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"SRB2 "VERSIONSTRING" Error",
			buffer, NULL);






	W_Shutdown();

#if defined (PARANOIA) && defined (__CYGWIN__)
	*(INT32 *)2 = 4;  
#endif

	exit(-1);
}

                              
  
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS];  




void I_AddExitFunc(void (*func)())
{
	INT32 c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}






void I_RemoveExitFunc(void (*func)())
{
	INT32 c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c < MAX_QUIT_FUNCS-1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

#if !(defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON))
static void Shittycopyerror(const char *name)
{
	I_OutputMsg(
			"Error copying log file: %s: %s\n",
			name,
			strerror(errno)
	);
}

static void Shittylogcopy(void)
{
	char buf[8192];
	FILE *fp;
	size_t r;
	if (fseek(logstream, 0, SEEK_SET) == -1)
	{
		Shittycopyerror("fseek");
	}
	else if (( fp = fopen(logfilename, "wt") ))
	{
		while (( r = fread(buf, 1, sizeof buf, logstream) ))
		{
			if (fwrite(buf, 1, r, fp) < r)
			{
				Shittycopyerror("fwrite");
				break;
			}
		}
		if (ferror(logstream))
		{
			Shittycopyerror("fread");
		}
		fclose(fp);
	}
	else
	{
		Shittycopyerror(logfilename);
	}
}
#endif 








void I_ShutdownSystem(void)
{
	INT32 c;

#ifdef NEWSIGNALHANDLER
	if (M_CheckParm("-nofork"))
#endif
		I_ShutdownConsole();

	for (c = MAX_QUIT_FUNCS-1; c >= 0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
#ifdef LOGMESSAGES
	if (logstream)
	{
		I_OutputMsg("I_ShutdownSystem(): end of logstream.\n");
#if !(defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON))
		                                                                    
                                                                       
		if (!M_CheckParm("-logfile"))
			Shittylogcopy();
#endif
		fclose(logstream);
		logstream = NULL;
	}
#endif

}

void I_GetDiskFreeSpace(INT64 *freespace)
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

char *I_GetEnv(const char *name)
{
#ifdef NEED_SDL_GETENV
	return SDL_getenv(name);
#else
	return getenv(name);
#endif
}

INT32 I_PutEnv(char *variable)
{
#ifdef NEED_SDL_GETENV
	return SDL_putenv(variable);
#else
	return putenv(variable);
#endif
}

size_t I_GetRandomBytes(char *destination, size_t count)
{
#if defined (__unix__) || defined (UNIXCOMMON) || defined(__APPLE__)
	FILE *rndsource;
	size_t actual_bytes = 0;

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

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	char storage[256];
	if (size > 255)
		size = 255;
	memcpy(storage, data, size);
	storage[size] = 0;

	if (SDL_SetClipboardText(storage))
		return 0;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	static char clipboard_modified[256];
	char *clipboard_contents, *i = clipboard_modified;

	if (!SDL_HasClipboardText())
		return NULL;

	clipboard_contents = SDL_GetClipboardText();
	strlcpy(clipboard_modified, clipboard_contents, 256);
	SDL_free(clipboard_contents);

	while (*i)
	{
		if (*i == '\n' || *i == '\r')
		{  
			*i = 0;
			break;
		}
		else if (*i == '\t')
			*i = ' ';  
		else if (*i < 32 || (unsigned)*i > 127)
			*i = '?';  
		++i;
	}
	return (const char *)&clipboard_modified;
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
	static char tempsw[MAX_WADPATH] = "";
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

	I_OutputMsg(",HOME/" DEFAULTDIR);
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

const CPUInfoFlags *I_CPUInfo(void)
{
#if defined (_WIN32)
	static CPUInfoFlags WIN_CPUInfo;
	SYSTEM_INFO SI;
	p_IsProcessorFeaturePresent pfnCPUID = (p_IsProcessorFeaturePresent)(LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsProcessorFeaturePresent");

	ZeroMemory(&WIN_CPUInfo,sizeof (WIN_CPUInfo));
	if (pfnCPUID)
	{
		WIN_CPUInfo.FPPE       = pfnCPUID( 0);  
		WIN_CPUInfo.FPE        = pfnCPUID( 1);  
		WIN_CPUInfo.cmpxchg    = pfnCPUID( 2);  
		WIN_CPUInfo.MMX        = pfnCPUID( 3);  
		WIN_CPUInfo.PPCMM64    = pfnCPUID( 4);  
		WIN_CPUInfo.ALPHAbyte  = pfnCPUID( 5);  
		WIN_CPUInfo.SSE        = pfnCPUID( 6);  
		WIN_CPUInfo.AMD3DNow   = pfnCPUID( 7);  
		WIN_CPUInfo.RDTSC      = pfnCPUID( 8);  
		WIN_CPUInfo.PAE        = pfnCPUID( 9);  
		WIN_CPUInfo.SSE2       = pfnCPUID(10);  

		WIN_CPUInfo.DEP        = pfnCPUID(12);  
		WIN_CPUInfo.SSE3       = pfnCPUID(13);  
		WIN_CPUInfo.cmpxchg16b = pfnCPUID(14);  
		WIN_CPUInfo.cmp8xchg16 = pfnCPUID(15);  
		WIN_CPUInfo.PFC        = pfnCPUID(16);  
	}
#ifdef HAVE_SDLCPUINFO
	else
	{
		WIN_CPUInfo.RDTSC       = SDL_HasRDTSC();
		WIN_CPUInfo.MMX         = SDL_HasMMX();
		WIN_CPUInfo.AMD3DNow    = SDL_Has3DNow();
		WIN_CPUInfo.SSE         = SDL_HasSSE();
		WIN_CPUInfo.SSE2        = SDL_HasSSE2();
		WIN_CPUInfo.AltiVec     = SDL_HasAltiVec();
	}
	WIN_CPUInfo.MMXExt      = SDL_FALSE;  
	WIN_CPUInfo.AMD3DNowExt = SDL_FALSE;  
#endif
	GetSystemInfo(&SI);
	WIN_CPUInfo.CPUs = SI.dwNumberOfProcessors;
	WIN_CPUInfo.IA64 = (SI.dwProcessorType == 2200);  
	WIN_CPUInfo.AMD64 = (SI.dwProcessorType == 8664);  
	return &WIN_CPUInfo;
#elif defined (HAVE_SDLCPUINFO)
	static CPUInfoFlags SDL_CPUInfo;
	memset(&SDL_CPUInfo,0,sizeof (CPUInfoFlags));
	SDL_CPUInfo.RDTSC       = SDL_HasRDTSC();
	SDL_CPUInfo.MMX         = SDL_HasMMX();
	SDL_CPUInfo.MMXExt      = SDL_FALSE;  
	SDL_CPUInfo.AMD3DNow    = SDL_Has3DNow();
	SDL_CPUInfo.AMD3DNowExt = SDL_FALSE;  
	SDL_CPUInfo.SSE         = SDL_HasSSE();
	SDL_CPUInfo.SSE2        = SDL_HasSSE2();
	SDL_CPUInfo.AltiVec     = SDL_HasAltiVec();
	return &SDL_CPUInfo;
#else
	return NULL;  
#endif
}


void I_RegisterSysCommands(void) {}

const char *I_GetSysName(void)
{
	return SDL_GetPlatform();
}


void I_SetTextInputMode(boolean active)
{
	if (active)
		SDL_StartTextInput();
	else
		SDL_StopTextInput();
}

boolean I_GetTextInputMode(void)
{
	return SDL_IsTextInputActive();
}

#endif
