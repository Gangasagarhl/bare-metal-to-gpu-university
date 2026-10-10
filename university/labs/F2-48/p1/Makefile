# P1: one build, two configurations.
#   target: freestanding code for the x86_64-unknown-none-elf target (never the host's default)
#   host:   unit tests that run on the build machine
TARGET_CXX   := clang++ --target=x86_64-unknown-none-elf
TARGET_FLAGS := -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -nostdlib \
                -O2 -Wall -Wextra -Werror
HOST_CXX     := g++
HOST_FLAGS   := -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined

.PHONY: all target host-test toolchain clean
all: toolchain target host-test

toolchain:
	@./tools/toolchain.sh

target: build/target/empty.o

build/target/empty.o: kernel/empty.cpp | build/target
	$(TARGET_CXX) $(TARGET_FLAGS) -c $< -o $@

host-test: build/host/test_trivial
	./build/host/test_trivial

build/host/test_trivial: tests/test_trivial.cpp | build/host
	$(HOST_CXX) $(HOST_FLAGS) $< -o $@

build/target build/host:
	mkdir -p $@

clean:
	rm -rf build
