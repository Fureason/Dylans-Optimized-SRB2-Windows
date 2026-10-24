#!/usr/bin/make -f








AWK = gawk
AWK = mawk
AWK = nawk
AWK = one-true-awk
AWK = awk   
CPP = $(CC) -E  

MOVE = mv
DELETE = rm -f
ECHO = echo
DFA_XTRA =  





DFNFLAGS = $(DEFS) $(CPPFLAGS) $(CFLAGS)


srcdir = .



pnglibconf.h: pnglibconf.dfn
	$(DELETE) $@ pnglibconf.c pnglibconf.out pnglibconf.tmp
	$(ECHO) '#include "pnglibconf.dfn"' >pnglibconf.c
	$(ECHO) "If '$(CC) -E' crashes try /lib/cpp (e.g. CPP='/lib/cpp')" >&2
	$(CPP) $(DFNFLAGS) pnglibconf.c >pnglibconf.out
	$(AWK) -f "$(srcdir)/scripts/dfn.awk" out="pnglibconf.tmp" pnglibconf.out 1>&2
	$(MOVE) pnglibconf.tmp $@

pnglibconf.dfn: $(srcdir)/scripts/pnglibconf.dfa $(srcdir)/scripts/options.awk $(srcdir)/pngconf.h $(srcdir)/pngusr.dfa $(DFA_XTRA)
	$(DELETE) $@ pnglibconf.pre pnglibconf.tmp
	$(ECHO) "Calling $(AWK) from scripts/pnglibconf.mak" >&2
	$(ECHO) "If 'awk' crashes try a better awk (e.g. AWK='nawk')" >&2
	$(AWK) -f $(srcdir)/scripts/options.awk out="pnglibconf.pre"\
	    version=search $(srcdir)/pngconf.h $(srcdir)/scripts/pnglibconf.dfa\
	    $(srcdir)/pngusr.dfa $(DFA_XTRA) 1>&2
	$(AWK) -f $(srcdir)/scripts/options.awk out="pnglibconf.tmp" pnglibconf.pre 1>&2
	$(MOVE) pnglibconf.tmp $@

clean-pnglibconf:
	$(DELETE) pnglibconf.h pnglibconf.c pnglibconf.out pnglibconf.pre \
	pnglibconf.dfn

clean: clean-pnglibconf
