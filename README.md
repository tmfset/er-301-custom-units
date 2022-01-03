# er-301-custom-units

A collection of _bespoke_ units for the ER-301 built using C++ and the **Middle Layer SDK**.

Go to the [hub](https://er301-hub.netlify.app/) for documentation.

## Building

We need [odevices/er-301](https://github.com/odevices/er-301) as a submodule to access it's header files during the build:
```
git submodule update --init
```

### Make Commands


| Command | Description |
|----|----|
| `make -j all` | Build everything |
| `make -j <mod>` | Build a single module |
| `make -j <mod>-install` | Build and copy to the emulator dir at `~/.od/front/ER-301/packages` |
| `make emu` | Build and start the emulator |
| `make release` | Build and package everything for release |
| `make <mod>-install-sd` | Copy a release package to the SD card |
