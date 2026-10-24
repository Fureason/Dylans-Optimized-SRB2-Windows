






include(LibFindMacros)

libfind_pkg_check_modules(SDL2_MIXER_PKGCONF SDL2_mixer)


find_path(SDL2_MIXER_INCLUDE_DIR
	NAMES SDL_mixer.h
	PATHS
		${SDL2_MIXER_PKGCONF_INCLUDE_DIRS}
		"/usr/include/SDL2"
		"/usr/local/include/SDL2"
)


find_library(SDL2_MIXER_LIBRARY
	NAMES SDL2_mixer
	PATHS
		${SDL2_MIXER_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)



set(SDL2_MIXER_PROCESS_INCLUDES SDL2_MIXER_INCLUDE_DIR)
set(SDL2_MIXER_PROCESS_LIBS SDL2_MIXER_LIBRARY)
libfind_process(SDL2_MIXER)

if(SDL2_MIXER_FOUND AND NOT TARGET SDL2_mixer::SDL2_mixer)
	add_library(SDL2_mixer::SDL2_mixer UNKNOWN IMPORTED)
	set_target_properties(
		SDL2_mixer::SDL2_mixer
		PROPERTIES
		IMPORTED_LOCATION "${SDL2_MIXER_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIR}"
	)
endif()
