# qmp_regs.py <socket> - ask a running QEMU, through its QMP socket, for the program
# counter of every CPU (the monitor command "info registers -a").
import json, socket, sys, time
s = socket.socket(socket.AF_UNIX)
s.connect(sys.argv[1])
f = s.makefile("rw")
def cmd(obj):
    f.write(json.dumps(obj) + "\n"); f.flush()
    while True:
        r = json.loads(f.readline())
        if "return" in r or "error" in r:
            return r
json.loads(f.readline())                      # greeting
cmd({"execute": "qmp_capabilities"})
r = cmd({"execute": "human-monitor-command", "arguments": {"command-line": "info registers -a"}})
for line in r["return"].splitlines():
    t = line.split()
    if line.startswith("CPU#") or (len(t) >= 2 and t[0] == "pc"):
        print(line.strip())
