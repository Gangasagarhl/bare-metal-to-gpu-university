# stats 1.0.0

Count, mean and median of a list of numbers: a C++ library (`uni::mean`, `uni::median`,
`uni::parse_number`) and a command-line tool (`stats_cli`). It started as the SP202 course
project (the F2-26 template, version 0.1.0) and was made production-grade in BR-09.

## Build and test

    ./ci.sh            # three builds (g++ Debug+sanitizers, g++ Release, clang++ Release),
                       # all tests, then the release checks; exit status 0 = mergeable

Needs CMake 3.20 or newer, a C++20 compiler and Python 3. Tested only where ci.sh ran
(see CHANGELOG.md, "Built and tested with"); everything else is untested (KI-3).

## Interface (this is the public contract; changing it changes the version)

    stats_cli <number>...      prints:  count <n> mean <m> median <d>
    stats_cli --version        prints:  stats_cli 1.0.0

Numbers: optional sign, decimal digits, optional '.', optional exponent ("1e3").
The decimal point is always '.', whatever the locale. Rejected: words, "3abc", "0x10",
spaces, "nan", "inf", values beyond the range of double. Output uses printf "%g"
(six significant digits), exactly as version 0.1 did (KI-2).

| exit status | code on stderr | meaning                                        |
|-------------|----------------|------------------------------------------------|
| 0           | (none)         | success                                        |
| 2           | E-USAGE        | no numbers given                               |
| 3           | E-PARSE        | an argument is not an accepted number          |
| 4           | E-RANGE        | a result does not fit in a double (KI-1)       |
| 5           | E-INTERNAL     | unexpected error; please report it             |

Error lines have the stable form `stats_cli: error <CODE>: <text>`, so logs can be
counted by code (BR-09 monitoring step).

Library contract: see the comment at the top of `include/uni/stats.h` and
`include/uni/parse.h`. Inputs must be finite; empty or non-finite inputs give no value.

## Versions

Semantic versioning: MAJOR when an existing caller can break, MINOR for compatible
additions, PATCH for compatible fixes. The version lives in one place, `CMakeLists.txt`.
Every release has an entry in `CHANGELOG.md`; open problems are in `KNOWN_ISSUES.md`.

## Reporting a problem, and who answers

Write the exact command, the full output, the exit status and `stats_cli --version`.
Maintainer and response promise: to be filled in by the owner (BR-09 decision box).
Security problems: report privately to the maintainer, not in a public issue.

## Licence

Own code: `LicenseRef-Uni-Lab` (placeholder until the owner decides, see
`licences.allow`). Copied files and their origins: `THIRD_PARTY.txt`.
