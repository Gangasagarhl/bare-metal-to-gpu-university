# F2-13 lab folder (SP102)

Chapter: F2-13 Copy and move.

Run: `university/labs/run_lab.sh university/labs/F2-13`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `copy_move.cpp` | Listing 1, which special member runs | exit code 0 |
| `rule_of_zero.cpp` | Listing 2, rule of zero | exit code 0 |
| `file_move.cpp` | Listing 3, movable non-copyable File | exit code 0; open files stay 5 |
| `file_copy.cpp` + `.expect-fail` | Listing 4, copying a non-copyable owner | compile fails: "use of deleted function 'File::File(const File&)'" |
| `move_buffer.cpp` | Listing 5, a move transfers the heap block | exit code 0; copy same block 0, move same block 1 |
| `move_noexcept.cpp` | Listing 6, copies and moves during vector growth | exit code 0; "0 copies, 227 moves" versus "127 copies, 100 moves" |
| `self_move.cpp` + `.expect-fail` | Listing 7, obvious self-move | compile fails with -Werror=self-move |
| `copy_lab.cpp` | lab reference solution (OrderBook) | exit code 0; "OrderBook tests: 0 failures" |
| `copy_lab_forgot_move.cpp` | lab step 3 sabotage: std::move forgotten in the move constructor | exit code 1; 1 FAIL line |
| `move_bug.cpp` | forensic evidence: moved-from strings read for the receipts | exit code 0; receipts with empty dish names |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
