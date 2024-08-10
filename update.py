import sys
import csv
import json
import io
import requests

CHECK = "--check" in sys.argv

BASE150 = "0x2d91000"
BASE160 = "0x3483000"

STUB = (
    "nullsub_",
    "sub_",
    "j_",
    "_init",
    "_fini",
)

def update_160():
    print("Updating ld160.ld...")
    base_int = int(BASE160, 16)
    addrs = set()
    out = []
    stub_out = []
    with open("listing_160.csv", "r") as f:
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

            if is_sys(symbol):
                print(f"ignoring {symbol}")
                continue
            if symbol.startswith(STUB):
                stub_symbol = make_stub(addr_int)
                stub_out.append((stub_symbol, f"{addr} - {BASE160}", symbol))
            else:
                out.append((symbol, f"{addr} - {BASE160}", ""))
            addrs.add(addr_int)

    make_script("ld/ld160.ld", out)
    make_script("ld/ld160_stubs.ld", stub_out)
    make_badge("badges/160.json", "v1.6.0", len(out), len(out) + len(stub_out))

def update_150():
    print("Updating ld150.ld...")
    PREFIX = "0x00000071"
    DATA_URL = "https://raw.githubusercontent.com/zeldaret/botw/master/data/data_symbols.csv"
    FUNC_URL = "https://raw.githubusercontent.com/zeldaret/botw/master/data/uking_functions.csv"

    addrs = set()
    out = []
    stub_out = []
    base_int = int(BASE150, 16)

    data_csv = requests.get(DATA_URL).content.decode("utf-8")
    if not data_csv:
        raise Exception("Failed to fetch data symbols")
    reader = csv.reader(io.StringIO(data_csv), delimiter=",")
    for row in reader:
        if len(row) < 2:
            print(f"Ignoring invalid row: {row}")
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

    func_csv = requests.get(FUNC_URL).content.decode("utf-8")
    if not func_csv:
        raise Exception("Failed to fetch data symbols")
    reader = csv.reader(io.StringIO(func_csv), delimiter=",")
    header = False
    for row in reader:
        if not header:
            header = True
            continue
        if len(row) < 4:
            print(f"Ignoring invalid row: {row}")
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
        if is_sys(symbol):
            print(f"ignoring {symbol}")
            continue
        if not symbol or symbol.startswith(STUB) or ":" in symbol:
            stub_symbol = make_stub(addr_int)
            stub_out.append((stub_symbol, f"{addr} - {BASE150}", symbol))
        else:
            out.append((symbol, f"{addr} - {BASE150}", ""))


    make_script("ld/ld150.ld", out)
    make_script("ld/ld150_stubs.ld", stub_out)
    make_badge("badges/150.json", "v1.5.0", len(out), len(out) + len(stub_out))


def is_sys(symbol):
    return symbol != "" and ":" not in symbol and "_" not in symbol and symbol.lower() == symbol

def make_stub(addr):
    return f"stub_{hex(addr)[2:].lower().zfill(8)}"

def make_script(file, script):
    out = "\n".join([ fmt_line(x) for x in script ])

    if CHECK:
        with open(file, "r") as f:
            actual = "\n".join(x for x in f)
        if out != actual:
            print("Output is not up to date!")
            print("Please run `python update.py`")
            sys.exit(1)

    with open(file, "w") as f:
        f.write(out)
        f.flush()

    print(f"Updated {file}")

def fmt_line(x):
    comment = f" /*{x[2]}*/" if x[2] else ""
    return f"PROVIDE_HIDDEN({x[0]} = {x[1]});{comment}"

def make_badge(file, label, count, total):
    percentage = f"{count / total * 100:.2f}"
    badge = {
        "schemaVersion": 1,
        "label":label,
        "message": f"{count}/{total} ({percentage}%)",
        "color": "blue",
    }
    with open(file, "w") as f:
        json.dump(badge, f)

if __name__ == "__main__":
    update_160()
    update_150()
