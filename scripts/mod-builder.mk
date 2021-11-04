include scripts/utils.mk

LIBNAME ?= lib$(PKGNAME)
SDKPATH ?= er-301
mod_dir = src/mods/$(PKGNAME)
common_dir = src/common/

c_sources    := $(call rwildcard, $(mod_dir), *.c)
cpp_sources  := $(call rwildcard, $(mod_dir), *.cpp)
swig_sources := $(call rwildcard, $(mod_dir), *.cpp.swig)

assets      := $(call rwildcard, $(mod_dir)/assets, *)
sources     := $(c_sources) $(cpp_sources) $(swig_sources)
common      := $(call rwildcard, $(common_dir), *.h)
headers     := $(call rwildcard, $(mod_dir), *.h) $(common)

# Do you need any additional preprocess symbols?
symbols = 

# Determine ARCH if it's not provided...
# linux | darwin | am335x
ifndef ARCH
  SYSTEM_NAME := $(shell uname -s)
  ifeq ($(SYSTEM_NAME),Linux)
    ARCH = linux
  else ifeq ($(SYSTEM_NAME),Darwin)
    ARCH = darwin
  else
    $(error Unsupported system $(SYSTEM_NAME))
  endif
endif

# Determine PROFILE if it's not provided...
# testing | release | debug
PROFILE ?= testing

out_dir = $(PROFILE)/$(ARCH)
lib_file = $(out_dir)/$(LIBNAME).so
package_file = $(out_dir)/$(PKGNAME)-$(PKGVERSION).pkg

swig_interface = $(filter %.cpp.swig,$(sources))
swig_wrapper = $(addprefix $(out_dir)/,$(swig_interface:%.cpp.swig=%_swig.cpp))
swig_object = $(swig_wrapper:%.cpp=%.o)

c_sources = $(filter %.c,$(sources))
cpp_sources = $(filter %.cpp,$(sources))

assembly = $(addprefix $(out_dir)/,$(c_sources:%.c=%.s))
assembly += $(addprefix $(out_dir)/,$(cpp_sources:%.cpp=%.s))
assembly += $(swig_wrapper:%.cpp=%.s)

objects = $(addprefix $(out_dir)/,$(c_sources:%.c=%.o))
objects += $(addprefix $(out_dir)/,$(cpp_sources:%.cpp=%.o))
objects += $(swig_object)

includes  = $(mod_dir) $(common_dir)
includes += $(SDKPATH) $(SDKPATH)/arch/$(ARCH) $(SDKPATH)/emu

ifeq ($(ARCH),am335x)
  CFLAGS.am335x = -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -mabi=aapcs -Dfar= -D__DYNAMIC_REENT__
  LFLAGS = -nostdlib -nodefaultlibs -r

  include $(SDKPATH)/scripts/am335x.mk
endif

ifeq ($(ARCH),linux)
  CFLAGS.linux = -Wno-deprecated-declarations -msse4 -fPIC
  LFLAGS = -shared

  include $(SDKPATH)/scripts/linux.mk
endif

ifeq ($(ARCH),darwin)
  INSTALLROOT.darwin = ~/.od/front
  CFLAGS.darwin = -Wno-deprecated-declarations -msse4 -fPIC
  LFLAGS = -dynamic -undefined dynamic_lookup -lSystem

  include $(SDKPATH)/scripts/darwin.mk
endif

CFLAGS.common = -Wall -ffunction-sections -fdata-sections
CFLAGS.speed = -O3 -ftree-vectorize -ffast-math
CFLAGS.size = -Os

CFLAGS.release = $(CFLAGS.speed) -Wno-unused
CFLAGS.testing = $(CFLAGS.speed) -DBUILDOPT_TESTING
CFLAGS.debug = -g -DBUILDOPT_TESTING

CFLAGS += $(CFLAGS.common) $(CFLAGS.$(ARCH)) $(CFLAGS.$(PROFILE))
CFLAGS += $(addprefix -I,$(includes)) 
CFLAGS += $(addprefix -D,$(symbols))

# swig-specific flags
SWIGFLAGS = -lua -no-old-metatable-bindings -nomoduleglobal -small -fvirtual
SWIGFLAGS += $(addprefix -I,$(includes)) 
CFLAGS.swig = $(CFLAGS.common) $(CFLAGS.$(ARCH)) $(CFLAGS.size)
CFLAGS.swig += $(addprefix -I,$(includes)) -I$(SDKPATH)/libs/lua54
CFLAGS.swig += $(addprefix -D,$(symbols))

#######################################################
# Rules

all: $(package_file)

asm: $(assembly)

$(swig_wrapper): $(headers) Makefile

$(objects): $(headers) Makefile

$(assembly): $(headers) Makefile

$(lib_file): $(objects)
	@echo [LINK $@]
	@$(CC) $(CFLAGS) -o $@ $(objects) $(LFLAGS)

$(package_file): $(lib_file) $(assets)
	@echo [ZIP $@]
	@rm -f $@
	@$(ZIP) -j $@ $(lib_file) $(assets)

list: $(package_file)
	@unzip -l $(package_file)

clean:
	rm -rf $(out_dir)

dist-clean:
	rm -rf testing release debug

install: $(package_file)
	cp $(package_file) $(INSTALLROOT.$(ARCH))/ER-301/packages/

install-sd:
	cp $(package_file) /Volumes/NO\ Name/ER-301/packages/

# C/C++ compilation rules

#  -fverbose-asm -fno-toplevel-reorder
$(out_dir)/%.s: %.cpp
	@echo [S $<]
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS) -w -std=gnu++11 -S $< -o $@

$(out_dir)/%.o: %.c
	@echo [C $<]
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

$(out_dir)/%.o: %.cpp
	@echo [C++ $<]
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS) -std=gnu++11 -c $< -o $@

# SWIG wrapper rules

.PRECIOUS: $(out_dir)/%_swig.c $(out_dir)/%_swig.cpp

$(out_dir)/%_swig.cpp: %.cpp.swig
	@echo [SWIG $<]
	@mkdir -p $(@D)
	@$(SWIG) -fvirtual -fcompact -c++ $(SWIGFLAGS) -o $@ $<

$(out_dir)/%_swig.o: $(out_dir)/%_swig.cpp
	@echo [C++ $<]
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS.swig) -std=gnu++11 -c $< -o $@

ifeq ($(ARCH),am335x)
# Optional check for any missing symbols
#
# make missing ARCH=am335x
#
# Warning: This assumes that the firmware has been compiled in the SDK source tree already. 
#

all_imports_file = $(out_dir)/imports.txt
missing_imports_file = $(out_dir)/missing.txt
app_exports_file = $(SDKPATH)/$(PROFILE)/$(ARCH)/app/exports.sym

$(all_imports_file): $(lib_file)
	@echo $(describe_env) NM $<
	@$(NM) --undefined-only --format=posix $< | awk '{print $$1;}' > $@

$(missing_imports_file): $(all_imports_file) $(app_exports_file)
	@sort -u $(app_exports_file) | comm -23 $< - > $@

# Check for any undefined symbols that are not exported by app.
missing: $(missing_imports_file)
	@[ ! -s $< ] || echo "Missing Symbols:"
	@cat $<
	@[ -s $< ] || echo "No missing symbols."	

.PHONY: missing

endif