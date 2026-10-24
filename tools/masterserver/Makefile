# Copyright (C) 2000 by DooM Legacy Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#







PORT=28900




CCFILES=common.cpp ipcs.cpp crypt.cpp md5.cpp
Main_CCFILES=server.cpp  



MYSQL_CONFIG?= mysql_config
MYSQL_CFLAGS?=$(shell $(MYSQL_CONFIG) --cflags)
MYSQL_LIBS?=$(shell $(MYSQL_CONFIG) --libs)

CXXFLAGS+=-Wall -O3 -Wextra
ifdef MINGW
LIBS+=-lws2_32
else
LIBS+=-lm
endif

CXXFLAGS+=$(MYSQL_CFLAGS)
LIBS+=$(MYSQL_LIBS)

ifdef DEBUG
CXXFLAGS+=-g -D__DEBUG__
endif

SRCS=$(CCFILES) $(Main_CCFILES)
OFILES=$(CCFILES:.cpp=.o)
Main_OFILES=$(Main_CCFILES:.cpp=.o)
EXEFILE=$(Main_CCFILES:.cpp=)



.SUFFIXES:	.cpp .h

default:	init $(EXEFILE) end

init:
		@echo
		@echo "Compiling options:"
		@echo "  CXX     : $(CXX)"
		@echo "  LDFLAGS : $(LDFLAGS)"
		@echo "  LIBS    : $(LIBS)"
		@echo "  CXXFLAGS: $(CXXFLAGS)"
		@echo
		@echo "Files:"
		@echo "  SRCS    : $(SRCS)"
		@echo "  OFILES  : $(OFILES) $(Main_OFILES)"
		@echo "  EXEFILE : $(EXEFILE)"
		@echo

end:
		@echo
		@echo "*** Makefile ended successfully  ***"
		@echo



$(EXEFILE): $(OFILES) $(Main_OFILES)
		@echo Linking ...
		@$(CXX) $(LDFLAGS) $(OFILES) $@.o -o $@ $(LIBS)
ifndef CYGWIN
ifndef MINGW
		@chmod 755 $@
endif
endif
		@echo File $@ has been created



.cpp.o:
		@echo Compiling $< -\> $@
		@$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@



debug:		debug_
debug_:
		@make "CXXFLAGS = $(CXXFLAGS) -D__DEBUG__"
		@echo

clean:		clean_
clean_:
		@echo
		@echo Removing obejcts files, executable files and eventually core file...
		@rm -f $(OFILES) $(Main_OFILES) $(EXEFILE) core
		@echo

realclean:	clean
		@echo Removing backup files...
		@rm -f *~ *.bak
		@echo

depend:		dep
dep:
		@echo
		@echo Make dependencies...
		@makedepend  -- $(CXXFLAGS) -- $(Main_CCFILES) $(CCFILES) 2> /dev/null
		@rm -f Makefile.bak
		@echo

BAKFILE = ../SRB2MasterServer
backup:		bak
bak:		clean
		@echo Copy files to $(BAKFILE).tgz...
		@tar cvf $(BAKFILE).tar * > /dev/null
		@gzip $(BAKFILE).tar
		@mv $(BAKFILE).tar.gz $(BAKFILE).tgz
		@echo Removing backup files...
		@rm -f *~ *.bak
		@echo


mrproper:       clean default

install:	mrproper
		@echo "Launching the server, port $(PORT)..."
		@./server $(PORT)
		@echo
