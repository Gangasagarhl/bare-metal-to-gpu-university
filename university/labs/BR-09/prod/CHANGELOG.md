# Changelog

Newest first. Each entry says what changed for the people who use this project, and
whether their code or scripts can break. Version numbers follow semantic versioning
(README.md, "Versions"). The newest number must equal the one in CMakeLists.txt
(checked by tools/check_release.py).

## [1.0.0] - 2026-10-10

First stable release: from here on, the interface in README.md is a promise.

### Compatibility
- Library: source-compatible with 0.1.0 for finite inputs. Version 0.1.0's unit tests
  compile unchanged against 1.0.0 and pass (lab step `compat`).
- Command line: for valid numbers the output line and exit status are byte-identical
  to 0.1.0 (CTest `cli.golden_v0_1`, 8 cases).
- Breaking for some scripts: arguments that 0.1.0 crashed on or accepted silently are
  now rejected with exit status 3 (E-PARSE). See "Changed" and KI-4.

### Changed
- Strict number parsing (`uni::parse_number`): "3abc" (0.1.0 read 3), "0x10"
  (0.1.0 read 16), "4,4" (0.1.0 read 4), "nan", "inf" and spaces are errors.
- `uni::mean` and `uni::median` return no value when any input is NaN or infinite.

### Fixed
- Crash (abort, exit status 134) on a non-number such as `abc` and on `1e999`.
- Median of a list containing NaN depended on where the NaN was (1, nan or 2).
- Median of two values near the largest double overflowed to inf (now std::midpoint).

### Added
- `stats_cli --version`.
- Exit statuses 2-5 with stable error codes E-USAGE, E-PARSE, E-RANGE, E-INTERNAL.
- Failure-path tests: 6 CLI tests that check the exact exit status and the message
  (9 CLI tests in all, 11 CTest tests with the 2 unit-test programs).
- `ci.sh` with three builds; `tools/check_release.py` (versions, known issues, licences,
  review rules); THIRD_PARTY.txt; licences.allow.

### Known issues
- KI-1 to KI-4, see KNOWN_ISSUES.md.

### Built and tested with
- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, Ubuntu clang version 18.1.3 (1ubuntu1),
  cmake version 3.28.3, Python 3.13, on Linux x86_64 (as printed by the tools in ci.log).

## [0.1.0] - 2026-10-09

The SP202 course project as written in F2-26: `uni::mean`, `uni::median`, `stats_cli`,
two tests. Not a stable interface (0.y.z).
