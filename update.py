import sys
import csv
import json
import os
import subprocess

CHECK = "--check" in sys.argv
# Base module addresses
BASE150 = "0x2d91000"
BASE160 = "0x3483000"
# Decomp project to use for 1.5.0
REPO = "https://github.com/Pistonight/botw-decomp"
BRANCH = "dev"
# Local path to repo
LOCAL = "botw"

STUB = (
    "nullsub_",
    "sub_",
    "j_",
    "_init",
    "_fini",
)

def parse_csv(file_path, base_int):
    """Parse our own csv format and return the addresses and symbols"""
    addrs = set()
    out = []
    stub_out = []
    with open(file_path, "r") as f:
        reader = csv.reader(f, delimiter=" ")
        for row in reader:
            if len(row) != 2:
                raise Exception(f"Invalid row: {row}")
            symbol = row[1]
            addr = row[0]
            addr_int = int(addr, 16)
            if addr_int >= base_int:
                raise Exception(f"Address out of range: {addr}")
            if addr_int in addrs:
                raise Exception(f"Duplicate address: {addr}")

            if should_ignore_symbol(symbol):
                print(f"-- ignoring {symbol}")
                continue
            if symbol.startswith(STUB):
                stub_symbol = make_stub_symbol(addr_int)
                stub_out.append((stub_symbol, f"{addr} - {BASE160}", symbol))
            else:
                out.append((symbol, f"{addr} - {BASE160}", ""))
            addrs.add(addr_int)
    return out, stub_out

def find_and_parse_decomp_project_csv():
    try:
        update_decomp_project()
        return parse_decomp_project_csv()
    except:
        print("failed to parse symbols from decomp project, will re-checkout and retry")
        checkout_decomp_project()
        return parse_decomp_project_csv()

def parse_decomp_project_csv():
    """Parse the decomp project CSV format"""
    PREFIX = "0x00000071"

    addrs = set()
    out = []
    stub_out = []
    base_int = int(BASE150, 16)
    with open("botw/data/data_symbols.csv", "r") as f:
        reader = csv.reader(f, delimiter=",")
        for row in reader:
            if len(row) < 2:
                print(f"-- ignoring invalid row: {row}")
                continue
            addr = row[0]
            if not addr.startswith(PREFIX):
                raise Exception(f"Invalid address: {addr}")
            addr = "0x" + addr[len(PREFIX):]
            addr_int = int(addr, 16)
            if addr_int >= base_int:
                raise Exception(f"Address out of range: {addr}")
            if addr_int in addrs:
                raise Exception(f"Duplicate address: {addr}")
            addrs.add(addr_int)

            symbol = row[1]
            # assume all data symbols are non stub
            out.append((symbol, f"{addr} - {BASE150}", ""))

    with open("botw/data/uking_functions.csv", "r") as f:
        reader = csv.reader(f, delimiter=",")
        header = False
        for row in reader:
            if not header:
                header = True
                continue
            if len(row) < 4:
                print(f"-- ignoring invalid row: {row}")
                continue
            addr = row[0]
            if not addr.startswith(PREFIX):
                raise Exception(f"Invalid address: {addr}")
            addr = "0x" + addr[len(PREFIX):]
            addr_int = int(addr, 16)
            if addr_int >= base_int:
                raise Exception(f"Address out of range: {addr}")
            if addr_int in addrs:
                raise Exception(f"Duplicate address: {addr}")
            addrs.add(addr_int)

            symbol = row[3]
            if should_ignore_symbol(symbol):
                print(f"-- ignoring {symbol}")
                continue
            if not symbol or symbol.startswith(STUB) or ":" in symbol:
                stub_symbol = make_stub_symbol(addr_int)
                stub_out.append((stub_symbol, f"{addr} - {BASE150}", symbol))
            else:
                out.append((symbol, f"{addr} - {BASE150}", ""))

    return out, stub_out

def update_decomp_project():
    """git pull the decomp project"""
    if not os.path.exists(LOCAL):
        checkout_decomp_project()
        return
    subprocess.run(["git", "switch", BRANCH], cwd=LOCAL, check=True)
    subprocess.run(["git", "pull", "origin", BRANCH], cwd=LOCAL, check=True)

def checkout_decomp_project():
    """Checkout decomp project"""
    import shutil
    CHECKOUT = [
        "data/data_symbols.csv",
        "data/uking_functions.csv",
    ]
    if os.path.exists(LOCAL):
        print("removing existing botw/ directory")
        shutil.rmtree(LOCAL)
    print("cloning decomp project")
    os.makedirs(LOCAL)
    subprocess.run(["git", "init"], cwd=LOCAL, check=True)
    subprocess.run(["git", "remote", "add", "origin", REPO], cwd=LOCAL, check=True)
    subprocess.run(["git", "config", "core.sparseCheckout", "true"], cwd=LOCAL, check=True)
    with open(os.path.join(LOCAL, ".git", "info", "sparse-checkout"), "w", encoding="utf-8") as f:
        for checkout_path in CHECKOUT:
            f.write(checkout_path + "\n")
    subprocess.run(["git", "pull", "--depth=1", "origin", BRANCH], cwd=LOCAL, check=True)

def should_ignore_symbol(symbol):
    """Some heuristics to ignore symbols that we don't want to include in the linker script"""
    return symbol != "" and ":" not in symbol and "_" not in symbol and symbol.lower() == symbol

def make_stub_symbol(addr):
    """Create a stub symbol name from int address"""
    return f"stub_{hex(addr)[2:].lower().zfill(8)}"

def make_script(file, script):
    """Create linker script file"""
    def fmt_line(x):
        """"""
        comment = f" /*{x[2]}*/" if x[2] else ""
        return f"PROVIDE_HIDDEN({x[0]} = {x[1]});{comment}"
    out = "\n".join([ fmt_line(x) for x in script ])

    with open(file, "r", encoding="utf-8") as f:
        actual = "\n".join(x.rstrip() for x in f)
    if out == actual:
        print(f"up-to-date: {file}")
        return
    if CHECK:
        print("Output is not up to date!")
        print("Please run `python update.py`")
        sys.exit(1)

    with open(file, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
        f.flush()

    print(f"updated {file}")

def make_badge(file, label, count, total):
    """Write a badge json"""
    percentage_num = count / total * 100
    percentage = f"{percentage_num:.2f}"
    badge = {
        "schemaVersion": 1,
        "label":label,
        "message": f"{count}/{total} ({percentage}%)",
        "color": "blue",
    }
    with open(file, "w") as f:
        json.dump(badge, f)

if __name__ == "__main__":
    print("=== 1.5.0")
    out, stub_out = find_and_parse_decomp_project_csv()
    make_script("ld/ld150.ld", out)
    make_script("ld/ld150_stubs.ld", stub_out)
    make_badge("badges/150.json", "v1.5.0", len(out), len(out) + len(stub_out))

    print("=== 1.5.0-toolkit")
    out, stub_out = parse_csv("toolkit_150.csv", int(BASE150, 16))
    make_script("ld/toolkit150.ld", out)

    print("=== 1.6.0")
    base_int = int(BASE160, 16)
    out, stub_out = parse_csv("listing_160.csv", base_int)
    make_script("ld/ld160.ld", out)
    make_script("ld/ld160_stubs.ld", stub_out)
    make_badge("badges/160.json", "v1.6.0", len(out), len(out) + len(stub_out))

    print("=== 1.6.0-toolkit")
    out, stub_out = parse_csv("toolkit_160.csv", base_int)
    make_script("ld/toolkit160.ld", out)
