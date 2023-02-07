V150 = "150"
V160 = "160"
BASE150 = "0x2d91000"
BASE160 = "0x3483000"

FLAG_UPDATE = "update"
FLAG_VERBOSE = "verbose"
FLAG_CLEAN = "clean"

BUILDCACHE = ".buildcache"

from dataclasses import dataclass
from typing import Callable

def print_error(text):
    print(f"\033[1;31m{text}\033[0m")

def print_good(text):
    print(f"\033[1;32m{text}\033[0m")

@dataclass
class Context:
    log: Callable
    tool_path: str
    version: str
    flags: set
    linker_config_dir: str
    output: str
    elf: str
    nso: str
    make_args: list
    ignored_symbols: list
    tasks: list

def version_defines(version):
    major, minor, patch = (7,3,2) if version == V160 else (4,4,0)
    return f"BOTW_VERSION_DEFINES=-DBOTW_VERSION={version} -DNN_SDK_MAJOR={major} -DNN_SDK_MINOR={minor} -DNN_SDK_PATCH={patch} -DNN_WARE_MAJOR={major} -DNN_WARE_MINOR={minor} -DNN_WARE_PATCH={patch}"