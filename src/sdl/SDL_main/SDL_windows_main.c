/*
    SDL_windows_main.c, placed in the public domain by Sam Lantinga  4/13/98

    The WinMain function -- calls your program's main() function
*/
#include "SDL_config.h"

#ifdef __WIN32__

#include <windows.h>

 
#include "SDL.h"
#include "SDL_main.h"

#ifdef main
#  undef main
#endif  

static void
UnEscapeQuotes(char *arg)
{
    char *last = NULL;

    while (*arg) {
        if (*arg == '"' && (last != NULL && *last == '\\')) {
            char *c_curr = arg;
            char *c_last = last;

            while (*c_curr) {
                *c_last = *c_curr;
                c_last = c_curr;
                c_curr++;
            }
            *c_last = '\0';
        }
        last = arg;
        arg++;
    }
}

 
static int
ParseCommandLine(char *cmdline, char **argv)
{
    char *bufp;
    char *lastp = NULL;
    int argc, last_argc;

    argc = last_argc = 0;
    for (bufp = cmdline; *bufp;) {
         
        while (SDL_isspace(*bufp)) {
            ++bufp;
        }
         
        if (*bufp == '"') {
            ++bufp;
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
             
            lastp = bufp;
            while (*bufp && (*bufp != '"' || *lastp == '\\')) {
                lastp = bufp;
                ++bufp;
            }
        } else {
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
             
            while (*bufp && !SDL_isspace(*bufp)) {
                ++bufp;
            }
        }
        if (*bufp) {
            if (argv) {
                *bufp = '\0';
            }
            ++bufp;
        }

         
        if (argv && last_argc != argc) {
            UnEscapeQuotes(argv[last_argc]);
        }
        last_argc = argc;
    }
    if (argv) {
        argv[argc] = NULL;
    }
    return (argc);
}

 
static BOOL
OutOfMemory(void)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", "Out of memory - aborting", NULL);
    return FALSE;
}

#if defined(_MSC_VER)
 
# define console_ansi_main main
# if UNICODE
#  define console_wmain wmain
# endif
#endif

 
static int
main_utf8(int argc, char *argv[])
{
    SDL_SetMainReady();

     
    return SDL_main(argc, argv);
}

 
int
console_ansi_main(int argc, char *argv[])
{
     
    return main_utf8(argc, argv);
}


#if UNICODE
 
int
console_wmain(int argc, wchar_t *wargv[], wchar_t *wenvp)
{
    int retval = 0;
    char **argv = SDL_stack_alloc(char*, argc);
    int i;

    for (i = 0; i < argc; ++i) {
        argv[i] = WIN_StringToUTF8(wargv[i]);
    }

    retval = main_utf8(argc, argv);

     
    SDL_stack_free(argv);

    return retval;
}
#endif

 
int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    char **argv;
    int argc;
    char *cmdline;

     
    TCHAR *text = GetCommandLine();
#if UNICODE
    cmdline = WIN_StringToUTF8(text);
#else
     
    cmdline = SDL_strdup(text);
#endif
    if (cmdline == NULL) {
        return OutOfMemory();
    }

     
    argc = ParseCommandLine(cmdline, NULL);
    argv = SDL_stack_alloc(char *, argc + 1);
    if (argv == NULL) {
        return OutOfMemory();
    }
    ParseCommandLine(cmdline, argv);

     
    main_utf8(argc, argv);

    SDL_stack_free(argv);

    SDL_free(cmdline);

     
    return 0;
}

#endif  

 
