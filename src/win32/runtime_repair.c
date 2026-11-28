// SPDX-License-Identifier: GPL-2.0-or-later

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>
#include <stdlib.h>
#include <wchar.h>

extern IMAGE_DOS_HEADER __ImageBase;
FARPROC WINAPI __delayLoadHelper2(PCImgDelayDescr descriptor, FARPROC *slot);

typedef struct
{
	WORD resource_id;
	const char *name;
} runtime_library_t;

static const runtime_library_t runtime_libraries[] = {
#include "windows-runtime-files.inc"
};

static INIT_ONCE runtime_init = INIT_ONCE_STATIC_INIT;
static wchar_t runtime_directory[MAX_PATH];

static void __attribute__((noreturn)) RuntimeStartupError(const wchar_t *action,
	const wchar_t *path, DWORD error)
{
	wchar_t detail[256] = L"";
	wchar_t message[768];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, 0, detail, ARRAYSIZE(detail), NULL);
	swprintf(message, ARRAYSIZE(message), L"%ls\n%ls\n\nWindows error %lu. %ls",
		action, path, (unsigned long)error, detail);
	MessageBoxW(NULL, message, L"Sonic Robo Blast 2", MB_OK | MB_ICONERROR);
	ExitProcess(EXIT_FAILURE);
}

static void RuntimeLibraryPath(wchar_t path[MAX_PATH], const char *name)
{
	const size_t length = wcslen(runtime_directory);
	wcscpy(path, runtime_directory);
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1,
		path + length, (int)(MAX_PATH - length)))
		RuntimeStartupError(L"Could not locate an SRB2 library.", runtime_directory, GetLastError());
}

static DWORD RestoreRuntimeLibrary(WORD resource_id, const wchar_t *path)
{
	DWORD attributes = GetFileAttributesW(path);
	if (attributes != INVALID_FILE_ATTRIBUTES)
		return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? ERROR_DIRECTORY : ERROR_SUCCESS;

	DWORD error = GetLastError();
	if (error != ERROR_FILE_NOT_FOUND)
		return error;

	HRSRC resource = FindResourceW(NULL, MAKEINTRESOURCEW(resource_id), RT_RCDATA);
	if (!resource)
		return GetLastError();

	DWORD size = SizeofResource(NULL, resource);
	HGLOBAL loaded = LoadResource(NULL, resource);
	const void *data = loaded ? LockResource(loaded) : NULL;
	if (!size || !data)
		return ERROR_INVALID_DATA;

	wchar_t temporary[MAX_PATH];
	if (!GetTempFileNameW(runtime_directory, L"srb", 0, temporary))
		return GetLastError();

	HANDLE file = CreateFileW(temporary, GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD written;
		if (!WriteFile(file, data, size, &written, NULL))
			error = GetLastError();
		else if (written != size)
			error = ERROR_WRITE_FAULT;
		else
			error = FlushFileBuffers(file) ? ERROR_SUCCESS : GetLastError();

		if (!CloseHandle(file) && error == ERROR_SUCCESS)
			error = GetLastError();

		if (error == ERROR_SUCCESS)
		{
			if (MoveFileExW(temporary, path, MOVEFILE_WRITE_THROUGH))
				return ERROR_SUCCESS;
			error = GetLastError();
		}
	}
	else
		error = GetLastError();

	DeleteFileW(temporary);

	attributes = GetFileAttributesW(path);
	if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
		return ERROR_SUCCESS;
	return error;
}

static BOOL CALLBACK RepairWindowsRuntime(PINIT_ONCE once, PVOID parameter, PVOID *context)
{
	(void)once;
	(void)parameter;
	(void)context;

	DWORD length = GetModuleFileNameW(NULL, runtime_directory, ARRAYSIZE(runtime_directory));
	if (!length || length >= ARRAYSIZE(runtime_directory))
		RuntimeStartupError(L"Could not locate the SRB2 folder.", L"", GetLastError());

	wchar_t *separator = wcsrchr(runtime_directory, L'\\');
	if (!separator)
		RuntimeStartupError(L"Could not locate the SRB2 folder.", runtime_directory, ERROR_BAD_PATHNAME);
	separator[1] = L'\0';

	for (size_t i = 0; i < ARRAYSIZE(runtime_libraries); ++i)
	{
		wchar_t path[MAX_PATH];
		RuntimeLibraryPath(path, runtime_libraries[i].name);
		DWORD error = RestoreRuntimeLibrary(runtime_libraries[i].resource_id, path);
		if (error != ERROR_SUCCESS)
			RuntimeStartupError(L"Could not restore an SRB2 library.", path, error);
	}

	return TRUE;
}

static HMODULE LoadRuntimeLibrary(const char *name, PVOID volatile *module_slot)
{
	wchar_t path[MAX_PATH];
	RuntimeLibraryPath(path, name);
	HMODULE library = LoadLibraryExW(path, NULL,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);

	if (!library && GetLastError() == ERROR_INVALID_PARAMETER)
		library = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!library)
		RuntimeStartupError(L"Could not load an SRB2 library.", path, GetLastError());

	HMODULE existing = (HMODULE)InterlockedCompareExchangePointer(module_slot, library, NULL);
	if (!existing)
		return library;

	FreeLibrary(library);
	return existing;
}



FARPROC WINAPI __delayLoadHelper2(PCImgDelayDescr descriptor, FARPROC *slot)
{
	if (!InitOnceExecuteOnce(&runtime_init, RepairWindowsRuntime, NULL, NULL))
		RuntimeStartupError(L"Could not check the SRB2 libraries.", L"", GetLastError());

	BYTE *image = (BYTE *)&__ImageBase;
	const char *name = (const char *)(image + descriptor->rvaDLLName);
	PVOID volatile *module_slot = (PVOID volatile *)(image + descriptor->rvaHmod);
	HMODULE library = (HMODULE)InterlockedCompareExchangePointer(module_slot, NULL, NULL);
	if (!library)
		library = LoadRuntimeLibrary(name, module_slot);

	const FARPROC *iat = (const FARPROC *)(image + descriptor->rvaIAT);
	const IMAGE_THUNK_DATA *names = (const IMAGE_THUNK_DATA *)(image + descriptor->rvaINT);
	ULONG_PTR import = names[slot - iat].u1.AddressOfData;
	const char *symbol = IMAGE_SNAP_BY_ORDINAL(import)
		? MAKEINTRESOURCEA(IMAGE_ORDINAL(import))
		: (const char *)((const IMAGE_IMPORT_BY_NAME *)(image + import))->Name;
	FARPROC function = GetProcAddress(library, symbol);
	if (!function)
	{
		DWORD error = GetLastError();
		wchar_t path[MAX_PATH];
		RuntimeLibraryPath(path, name);
		RuntimeStartupError(L"An SRB2 library is missing a required function.", path, error);
	}

	InterlockedExchangePointer((PVOID volatile *)slot, (PVOID)function);
	return function;
}
