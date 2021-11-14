include scripts/env.mk

LIBNAME ?= lib$(PKGNAME)

OUT_DIR      = $(PROFILE)/$(ARCH)
LIB_FILE     = $(OUT_DIR)/$(LIBNAME).so
PACKAGE_DIR  = $(OUT_DIR)/$(PKGNAME)-$(PKGVERSION)
PACKAGE_FILE = $(PACKAGE_DIR).pkg

# Common code to all mods
COMMON_DIR          = src/common
COMMON_ASSETS_DIR   = $(COMMON_DIR)/ASSETS
COMMON_ASSETS      := $(call rwildcard, $(COMMON_ASSETS_DIR), *)
COMMON_C_SOURCES   := $(call rwildcard, $(COMMON_DIR), *.c)
COMMON_CPP_SOURCES := $(call rwildcard, $(COMMON_DIR), *.cpp)
COMMON_HEADERS     := $(call rwildcard, $(COMMON_DIR), *.h)

# Mod specific code
MOD_DIR           = src/mods/$(PKGNAME)
MOD_ASSETS_DIR    = $(MOD_DIR)/ASSETS
MOD_ASSETS       := $(call rwildcard, $(MOD_ASSETS_DIR), *)
MOD_C_SOURCES    := $(call rwildcard, $(MOD_DIR), *.c)
MOD_CPP_SOURCES  := $(call rwildcard, $(MOD_DIR), *.cpp)
MOD_SWIG_SOURCES := $(call rwildcard, $(MOD_DIR), *.cpp.swig)
MOD_HEADERS      := $(call rwildcard, $(MOD_DIR), *.h)

SWIG_SOURCES := $(MOD_SWIG_SOURCES)
C_SOURCES    := $(MOD_C_SOURCES) $(COMMON_C_SOURCES)
CPP_SOURCES  := $(MOD_CPP_SOURCES) $(COMMON_CPP_SOURCES)
HEADERS      := $(MOD_HEADERS) $(COMMON_HEADERS)
ASSETS       := $(MOD_ASSETS) $(COMMON_ASSETS)

SWIG_WRAPPER = $(addprefix $(OUT_DIR)/,$(SWIG_SOURCES:%.cpp.swig=%_swig.cpp))
SWIG_OBJECT  = $(SWIG_WRAPPER:%.cpp=%.o)

ASSEMBLY  = $(addprefix $(OUT_DIR)/,$(C_SOURCES:%.c=%.s))
ASSEMBLY += $(addprefix $(OUT_DIR)/,$(CPP_SOURCES:%.cpp=%.s))
ASSEMBLY += $(SWIG_WRAPPER:%.cpp=%.s)

OBJECTS  = $(addprefix $(OUT_DIR)/,$(C_SOURCES:%.c=%.o))
OBJECTS += $(addprefix $(OUT_DIR)/,$(CPP_SOURCES:%.cpp=%.o))
OBJECTS += $(SWIG_OBJECT)

ifeq ($(ARCH),am335x)
  CFLAGS.am335x = -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -mabi=aapcs -Dfar= -D__DYNAMIC_REENT__
  LFLAGS = -nostdlib -nodefaultlibs -r
endif

ifeq ($(ARCH),linux)
  CFLAGS.linux = -Wno-deprecated-declarations -msse4 -fPIC
  LFLAGS = -shared
endif

ifeq ($(ARCH),darwin)
  INSTALLROOT.darwin = ~/.od/front
  CFLAGS.darwin = -Wno-deprecated-declarations -msse4 -fPIC
  LFLAGS = -dynamic -undefined dynamic_lookup -lSystem
endif

CFLAGS.common = -Wall -ffunction-sections -fdata-sections
CFLAGS.speed  = -O3 -ftree-vectorize -ffast-math
CFLAGS.size   = -Os

CFLAGS.release = $(CFLAGS.speed) -Wno-unused
CFLAGS.testing = $(CFLAGS.speed) -DBUILDOPT_TESTING
CFLAGS.debug   = -g -DBUILDOPT_TESTING

# Do you need any additional preprocess SYMBOLS?
SYMBOLS =

INCLUDES  = $(MOD_DIR) $(COMMON_DIR)
INCLUDES += $(SDKPATH) $(SDKPATH)/arch/$(ARCH) $(SDKPATH)/emu

CFLAGS += $(CFLAGS.common) $(CFLAGS.$(ARCH)) $(CFLAGS.$(PROFILE))
CFLAGS += $(addprefix -I,$(INCLUDES))
CFLAGS += $(addprefix -D,$(SYMBOLS))

