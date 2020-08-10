# Setup payload
This is a payload that should be run with [CustomRPXLoader](https://github.com/wiiu-env/CustomRPXLoader).

## Usage
Put the `payload.rpx` in the `sd:/wiiu/` folder of your sd card and use the `CustomRPXLoader` to run this setup payload.

Put modules (in form of `.wms` files) that should be used a main()-hook into `sd:/wiiu/modules/` and one time modules into `sd:/wiiu/modules/setup`.
- Make sure not to call `exit` in the modules (by using the WiiUModuleSystem)
- The one time setups will be run in the order of their ordered filenames.

The area between `0x00800000` and whereever this setup is loaded, will be used.

## Building
Make you to have [wut](https://github.com/devkitPro/wut/) and [WiiUModuleSystem](https://github.com/wiiu-env/WiiUModuleSystem) installed and use the following command for build:
```
make install
```

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t setuppayload-builder

# make 
docker run -it --rm -v ${PWD}:/project setuppayload-builder make

# make clean
docker run -it --rm -v ${PWD}:/project setuppayload-builder make clean
```

## Credits
- maschell
- Copy paste stuff from dimok
- Copy pasted the solution for using wut header in .elf files from [RetroArch](https://github.com/libretro/RetroArch)
- Copy pasted resolving the ElfRelocations from [decaf](https://github.com/decaf-emu/decaf-emu)