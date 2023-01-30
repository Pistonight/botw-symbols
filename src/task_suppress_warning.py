import os
import json
from hashlib import sha256
from src.util import Context, BUILDCACHE
from shutil import copy2

CACHE_FILE = os.path.join(BUILDCACHE, "suppress_warning.json")

class SuppressWarningTask:
    context: Context
    def __init__(self, context, file_name, warnings, search_string):
        self.context = context
        self.file_name = file_name
        self.file_name_old = f"{file_name}.old"
        self.warnings = warnings
        self.search_string = search_string
        self.done = False
    def execute(self):
        if not os.path.exists(self.file_name):
            return

        self.context.log(f"Suppressing warnings in {self.file_name}")
        cache = _load_cache(self.context.tool_path)

        with open(self.file_name, "r", encoding="utf-8") as file:
            input_hash = sha256(file.read().encode("utf-8"))
            
        input_hash.update("filename".encode("utf-8"))
        input_hash.update(self.file_name.encode("utf-8"))
        input_hash.update("search".encode("utf-8"))
        input_hash.update(self.search_string.encode("utf-8"))
        input_hash.update("warnings".encode("utf-8"))
        input_hash.update(" ".join(self.warnings).encode("utf-8"))
        input_hash = input_hash.hexdigest()
        os.rename(self.file_name, self.file_name_old)

        cache_file = os.path.join(self.context.tool_path, BUILDCACHE, input_hash)
        
        if self.file_name in cache and cache[self.file_name] == input_hash:
            
            # cache hit, copy from cache
            copy2(cache_file, self.file_name)
            self.context.log(f"{self.file_name} cache hit")
            return
            
        with open(self.file_name, "w+", encoding="utf-8") as file:
            with open(self.file_name_old, "r", encoding="utf-8") as file_old:
                for line in file_old:
                    if not self.done:
                        if self.search_string in line:
                            file.write("#pragma GCC diagnostic push\n")
                            for warning in self.warnings:
                                file.write(f"#pragma GCC diagnostic ignored \"{warning}\"\n")
                            
                            self.done = True
                    file.write(line)
            file.write("#pragma GCC diagnostic pop\n")
        _update_cache(self.file_name, input_hash)
        copy2(self.file_name, cache_file)

    
    def cleanup(self):
        _save_cache(self.context.tool_path)
        self.context.log(f"Restoring {self.file_name}")
        if os.path.exists(self.file_name_old) and os.path.exists(self.file_name):
            copy2(self.file_name_old, self.file_name)
            os.remove(self.file_name_old)


_CACHE = None
def _load_cache(tool_path):
    global _CACHE
    if _CACHE is None:
        path = os.path.join(tool_path, CACHE_FILE)
        _CACHE = {}
        if os.path.exists(path):
            with open(path, "r", encoding="utf-8") as cache_file:
                _CACHE = json.load(cache_file)
    return _CACHE

def _update_cache(file_name, hash):
    _CACHE[file_name] = hash

def _save_cache(tool_path):
    global _CACHE
    if _CACHE is not None:
        with open(os.path.join(tool_path, CACHE_FILE), "w+", encoding="utf-8") as cache_file:
            json.dump(_CACHE, cache_file, indent=2)
        _CACHE = None