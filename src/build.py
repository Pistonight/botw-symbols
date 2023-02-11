import subprocess
import os
import sys

from shutil import rmtree

from src.linker_config import LinkerConfig
from src.symbol_differ import SymbolDiffer, elf_path_to_syms
from src.listing_io import SymbolListing, listing_path
from src.util import Context, FLAG_CLEAN, BUILDCACHE, FLAG_VERBOSE, print_error, print_good, V150, version_defines

class Build:
    
    stack: list
    current_step: int
    linker_config: LinkerConfig
    symbol_differ: SymbolDiffer
    error: str
    error_source: str
    make_iteration: int
    difference_cache: set
    botw_symbols: SymbolListing
    dried: bool
    context: Context
    def __init__(self, context):
        self.context = context
        self.stack = []
        self.current_step = 0
        self.error = ""
        self.linker_config = LinkerConfig(context)
        self.symbol_differ = SymbolDiffer(context)
        self.make_iteration = 0
        self.difference_cache = 0
        self.botw_symbols = SymbolListing(context)
        self.dried = False

    def build(self):
        self.stack = [
            lambda: self.cleanup()
        ]
        build_cache_path = os.path.join(self.context.tool_path, BUILDCACHE)
        if FLAG_CLEAN in self.context.flags:
            if os.path.exists(build_cache_path):
                rmtree(build_cache_path)
            self.stack.append(
                lambda: self.prepare_rebuild(),
            )
        else:
            self.stack.extend([
                lambda: self.check_binary(),
                lambda: self.run_make(),
                lambda: self.linker_script()
            ])

        self.stack.append(lambda: self.initialize())

        self.current_step = 0
        os.makedirs(build_cache_path, exist_ok=True)
        
        while self.stack:
            task = self.stack.pop()
            self.current_step += 1
            try:
                self.error = task()
                if self.error:
                    self.stack.append(lambda: self.cleanup())
            except Exception as e:
                print(e)
                self.error = e
                self.stack.append(lambda: self.cleanup())


        return 1 if self.error else 0


    def start_step(self, step, *, set_error_source=True):
        if set_error_source:
            self.error_source = step
        print(f"\033[1;33m({self.current_step}/{self.current_step+len(self.stack)}) {step}\033[0m")

    def initialize(self):
        self.start_step("Initialize")
        for task in self.context.tasks:
            task.execute()
        
        return self.linker_config.load()
    
    def linker_script(self):
        self.start_step("Linker Script")
        self.linker_config.build()

    def run_make(self):
        self.make_iteration += 1
        self.start_step(f"Run Make (Iteration {self.make_iteration})")
        make_args = ["make"]
        if FLAG_VERBOSE in self.context.flags:
            make_args.append("V=1")
        make_args.extend(self.context.make_args)
        make_args.append(version_defines(self.context.version))
        result = subprocess.run(make_args)
        if result.returncode:
            return "make failed"
        self.context.log("Dumping symbols from elf...")
        with open(elf_path_to_syms(self.context.elf), "w+", encoding="utf-8") as syms:
            result = subprocess.run([
                "aarch64-none-elf-objdump",
                "-T",
                self.context.elf,
            ], stdout=syms)
        if result.returncode:
            return "objdump failed"
        return None
        
    def check_binary(self):
        self.start_step("Check Binary")
        self.difference_cache = self.symbol_differ.get_difference()
        if self.difference_cache:
            print_error(f"{len(self.difference_cache)} Unlinked symbol(s) found!")
            if not self.dried:
                self.stack.append(lambda: self.prepare_rebuild())
            else:
                self.stack.append(lambda: self.configure_linker())
            return
        print_good("All symbols appeared to be linked")
        

    def prepare_rebuild(self):
        self.start_step("Prepare Rebuild")
        self.linker_config.build_dry()
        self.dried = True
        self.stack.append(lambda: self.check_binary())
        self.stack.append(lambda: self.run_make())
        self.stack.append(lambda: self.prepare_relink())

    def configure_linker(self):
        self.start_step("Configure Linker")
        new_difference = set()
        for symbol in self.difference_cache:
            addr = self.linker_config.find(symbol)
            if not addr:
                new_difference.add(symbol)
            else:
                self.context.log(f"Resolved {symbol} = {addr}")
        self.context.log(f"Remaining symbols: {len(new_difference)}")
        self.difference_cache = new_difference
        if new_difference:
            self.stack.append(lambda: self.botw_symbol_scan())
        else:
            self.stack.append(lambda: self.check_binary())
            self.stack.append(lambda: self.run_make())
            self.stack.append(lambda: self.linker_script())
            self.stack.append(lambda: self.prepare_relink())

    def botw_symbol_scan(self):
        self.start_step("Botw Symbol Scan")

        new_difference = set()
        for symbol in self.difference_cache:
            addr, error = self.botw_symbols.find_symbol(symbol)
            if error:
                return error
            if not addr:
                new_difference.add(symbol)
            else:
                self.context.log(f"Resolved {symbol} = {addr}")
                self.linker_config.add_auto(addr, symbol)
        self.context.log(f"Remaining symbols: {len(new_difference)}")
        self.difference_cache = new_difference 
        if new_difference:
            return f"{len(new_difference)} symbol(s) cannot be automatically matched.\n"
        else:
            self.stack.append(lambda: self.check_binary())
            self.stack.append(lambda: self.run_make())
            self.stack.append(lambda: self.linker_script())
            self.stack.append(lambda: self.prepare_relink())

    def prepare_relink(self):
        self.start_step("Prepare Relink")
        files = [
            self.context.elf,
            self.context.nso,
            elf_path_to_syms(self.context.elf),
        ]
        for file in files:
            if os.path.exists(file):
                self.context.log(f"Removing {file}")
                os.remove(file)


    def cleanup(self):
        self.stack = []
        self.start_step("Cleanup", set_error_source=False)
        try:
            for task in self.context.tasks:
                task.cleanup()
            self.linker_config.save()
        except Exception as e:
            print(e)
            self.error = e
            self.error_source = "Cleanup"
        
        print()
        if self.error:
            print_error("BUILD FAILED")
            print()
            print(f"Step that failed: {self.error_source}")
            print(self.error)

            if self.difference_cache:
                print()
                print("Looks like some symbol(s) cannot be linked:")
                for symbol in self.difference_cache:
                    print(f"  {symbol}")
                print()
                print("Try one of the following suggestions:")
                suggestions = []
                if self.context.version == V150:
                    suggestions.append("List the mangled names of the symbols in botw decomp project, and run with --update")
                else:
                    suggestions.append(f"List the mangled names of the symbols in {listing_path(self.context.tool_path, self.context.version)}")
                suggestions.append(f"Add entries to the \"manual\" section of {self.linker_config.config_filename()}")
                
                for i, suggestion in enumerate(suggestions):
                    print(f"  {i+1}. {suggestion}")
        else:
            print_good("BUILD SUCCESS")

def build(context):
    return Build(context).build()
