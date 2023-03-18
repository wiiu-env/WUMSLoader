[![CI-Release](https://github.com/wiiu-env/WUMSLoader/actions/workflows/ci.yml/badge.svg)](https://github.com/wiiu-env/WUMSLoader/actions/workflows/ci.yml)

# Wii U Module System Loader
This is a payload that should be run with [EnvironmentLoader](https://github.com/wiiu-env/EnvironmentLoader).

## Usage
Put the `10_wums_loader.rpx` in the `fs:/vol/external01/wiiu/environments/[ENVIRONMENT]/modules/setup` folder of your sd card and use the `EnvironmentLoader` to run this setup payload.

Put modules (in form of `.wms` files) that should be used a main()-hook into `fs:/vol/external01/wiiu/environments/[ENVIRONMENT]/modules/`.

The area between `0x00809000` and `0x00FFF000` will be used.

## Buildflags

### Logging
Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).  
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).  
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

If the [LoggingModule](https://github.com/wiiu-env/LoggingModule) is not present, it'll fallback to UDP (Port 4405) and [CafeOS](https://github.com/wiiu-env/USBSerialLoggingModule) logging.

## Building
Make you to have [wut](https://github.com/devkitPro/wut/) and [WiiUModuleSystem](https://github.com/wiiu-env/WiiUModuleSystem) installed and use the following command for build:
```
make install
```

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t wumsloader-builder

# make 
docker run -it --rm -v ${PWD}:/project wumsloader-builder make

# make clean
docker run -it --rm -v ${PWD}:/project wumsloader-builder make clean
```

## Format the code via docker

`docker run --rm -v ${PWD}:/src ghcr.io/wiiu-env/clang-format:13.0.0-2 -r ./source ./wumsloader/src -i   --exclude ./wumsloader/src/elfio -i`

## Credits
- Maschell
- Copy paste stuff from dimok
- Copy pasted the solution for using wut header in .elf files from [RetroArch](https://github.com/libretro/RetroArch)
- Copy pasted resolving the ElfRelocations from [decaf](https://github.com/decaf-emu/decaf-emu)
