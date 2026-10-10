# Known issues of stats 1.0.0

Each entry: what happens, who is affected, status, workaround. A test pins every
issue that can be tested, so a fix shows up as a failing test to update.

## KI-1 Mean of very large values is rejected although it would fit

`stats_cli 1e308 1e308` exits with status 4 (E-RANGE): the sum overflows before the
division, although the mean (1e308) fits in a double. `uni::mean` returns no value.
Affects inputs whose sum exceeds the largest finite double.
Status: open; candidate fix for 1.1.0 (compute the mean without a plain sum).
Pinned by: test `known_issue_ki1_mean_of_two_huge_values_has_no_value`, CTest `cli.overflow`.
Workaround: scale the inputs down (for example divide by 1e10) and scale the result back.

## KI-2 Output keeps only six significant digits

`stats_cli 1234567 1234568` prints `mean 1.23457e+06`. The format is version 0.1's,
kept on purpose because scripts parse this line; changing it is a breaking change.
Status: by design until 2.0.0 (a full-precision output needs a new option or a major version).
Pinned by: CTest `cli.golden_v0_1`.
Workaround: call `uni::mean` / `uni::median` from C++ for full precision.

## KI-3 Tested on one platform only

ci.sh ran on Linux x86-64 with the g++ and clang++ versions in CHANGELOG.md.
Other operating systems, CPUs and compilers have never been tested.
Status: open.
Workaround: run `./ci.sh` on your platform before relying on it, and report the result.

## KI-4 Arguments that version 0.1 accepted silently are now errors

`3abc` (read as 3 by 0.1), `0x10` (read as 16), arguments with spaces, `nan`, `inf`
now give exit status 3 (E-PARSE). Scripts that passed such text break on purpose.
Status: by design (CHANGELOG.md 1.0.0, "Changed").
Pinned by: test `rejects_what_version_0_1_accepted_silently`, CTest `cli.trailing`, `cli.nan`.
Workaround: pass clean decimal numbers.
