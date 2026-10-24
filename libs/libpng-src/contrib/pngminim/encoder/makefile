



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


ZSRCS  = adler32$(C) compress$(C) crc32$(C) deflate$(C) \
	 trees$(C) zutil$(C)



ZH     = zlib.h crc32.h deflate.h trees.h zutil.h


ZCONF  = zconf.h


ZINC   = zlib.h $(ZCONF)


ZHDRS  = $(ZH) $(ZCONF)




ZOBJS  = adler32$(O) compress$(O) crc32$(O) deflate$(O) \
	 trees$(O) zutil$(O)


PNGSRCS=png$(C) pngerror$(C) pngget$(C) pngmem$(C) \
	pngset$(C) pngtrans$(C) pngwio$(C) pngwrite$(C) \
	pngwtran$(C) pngwutil$(C)


PNGH   =png.h pngconf.h pngdebug.h pnginfo.h pngpriv.h pngstruct.h


PNGCONF=pnglibconf.h


PNGINC= png.h pngconf.h pngusr.h $(PNGCONF)


PNGHDRS=$(PNGH) $(PNGCONF) pngusr.h

PNGOBJS=png$(O) pngerror$(O) pngget$(O) pngmem$(O) \
	pngset$(O) pngtrans$(O) pngwio$(O) pngwrite$(O) \
	pngwtran$(O) pngwutil$(O)

PROGSRCS= pnm2pngm$(C)
PROGHDRS=
PROGDOCS=
PROGOBJS= pnm2pngm$(O)

OBJS    = $(PROGOBJS) $(PNGOBJS) $(ZOBJS)



.c$(O):
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<



all: pnm2pngm$(E)

pnm2pngm$(E): $(OBJS)
	$(LD) -o pnm2pngm$(E) $(OBJS)






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
	$(RM) pnm2pngm$(O)
	$(RM) pnm2pngm$(E)
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

pnm2pngm.c: $(PROGSRC)/pnm2png.c
	$(RM) $@
	$(COPY) $(PROGSRC)/pnm2png.c $@


