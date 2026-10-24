# GNU Makefile for SRB2
# the poly3 Makefile adapted over and over...
#
# Copyright 1998-2000 DooM Legacy Team.
# Copyright 2020-2023 James R.
# Copyright 2003-2023 Sonic Team Junior.
#
# This program is free software distributed under the
# terms of the GNU General Public License, version 2.
# See the 'LICENSE' file for more details.
#












































































































clean_targets=cleandep clean distclean info

.PHONY : $(clean_targets) all linux-objects-only

goals:=$(or $(MAKECMDGOALS),all)
cleanonly:=$(filter $(clean_targets),$(goals))
destructive:=$(filter-out info,$(cleanonly))

ifndef cleanonly
include Makefile.d/old.mk
endif

include Makefile.d/util.mk

ifdef PREFIX
CC:=$(PREFIX)-gcc
endif

OBJDUMP_OPTS?=--wide --source --line-numbers

OBJCOPY?=$(call Prefix,objcopy)
OBJDUMP?=$(call Prefix,objdump)
WINDRES?=$(call Prefix,windres)

GZIP?=gzip
GZIP_OPTS?=-9 -f -n
ifdef WINDOWSHELL
GZIP_OPTS+=--rsyncable
endif

UPX_OPTS?=--best --preserve-build-id
ifndef ECHO
UPX_OPTS+=-qq
endif

include Makefile.d/detect.mk


.PRECIOUS : %/ comptime.c

sources:=
makedir:=../make


opts:=-DCOMPVERSION -g -fwrapv
libs:=



passthru_opts:=

include Makefile.d/platform.mk
include Makefile.d/features.mk
include Makefile.d/versions.mk

ifdef DEBUGMODE
makedir:=$(makedir)/debug
endif

depdir:=$(makedir)/deps
objdir:=$(makedir)/objs


sources+=\
	$(call List,Sourcefile)\
	$(call List,blua/Sourcefile)\
	$(call List,netcode/Sourcefile)\

depends:=$(basename $(filter %.c %.s,$(sources)))
objects:=$(basename $(filter %.c %.s %.nas,$(sources)))

depends:=$(depends:%=$(depdir)/%.d)





objects:=$(objects:=.o) comptime.o


rc_file:=$(basename $(filter %.rc,$(sources)))
ifdef rc_file
objects+=$(rc_file:=.res)
endif

objects:=$(addprefix $(objdir)/,$(objects))

ifdef DEBUGMODE
bin:=../bin/debug
else
bin:=../bin
endif


EXENAME?=srb2
DBGNAME?=$(EXENAME).debug

exe:=$(bin)/$(EXENAME)
dbg:=$(bin)/$(DBGNAME)

build_done==== Build is done, look for \
           $(<F) at $(abspath $(<D)) ===

linux-objects-only : $(objects)

all : $(exe)
	$(call Echo,$(build_done))

ifndef VALGRIND
dump : $(dbg).txt
endif

ifdef STATIC
libs+=-static
endif


ifdef PROFILEMODE
opts+=-pg
libs+=-pg
endif

ifdef DEBUGMODE
debug_opts=-D_DEBUG
else  
debug_opts=-DNDEBUG
opts+=-O3
endif


opts+=$(debug_opts)

opts+=$(foreach v,$(passthru_opts),$(if $($(v)),-D$(v)))

opts+=$(WFLAGS) $(CPPFLAGS) $(CFLAGS)
libs+=$(LDFLAGS)
asflags:=$(ASFLAGS) -x assembler-with-cpp

cc=$(CC)

ifdef DISTCC
cc=distcc $(CC)
endif

ifdef CCACHE
cc=ccache $(CC)
endif

ifndef SILENT


ifndef MAKE_RESTARTS
ifndef destructive
$(shell $(CC) -v)
define flags =

SHELL ..... $(SHELL)

CC ........ $(cc)

CFLAGS .... $(opts)

LDFLAGS ... $(libs)

endef
$(info $(flags))
endif

ifndef cleanonly
$(info Checking dependency files...)
include $(depends)
endif
endif
endif

LD:=$(CC)
cc:=$(cc) $(opts)
ifdef UPX
upx=$(UPX) $(UPX_OPTS)
endif
windres=$(WINDRES) $(WINDRESFLAGS)\
	$(debug_opts) --include-dir=win32 -O coff

%/ :
	$(.)$(mkdir) $(call Windows_path,$@)



.SECONDEXPANSION :


unexport UPX


$(exe) : $(dbg) | $$(@D)/
	$(.)$(OBJCOPY) --strip-debug $< $@
	$(.)-$(OBJCOPY) --add-gnu-debuglink=$< $@
ifdef UPX
	$(call Echo,Compressing final executable...)
	$(.)-$(upx) $@
endif


$(dbg) : $(objects) | $$(@D)/
	$(call Echo,Linking $(@F)...)
	$(.)$(LD) -o $@ $^ $(libs)


$(dbg).txt : $(dbg)
	$(call Echo,Dumping debugging info...)
	$(.)$(OBJDUMP) $(OBJDUMP_OPTS) $< > $@
	$(.)$(GZIP) $(GZIP_OPTS) $@



comptime.c ::
ifdef WINDOWSHELL
	-$(.)..\comptime.bat .
else
	-$(.)sh ../comptime.sh .
endif


$(objdir)/win32/Srb2win.res : \
	win32/afxres.h win32/resource.h




define _recipe =
$(depdir)/%.d : %.$(1) | $$$$(@D)/
ifndef WINDOWSHELL
ifdef Echo_name
	@printf '%-20.20s\r' $$<
endif
endif
	$(.)$(cc) -MM -MF $$@ -MT $(objdir)/$$*.o $(2) $$<
endef

$(eval $(call _recipe,c))
$(eval $(call _recipe,s,$(asflags)))





define _recipe =
$(objdir)/%.$(1) : %.$(2) | $$$$(@D)/
	$(call Echo_name,$$<)
	$(.)$(3)
endef

$(eval $(call _recipe,o,c,$(cc) -c -o $$@ $$<))
$(eval $(call _recipe,o,s,$(cc) $(asflags) -c -o $$@ $$<))
$(eval $(call _recipe,res,rc,$(windres) -i $$< -o $$@))

_rm=$(.)$(rmrf) $(call Windows_path,$(1))

cleandep :
	$(call _rm,$(depends) comptime.h)

clean :
	$(call _rm,$(exe) $(dbg) $(dbg).txt $(objects))

distclean :
	$(call _rm,../bin ../objs ../dep ../make comptime.h)

info:
ifdef WINDOWSHELL
	@REM
else
	@:
endif


