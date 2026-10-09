# P1 skeleton: cross toolchain and reproducible build

Current milestone: P1 (see SYSTEMS_CURRICULUM.html, section 1.6).

One command builds everything from a clean clone:

    make

- `make target`    freestanding objects for x86_64-unknown-none-elf (kernel side)
- `make host-test` host unit tests, built with the host compiler and sanitizers, then run
- `make toolchain` prints the versions of every tool the build uses (record them in docs/log)
