include scripts/utils.mk

SDKPATH ?= er-301

# Determine PROFILE if it's not provided...
# testing | release | debug
PROFILE ?= testing

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

ifeq ($(ARCH),am335x)
  include $(SDKPATH)/scripts/am335x.mk
endif

ifeq ($(ARCH),linux)
  include $(SDKPATH)/scripts/linux.mk
endif

ifeq ($(ARCH),darwin)
  include $(SDKPATH)/scripts/darwin.mk
endif
