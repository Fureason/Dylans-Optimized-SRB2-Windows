# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.




VALID_TOOLCHAINS := pnacl


include $(NACL_SDK_ROOT)/tools/common.mk


TARGET = sdl_app
DEPS = ppapi_simple nacl_io

LIBS = SDL2_test SDL2 ppapi_simple SDL2main SDL2 $(DEPS) ppapi_gles2 ppapi_cpp ppapi pthread 

CFLAGS := -Wall
SOURCES ?= testgles2.c














define LINKER_RULE
all: $(1).pexe 
$(1)_x86_32.nexe : $(1).bc
	$(call LOG,TRANSLATE,$$@,$(PNACL_TRANSLATE) --allow-llvm-bitcode-input -arch x86-32 $$^ -o $$@)

$(1)_x86_64.nexe : $(1).bc
	$(call LOG,TRANSLATE,$$@,$(PNACL_TRANSLATE) --allow-llvm-bitcode-input -arch x86-64 $$^ -o $$@)

$(1)_arm.nexe : $(1).bc
	$(call LOG,TRANSLATE,$$@,$(PNACL_TRANSLATE) --allow-llvm-bitcode-input -arch arm $$^ -o $$@)

$(1).pexe: $(1).bc
	$(call LOG,FINALIZE,$$@,$(PNACL_FINALIZE) -o $$@ $$^)

$(1).bc: $(2) $(foreach dep,$(4),$(STAMPDIR)/$(dep).stamp)
	$(call LOG,LINK,$$@,$(PNACL_LINK) -o $$@ $(2) $(PNACL_LDFLAGS) $(foreach path,$(5),-L$(path)/pnacl/$(CONFIG)) -L./lib $(foreach lib,$(3),-l$(lib)) $(6))
endef

$(foreach dep,$(DEPS),$(eval $(call DEPEND_RULE,$(dep))))
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CFLAGS))))

ifeq ($(CONFIG),Release)
$(eval $(call LINK_RULE,$(TARGET)_unstripped,$(SOURCES),$(LIBS),$(DEPS)))
$(eval $(call STRIP_RULE,$(TARGET),$(TARGET)_unstripped))
else
$(eval $(call LINK_RULE,$(TARGET),$(SOURCES),$(LIBS),$(DEPS)))
endif

$(eval $(call NMF_RULE,$(TARGET),))