# swig-specific flags
SWIGFLAGS    = -lua -no-old-metatable-bindings -nomoduleglobal -small -fvirtual
SWIGFLAGS   += $(addprefix -I,$(INCLUDES)) 
CFLAGS.swig  = $(CFLAGS.common) $(CFLAGS.$(ARCH)) $(CFLAGS.size)
CFLAGS.swig += $(addprefix -I,$(INCLUDES)) -I$(SDKPATH)/libs/lua54
CFLAGS.swig += $(addprefix -D,$(SYMBOLS))

#######################################################
# Rules

all: $(PACKAGE_FILE)

asm: $(ASSEMBLY)

$(SWIG_WRAPPER): $(HEADERS) Makefile

$(OBJECTS): $(HEADERS) Makefile

$(ASSEMBLY): $(HEADERS) Makefile

$(LIB_FILE): $(OBJECTS)
	@echo [LINK $@]
	@$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LFLAGS)

$(PACKAGE_DIR): $(LIB_FILE) $(ASSETS)
	@echo [STAGE $@]
	@rm -fr $@
	@mkdir -p $@
	@cp $(LIB_FILE) $@/
	@rsync -ru $(MOD_ASSETS_DIR)/ $@/
	@rsync -ru $(COMMON_ASSETS_DIR)/ $@/
	@find $@ -type f -name "*.lua" -print0 | xargs -0 sed -i.bak 's/common\.assets/$(PKGNAME)/g'
	@find $@ -type f -name "*.bak" -print0 | xargs -0 rm

.PHONY: $(PACKAGE_DIR)

$(PACKAGE_FILE): $(PACKAGE_DIR)
	@echo [ZIP $@]
	@rm -f $@
	@pwd
	@cd $<; $(ZIP) -r ../$(@F) ./

list: $(PACKAGE_FILE)
	@unzip -l $(PACKAGE_FILE)

clean:
	rm -rf $(OUT_DIR)

dist-clean:
	rm -rf testing release debug

install: $(PACKAGE_FILE)
	cp $(PACKAGE_FILE) $(INSTALLROOT.$(ARCH))/ER-301/packages/

install-sd:
	cp $(PACKAGE_FILE) /Volumes/NO\ Name/ER-301/packages/

# C/C++ compilation rules

#  -fverbose-asm -fno-toplevel-reorder
$(OUT_DIR)/%.s: %.cpp
	@echo [S $<]
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS) -w -std=gnu++11 -S $< -o $@

$(OUT_DIR)/%.o: %.c
	@echo [C $<]
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

$(OUT_DIR)/%.o: %.cpp
	@echo [C++ $<]
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS) -std=gnu++11 -c $< -o $@

# SWIG wrapper rules

.PRECIOUS: $(OUT_DIR)/%_swig.c $(OUT_DIR)/%_swig.cpp

$(OUT_DIR)/%_swig.cpp: %.cpp.swig
	@echo [SWIG $<]
	@mkdir -p $(@D)
	@$(SWIG) -fvirtual -fcompact -c++ $(SWIGFLAGS) -o $@ $<

$(OUT_DIR)/%_swig.o: $(OUT_DIR)/%_swig.cpp
	@echo [C++ $<]
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS.swig) -std=gnu++11 -c $< -o $@

ifeq ($(ARCH),am335x)
# Optional check for any missing SYMBOLS
#
# make missing ARCH=am335x
#
# Warning: This assumes that the firmware has been compiled in the SDK source tree already. 
#

ALL_IMPORTS_FILE     = $(OUT_DIR)/imports.txt
MISSING_IMPORTS_FILE = $(OUT_DIR)/missing.txt
APP_EXPORTS_FILE     = $(SDKPATH)/$(PROFILE)/$(ARCH)/app/exports.sym

$(ALL_IMPORTS_FILE): $(LIB_FILE)
	@echo $(describe_env) NM $<
	@$(NM) --undefined-only --format=posix $< | awk '{print $$1;}' > $@

$(MISSING_IMPORTS_FILE): $(ALL_IMPORTS_FILE) $(APP_EXPORTS_FILE)
	@sort -u $(APP_EXPORTS_FILE) | comm -23 $< - > $@

# Check for any undefined SYMBOLS that are not exported by app.
missing: $(MISSING_IMPORTS_FILE)
	@[ ! -s $< ] || echo "Missing Symbols:"
	@cat $<
	@[ -s $< ] || echo "No missing SYMBOLS."	

.PHONY: missing

endif