






include(LibFindMacros)

libfind_pkg_check_modules(SDL2_MAIN_PKGCONF SDL2)


find_path(SDL2_MAIN_INCLUDE_DIR
	NAMES SDL.h
	PATHS
		${SDL2_MAIN_PKGCONF_INCLUDE_DIRS}
		"/usr/include/SDL2"
		"/usr/local/include/SDL2"
)


find_library(SDL2_MAIN_LIBRARY
	NAMES SDL2_main
	PATHS
		${SDL2_MAIN_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)



set(SDL2_MAIN_PROCESS_INCLUDES SDL2_MAIN_INCLUDE_DIR)
set(SDL2_MAIN_PROCESS_LIBS SDL2_MAIN_LIBRARY)
libfind_process(SDL2_MAIN)
