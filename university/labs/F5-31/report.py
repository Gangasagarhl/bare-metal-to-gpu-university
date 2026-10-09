#!/usr/bin/env python3
"""report.py - collect the "inventory: key=value" lines that netboot.efi printed on each
server's console and print one table: one row per key, one column per server.

    python3 report.py server-a=netboot_a.out server-b=netboot_b.out
"""
import sys


def inventory(path):
    items = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line.startswith("inventory: ") and "=" in line:
                key, value = line[len("inventory: "):].split("=", 1)
                items[key] = value
    return items


def main():
    servers = [arg.split("=", 1) for arg in sys.argv[1:]]
    data = {name: inventory(path) for name, path in servers}
    keys = sorted({k for items in data.values() for k in items})
    width = max(len(k) for k in keys) + 2
    cols = [max(len(name), *(len(data[name].get(k, "-")) for k in keys)) + 2 for name, _ in servers]
    print("key".ljust(width) + "".join(name.ljust(c) for (name, _), c in zip(servers, cols)))
    for k in keys:
        print(k.ljust(width) + "".join(data[name].get(k, "-").ljust(c) for (name, _), c in zip(servers, cols)))
    missing = [name for name, _ in servers if not data[name]]
    if missing:
        print("no inventory from: " + ", ".join(missing))
        sys.exit(1)


if __name__ == "__main__":
    main()
