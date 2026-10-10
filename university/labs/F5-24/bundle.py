# bundle.py - edit or summarise the OCI config.json that `runc spec` generated.
#   python3 bundle.py summary  <config.json>
#   python3 bundle.py run      <config.json> <program> <hostname> [memory-limit-bytes]
import json
import sys

mode, path = sys.argv[1], sys.argv[2]
with open(path) as f:
    cfg = json.load(f)
if mode == "summary":
    print("ociVersion:", cfg["ociVersion"])
    print("process.args:", cfg["process"]["args"])
    print("process.user:", cfg["process"]["user"])
    print("root.path:", cfg["root"]["path"], "readonly:", cfg["root"].get("readonly"))
    print("linux.namespaces:", [n["type"] for n in cfg["linux"]["namespaces"]])
    print("mounts (destinations):", [m["destination"] for m in cfg["mounts"]])
    print("capabilities kept (bounding):", cfg["process"]["capabilities"]["bounding"])
    print("linux.resources:", json.dumps(cfg["linux"].get("resources", {}), sort_keys=True))
else:
    cfg["process"]["terminal"] = False          # no terminal: output goes to our pipe
    cfg["process"]["args"] = [sys.argv[3]]
    cfg["hostname"] = sys.argv[4]
    if len(sys.argv) > 5:
        cfg["linux"].setdefault("resources", {})["memory"] = {"limit": int(sys.argv[5])}
    with open(path, "w") as f:
        json.dump(cfg, f, indent=2)
