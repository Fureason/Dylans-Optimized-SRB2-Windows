






include(LibFindMacros)

libfind_pkg_check_modules(SDL2_PKGCONF SDL2)


find_path(SDL2_INCLUDE_DIR
	NAMES SDL.h
	PATHS
		${SDL2_PKGCONF_INCLUDE_DIRS}
		"/usr/include/SDL2"
		"/usr/local/include/SDL2"
)


find_library(SDL2_LIBRARY
	NAMES SDL2
	PATHS
		${SDL2_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)


set(SDL2_PROCESS_INCLUDES SDL2_INCLUDE_DIR)
set(SDL2_PROCESS_LIBS SDL2_LIBRARY)
libfind_process(SDL2)

if(SDL2_FOUND AND NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 UNKNOWN IMPORTED)
	set_target_properties(
		SDL2::SDL2
		PROPERTIES
		IMPORTED_LOCATION "${SDL2_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
	)
endif()
