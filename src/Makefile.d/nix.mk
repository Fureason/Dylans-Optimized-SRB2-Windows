



opts+=-DUNIXCOMMON -DLUA_USE_POSIX


libs+=-lm -rdynamic

ifndef NOHW
opts+=-I/usr/X11R6/include
libs+=-L/usr/X11R6/lib
endif

ifndef DEDICATED
ifndef DUMMY
SDL?=1
DEDICATED?=0
endif
endif

ifeq (${SDL},1)
EXENAME?=lsdl2srb2
endif

ifeq (${DEDICATED},1)
EXENAME?=lsrb2d
endif


ifdef LINUX
libs+=-lrt
passthru_opts+=NOTERMIOS
endif


ifdef FREEBSD
opts+=-I/usr/X11R6/include -DLINUX -DFREEBSD
libs+=-L/usr/X11R6/lib -lkvm -lexecinfo
endif







