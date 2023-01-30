import os
from src.util import Context

class SymbolDiffer:
    dll_symbols: set
    context: Context
    def __init__(self, context):
        self.dll_symbols = None
        self.context = context

    def get_difference(self):
        if not self.dll_symbols:
            self.context.log(f"Start building symbol cache")
            self.dll_symbols = set()
            botw_symbols_dir = os.path.join(
                self.context.tool_path,
                f"symbols/{self.context.version}"
            )
            _read_symbols_from(self.context.log, f"{botw_symbols_dir}/main.syms", self.dll_symbols)
            _read_symbols_from(self.context.log, f"{botw_symbols_dir}/rtld.syms", self.dll_symbols)
            _read_symbols_from(self.context.log, f"{botw_symbols_dir}/sdk.syms", self.dll_symbols)
            _read_symbols_from(self.context.log, f"{botw_symbols_dir}/subsdk0.syms", self.dll_symbols)
            self.context.log(f"Ignoring {len(self.context.ignored_symbols)} known symbols")
            for ignore in self.context.ignored_symbols:
                self.dll_symbols.add(ignore)
            self.context.log(f"Loaded {len(self.dll_symbols)} botw symbols")

        input_symbols = set()
        _read_symbols_from(self.context.log, elf_path_to_syms(self.context.elf), input_symbols)
        return input_symbols - self.dll_symbols

def _read_symbols_from(log, symbol_file, output):
    log(f"Loading symbols from {symbol_file}")
    count = 0
    with open(symbol_file, "r", encoding="utf-8") as file:
        for i,line  in enumerate(file):
            if i<4:
                continue # skip the header stuff
            if len(line.strip())==0:
                continue
            # Example
            # 0000000000000000      DF *UND*	0000000000000000 nnsocketGetPeerName
            symbol = line[25:].split(" ")[1].strip() 
            output.add(symbol)
            count += 1
    log(f"Loaded {count} symbol(s)")

def elf_path_to_syms(elf_path):
    return os.path.splitext(elf_path)[0]+".syms"