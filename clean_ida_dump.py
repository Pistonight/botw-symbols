import sys

BAD_PREFICES = [
    "sub_",
    "nullsub_",
    "j_"
]
with open(sys.argv[1], "r", encoding="utf-8") as f:
    for line in f:
        parts = line.split("\t")
        if parts[0] == "Function name":
            continue
        addr = f"0x{parts[2][8:]}"
        name = parts[0].replace(".", "_")
        bad = False
        for prefix in BAD_PREFICES:
            if name.startswith(prefix):
                bad = True
                name = name+"_________UKING_160_CHANGE_ME"
                break
        print(f"{addr} {name}")