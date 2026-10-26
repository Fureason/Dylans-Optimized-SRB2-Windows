#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <wchar.h>

#define RULE_NAME L"SRB2 Optimized Easy Host UDP 5029"

static int RunNetsh(const wchar_t *arguments)
{
	wchar_t netsh[MAX_PATH];
	wchar_t command[4096];
	STARTUPINFOW startup;
	PROCESS_INFORMATION process;
	DWORD exitcode = 1;

	if (!GetSystemDirectoryW(netsh, MAX_PATH))
		return 1;
	wcsncat(netsh, L"\\netsh.exe", MAX_PATH - wcslen(netsh) - 1);
	_snwprintf(command, sizeof command / sizeof *command,
		L"\"%ls\" %ls", netsh, arguments);
	command[(sizeof command / sizeof *command) - 1] = L'\0';

	ZeroMemory(&startup, sizeof startup);
	ZeroMemory(&process, sizeof process);
	startup.cb = sizeof startup;
	startup.dwFlags = STARTF_USESHOWWINDOW;
	startup.wShowWindow = SW_HIDE;

	if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW,
		NULL, NULL, &startup, &process))
		return 1;

	if (WaitForSingleObject(process.hProcess, 30000) == WAIT_OBJECT_0)
		GetExitCodeProcess(process.hProcess, &exitcode);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return exitcode == 0 ? 0 : 1;
}

static int InstallRule(void)
{
	wchar_t program[MAX_PATH];
	wchar_t *slash;
	wchar_t *parentslash;
	wchar_t arguments[4096];

	if (!GetModuleFileNameW(NULL, program, MAX_PATH))
		return 1;
	slash = wcsrchr(program, L'\\');
	if (!slash)
		return 1;
	wcscpy(slash + 1, L"srb2win.exe");
	if (GetFileAttributesW(program) == INVALID_FILE_ATTRIBUTES)
	{
		/* Packaged builds keep this tool in an Easy Host subfolder. Look one
		 * directory above it while retaining same-folder compatibility. */
		*slash = L'\0';
		parentslash = wcsrchr(program, L'\\');
		if (!parentslash)
			return 1;
		wcscpy(parentslash + 1, L"srb2win.exe");
		if (GetFileAttributesW(program) == INVALID_FILE_ATTRIBUTES)
		{
			MessageBoxW(NULL,
				L"Keep the Easy Host folder beside srb2win.exe. The setup program could not find the client one folder above it.",
				L"SRB2 Easy Host Setup", MB_OK | MB_ICONERROR);
			return 1;
		}
	}

	/* Delete every old copy first. netsh returns an error when none exist,
	 * which is harmless because the new rule is created immediately after. */
	RunNetsh(L"advfirewall firewall delete rule name=\"" RULE_NAME L"\"");

	_snwprintf(arguments, sizeof arguments / sizeof *arguments,
		L"advfirewall firewall add rule name=\"%ls\" dir=in action=allow "
		L"protocol=UDP localport=5029 profile=any program=\"%ls\" enable=yes",
		RULE_NAME, program);
	arguments[(sizeof arguments / sizeof *arguments) - 1] = L'\0';
	if (RunNetsh(arguments))
	{
		MessageBoxW(NULL,
			L"Windows could not create the SRB2 firewall rule. No other firewall settings were changed.",
			L"SRB2 Easy Host Setup", MB_OK | MB_ICONERROR);
		return 1;
	}

	MessageBoxW(NULL,
		L"Windows Firewall setup is complete. One inbound UDP 5029 rule now allows this copy of SRB2 on Public and Private network profiles.\n\n"
		L"This confirms only the PC firewall rule. It does not prove that the router opened the port or that the server is publicly reachable. Start the server and wait for SRB2's automatic Hosting Status check.",
		L"SRB2 Easy Host Setup", MB_OK | MB_ICONINFORMATION);
	return 0;
}

static int RemoveRule(void)
{
	/* Delete removes every duplicate with this name. A nonzero delete result is
	 * also returned when no matching rule exists, so verify with a separate
	 * query instead of treating that as a failure. */
	RunNetsh(L"advfirewall firewall delete rule name=\"" RULE_NAME L"\"");
	if (!RunNetsh(L"advfirewall firewall show rule name=\"" RULE_NAME L"\""))
	{
		MessageBoxW(NULL,
			L"Windows still reports an Easy Host firewall rule. No unrelated firewall rules were changed.",
			L"SRB2 Easy Host Setup", MB_OK | MB_ICONERROR);
		return 1;
	}

	MessageBoxW(NULL,
		L"The SRB2 Optimized Easy Host UDP 5029 firewall rule was removed. It should no longer appear in Allowed apps after that window is reopened or refreshed.",
		L"SRB2 Easy Host Setup", MB_OK | MB_ICONINFORMATION);
	return 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR commandline, int show)
{
	wchar_t executable[MAX_PATH];
	SHELLEXECUTEINFOW elevate;
	DWORD exitcode = 1;
	int choice;
	const wchar_t *elevated_argument;

	(void)instance;
	(void)previous;
	(void)show;

	if (commandline && wcsstr(commandline, L"--elevated-install"))
		return InstallRule();
	if (commandline && wcsstr(commandline, L"--elevated-remove"))
		return RemoveRule();

	choice = MessageBoxW(NULL,
		L"Choose what to do with the SRB2 Optimized Easy Host UDP 5029 firewall rule:\n\n"
		L"Yes — Install or refresh the rule for Public and Private profiles.\n\n"
		L"No — Remove every Easy Host rule with this name.\n\n"
		L"Cancel — Make no changes.\n\n"
		L"Neither option enables Network Discovery or File Sharing.",
		L"SRB2 Easy Host Firewall Setup",
		MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);
	if (choice == IDCANCEL)
		return 0;
	elevated_argument = choice == IDYES
		? L"--elevated-install" : L"--elevated-remove";

	if (!GetModuleFileNameW(NULL, executable, MAX_PATH))
		return 1;
	ZeroMemory(&elevate, sizeof elevate);
	elevate.cbSize = sizeof elevate;
	elevate.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	elevate.lpVerb = L"runas";
	elevate.lpFile = executable;
	elevate.lpParameters = elevated_argument;
	elevate.nShow = SW_HIDE;
	if (!ShellExecuteExW(&elevate))
	{
		if (GetLastError() != ERROR_CANCELLED)
			MessageBoxW(NULL, L"Windows could not start Easy Host Setup as administrator.",
				L"SRB2 Easy Host Setup", MB_OK | MB_ICONERROR);
		return 1;
	}

	WaitForSingleObject(elevate.hProcess, INFINITE);
	GetExitCodeProcess(elevate.hProcess, &exitcode);
	CloseHandle(elevate.hProcess);
	return (int)exitcode;
}
