# Setup payload
This is a payload that should be run with [CustomRPXLoader](https://github.com/wiiu-env/CustomRPXLoader).

## Usage
Put the `payload.rpx` in the `sd:/wiiu/` folder of your sd card and use the `CustomRPXLoader` to run this setup payload.

Put modules (in form of `.rpx` files) that should be used a main()-hook into `sd:/wiiu/modules/` and one time modules into `sd:/wiiu/modules/setup`.
- Make sure not to call `exit` in the modules (for example by using a custom crt instead of the wut one)
- The one time setups will be run in the order of their ordered filenames.

The area between `0x00800000` and whereever this setup is loaded, will be used.

## Building
Make you to have [wut](https://github.com/devkitPro/wut/) installed and use the following command for build:
```
make install
```

## Credits
- maschell
- Copy paste stuff from dimok
- Copy pasted the solution for using wut header in .elf files from [RetroArch](https://github.com/libretro/RetroArch)
- Copy pasted resolving the ElfRelocations from [decaf](https://github.com/decaf-emu/decaf-emu)