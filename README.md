# botw-link
A tool for linking modules against botw

## How does it work?
This tool is a wrapper for `make`. It generates a linker script that binds symbols to addresses, calls `make` to build your module, then checks the binary for dynamically linked symbols that doesn't exist (and are supposed to be statically linked)

`make` and `objdump` are required for the program to work, both of which should be already installed if you are running your project in a linux or WSL environment.

### Build Graph
```
       │
       │
    ┌──▼───────────────┐
    │ Initialize       │
    └──┬───────────────┘
       │
FAIL┌──▼───────────────┐      ┌──────────────────┐
  ┌─┤ Linker Script    ◄──────┤ Prepare Relink   ◄─┬──────┐
  │ └──┬───────────────┘      └──────────────────┘ │      │
  │    │                                           │      │OK
FAIL┌──▼───────────────┐      ┌──────────────────┐ │   ┌──┴───────────────┐
  ├─┤ Run Make         ◄──────┤ Prepare Rebuild  │ │   │ Botw Symbol Scan │
  │ └──┬───────────────┘      └──▲───────────────┘ │   └──▲───┬───────────┘
  │    │OK                       │1st              │OK    │   │FAIL
  │ ┌──▼───────────────┐         │ 2+┌─────────────┴────┐ │   │
  │ │ Check Binary     ├─────────┴───► Configure Linker ├─┘   │
  │ └──┬───────────────┘-c / FAIL    └──────────────────┘FAIL │
  │    │OK                                                    │
  │ ┌──▼───────────────┐                                      │
  └─► Cleanup          ◄──────────────────────────────────────┘
    └──┬───────────────┘
       │
       ▼

Initialize:       Read config files, run tasks
Linker Script:    Generate linker script from config
Run Make:         call make and objdump
Check Binary:     Check if the elf contains unlinked symbols
Prepare Rebuild:  Remove existing linker script and binary
Configure Linker: Change linker config based on missing symbols
Botw Symbol Scan: Scan botw symbol listing for missing symbols
Prepare Relink:   Removing binary to relink
Cleanup:          Save config files, cleanup tasks
```


## Installation
1. Include this repo as a submodule to your project. Replace LOCAL_PATH with where you want to clone this tool
   ```
   git submodule add git@github.com:iTNTPiston/botw-link LOCAL_PATH
   ```
1. Install the dependencies
   ```
   cd path/to/botw-link
   pip install -r requirements.txt
   ```
1. Optionally, add the BotW decomp project as a submodule as well so you have access to the headers. Replace LOCAL_PATH where you want to clone the botw code
   ```
   git submodule add git@github.com:zeldaret/botw LOCAL_PATH
   ```
1. Later, to update to the latest version of the tool:
   ```
   cd path/to/botw-link
   git checkout main
   git pull
   cd path/to/your/repo
   git add path/to/botw-link
   git commit -m "Update botw-link"
   git push
   ```
   In collab scenarios, to update your copy of the tool to whatever the repo uses:
   ```
   git submodule update
   ```
Follow the steps below to see how to make your code interact with BotW and how to change your build system to use this tool.

## Code Changes
You can call functions or use data in BotW with the headers from the decomp project or declare your own and link them later.

### Use Decomp Headers (Recommended)
This is the recommended way to make your module interact with BotW.

1. Add the decomp project to your include path
   ```
   path/to/botw/src
   ```
   **Note: do not add the source `*.cpp` files.** Just the headers would be enough. However, in rare situations you might want to use the decompiled source code if you can't find the function address.
1. Locate the header that contains the function/class you want to use and `#include` it in your source. The function does not need to be decompiled to be used. For example:
   ```c++
   #include <KingSystem/System/SystemTimers.h>
   ```
   Make sure the include path is set correctly in your IDE and your makefile if you see include errors. 
1. Call the function or data just like any regular c++ function
   ```c++
   const auto pSystemTimers = ksys::SystemTimers::getInstance();
   ```
   In this case we are using the data symbol `_ZN4ksys12SystemTimers9sInstanceE`. Since this is listed in the decomp project, the build tool will automatically find it and link it for you

### Declare the Symbol Yourself
Sometimes the symbol isn't in the decomp project. You can either:

1. Declare it in a header by yourself and link the symbol manually.
1. Contribute to the decomp project by adding the header and the symbol listing

### Target the Code against BotW Versions
With this tool, you can target the same code against both BotW 1.5.0 and 1.6.0 versions at build time. However, whatever symbols you are using must be present in the listing of the corresponding version.

Sometimes it's needed to target code only for a specific version. To do that, you can use preprocessor macros:
```c++
#if BOTW_VERSION == 150
    word_only_on_150();
#endif
```
The tool will define `BOTW_VERSION` as either `150` or `160` based on the args and pass it to `make`.

## Build System Changes
You need to apply some changes to the makefile and other files in the build system to use this tool

1. Decide a place to put the generated linker script in your project. For example
   ```
   config/linker/syms.ld
   ```
1. In your linker specs (the `.specs` file passed in from the ld flag `-specs=`), add this file to the command. For example
   ```
   // OLD:
   %(old_link) -T ../libs/exlaunch/misc/link.ld --shared --export-dynamic 

   // NEW:
   %(old_link) -T ../libs/exlaunch/misc/link.ld ../config/linker/syms.ld --shared --export-dynamic 
   ```
   The path should be relative to your build directory
1. In your `makefile`, include `BOTW_VERSION` as part of the compiler flags. For example:
   ```makefile
   CLFAGS=$(CLFAGS) -DBOTW_VERSION=$(BOTW_VERSION)
   ```
1. When calling the tool, specify the linker script output location
   ```
   --output config/linker/syms.ld
   ```


## Usage
The tool uses command line arguments to accept inputs so it doesn't depend on external tooling for argument parsing. You can make a script or use a script runner like [Just](https://github.com/casey/just) to avoid typing them out every time.

You should invoke the tool by calling it from python with the directory path to make sure it works properly, for example:
```
python path/to/botw-link path/to/config.toml -V 150
```

You can see an example of the build config in `example.toml`

### Required Build Flags

#### Version
```
--version/-V VERSION
```
Version of BotW to link against. VERSION must be either "150" or "160". This will affect what symbol data is used and output/input file names. The same string will be passed to the makefile through the BOTW_VERSION variable.


### Optional Build Flags

#### Clean
```
--clean/-c
```
This will clean the symbols defined in the linker config `.yaml` files and make sure they are up-to-date. Also cleans internal caches used by this tool. The default behavior is only clean when unlinked symbols are found
#### Update
```
--update/-u
```
This will fetch the latest CSV symbol listing for BotW 1.5.0 from the decomp project. The default behavior is only fetch if missing
#### Verbose
```
--verbose/-v
```
Show more output

