# top-level makefile

PROJECTS = lojik sloop strike polygon

docker_image = tomjfiset/er-301-am335x-build-env:1.1.0

all: $(PROJECTS)

asm: $(addsuffix -asm,$(PROJECTS))

$(PROJECTS):
	+$(MAKE) -f src/mods/$@/mod.mk PKGNAME=$@

$(addsuffix -install,$(PROJECTS)): $(@:-install=)
	$(eval PROJECT := $(@:-install=))
	+$(MAKE) -f src/mods/$(PROJECT)/mod.mk install PKGNAME=$(PROJECT)

$(addsuffix -install-sd,$(PROJECTS)):
	$(eval PROJECT := $(@:-install-sd=))
	+$(MAKE) -f src/mods/$(PROJECT)/mod.mk install-sd PKGNAME=$(PROJECT) ARCH=am335x PROFILE=release

$(addsuffix -install-sd-testing,$(PROJECTS)):
	$(eval PROJECT := $(@:-install-sd-testing=))
	+$(MAKE) -f src/mods/$(PROJECT)/mod.mk install-sd PKGNAME=$(PROJECT) ARCH=am335x PROFILE=testing

$(addsuffix -missing,$(PROJECTS)):
	$(eval PROJECT := $(@:-missing=))
	+$(MAKE) -f src/mods/$(PROJECT)/mod.mk missing PKGNAME=$(PROJECT)

$(addsuffix -asm,$(PROJECTS)):
	$(eval PROJECT := $(@:-asm=))
	+$(MAKE) -f src/mods/$(PROJECT)/mod.mk asm PKGNAME=$(PROJECT)

$(addsuffix -list,$(PROJECTS)):
	$(eval PROJECT := $(@:-list=))
	+$(MAKE) -f src/mods/$(PROJECT)/mod.mk list PKGNAME=$(PROJECT)

emu:
	@cd er-301; make -j emu && testing/darwin/emu/emu.elf

am335x-docker:
	docker build docker/er-301-am335x-build-env/ -t er-301-am335x-build-env --platform=linux/amd64

release:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units --platform=linux/amd64 $(docker_image)  \
		make -j 4 all ARCH=am335x PROFILE=release

testing:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units --platform=linux/amd64 $(docker_image) \
		make -j 4 all ARCH=am335x PROFILE=testing

release-asm:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units --platform=linux/amd64 $(docker_image) \
		make -j asm ARCH=am335x PROFILE=release

er-301-docker:
	docker run --privileged -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units/er-301 --platform=linux/amd64 $(docker_image) \
		make -j 4 ARCH=am335x PROFILE=release

er-301-docker-testing:
	docker run --privileged -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units/er-301 --platform=linux/amd64 $(docker_image) \
		make -j 4 ARCH=am335x PROFILE=testing

release-missing:
	docker run -it -v `pwd`:/er-301-custom-units -w /er-301-custom-units --platform=linux/amd64 $(docker_image) \
		make -j 4 strike-missing ARCH=am335x PROFILE=release

clean:
	rm -rf testing debug release

.PHONY: all clean $(PROJECTS) $(addsuffix -install,$(PROJECTS)) $(addsuffix -install-sd,$(PROJECTS)) $(addsuffix -install-sd-testing,$(PROJECTS)) $(addsuffix -missing,$(PROJECTS)) emu am335x-docker release testing er-301-docker release-missing clean
