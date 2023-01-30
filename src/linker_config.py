import os
import json
from src.util import Context, BASE150, BASE160, V160, BUILDCACHE
from hashlib import sha256

AUTO = "auto"
MANL = "manual"
UUSD = "unused"
LINKINFO = os.path.join(BUILDCACHE, "link_info.json")
INFO_INPUT = "input"
INFO_OUTPUT = "output"
LINKER_CONFIG_HEADER = """
# Linker Config
#
# This file contains symbols and their relative address in the botw main module
# 
# "manual" and "auto" are used to generate linker script
# "auto" are the ones found by the build tool from the symbol listing
# "unused" are not used to generate linker script
#
# Note that this is not parsed as a standard yaml file. Comments work, but many yaml features don't
"""
LINKER_SCRIPT_HEADER = """
/*
 * This linker script is generated from config files in config/linker
 * CHANGES WILL BE LOST
 */
"""
class LinkerConfig:
    base: str
    context: Context
    modified: bool

    entries: dict # mode -> addr -> symbol
    addr_comments: dict # addr -> comment
    symbol_to_addr: dict # symbol -> addr
    manual_unused: set
    

    def __init__(self, context):
        self.base = BASE160 if context.version == V160 else BASE150
        self.context = context
        self.modified = False
        self.entries = {
            AUTO: {},
            MANL: {},
            UUSD: {}
        }
        self.symbol_to_addr = None
        self.addr_comments = {}
        self.manual_unused = set()

    def input_hash(self):
        sha = sha256("LinkerConfig".encode("utf-8"))
        self.delete_unused()
        for mode in self.entries:
            for addr in self.entries[mode]:
                sha.update("entry".encode("utf-8"))
                sha.update(addr.encode("utf-8"))
                sha.update(self.entries[mode][addr].encode("utf-8"))
        return sha.hexdigest()

    def load(self):
        self.entries = {
            AUTO: {},
            MANL: {},
            UUSD: {}
        }
        self.symbol_to_addr = None
        self.manual_unused = set()
        self.addr_comments = []
        reading_mode = ""
        filename = self.config_filename()
        if not os.path.exists(filename):
            self.modified = False
            return

        self.context.log(f"Loading linker config from {filename}")
        count = 0
        with open(filename, "r", encoding="utf-8") as config_file:
            for line in config_file:
                parts = line.split("#", 1)
                line =  parts[0].strip()
                if not line:
                    continue
                key, value = [ x.strip() for x in line.split(":", 1)]
                if not value:
                    reading_mode = key
                    continue
                if not reading_mode in self.entries:
                    return f"Unknown mode \"{reading_mode}\""
                self.entries[reading_mode][key] = value
                if len(parts) > 1:
                    self.addr_comments[key] = "#"+parts[1].rstrip()
                count+=1
        self.context.log(f"Loading {count} entrie(s) from linker config")
        self.modified = False
        return None

    def save(self):
        filename = self.config_filename()
        self.context.log(f"Saving linker config to {filename}")
        count = 0
        self.delete_unused()
        os.makedirs(os.path.dirname(filename), exist_ok=True)

        with open(filename, "w+", encoding="utf-8") as config_file:
            config_file.write(LINKER_CONFIG_HEADER+"\n\n")
            for mode in self.entries:
                config_file.write("\n")
                config_file.write(f"{mode}:\n")
                for addr in sorted(self.entries[mode]):
                    comment = ""
                    if addr in self.addr_comments:
                        comment = self.addr_comments[addr]
                    count+=1
                    config_file.write(f"  {addr}: {self.entries[mode][addr]} {comment}\n")
        self.context.log(f"Written {count} entrie(s) to linker config")

    def build(self):
        if not self.modified:
            # Check hash
            linkinfo = self.load_linkinfo()
            
            if self.output_hash() == linkinfo[INFO_OUTPUT] and self.input_hash() == linkinfo[INFO_INPUT]:
                self.context.log("Linker config is up to date")
                return

        filename = self.context.output
        self.context.log(f"Writing linker script to {filename}")
        count = 0
        self.delete_unused()
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "w+", encoding="utf-8") as linker_script:
            linker_script.write(LINKER_SCRIPT_HEADER+"\n\n")
            for mode in [AUTO, MANL]:
                for addr in self.entries[mode]:
                    count+=1
                    linker_script.write(f"{self.entries[mode][addr]} = {addr} - {self.base};\n")
        self.context.log(f"Written {count} entrie(s) to linker script")
        self.save_linkinfo()
        self.modified = False

    def output_hash(self):
        if not os.path.exists(self.context.output):
            return ""
        with open(self.context.output, "r", encoding="utf-8") as output_file:
            return sha256(output_file.read().encode("utf-8")).hexdigest()

    def load_linkinfo(self):
        filename = self.linkinfo_filename()
        if not os.path.exists(filename):
            return {
                INFO_INPUT: "",
                INFO_OUTPUT: ""
            }
        with open(filename, "r", encoding="utf-8") as linkinfo_file:
            return json.load(linkinfo_file)

    def save_linkinfo(self):
        filename = self.linkinfo_filename()
        with open(filename, "w+", encoding="utf-8") as linkinfo_file:
            json.dump({
                INFO_INPUT: self.input_hash(),
                INFO_OUTPUT: self.output_hash()
            }, linkinfo_file, indent=2)

    def build_dry(self):
        self.context.log(f"Emptying linker script...")
        with open(self.context.output, "w+", encoding="utf-8") as linker_script:
            linker_script.write(LINKER_SCRIPT_HEADER+"\n\n")

    def find(self, symbol):
        self.modified = True
        if self.symbol_to_addr is None:
            self.context.log("Optimizing linker config")
            self.symbol_to_addr = {}
            
            # not adding auto here, for autos to be resolved by
            # scanning botw symbols
            for mode in [MANL, UUSD]: 
                for addr in self.entries[mode]:
                    self.symbol_to_addr[self.entries[mode][addr]] = addr
            # mark all manual entries as unused
            self.manual_unused = set(self.entries[MANL].values())
            # delete all auto entries
            self.entries[AUTO] = {}
        if symbol not in self.symbol_to_addr:
            return None
        addr = self.symbol_to_addr[symbol]
        if addr in self.manual_unused:
            self.manual_unused.remove(addr)
        return addr

    def add_auto(self, addr, symbol):
        self.entries[AUTO][addr] = symbol
        self.modified = True

    def delete_unused(self):
        if self.symbol_to_addr is not None:
            self.context.log("Rebuilding linker config")
            for manual_unused_symbol in self.manual_unused:
                addr = self.symbol_to_addr[manual_unused_symbol]
                self.entries[UUSD][addr] = manual_unused_symbol
                del self.entries[MANL][addr]
        self.symbol_to_addr = None
        self.manual_unused = set()

    def config_filename(self):
        return os.path.join(self.context.linker_config_dir, f"{self.context.version}.yaml")

    def linkinfo_filename(self):
        return os.path.join(self.context.tool_path, LINKINFO)