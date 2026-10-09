#!/usr/bin/env bash
# F4-03 run.sh: milestone C2 tests in QEMU ("pc" machine: i8042 and two 16550 UARTs).
#   build    : the F4-03 kernel (F4-01 base + isr.S irq.cc i8042.cc tty.cc uart_irq.cc)
#   keyboard : keys and mouse moves injected through QEMU's monitor (qmp_drive.py)
#   paste    : 64 KiB into COM2 from a file, echoed back by the transmit interrupt path
#   forensic : the keyboard test with a kernel that never sends EOI for line 1
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/driver.cc isr.S irq.cc i8042.cc tty.cc uart_irq.cc f403_main.cc"
QM="-machine pc -m 64M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
KEYS=(hmp:"sendkey h" hmp:"sendkey e" hmp:"sendkey l" hmp:"sendkey l" hmp:"sendkey p"
      hmp:"sendkey backspace" hmp:"sendkey o" hmp:"sendkey spc" hmp:"sendkey w" hmp:"sendkey o"
      hmp:"sendkey r" hmp:"sendkey l" hmp:"sendkey d" hmp:"sendkey ret"
      hmp:"sendkey shift-d" hmp:"sendkey shift-r" hmp:"sendkey 3" hmp:"sendkey 0" hmp:"sendkey 1"
      hmp:"sendkey spc" hmp:"sendkey o" hmp:"sendkey k" hmp:"sendkey ret")
MOUSE=(hmp:"mouse_move 10 -5" hmp:"mouse_button 1" hmp:"mouse_button 0")

kbuild k403.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k403.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k403.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

# 1. keyboard and mouse
rm -f .ser.txt
python3 -I qmp_drive.py .ser.txt wait:"tty ready" "${KEYS[@]}" "${MOUSE[@]}" exit -- \
    $QEMU $QM -serial file:.ser.txt -kernel k403.elf -trace ps2_put_keycode -D .trace.txt > .drive.txt 2>&1; rc=$?
{ echo "== serial log of the kernel =="; sed 's/\r$//' .ser.txt;
  echo "== injected through QEMU's monitor (qmp_drive.py) =="; cat .drive.txt;
  echo "== QEMU trace: scan codes the emulated keyboard produced (ps2_put_keycode), count and first 6 =="
  grep -c ps2_put_keycode .trace.txt; grep ps2_put_keycode .trace.txt | head -n 6 | sed 's/^[0-9]*@[0-9.]*://'; } > keyboard.out
rec keyboard "f403_main.cc keyboard_test (kernel k403.elf)" "$QEMU_VER; $(python3 --version)" \
    "python3 -I qmp_drive.py ser.txt wait:'tty ready' <23 sendkey steps> <3 mouse steps> exit -- $QEMU $QM -serial file:ser.txt -kernel k403.elf -trace ps2_put_keycode -D trace.txt" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
grep -q 'shell: line "hello world"' keyboard.out && grep -q 'shell: line "DR301 ok"' keyboard.out || status=1

# 2. 64 KiB through COM2 from a host program (socket chardev), echoed back and compared.
# A failed attempt is kept in paste.out and the test runs once more; the log says so.
python3 -I mkpaste.py .paste.bin > .host0.txt
paste_once() {
    cp .host0.txt .host.txt
    rm -f .com2.sock .paste.txt
    ( timeout 120 $QEMU -machine pc $QCOMMON -serial file:.paste.txt -kernel k403.elf \
          -chardev socket,id=c2,path=.com2.sock,server=on,wait=off -serial chardev:c2 -append paste \
          > .qerr.txt 2>&1; echo "$?" > .qrc.txt ) &
    python3 -I paste_feed.py .com2.sock .paste.bin .paste.txt >> .host.txt; frc=$?
    wait
    rc="$(cat .qrc.txt)"
    sed -i 's/\r$//' .paste.txt; cat .qerr.txt >> .paste.txt
    k="$(grep -o 'fnv1a [0-9a-f]*' .paste.txt)"; h="$(grep -o 'fnv1a [0-9a-f]*' .host.txt)"
    [ "$rc" = 33 ] && [ "$frc" = 0 ] && [ -n "$k" ] && [ "$k" = "$h" ]
}
attempts=1; : > .failed.txt
if ! paste_once; then
    { echo "== attempt 1 failed (QEMU exit $rc, feeder exit $frc); its logs: =="; cat .paste.txt .host.txt; } > .failed.txt
    attempts=2; paste_once
fi
pok=$?
{ cat .paste.txt; echo "== host side =="; cat .host.txt; cat .failed.txt; } > paste.out
rec paste "f403_main.cc paste_test; mkpaste.py; paste_feed.py" "$QEMU_VER; $(python3 --version)" \
    "python3 -I mkpaste.py paste.bin; $QEMU -machine pc $QCOMMON -serial file:paste.txt -kernel k403.elf -chardev socket,id=c2,path=com2.sock,server=on,wait=off -serial chardev:c2 -append paste & python3 -I paste_feed.py com2.sock paste.bin paste.txt" "$rc" \
    "note:      kernel $k, host $h; paste_feed.py exit code $frc; attempts: $attempts (a failed first attempt is shown at the end of paste.out)" "$HW_NOTE"
[ "$pok" = 0 ] || status=1

# 3. forensic: no EOI for line 1
KEXTRA="-DF403_FORGET_EOI_LINE=1" kbuild k403f.elf $SRC > .kbf.txt 2>&1 || status=1
rm -f .serf.txt
python3 -I qmp_drive.py .serf.txt wait:"tty ready" "${KEYS[@]}" "${MOUSE[@]}" sleep:3 quit -- \
    $QEMU $QM -serial file:.serf.txt -kernel k403f.elf -trace ps2_put_keycode -trace pckbd_kbd_read_data -D .tracef.txt > .drivef.txt 2>&1; rc=$?
{ echo "== serial log of the kernel =="; sed 's/\r$//' .serf.txt;
  echo "== QEMU trace summary =="
  echo "ps2_put_keycode events (keyboard produced a scan code): $(grep -c ps2_put_keycode .tracef.txt)"
  echo "pckbd_kbd_read_data events (CPU read port 0x60):       $(grep -c pckbd_kbd_read_data .tracef.txt)"; } > forensic.out
rec forensic "f403_main.cc keyboard_test, kernel built with -DF403_FORGET_EOI_LINE=1" "$QEMU_VER; $(python3 --version)" \
    "python3 -I qmp_drive.py serf.txt wait:'tty ready' <23 sendkey steps> <3 mouse steps> sleep:3 quit -- $QEMU $QM -serial file:serf.txt -kernel k403f.elf -trace ps2_put_keycode -trace pckbd_kbd_read_data -D tracef.txt" "$rc" \
    "note:      exit code 0: the script ended the run with 'quit'; the kernel never reached 'F4-03 done'" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

rm -f k403.elf k403f.elf .kb.txt .kbf.txt .ser.txt .serf.txt .trace.txt .tracef.txt .drive.txt .drivef.txt .paste.bin .paste.txt .host.txt .host0.txt .failed.txt .com2.sock .qrc.txt .qerr.txt
exit $status
