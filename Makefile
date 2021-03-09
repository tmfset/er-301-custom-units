# top-level makefile

PROJECTS := lojik

all: $(PROJECTS)

$(PROJECTS) &:

	+$(MAKE) -f src/$@/mod.mk PKGNAME=$@

$(addsuffix -install,$(PROJECTS)) &:
	$(eval PROJECT := $(@:-install=))
	+$(MAKE) -f src/$(PROJECT)/mod.mk install PKGNAME=$(PROJECT)

$(addsuffix -install-sd,$(PROJECTS)) &:
	$(eval PROJECT := $(@:-install-sd=))
	+$(MAKE) -f src/$(PROJECT)/mod.mk install-sd PKGNAME=$(PROJECT)

am335x-docker:
	docker build docker/am335x-build/ -t am335x-build

release:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units am335x-build:latest \
		make clean all ARCH=am335x PROFILE=debug

clean:
	rm -rf testing debug release

.PHONY: $(PROJECTS) install am335x release am335x-docker install-sd
