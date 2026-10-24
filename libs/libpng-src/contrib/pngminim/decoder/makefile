



CC=gcc
LD=$(CC)







RM=rm -f
COPY=cp

CPPFLAGS=-I. -DPNG_USER_CONFIG -DNO_GZCOMPRESS -DZ_SOLO -DNO_GZIP
CFLAGS=-O1 -Wall

C=.c
O=.o
L=.a
E=


PNGSRC =../../..
ZLIBSRC=$(PNGSRC)/../zlib
PROGSRC=$(PNGSRC)/contrib/pngminus



ZSRCS  = adler32$(C) crc32$(C) \
	 inffast$(C) inflate$(C) inftrees$(C) \
	 zutil$(C)


ZH     = zlib.h crc32.h inffast.h inffixed.h \
	 inflate.h inftrees.h zutil.h


ZCONF  = zconf.h


ZINC   = zlib.h $(ZCONF)


ZHDRS  = $(ZH) $(ZCONF)

ZOBJS  = adler32$(O) crc32$(O) \
	 inffast$(O) inflate$(O) inftrees$(O) \
	 zutil$(O)


PNGSRCS=png$(C) pngerror$(C) pngget$(C) pngmem$(C) \
	pngread$(C) pngrio$(C) pngrtran$(C) pngrutil$(C) \
	pngset$(C) pngtrans$(C)
	

PNGH   =png.h pngconf.h pngdebug.h pnginfo.h pngpriv.h pngstruct.h


PNGCONF=pnglibconf.h


PNGINC= png.h pngconf.h pngusr.h $(PNGCONF)


PNGHDRS=$(PNGH) $(PNGCONF) pngusr.h

PNGOBJS=png$(O) pngerror$(O) pngget$(O) pngmem$(O) \
	pngread$(O) pngrio$(O) pngrtran$(O) pngrutil$(O) \
	pngset$(O) pngtrans$(O)

PROGSRCS= pngm2pnm$(C)
PROGHDRS=
PROGDOCS=
PROGOBJS= pngm2pnm$(O)

OBJS    = $(PROGOBJS) $(PNGOBJS) $(ZOBJS)




.c$(O):
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<



all: pngm2pnm$(E)

pngm2pnm$(E): $(OBJS)
	$(LD) -o pngm2pnm$(E) $(OBJS)






$(PNGCONF): $(PNGSRC)/scripts/pnglibconf.mak $(ZH)\
	$(PNGSRC)/scripts/pnglibconf.dfa \
	$(PNGSRC)/scripts/options.awk pngusr.h pngusr.dfa
	$(RM) pnglibconf.h pnglibconf.dfn
	$(MAKE) -f $(PNGSRC)/scripts/pnglibconf.mak $(MAKEFLAGS)\
	    srcdir=$(PNGSRC) CPPFLAGS="-DPNG_USER_CONFIG -I."\
	    DFA_XTRA="pngusr.dfa" $@

clean:
	$(MAKE) -f $(PNGSRC)/scripts/pnglibconf.mak $(MAKEFLAGS)\
	    srcdir=$(PNGSRC) clean
	$(RM) pngm2pnm$(O)
	$(RM) pngm2pnm$(E)
	$(RM) $(OBJS)


distclean: clean
	$(RM) -r scripts  
	$(RM) $(PNGSRCS) $(PNGH)
	$(RM) $(ZSRCS) $(ZH) $(ZCONF)
	$(RM) $(PROGSRCS) $(PROGHDRS) $(PROGDOCS)


$(PROGOBJS): $(PROGHDRS) $(PNGINC) $(ZINC)
$(PNGOBJS): $(PNGHDRS) $(ZINC)
$(ZOBJS): $(ZHDRS)


$(PNGSRCS) $(PNGH): $(PNGSRC)/$@
	$(RM) $@
	$(COPY) $(PNGSRC)/$@ $@



$(ZSRCS) $(ZH):
	$(RM) $@
	$(COPY) $(ZLIBSRC)/$@ $@



$(ZCONF):
	$(RM) $@
	@for f in zconf.h.in zconf.in.h zconf.h; do\
	    test -r $(ZLIBSRC)/$$f &&\
	    echo $(COPY) $(ZLIBSRC)/$$f $@ &&\
	    $(COPY) $(ZLIBSRC)/$$f $@ && exit 0;\
	done; echo copy: $(ZLIBSRC)/zconf.h not found; exit 1

pngm2pnm.c: $(PROGSRC)/png2pnm.c
	$(RM) $@
	$(COPY) $(PROGSRC)/png2pnm.c $@


