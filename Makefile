# top-level makefile

PROJECTS = lojik sloop

all: $(PROJECTS)

$(PROJECTS):
	+$(MAKE) -f src/$@/mod.mk PKGNAME=$@

$(addsuffix -install,$(PROJECTS)): $(@:-install=)
	$(eval PROJECT := $(@:-install=))
	+$(MAKE) -f src/$(PROJECT)/mod.mk install PKGNAME=$(PROJECT)

$(addsuffix -install-sd,$(PROJECTS)):
	$(eval PROJECT := $(@:-install-sd=))
	+$(MAKE) -f src/$(PROJECT)/mod.mk install-sd PKGNAME=$(PROJECT) ARCH=am335x PROFILE=release

$(addsuffix -missing,$(PROJECTS)):
	$(eval PROJECT := $(@:-missing=))
	+$(MAKE) -f src/$(PROJECT)/mod.mk missing PKGNAME=$(PROJECT)

am335x-docker:
	docker build docker/er-301-am335x-build-env/ -t er-301-am335x-build-env

release:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units er-301-am335x-build-env:latest \
		make -j all ARCH=am335x PROFILE=release

er-301-docker:
	docker run --privileged -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units/er-301 er-301-am335x-build-env:latest \
		make -j ARCH=am335x PROFILE=release

release-missing:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units er-301-am335x-build-env:latest \
		make -j sloop-missing ARCH=am335x PROFILE=release

clean:
	rm -rf testing debug release

.PHONY: all clean $(PROJECTS) $(addsuffix -install,$(PROJECTS)) $(addsuffix -install-sd,$(PROJECTS)) $(addsuffix -missing,$(PROJECTS)) am335x-docker release er-301-docker release-missing clean
