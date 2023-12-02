# botw-symbols
![v150 badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/Pistonight/botw-symbols/main/badges/150.json)
![v160 badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/Pistonight/botw-symbols/main/badges/160.json)

Linker scripts (*.ld) for linking modules against BOTW 1.5.0 and 1.6.0

**If you are looking for the botw-link build tool, see the [`botw-link`](https://github.com/Pistonight/botw-symbols/tree/botw-link) branch.
I have a new build tool [megaton](https://github.com/Pistonight/megaton)**

## Usage
The symbols in the linker scripts are meant to be used with the headers in the [decomp project](https://github.com/zeldaret/botw).

The scripts are passed to the linker (`ld`) using the `-T` flag:
```
ld -T path/to/ld/ld150.ld
```
Or in a makefile (and you are probably using CXX, `-Wl` means pass the flag to the linker):
```
LDFLAGS = -Wl,-T path/to/ld/ld150.ld
```
Replace `ld150.ld` with `ld160.ld` for version 1.6.0. See [version difference](#version-difference)

When you call a function defined in those headers, the linker will use the address defined in the linker script to link your module, allowing it to jump to the function
in the game's binary at run time.

If you are using the function symbol for hooking, declare it as `extern "C" <return type> <mangled name> (<args>);`
and look for the mangled name in the linker script

## Stubs
The linker scripts are generated based on the decomp project. For symbols that have not been decompiled yet, they are stubbed in the linker script
as `stub_<hex>` where `<hex>` is the 8-digit address of the symbol.

For version 1.6.0, most symbols are stubbed because there's no decompilation done. If you figure out the address of a function in 1.6.0, feel free to
contribute by adding it to `listing_160.csv` (using the mangled name from the decomp project)

**The stubs are included in separate files**. For example, if you need stubs from 1.6.0, also feed `ld/ld160_stubs.ld` to the linker.

## Version Difference
Here are some differences and things to note when working with v1.5.0 versus v1.6.0:
1. 1.6.0 has more stubs (see above)
2. 1.6.0 is more optimized. It is possible that a function in 1.5.0 doesn't exist in 1.6.0 (either inlined or removed).
   - In that case, you can use the decomp project to include the implementation in your cpp files

## Keeping Up to Date
The `update.py` script will update the linker scripts from `listing_160.csv` and the decomp project
```bash
pip install requests
python update.py
```
