









makedir:=$(makedir)/SDL

sources+=$(call List,sdl/Sourcefile)
opts+=-DDIRECTFULLSCREEN -DHAVE_SDL











ifndef NOHW
sources+=sdl/ogl_sdl.c
endif

ifdef NOMIXER
sources+=sdl/sdl_sound.c
else
opts+=-DHAVE_MIXER
sources+=sdl/mixer_sound.c

  ifndef HAIKU  
    ifdef HAVE_MIXERX
    opts+=-DHAVE_MIXERX
    libs+=-lSDL2_mixer_ext
    else
    SDL_MIXER_LDFLAGS?=-lSDL2_mixer
    libs+=$(SDL_MIXER_LDFLAGS)
    endif
  endif
endif

ifndef NOTHREADS
opts+=-DHAVE_THREADS
sources+=sdl/i_threads.c
endif

ifdef SDL_PKGCONFIG
$(eval $(call Use_pkg_config,SDL))
else
SDL_CONFIG?=$(call Prefix,sdl2-config)
SDL_CFLAGS?=$(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS?=$(shell $(SDL_CONFIG) \
		$(if $(STATIC),--static-libs,--libs))
$(eval $(call Propogate_flags,SDL))
endif

ifdef MINGW
ifndef NOSDLMAIN
SDLMAIN=1
endif
endif

ifdef SDLMAIN
opts+=-DSDLMAIN
else
ifdef MINGW
opts+=-Umain
libs+=-mconsole
endif
endif
