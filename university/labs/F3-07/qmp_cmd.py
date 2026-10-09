# qmp_cmd.py <socket> <monitor command> - run one QEMU monitor command through QMP
# (QEMU's machine-readable control socket) and print its text output.
import json, socket, sys
s = socket.socket(socket.AF_UNIX)
s.connect(sys.argv[1])
f = s.makefile("rw")
def cmd(obj):
    f.write(json.dumps(obj) + "\n"); f.flush()
    while True:
        r = json.loads(f.readline())
        if "return" in r or "error" in r:
            return r
json.loads(f.readline())                      # QEMU's greeting
cmd({"execute": "qmp_capabilities"})
r = cmd({"execute": "human-monitor-command", "arguments": {"command-line": sys.argv[2]}})
print(r.get("return", r))
