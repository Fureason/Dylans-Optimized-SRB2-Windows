
.SUFFIXES:


ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules








TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	../../
DATA		:=	data
INCLUDES	:=	include




ARCH	:=	-mthumb -mthumb-interwork

CFLAGS	:=	-Wall -O2\
		-march=armv5te -mtune=arm946e-s \
		-fomit-frame-pointer -ffast-math \
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	$(ARCH) -march=armv5te -mtune=arm946e-s
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)





LIBDIRS	:=	$(LIBNDS)





ifneq ($(BUILD),$(notdir $(CURDIR)))


export OUTPUT	:=	$(CURDIR)/lib/libz.a

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))




ifeq ($(strip $(CPPFILES)),)

	export LD	:=	$(CC)

else

	export LD	:=	$(CXX)

endif


export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

.PHONY: $(BUILD) clean all


all: $(BUILD)
	@[ -d $@ ] || mkdir -p include
	@cp ../../*.h include

lib:
	@[ -d $@ ] || mkdir -p $@
	
$(BUILD): lib
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile


clean:
	@echo clean ...
	@rm -fr $(BUILD) lib


else

DEPENDS	:=	$(OFILES:.o=.d)




$(OUTPUT)	:	$(OFILES)


%.bin.o	:	%.bin

	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)


endif

