# F6-40 Listing 3: counts the lines of code of named kernels (blank lines and comment-only lines
# are not counted). C++/CUDA functions are found by brace matching, Python functions by indentation.
# A size measure only: it says nothing about speed or about how long the code took to write.
import sys


def cxx_function(lines, name):
    for i, line in enumerate(lines):
        if f" {name}(" in line and not line.rstrip().endswith(";"):
            depth, body, started = 0, [], False
            for row in lines[i:]:
                body.append(row)
                depth += row.count("{") - row.count("}")
                started = started or "{" in row
                if started and depth == 0:
                    return body
    raise SystemExit(f"{name} not found")


def py_function(lines, name):
    for i, line in enumerate(lines):
        if line.startswith(f"def {name}("):
            body = [lines[i - 1]] if i > 0 and lines[i - 1].startswith("@") else []
            body.append(line)
            for row in lines[i + 1:]:
                if row.strip() and not row.startswith((" ", "\t")):
                    break
                body.append(row)
            return body
    raise SystemExit(f"{name} not found")


def count(body, comment):
    return sum(1 for row in body if row.strip() and not row.strip().startswith(comment))


def main():
    total = {}
    for spec in sys.argv[1:]:
        label, path, names = spec.split(":")
        with open(path, encoding="utf-8") as f:
            lines = f.read().splitlines()
        py = path.endswith(".py")
        n = 0
        for name in names.split(","):
            body = py_function(lines, name) if py else cxx_function(lines, name)
            n += count(body, "#" if py else "//")
        total[label] = n
        print(f"{label:28s} {n:4d} lines  ({path}: {names})")


if __name__ == "__main__":
    main()
