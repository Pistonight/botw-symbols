TOOL_VERSION = "0.2.0"
import sys
import os
from src.util import print_error
# Check if called in botw directory
TOOL_PATH = sys.argv[0]
if not os.path.abspath(TOOL_PATH).endswith("botw-link") or not os.path.isdir(TOOL_PATH):
    print_error("fatal: Not invoking botw-link directory")
    print("Please call the program as `python path/to/botw-link`")
    exit(2)


# Check if directory contains space. make will not work with spaces
if " " in os.getcwd():
    print_error("fatal: Paths cannot contain spaces")
    print()
    print(f"Your path is \"{os.getcwd()}\"")
    print()
    print("GNU make does not support spaces in paths.")
    print("Please make sure your project does not contain spaces in its absolute path")
    print("For more info: http://savannah.gnu.org/bugs/?712")
    exit(3)

import argparse
parser = argparse.ArgumentParser(
    description=f"Tool for building modules to statically link with BotW ({TOOL_VERSION})",
)

parser.add_argument(
    'config',
    type=str,
    help='TOML configuration file',
    metavar='CONFIG',
)
parser.add_argument(
    '--version', '-V',
    type=str,
    required=True,
    help='BotW version. Either 150 or 160',
    metavar='VERSION',
)
parser.add_argument(
    '--clean', '-c',
    action='store_true',
    help='Clean configs and caches',
)
parser.add_argument(
    '--update', '-u',
    action='store_true',
    help='Update cached symbol listing from decomp project',
)
parser.add_argument(
    '--verbose', '-v',
    action='store_true',
    help='Show more output',
)

args = parser.parse_args(sys.argv[1:])
if args.version not in ["150", "160"]:
    print_error("fatal: Version is not valid. Use either \"-V 150\" for 1.5.0 or \"-V 160\" for 1.6.0")
    exit(4)

from src.util import Context, FLAG_UPDATE, FLAG_VERBOSE, FLAG_CLEAN

CONFIG_PATH = args.config
if not os.path.isfile(CONFIG_PATH):
    print_error(f"fatal: Config file \"{CONFIG_PATH}\" does not exist")
    exit(5)

import toml
try:
    config = toml.load(CONFIG_PATH)
except:
    print_error(f"fatal: Config file \"{CONFIG_PATH}\" is not a valid TOML file")
    exit(5)

for key in ["linker_config_dir", "output", "elf", "nso", "make_args", "ignored_symbols"]:
    if key not in config:
        print_error(f"fatal: Config file \"{CONFIG_PATH}\" is missing \"{key}\"")
        exit(5)

context = Context(
    print if args.verbose else lambda *_args, **_kwargs: None,
    TOOL_PATH,
    args.version,
    set([flag for flag in [FLAG_UPDATE, FLAG_VERBOSE, FLAG_CLEAN] if getattr(args, flag)]),
    config["linker_config_dir"],
    config["output"],
    config["elf"],
    config["nso"],
    config["make_args"],
    config["ignored_symbols"],
    []
)

from src.task_rename import RenameTask
from src.task_suppress_warning import SuppressWarningTask
TASKS = []

if "task" in config:
    if "rename" in config["task"]:
        for task in config["task"]["rename"]:
            if "old" not in task or "new" not in task:
                print_error(f"fatal: Config file \"{CONFIG_PATH}\" has an invalid task in \"tasks.rename\"")
                exit(5)
            TASKS.append(RenameTask(task["old"], task["new"]))
    if "suppress_warning" in config["task"]:
        for task in config["task"]["suppress_warning"]:
            if "file" not in task or "search" not in task or "suppress" not in task:
                print_error(f"fatal: Config file \"{CONFIG_PATH}\" has an invalid task in \"tasks.suppress_warning\"")
                exit(5)
            TASKS.append(SuppressWarningTask(context, task["file"], task["suppress"], task["search"]))

context.tasks = TASKS

from src.build import build
exit(build(context))