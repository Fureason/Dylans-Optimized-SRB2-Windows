


#include <time.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

void I_WaitForRestart(void)
{
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
	INT32 p = M_CheckParm("-waitforpid");
	char *end;
	unsigned long pid;
	if (!p || !M_IsNextParm()) return;
	pid = strtoul(M_GetNextParm(), &end, 10);
	if (!pid || *end) return;
#ifdef _WIN32
	{
		HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
		if (parent)
		{
			DWORD status = WaitForSingleObject(parent, 15000);
			CloseHandle(parent);
			if (status != WAIT_OBJECT_0) exit(1);
		}
	}
#else
	{
		unsigned int waited;
		for (waited = 0; waited < 15000 && (kill((pid_t)pid, 0) == 0 || errno == EPERM); waited += 20)
			usleep(20000);
		if (waited == 15000) exit(1);
	}
#endif
#endif
}

#ifdef _WIN32


static boolean I_RestartAppendArg(wchar_t *line, size_t capacity, const char *arg)
{
	wchar_t wide[4096];
	size_t used = wcslen(line), slashes = 0, i, count;
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, arg, -1, wide, 4096)) return false;
	count = wcslen(wide);
	if (used + count*2 + 5 >= capacity) return false;
	if (used) line[used++] = L' ';
	line[used++] = L'"';
	for (i = 0; i < count; i++)
	{
		if (wide[i] == L'\\') { slashes++; continue; }
		if (wide[i] == L'"') slashes = slashes*2 + 1;
		while (slashes) { line[used++] = L'\\'; slashes--; }
		line[used++] = wide[i];
	}
	while (slashes) { line[used++] = L'\\'; line[used++] = L'\\'; slashes--; }
	line[used++] = L'"';
	line[used] = 0;
	return true;
}
#endif

boolean I_RestartForServer(const char *address)
{
	size_t i;
	if (!address || !*address || strlen(address) >= 96) return false;
	for (i = 0; address[i]; i++)
	{
		unsigned char ch = (unsigned char)address[i];
		if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z')
			|| (ch >= 'a' && ch <= 'z') || strchr(".:-[]%_", ch))) return false;
	}
#ifdef __ANDROID__
	{
		JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
		jobject activity = (jobject)SDL_AndroidGetActivity();
		jclass cls;
		jmethodID method;
		jstring target;
		jboolean result = JNI_FALSE;
		if (!env || !activity) return false;
		cls = (*env)->GetObjectClass(env, activity);
		method = (*env)->GetMethodID(env, cls, "prepareCleanRestart", "(Ljava/lang/String;)Z");
		target = (*env)->NewStringUTF(env, address);
		if (method && target) result = (*env)->CallBooleanMethod(env, activity, method, target);
		if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); result = JNI_FALSE; }
		if (target) (*env)->DeleteLocalRef(env, target);
		(*env)->DeleteLocalRef(env, cls);
		(*env)->DeleteLocalRef(env, activity);
		return result == JNI_TRUE;
	}
#elif defined(_WIN32) || defined(__linux__)
	{
		const char *args[32], *home = D_Home();
		char parentpid[32];
		char logfile[MAX_WADPATH];
		INT32 count = 0;
#ifdef _WIN32
		wchar_t executable[4096], line[16384] = {0};
		char utf8exe[12288];
		STARTUPINFOW startup;
		PROCESS_INFORMATION process;
		DWORD length = GetModuleFileNameW(NULL, executable, 4096);
		if (!length || length >= 4096 || !WideCharToMultiByte(CP_UTF8, 0, executable, -1,
			utf8exe, sizeof utf8exe, NULL, NULL)) return false;
		args[count++] = utf8exe;
		snprintf(parentpid, sizeof parentpid, "%lu", (unsigned long)GetCurrentProcessId());
#else
		char executable[4096];
		int pipefd[2], failure = 0;
		ssize_t length = readlink("/proc/self/exe", executable, sizeof executable-1), result;
		pid_t child;
		long limit = sysconf(_SC_OPEN_MAX);
		if (length <= 0 || length >= (ssize_t)sizeof executable-1) return false;
		executable[length] = 0;
		args[count++] = executable;
		snprintf(parentpid, sizeof parentpid, "%lu", (unsigned long)getpid());
#endif
		args[count++] = "-cleanjoin";
		args[count++] = "-waitforpid"; args[count++] = parentpid;
		args[count++] = "-nofork";
		args[count++] = "-noendtxt";
		if (home) { args[count++] = "-home"; args[count++] = home; }
		args[count++] = "-config"; args[count++] = configfile;
		args[count++] = rendermode == render_opengl ? "-opengl" : "-software";
		if (M_CheckParm("-noaudio")) args[count++] = "-noaudio";
		if (M_CheckParm("-nosound")) args[count++] = "-nosound";
		if (M_CheckParm("-nomusic")) args[count++] = "-nomusic";
		if (M_CheckParm("-nodiscord")) args[count++] = "-nodiscord";
		if (M_CheckParm("-logfile") && M_IsNextParm())
		{
			const char *old = M_GetNextParm(), *slash = strrchr(old, '/'), *backslash = strrchr(old, '\\');
			size_t directory;
			if (!slash || (backslash && backslash > slash)) slash = backslash;
			directory = slash ? (size_t)(slash-old+1) : 0;
			if (snprintf(logfile, sizeof logfile, "%.*slog-restart-%s-%llu.txt", (int)directory,
				old, parentpid, (unsigned long long)time(NULL)) >= (int)sizeof logfile) return false;
			args[count++] = "-logfile"; args[count++] = logfile;
		}
		args[count++] = "+connect"; args[count++] = address;
		args[count] = NULL;
#ifdef _WIN32
		for (i = 0; i < (size_t)count; i++)
			if (!I_RestartAppendArg(line, sizeof line/sizeof *line, args[i])) return false;
		memset(&startup, 0, sizeof startup); startup.cb = sizeof startup;
		memset(&process, 0, sizeof process);
		if (!CreateProcessW(executable, line, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) return false;
		CloseHandle(process.hThread); CloseHandle(process.hProcess);
		return true;
#else
		if (pipe(pipefd)) return false;
		if (fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) < 0) { close(pipefd[0]); close(pipefd[1]); return false; }
		child = fork();
		if (!child)
		{
			int fd;
			close(pipefd[0]);
			for (fd = 3; fd < limit; fd++) if (fd != pipefd[1]) close(fd);
			execv(executable, (char *const *)args);
			failure = errno;
			(void)write(pipefd[1], &failure, sizeof failure);
			_exit(127);
		}
		close(pipefd[1]);
		if (child < 0) { close(pipefd[0]); return false; }
		do { result = read(pipefd[0], &failure, sizeof failure); } while (result < 0 && errno == EINTR);
		close(pipefd[0]);
		if (result != 0) { if (result > 0) waitpid(child, NULL, 0); return false; }
		return true;
#endif
	}
#else
	return false;
#endif
}
