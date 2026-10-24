#!/usr/bin/env make -f

## wadconv: SRB2 WAD Converter.
## Copyright (C) 2011 by Callum Dickinson.
#
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
#
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.


PRONAME = wadconv

PROVER	= 1.0.0


CPPFLAGS	+= -DPRONAME="$(PRONAME)" -DPROVER="$(PROVER)"


SRC	= wadconv.c


LIB	= -lwad







PRCC	= CC      
PRINSTALL=INSTALL 
PRCLEAN	= CLEAN   


LIBPFX	= lib


PREFIX	?= /usr/local
LIBDIR	:= $(PREFIX)/lib
INCLUDE := $(PREFIX)/include


OBJ	= $(SRC:.c=.o)
DEP	= $(PRONAME).d
ifdef MINGW
BIN     = $(PRONAME).exe
else
BIN	= $(PRONAME)
endif


ECHO	:= echo
RM	:= rm -rf
FIND	:= find
GREP	:= grep
INSTALLX:= install -m 755
FOR	:= for
SED	:= sed
CC	:= $(CROSS_COMPILE)$(CC)


SHECHO	:= $(ECHO)
SHFIND	:= $(FIND)
SHGREP	:= $(GREP)
SHINSTALL:= $(INSTALL)
SHSED	:= $(SED)
SHCC	:= $(CC)


NULL	:= /dev/null 2>&1
ECHO	:= @$(ECHO)
ifndef V
RM	:= @$(RM)
LDCONFIG:= @$(LDCONFIG)
FIND	:= @$(FIND)
GREP	:= @$(GREP)
INSTALL	:= @$(INSTALL)
MKDIR	:= @$(MKDIR)
FOR	:= @$(FOR)
SED	:= @$(SED)
CC	:= @$(CC)
endif


ifdef DEBUG
CFLAGS	+= -g -O0 -Wall
else
CFLAGS	+= -O3
endif








all: $(DEP) $(BIN)

$(DEP): $(SRC)
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
	-$(RM) $@
	$(FOR) f in $(SRC); do \
		$(SHCC) $(CPPFLAGS) $(CFLAGS) -MM $$f >> $@; \
	done
-include $@

%.o: %.c
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
ifndef V
	$(ECHO) "  $(PRCC)$@"
endif
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIB)

install: all
ifndef V
	$(SHECHO) "  $(PRINSTALL)$(BIN)"
endif
	$(INSTALLX) $(BIN) $(INSTALLDIR)

clean:
ifndef V
	$(ECHO) "  $(PRCLEAN)$(BIN) $(OBJ) $(DEP)"
endif
	-$(RM) $(BIN) $(OBJ) $(DEP)

.PHONY : all install clean
