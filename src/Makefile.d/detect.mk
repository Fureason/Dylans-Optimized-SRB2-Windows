




all_systems:=\
	LINUX64\
	MINGW64\
	MINGW\
	UNIX\
	LINUX\
	FREEBSD\
	HAIKU\


ifeq (,$(filter $(all_systems),$(.VARIABLES)))
ifeq ($(OS),Windows_NT)  

_m=Detected a Windows system,\
	compiling for 32-bit MinGW SDL...)
$(call Print,$(_m))


MINGW:=1

else  

system:=$(shell uname -s)

ifeq ($(system),Linux)
new_system:=LINUX
else ifeq ($(system),Haiku)
new_system:=HAIKU
else

$(error \
	Could not automatically detect your system,\
	try specifying a system manually)

endif

ifeq ($(shell getconf LONG_BIT),64)
system+=64-bit
new_system:=$(new_system)64
endif

$(call Print,Detected $(system) ($(new_system))...)
$(new_system):=1

endif
endif


gcc_versions:=\
	132 131 130\
	123 122 121 120\
	114 113 112 111 110\
	105 104 103 102 101 100\
	95 94 93 92 91 90\
	85 84 83 82 81 80\
	75 74 73 72 71 70\
	64 63 62 61 60\
	55 54 53 52 51 50\
	49 48 47 46 45 44 43 42 41 40

latest_gcc_version:=13.2




ifeq (,$(call Wildvar,GCC% destructive))




shellversion:=$(shell $(CC) -v 2>&1)

version:=$(subst -win32,.0,$(shellversion))


ifneq (,$(findstring gcc version,$(version)))


shellversion:=$(shell $(CC) -dumpfullversion)


version:=$(subst -win32,.0,$(shellversion))


v:=$(subst ., ,$(version))

v:=$(word 1,$(v))$(word 2,$(v))



ifeq (,$(filter $(v),$(gcc_versions)))
define line =
Your compiler version, GCC $(version), \
is not supported by the Makefile.
The Makefile will assume GCC $(latest_gcc_version).
endef
$(call Print,$(line))
GCC$(subst .,,$(latest_gcc_version)):=1
else
$(call Print,Detected GCC $(version) (GCC$(v)))
GCC$(v):=1
endif

endif
endif
