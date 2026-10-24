



Ifnot=$(if $(1),$(3),$(2))
Ifndef=$(call Ifnot,$($(1)),$(2),$(3))


Wildvar=$(foreach v,$(filter $(1),$(.VARIABLES)),$($(v)))



_cat=$(shell $(cat) $(call Windows_path,$(1)))
List=$(addprefix $(dir $(1)),$(call _cat,$(1)))


Windows_path=$(if $(WINDOWSHELL),$(subst /,\,$(1)),$(1))

define Propogate_flags =
opts+=$$($(1)_CFLAGS)
libs+=$$($(1)_LDFLAGS)
endef









define Configure =
$(1)_CFLAGS?=$$(shell $(2) $(or $(3),--cflags) $(5))
$(1)_LDFLAGS?=$$(shell $(2) $(or $(4),--libs) $(5))
$(call Propogate_flags,$(1))
endef







define Use_pkg_config =
$(call Configure,$(1),$(PKG_CONFIG),,,$($(1)_PKGCONFIG))
endef














define Check_pkg_config =
_p:=$(subst /, ,$(1))
_v1:=$$(word 1,$$(_p))
_v2:=$$(or $$(word 3,$$(_p)),$$(_v1))
ifndef NO$$(_v1)
$$(_v2)_PKGCONFIG?=$$(word 2,$$(_p))
$$(eval $$(call Use_pkg_config,$$(_v2)))
opts+=-DHAVE_$$(_v1)
endif
endef


Prefix=$(if $(PREFIX),$(PREFIX)-)$(1)

Echo=
Echo_name=
Print=

ifndef SILENT
Echo=@echo $(1)
ifndef ECHO
ifndef NOECHOFILENAMES
Echo_name=$(call Echo,-- $(1) ...)
endif
endif
ifndef MAKE_RESTARTS
ifndef destructive
Print=$(info $(1))
endif
endif
endif

.=$(call Ifndef,ECHO,@)
