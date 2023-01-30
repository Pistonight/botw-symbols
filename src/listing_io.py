"""Download symbol listing for 1.5.0 from BotW decomp project"""

_DATA_LINK = "https://raw.githubusercontent.com/zeldaret/botw/master/data/data_symbols.csv"
_FUNC_LINK = "https://raw.githubusercontent.com/zeldaret/botw/master/data/uking_functions.csv"
# Address Prefix to strip (and check) in botw csv files
_ADDR_PREFIX = "0x00000071"

import requests
import csv
import io
import os
from src.util import V150, V160, Context, FLAG_UPDATE

class SymbolListing:
    symbol_to_addr: dict
    context: Context

    def __init__(self, context):
        self.symbol_to_addr = None
        self.context = context

    def find_symbol(self, symbol):
        if not self.symbol_to_addr:
            self.load()

        if symbol in self.symbol_to_addr:
            return self.symbol_to_addr[symbol], None
        return None, None

    def load(self):
        addr_to_symbol, error = _init_listing(
            self.context.log,
            self.context.tool_path,
            self.context.version,
            force_update=FLAG_UPDATE in self.context.flags
        )
        if error:
            return error
        self.symbol_to_addr = {}
        for addr, symbol in addr_to_symbol.items():
            self.symbol_to_addr[symbol] = addr


def _init_listing(log, tool_path, version, *, force_update=False):
    """Returns listing, error"""
    if version == V160:
        force_update = False
    file_path = listing_path(tool_path, version)
    if not os.path.isfile(file_path) or force_update:
        if version == V150:
            _fetch_listing_from_decomp_project(log, tool_path)
        else:
            return None, "Missing symbol file for 1.6.0! Reinstalling might fix the issue"
    listing = {}
    error = _load_listing(log, file_path, listing)
    return listing, error


def _fetch_listing_from_decomp_project(log, tool_path):
    """Returns error or None"""
    listing = {}
    data, error = _fetch_from(log, _DATA_LINK)
    if error:
        return error
    error = _parse_data(log, data.decode(), listing)
    if error:
        return error
    func, error = _fetch_from(log, _FUNC_LINK)
    if error:
        return error
    error = _parse_func(log, func.decode(), listing)
    if error:
        return error
    _save_listing(log, listing_path(tool_path, V150), listing)
    return None



def listing_path(tool_path, version):
    return os.path.join(tool_path, f"listing/{version}.txt")


def _fetch_from(log, url):
    """Fetch resource. Return content, error"""
    log(f"Fetching {url}")
    try:
        result = requests.get(url)
        if not result.content:
            return None, f"Failed to get {url}: failed to parse response"
        return result.content, None

    except:
        return None, f"Failed to get {url}: an error happened"


def _parse_data(log, data, listing):
    """
    Parse downloaded data symbols.
    No header rows
    Each row has 2 entries: address and symbol
    Example:
    0x0000007101e9fe68,_ZN4sead8Matrix34IfE5identE

    Returns error
    """
    log(f"Parsing data symbols...")
    count = 0
    reader = csv.reader(io.StringIO(data), delimiter=",")
    for row in reader:
        # Skip invalid rows
        if len(row) < 2:
            continue
        raw_addr = row[0].strip()
        name = row[1].strip()
        if name:
            result = _add_to_listing(name, raw_addr, listing)
            if result:
                return result
            count += 1
    log(f"Parsed {count} data symbols")
    return None


def _parse_func(log, func, listing):
    """
    Parse downloaded func symbols.
    1 header row
    Each row has 4 entires: Address,Quality,Size,Name
    Example:
    0x0000007100000024,U,000316,_init

    Returns error
    """
    log(f"Parsing func symbols...")
    count = 0
    reader = csv.reader(io.StringIO(func))
    header = False
    for row in reader:
        if not header:
            header = True
            continue
        # Skip invalid rows
        if len(row) < 4:
            continue
        raw_addr = row[0].strip()
        name = row[3].strip()
        if name:
            result = _add_to_listing(name, raw_addr, listing)
            if result:
                return result
            count += 1
    log(f"Parsed {count} func symbols")
    return None


def _add_to_listing(name, raw_addr, listing):
    """Parse address and add to listing, returns error"""
    parsed_addr = _parse_address(raw_addr)
    if not parsed_addr:
        return f"Failed to parse address: {raw_addr}"
    if parsed_addr in listing:
        return f"Duplicate address: {raw_addr}"
    listing[parsed_addr]=name
    return None


def _parse_address(raw_addr):
    """Strip the 0x00000071"""
    if not raw_addr.startswith(_ADDR_PREFIX):
        return None
    return "0x" + raw_addr[len(_ADDR_PREFIX):]


def _load_listing(log, file_name, listing):
    """
    Loading listing from file
    Each line in the file would be 0xXXXXXXXX Symbol
    Returns error
    """
    log(f"Loading symbol listings from {file_name}")
    with open(file_name, "r", encoding="utf-8") as in_file:
        for line in in_file:
            if line[0:2] != "0x" or line[10] != " ":
                return f"{file_name} is not formated correctly as a listing file"
            addr = line[0:10]
            
            if addr in listing:
                return f"Duplicating listing for address: {addr}"
            symbol = line[11:].strip()
            listing[addr] = symbol
    log(f"Loaded {len(listing)} symbol listings")
    return None


def _save_listing(log, file_name, listing):
    """
    Saving listing to file
    Each line in the file would be 0xXXXXXXXX Symbol
    Returns error
    """
    log(f"Saving {len(listing)} symbol listings to {file_name}")
    with open(file_name, "w+", encoding="utf-8") as out_file:
        for addr in sorted(listing):
            out_file.write(f"{addr} {listing[addr]}\n")
    return None

# Test driver
# if __name__ == "__main__":
#     print("output:", _fetch_listing_from_decomp_project(print, ""))