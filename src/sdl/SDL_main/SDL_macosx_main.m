                                                            
                                                              
                                                             

                                                       
  

#import "SDL.h"
#import "SDL_macosx_main.h"
#import <sys/param.h>  
#import <unistd.h>

                                                                       
                                                                       
                      
@interface NSApplication(SDL_Missing_Methods)
- (void)setAppleMenu:(NSMenu *)menu;
@end

 
#define		SDL_USE_NIB_FILE	0

 
#define		SDL_USE_CPS		1
#if SDL_USE_CPS
 
typedef struct CPSProcessSerNum
{
	UInt32		lo;
	UInt32		hi;
} CPSProcessSerNum;

extern OSErr	CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr 	CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr	CPSSetFrontProcess( CPSProcessSerNum *psn);

#endif  

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;

static void addArgument(const char *value)
{
	if(!gArgc)
		gArgv = (char **)malloc(sizeof(*gArgv));
	else
	{
		char **newgArgv = (char **)realloc(gArgv, sizeof(*gArgv) * (gArgc + 1));
		if (!newgArgv)
		{
			newgArgv = malloc(sizeof(*gArgv) * (gArgc + 1));
			memcpy(newgArgv, gArgv, sizeof(*gArgv) * gArgc);
			free(gArgv);
		}
		gArgv = newgArgv;
	}
	gArgc++;
	gArgv[gArgc - 1] = (char *)malloc(sizeof(char) * (strlen(value) + 1));
	strcpy(gArgv[gArgc - 1], value);
}

static NSString *getApplicationName(void)
{
    NSDictionary *dict;
    NSString *appName = NULL;

     
    dict = ( NSDictionary *)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
    if (dict)
        appName = [dict objectForKey: @"CFBundleName"];

    if (![appName length])
        appName = [[NSProcessInfo processInfo] processName];

    return appName;
}

#if SDL_USE_NIB_FILE
 
@interface NSString (ReplaceSubString)
- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString;
@end
#endif

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
 
- (void)terminate:(id)sender
{
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
    (void)sender;
#endif
     
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
@end

 
@implementation SDLMain

 
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
    if (shouldChdir)
    {
        char parentdir[MAXPATHLEN];
	CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
	if (CFURLGetFileSystemRepresentation(url2, true, (UInt8 *)parentdir, MAXPATHLEN))
        {
	       assert ( chdir (parentdir) == 0 );    
	}
	CFRelease(url);
	CFRelease(url2);
    }

}

#if SDL_USE_NIB_FILE

 
- (void)fixMenu:(NSMenu *)aMenu withAppName:(NSString *)appName
{
    NSRange aRange;
    NSEnumerator *enumerator;
    NSMenuItem *menuItem;

    aRange = [[aMenu title] rangeOfString:@"SDL App"];
    if (aRange.length != 0)
        [aMenu setTitle: [[aMenu title] stringByReplacingRange:aRange with:appName]];

    enumerator = [[aMenu itemArray] objectEnumerator];
    while ((menuItem = [enumerator nextObject]))
    {
        aRange = [[menuItem title] rangeOfString:@"SDL App"];
        if (aRange.length != 0)
            [menuItem setTitle: [[menuItem title] stringByReplacingRange:aRange with:appName]];
        if ([menuItem hasSubmenu])
            [self fixMenu:[menuItem submenu] withAppName:appName];
    }
    [ aMenu sizeToFit ];
}

#else

static void setApplicationMenu(void)
{
     
    NSMenu *appleMenu;
    NSMenuItem *menuItem;
    NSString *title;
    NSString *appName;

    appName = getApplicationName();
    appleMenu = [[NSMenu alloc] initWithTitle:@""];

     
    title = [@"About " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Hide " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

    menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

    [appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Quit " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];


     
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];

     
    [NSApp setAppleMenu:appleMenu];

     
    [appleMenu release];
    [menuItem release];
}

 
static void setupWindowMenu(void)
{
    NSMenu      *windowMenu;
    NSMenuItem  *windowMenuItem;
    NSMenuItem  *menuItem;

    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

     
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];

     
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];

     
    [NSApp setWindowsMenu:windowMenu];

     
    [windowMenu release];
    [windowMenuItem release];
}

 
static void CustomApplicationMain (int argc, char **argv)
{
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
    (void)argc;
    (void)argv;
#endif
    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
    SDLMain				*sdlMain;

     
    [SDLApplication sharedApplication];

#if SDL_USE_CPS
    {
        CPSProcessSerNum PSN;
         
        if (!CPSGetCurrentProcess(&PSN))
            if (!CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103))
                if (!CPSSetFrontProcess(&PSN))
                    [SDLApplication sharedApplication];
    }
#endif  

     
    [NSApp setMainMenu:[[NSMenu alloc] init]];
    setApplicationMenu();
    setupWindowMenu();

     
    sdlMain = [[SDLMain alloc] init];
    [NSApp setDelegate:sdlMain];

     
    [NSApp run];

    [sdlMain release];
    [pool release];
}

#endif

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
    (void)theApplication;
#endif
    addArgument("-iwad");
    addArgument([filename UTF8String]);
    return YES;
}

 
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
    (void)note;
#endif
    int status;

     
    [self setupWorkingDirectory:gFinderLaunch];

#if SDL_USE_NIB_FILE
     
    [self fixMenu:[NSApp mainMenu] withAppName:getApplicationName()];
#endif

    if (!getenv("SRB2WADDIR"))
        setenv("SRB2WADDIR", [[[NSBundle mainBundle] resourcePath] UTF8String], 1);

     
    status = SDL_main (gArgc, gArgv);

     
    exit(status);
}
@end


@implementation NSString (ReplaceSubString)

- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString
{
    size_t bufferSize;
    size_t selfLen = [self length];
    size_t aStringLen = [aString length];
    unichar *buffer;
    NSRange localRange;
    NSString *result;

    bufferSize = selfLen + aStringLen - aRange.length;
    buffer = NSAllocateMemoryPages(bufferSize*sizeof(unichar));

     
    localRange.location = 0;
    localRange.length = aRange.location;
    [self getCharacters:buffer range:localRange];

     
    localRange.location = 0;
    localRange.length = aStringLen;
    [aString getCharacters:(buffer+aRange.location) range:localRange];

     
    localRange.location = aRange.location + aRange.length;
    localRange.length = selfLen - localRange.location;
    [self getCharacters:(buffer+aRange.location+aStringLen) range:localRange];

     
    result = [NSString stringWithCharacters:buffer length:bufferSize];

    NSDeallocateMemoryPages(buffer, bufferSize);

    return result;
}

@end



#ifdef main
#  undef main
#endif


 
int main (int argc, char **argv)
{

     

     
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgc = 1;
        gFinderLaunch = YES;
    } else {
        gArgc = argc;
        gFinderLaunch = NO;
    }
    gArgv = argv;

     


#if SDL_USE_NIB_FILE
    [SDLApplication poseAsClass:[NSApplication class]];
    NSApplicationMain (argc, argv);
#else
    CustomApplicationMain (argc, argv);
#endif
    return 0;
}
