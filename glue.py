import cxxfilt
import os
from dataclasses import dataclass
import sys
from src.listing_io import init_listing
from src.util import print_error, V150, V160, BASE150, BASE160

LENGTH_LIMIT = 100

# Check if called in botw directory
TOOL_PATH = os.path.dirname(os.path.abspath(sys.argv[0]))
if not TOOL_PATH.endswith("botw-link") or not os.path.isdir(TOOL_PATH):
    print_error("fatal: Not invoking botw-link directory")
    print("Please call the program as `python path/to/botw-link`")
    exit(2)

ADDROF = "mainoff"
BOTW_VERSION = "BOTW_VERSION"
HEADER = f"""
/// Glue header for BotW
/// See README.md for more info
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#ifndef {ADDROF}
#define {ADDROF}(x) _BOTW_LINK_GLUE_##x
#endif
"""

FOOTER = """
#pragma GCC diagnostic pop
"""

def _extract_namespace_func(mangled_symbol, included_top_namespaces):
    """Returns (hierarchy, demangled_symbol), hierarchy will be None if fails"""
    if not mangled_symbol.startswith("_"):
        return None, mangled_symbol
    try:
        demangled_symbol = cxxfilt.demangle(mangled_symbol)
    except:
        return None, mangled_symbol

    if demangled_symbol.endswith("const"):
        demangled_symbol = demangled_symbol[:-5].rstrip()

    if "(" not in demangled_symbol:
        return None, demangled_symbol # global function or not a function
    func_name_part: str = _clean_paren(demangled_symbol)

    # special cases
    if func_name_part.startswith("non-virtual thunk to ") or func_name_part.startswith("non virtual thunk to "):
        func_name_part = func_name_part[len("non-virtual thunk to "):]
    #print(func_name_part)
    if "::" not in func_name_part:
        return None, demangled_symbol # global function probably

    heirarchy = _split_hierarchy(func_name_part, demangled_symbol)
    if heirarchy[0] not in included_top_namespaces:
        return None, demangled_symbol # not in included namespaces
    classname = "::".join(heirarchy[:-1])
    if classname.lower() == classname:
        return None, demangled_symbol # probably not a class
    return heirarchy, demangled_symbol

def _clean_paren(name):
    buf = ""
    nested = 0
    for c in name:
        if c == "(":
            nested+=1
        elif c == ")":
            nested-=1
        else:
            if nested == 0:
                buf += c
    if nested != 0:
        print_error(f"fatal: nesting unclosed when parsing paren {name}, to {buf}")
        exit(1)
    return buf

def _split_hierarchy(name, demangled_symbol):
    heirarchy_parts = []
    nested = 0
    for part in name.split("::"):
        if " " in part and not heirarchy_parts:
            part = part.rsplit(" ", 1)[-1]
        if nested == 0:
            if part.startswith("operator"):
                heirarchy_parts.append("operator")
                part = part[8:].lstrip()
                for operator in sorted(OPERATORS, key=lambda k: len(k), reverse=True):
                    if part.startswith(operator):
                        part = OPERATORS[operator]+part[len(operator):]
                        break
                heirarchy_parts.append(part)
            else:
                if "<" in part:
                    i = part.index("<")
                    heirarchy_parts.append(part[:i])
                    heirarchy_parts.append(part[i:])
                else:
                    heirarchy_parts.append(part)
        else:
            heirarchy_parts[-1] = heirarchy_parts[-1] + part

        for c in part:
            if c == "<":
                nested += 1
            elif c == ">":
                if nested == 0:
                    print_error(f"fatal: nesting underflow when parsing {demangled_symbol}")
                    exit(1)
                nested -= 1

    if nested != 0:
        print_error(f"fatal: nesting unclosed when parsing {demangled_symbol}, to {heirarchy_parts}")
        exit(1)

    if heirarchy_parts[-1].startswith("<"):
        heirarchy_parts.pop()
    
    return heirarchy_parts

