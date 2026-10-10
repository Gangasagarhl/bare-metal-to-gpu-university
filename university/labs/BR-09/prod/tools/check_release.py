# SPDX-License-Identifier: LicenseRef-Uni-Lab
"""check_release.py: the release and review rules of this project, as code.

    python3 tools/check_release.py <root>                     release checks
    python3 tools/check_release.py <root> --review BASE HEAD  rules for one change (git)

Release checks: (1) the version in CMakeLists.txt equals the newest CHANGELOG.md entry;
(2) every KNOWN_ISSUES.md entry has a Status and a Workaround, and every KI-n named in the
code exists there; (3) every code file carries an SPDX-License-Identifier from
licences.allow, or is listed in THIRD_PARTY.txt with its origin, licence and SHA-256.
Prints one line per finding; exit status 1 if there is any finding, else 0.
"""
import hashlib
import pathlib
import re
import subprocess
import sys

CODE_SUFFIXES = {".cpp", ".cc", ".h", ".hpp", ".sh", ".py"}
CODE_DIRS = ("src", "include", "app", "tests", "tools")


def version_findings(root):
    found = []
    cmake = (root / "CMakeLists.txt").read_text()
    m = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+\.\d+)", cmake)
    log = (root / "CHANGELOG.md").read_text()
    n = re.search(r"^## \[(\d+\.\d+\.\d+)\]", log, re.M)
    if not m:
        found.append("CMakeLists.txt: no project(... VERSION x.y.z)")
    if not n:
        found.append("CHANGELOG.md: no '## [x.y.z]' entry")
    if m and n and m.group(1) != n.group(1):
        found.append("version mismatch: CMakeLists.txt says %s, newest CHANGELOG.md entry is %s"
                     % (m.group(1), n.group(1)))
    return found


def known_issue_findings(root):
    found = []
    text = (root / "KNOWN_ISSUES.md").read_text()
    blocks = re.split(r"^## ", text, flags=re.M)[1:]
    ids = set()
    for block in blocks:
        kid = block.split()[0]
        if not re.fullmatch(r"KI-\d+", kid):
            continue
        if kid in ids:
            found.append("KNOWN_ISSUES.md: %s appears twice" % kid)
        ids.add(kid)
        for field in ("Status:", "Workaround:"):
            if field not in block:
                found.append("KNOWN_ISSUES.md: %s has no '%s' line" % (kid, field))
    for path in code_files(root):
        for kid in sorted(set(re.findall(r"\bKI-\d+\b", path.read_text()))):
            if kid not in ids:
                found.append("%s: names %s, which is not in KNOWN_ISSUES.md"
                             % (path.relative_to(root), kid))
    return found


def code_files(root):
    files = [root / "CMakeLists.txt", root / "ci.sh"]
    for d in CODE_DIRS:
        files += sorted(p for p in (root / d).rglob("*") if p.suffix in CODE_SUFFIXES)
    return [f for f in files if f.exists()]


def licence_findings(root):
    found = []
    allowed = {line.split()[0] for line in (root / "licences.allow").read_text().splitlines()
               if line.strip() and not line.startswith("#")}
    third = {}
    for line in (root / "THIRD_PARTY.txt").read_text().splitlines():
        if line.strip() and not line.startswith("#"):
            fields = [f.strip() for f in line.split("|")]
            third[fields[0]] = fields  # path | origin | licence | sha256
    for path in code_files(root):
        rel = str(path.relative_to(root))
        head = path.read_text().splitlines()[:5]
        tags = [re.search(r"SPDX-License-Identifier:\s*(\S+)", h) for h in head]
        tags = [t.group(1) for t in tags if t]
        if tags:
            if tags[0] not in allowed:
                found.append("%s: licence %s is not in licences.allow (the owner decides)"
                             % (rel, tags[0]))
        elif rel in third:
            _, origin, licence, digest = third[rel]
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
            if licence not in allowed:
                found.append("%s: listed licence %s is not in licences.allow" % (rel, licence))
            if actual != digest:
                found.append("%s: changed since it was copied from %s (SHA-256 differs)"
                             % (rel, origin))
        else:
            found.append("%s: no SPDX-License-Identifier and not listed in THIRD_PARTY.txt"
                         " (where did it come from, under which licence?)" % rel)
    return found


def review_findings(root, base, head):
    out = subprocess.run(["git", "-C", str(root), "diff", "--name-only", base, head],
                         check=True, capture_output=True, text=True).stdout.split()
    code = [f for f in out if f.split("/")[0] in ("src", "app", "include")]
    found = []
    if code and not any(f.startswith("tests/") for f in out):
        found.append("review: code changed (%s) but no file in tests/ changed" % ", ".join(code))
    if code and "CHANGELOG.md" not in out:
        found.append("review: code changed but CHANGELOG.md has no entry for it")
    if any(f.startswith("include/") for f in out) and "CMakeLists.txt" not in out:
        found.append("review: public header changed but the version in CMakeLists.txt did not;"
                     " decide the bump (major if any old caller breaks)")
    print("review: %d file(s) changed: %s" % (len(out), ", ".join(out)))
    return found


def main(argv):
    root = pathlib.Path(argv[1]).resolve()
    if len(argv) == 5 and argv[2] == "--review":
        findings = review_findings(root, argv[3], argv[4])
    else:
        findings = version_findings(root) + known_issue_findings(root) + licence_findings(root)
    for f in findings:
        print("FINDING " + f)
    print("check_release: %d finding(s)" % len(findings))
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
