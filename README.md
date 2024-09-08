# botw-symbols
![v150 badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/Pistonight/botw-symbols/main/badges/150.json)
![v160 badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/Pistonight/botw-symbols/main/badges/160.json)

This repo contains:
- Linker scripts (*.ld) for linking modules against BOTW 1.5.0 and 1.6.0
- A toolkit containing helper functions for modding BOTW

## Install
Clone the repo with git. Add `--depth 1` since the diffs might be large for generated files.
```
git clone https://github.com/Pistonight/botw-symbols --depth 1
```
Or add as a submodule to your repo
```bash
# with git
git submodule add --name botw-symbols --branch main https://github.com/Pistonight/botw-symbols <path>
# with magoo
magoo install https://github.com/Pistonight/botw-symbols <path> --name botw-symbols --branch main --depth 1
```

## Linker Scripts
The linker scripts are located in `/ld`. They contain addresses for symbols
from the [decomp project](https://github.com/zeldaret/botw) so you can call them in your code.

The scripts are passed to the linker via the `-T` flag, or `-Wl,-T` when invoking the linker through the compiler.

The scripts are separated into 2 files: 
- normal functions: (`ld150.ld` and `ld160.ld`)
- stub functions: (`ld150_stubs.ld` and `ld160_stubs.ld`)

Generally you only need the normal functions. The stub functions are address for undecompiled functions.

For version 1.6.0, most symbols are stubbed because there's no decompilation done. If you figure out the address of a function in 1.6.0, feel free to
contribute by adding it to `listing_160.csv` (using the mangled name from the decomp project)

## Toolkit Library
The toolkit library has some common code that I use for my mods. Feel free to use them.
You also need to add `toolkit150.ld` or `toolkit160.ld` to the linker.
These scripts serve as temporary solutions for symbols that are not in the decomp project yet.

Make sure to `-DBOTW_VERSION=150` or `-DBOTW_VERSION=160` for the correct version when compiling.

## Development - Adding Symbols
To add new symbols:
- For 1.6.0, add it to `listing_160.csv`
- For 1.5.0, the symbols are sourced from [my fork of the decomp project](https://github.com/Pistonight/botw-decomp) and are updated every so often with upstream
- Add to `toolkit_150.csv` or `toolkit_160.csv` for symbols neede in the toolkit

The `update.py` script will update the linker scripts from the sources
```bash
pip install requests
python update.py
```