OPERATORS = {
    "+": "_plus_",
    "-": "_minus_",
    "*": "_mul_",
    "/": "_div_",
    "%": "_mod_",
    "++": "_inc_",
    "--": "_dec_",
    "==": "_eq_",
    "!=": "_ne_",
    "<": "_lt_",
    ">": "_gt_",
    "<=": "_le_",
    ">=": "_ge_",
    "<=>": "_cmp_",
    "!": "_not_",
    "&&": "_and_",
    "||": "_or_",
    "~": "_tilde_",
    "&": "_amp_",
    "|": "_pipe_",
    "^": "_caret_",
    "<<": "_lshift_",
    ">>": "_rshift_",
    "=": "_assign_",
    "+=": "_plus_eq_",
    "-=": "_minus_eq_",
    "*=": "_mul_eq_",
    "/=": "_div_eq_",
    "%=": "_mod_eq_",
    "&=": "_amp_eq_",
    "|=": "_pipe_eq_",
    "^=": "_caret_eq_",
    "<<=": "_lshift_eq_",
    ">>=": "_rshift_eq_",
    "[]": "_index_",
    "*": "_deref_",
    "&": "_ref_",
    "->": "_arrow_",
    "->*": "_arrow_star_",
    "()": "_call_",
    ",": "_comma_",
    "new": "_new_",
    "new[]": "_new_array_",
    "delete": "_delete_",
    "delete[]": "_delete_array_",
    "\"\"": "_string_",
}

def _clean_func_name(name):
    return name.replace(" ", "_").replace("~", "_tilde_").replace("\"", "_quote_").replace("[", "_bra").replace("]", "ket_")


B150 = int(BASE150, 16)
B160 = int(BASE160, 16)
@dataclass
class Symbol:
    name: str
    comment: str
    addr: int

    def to_string(self, overload, _version):
        name = _clean_func_name(self.name)
        if "~" in name:
            print_error(f"fatal: too many tilde in symbol name {self.name}")
            exit(1)
        return f"static ptrdiff_t {name}{overload}=0x{int(self.addr, 16):x};\n//               ^ [{self.addr}] {self.comment}\n"

def _clean_namespace_name(name):
    if not name:
        return name
    return name.replace("<", "_tmpl_").replace(">", "_").replace(" ", "").replace("-", "_").replace(",", "_").replace("*", "_ptr").replace("&", "_ref").replace("(", "_").replace(")", "_")

