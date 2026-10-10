# F2-10 lab folder (SP102)

Chapter: F2-10 Constructors and destructors.

Run: `university/labs/run_lab.sh university/labs/F2-10`

Every `.cpp` is built with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; files with a `.expect-fail` marker must fail to compile.

| file | role in the chapter | expected result |
|---|---|---|
| `lifetime.cpp` | Listing 1, watching lifetimes with a Tracer | exit code 0; destruction in reverse order of construction |
| `init_order.cpp` + `.expect-fail` | Listing 2, initialiser list in the wrong order | compile fails with -Werror=reorder (and -Wuninitialized) |
| `defaults.cpp` | Listing 3, several constructors, no member left unset | exit code 0 |
| `ticket_fixed.cpp` | Listing 4, a ticket that cannot be copied | exit code 0; one close per ticket |
| `ticket_copy_refused.cpp` + `.expect-fail` | Listing 5, the by-value function with the deleted copy | compile fails: "use of deleted function 'Ticket::Ticket(const Ticket&)'" |
| `worked.cpp` | worked example program | exit code 0 |
| `shift_lab.cpp` | lab reference solution (shift log) | exit code 0; "shift log test: pass" |
| `ticket_bug.cpp` | forensic evidence: a copied ticket closed twice | exit code 0; "close ticket for table 4" printed twice |
| `run.sh` -> `main_calls.out` | hardware section: calls in main's machine code (objdump) | exit code 0; 14 call lines including `_Unwind_Resume` |

Recorded runs (g++ 13.3.0, Linux x86_64, 2026-10-09) are the `.out` and `.log` files next to each listing. Non-zero exit codes listed above are intended (sabotage runs, sanitizer reports, an uncaught exception).