class Namespace:
    name: str
    namespaces: dict
    symbols_150: dict
    symbols_160: dict

    def __init__(self, name):
        self.name = (name)
        self.namespaces = {}
        self.symbols_150 = {}
        self.symbols_160 = {}

    def add_symbol(self, hierarchy: list, version, comment, addr):
        if len(hierarchy) == 1:
            func_name = hierarchy[0]
            if version == V150:
                self.add_symbol_to(self.symbols_150, func_name, comment, addr)
            else:
                self.add_symbol_to(self.symbols_160, func_name, comment, addr)
            return
        name = _clean_namespace_name(hierarchy[0])
        if name not in self.namespaces:
            self.namespaces[name] = Namespace(name)
        self.namespaces[name].add_symbol(hierarchy[1:], version, comment, addr)

    def add_symbol_to(self, symbols, func_name, comment, addr):
        if func_name not in symbols:
            symbols[func_name] = []
        for symbol in symbols[func_name]:
            if symbol.addr == addr:
                return
        symbols[func_name].append(Symbol(func_name, comment, addr))

    # return how many headers written, how many symbols written
    def export_file(self, parent_namespace_dir, parent_namespace, parent_include_path):
        if self.name:
            filename = os.path.join(parent_namespace_dir, f"{self.name}.h")
            namespaced_name = f"{parent_namespace}::{self.name}" if parent_namespace else self.name
            sub_dir = os.path.join(parent_namespace_dir, self.name)
            sub_inc_path = f"{parent_include_path}/{self.name}"
        else:
            filename = os.path.join(parent_namespace_dir, "all.hpp")
            namespaced_name = ""
            sub_dir = parent_namespace_dir
            sub_inc_path = parent_include_path

        if LENGTH_LIMIT > 0:
            if len(filename) > LENGTH_LIMIT or len(namespaced_name) > LENGTH_LIMIT:
                display = namespaced_name[:LENGTH_LIMIT-3]+"..."
                print(f"Ignoring name too long: {display}")
                return 0, 0

        buffer = []
        header_count = 0
        symbol_count = 0
        if self.namespaces:
            for sub_ns in self.namespaces:
                sub_headers_count, sub_symbol_count = self.namespaces[sub_ns].export_file(sub_dir, namespaced_name, sub_inc_path)
                if sub_headers_count:
                    header_count+=sub_headers_count
                    symbol_count+=sub_symbol_count
                    buffer.append(f"#include <{sub_inc_path}/{sub_ns}.h>\n")

        if (not header_count) and (not self.symbols_150) and (not self.symbols_160):
            return 0, 0

        if not namespaced_name:
            # write all.hpp by scanning files
            os.makedirs(parent_namespace_dir, exist_ok=True)
            with open(filename, "w+", encoding="utf-8") as output_file:
                output_file.write(HEADER)

                for sub_path in os.listdir(parent_namespace_dir):
                    if sub_path.endswith(".h"):
                        output_file.write(f"#include <{parent_include_path}/{sub_path}>\n")

            return header_count+1, symbol_count

        try:
            os.makedirs(parent_namespace_dir, exist_ok=True)
            with open(filename, "w+", encoding="utf-8") as output_file:
                output_file.write(HEADER)

                for line in buffer:
                    output_file.write(line)


                symbol_count+=self.write(output_file, namespaced_name)

                output_file.write(FOOTER)
            return header_count+1, symbol_count
        except Exception as e:
            print_error(f"could not write to {filename}: {e}")
            print("Try reducing the length limit")
            exit(1)

    # return symbols count
    def write(self, output_file, namespaced_name):
        count = 0
        if not self.symbols_150 and not self.symbols_160:
            return count
        output_file.write(f"namespace {ADDROF}({namespaced_name}) {{\n")
        if self.symbols_150:
            output_file.write("#if BOTW_VERSION == 150\n")
            count+=self.write_symbols(output_file, self.symbols_150, V150)
            output_file.write("#endif\n")
        if self.symbols_160:
            output_file.write("#if BOTW_VERSION == 160\n")
            count+=self.write_symbols(output_file, self.symbols_160, V160)
            output_file.write("#endif\n")
        output_file.write("}\n")
        return count

    # return symbol count
    def write_symbols(self, output_file, symbols, version):
        count = 0
        for name in sorted(symbols):
            if len(symbols[name]) == 1:
                output_file.write(symbols[name][0].to_string("", version))
                count+=1
            else:
                count+=len(symbols[name])
                for i, symbol in enumerate(symbols[name]):
                    output_file.write(symbol.to_string(f"_{i+1}", version))
        return count

def run_generate(included_top_namespaces):
    global_namespace = Namespace(None)
    count = 0
    print("Parsing symbols...")
    for version in [V150, V160]:
        listing, error = init_listing(print, TOOL_PATH, version, force_update=True)
        if error:
            print_error(error)
            exit(1)
        
        
        for addr in listing:
            symbol = listing[addr]
            hierarchy, comment = _extract_namespace_func(symbol, included_top_namespaces)
            if hierarchy is None:
                continue
            global_namespace.add_symbol(hierarchy, version, comment, addr)
            count+=1
    

    count, symbol_count = global_namespace.export_file(os.path.join(TOOL_PATH, "include/glue"), None, "glue")
    print(f"Found {count} symbols")
    print(f"Written {count} headers")
    print(f"Written {symbol_count} symbols")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <namespace> [<namespace> ...]")
        exit(1)
    run_generate(sys.argv[1:])
    #print(_split_hierarchy("a::b::c<d<e::f<g>>>::bad", ""))