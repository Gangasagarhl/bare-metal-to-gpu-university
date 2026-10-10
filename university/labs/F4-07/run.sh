#!/usr/bin/env bash
# F4-07 run.sh: milestone C6 in QEMU (q35, one NVMe controller with two namespaces).
#   build    : the F4-07 kernel
#   nvme     : Identify, PRP lists, random I/O with 1, 4 and 8 queue pairs, the USTAR root
#   identify : the Identify fields compared with the QEMU command line
#   image    : the host checks namespace 1's image (verify_image.py of F4-05)
#   root     : the kernel's file list compared with the host's (mkroot.py)
#   nvme_trace: QEMU's own trace of the admin commands (pci_nvme_* events)
#   forensic : the completion loop that ignores the phase tag
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-03/isr.S ../F4-03/irq.cc \
../F4-05/iotest.cc nvme.cc ustar.cc f407_main.cc"
KEXTRA="-I../F4-03"
DEVS="-drive file=.ns1.img,if=none,id=n1,format=raw -drive file=.root.img,if=none,id=n2,format=raw,readonly=on \
-device nvme,id=nvme0,serial=DR301NVME0001,mdts=5 -device nvme-ns,drive=n1,bus=nvme0,nsid=1 \
-device nvme-ns,drive=n2,bus=nvme0,nsid=2,logical_block_size=4096,physical_block_size=4096"
DEVS_SHOWN="-drive file=ns1.img,if=none,id=n1,format=raw -drive file=root.img,if=none,id=n2,format=raw,readonly=on -device nvme,id=nvme0,serial=DR301NVME0001,mdts=5 -device nvme-ns,drive=n1,bus=nvme0,nsid=1 -device nvme-ns,drive=n2,bus=nvme0,nsid=2,logical_block_size=4096,physical_block_size=4096"

kbuild k407.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k407.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -I../F4-03 -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k407.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

rm -f .ns1.img .root.img; truncate -s 1G .ns1.img
python3 -I mkroot.py .root.img > .root.expect || status=1
qboot nvme.out 600 q35 k407.elf $DEVS; rc=$?
rec nvme "f407_main.cc (kernel k407.elf)" "$QEMU_VER" \
    "truncate -s 1G ns1.img; python3 -I mkroot.py root.img; $QEMU -machine q35 $QCOMMON -serial stdio -kernel k407.elf $DEVS_SHOWN" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10); one CPU drives all queues (the 8-CPU part of C6 is not run here)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# Identify against the configuration: serial= and mdts= from the command line, nn = 2
# namespaces, sizes from the image files (1 GiB / 512; root image size / 4096).
ROOT_BLOCKS=$(( $(stat -c %s .root.img) / 4096 ))
( echo "configured: serial=DR301NVME0001 mdts=5 namespaces=2 ns1=$((1073741824 / 512)) x 512 ns2=$ROOT_BLOCKS x 4096"
  rc=0
  for want in 'serial "DR301NVME0001"' 'mdts 5' 'active namespaces: 2' "namespace 1: nsze 2097152 blocks of 512 bytes" \
              "namespace 2: nsze $ROOT_BLOCKS blocks of 4096 bytes"; do
      if grep -qF "$want" nvme.out; then echo "match:   $want"; else echo "MISSING: $want"; rc=1; fi
  done; exit $rc ) > identify.out; rc=$?
rec identify "(run.sh: grep of nvme.out)" "$(grep --version | head -n 1)" "grep -F '<each expected field>' nvme.out" "$rc"
[ "$rc" = 0 ] || status=1

python3 -I ../F4-05/verify_image.py .ns1.img 0x2F4A0701,0x2F4A0704,0x2F4A0708 4000 > image.out; rc=$?
rec image "../F4-05/verify_image.py" "$(python3 --version)" "python3 -I ../F4-05/verify_image.py ns1.img 0x2F4A0701,0x2F4A0704,0x2F4A0708 4000" "$rc"
[ "$rc" = 0 ] || status=1

( echo "host (mkroot.py):"; cat .root.expect; echo "kernel (nvme.out):"; grep -E '^root: .* bytes fnv1a' nvme.out
  diff <(cat .root.expect) <(grep -E '^root: .* bytes fnv1a' nvme.out) > /dev/null && echo "identical" || { echo "DIFFERENT"; exit 1; }
) > root.out; rc=$?
rec root "mkroot.py" "$(python3 --version)" "python3 -I mkroot.py root.img > root.expect; diff root.expect <(grep '^root: .* bytes fnv1a' nvme.out)" "$rc"
[ "$rc" = 0 ] || status=1

# QEMU's view of the initialisation: the admin commands it received
rm -f .ns1.img; truncate -s 1G .ns1.img
qboot .t.txt 300 q35 k407.elf $DEVS -trace 'pci_nvme_admin_cmd' -trace 'pci_nvme_create_*' -trace 'pci_nvme_identify_*' \
    -trace 'pci_nvme_setfeat_numq' -trace 'pci_nvme_mmio_start_success' -D .ntrace.txt; rc=$?
# The firmware (SeaBIOS) drives the controller first; our kernel starts at the last enable.
n_en=$(grep -c pci_nvme_mmio_start_success .ntrace.txt)
first=$(grep -n pci_nvme_mmio_start_success .ntrace.txt | tail -n 1 | cut -d: -f1)
{ echo "QEMU trace events pci_nvme_admin_cmd, _create_*, _identify_*, _setfeat_numq, _mmio_start_success"
  echo "$(wc -l < .ntrace.txt) lines in all; the controller was enabled $n_en times."
  echo "== the firmware's use of the controller before the kernel ran (first 8 lines) =="
  sed 's/^[0-9]*@[0-9.]*://' .ntrace.txt | head -n 8
  echo "== the lab kernel's initialisation (from the last enable, first 24 lines; process id and time stamp removed) =="
  tail -n +"$first" .ntrace.txt | sed 's/^[0-9]*@[0-9.]*://' | head -n 24; } > nvme_trace.out
rec nvme_trace "f407_main.cc (kernel k407.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k407.elf $DEVS_SHOWN -trace pci_nvme_admin_cmd -trace 'pci_nvme_create_*' -trace 'pci_nvme_identify_*' -trace pci_nvme_setfeat_numq -trace pci_nvme_mmio_start_success -D trace.txt" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f .ns1.img; truncate -s 1G .ns1.img
qboot forensic.out 120 q35 k407.elf -append nophase $DEVS -trace 'pci_nvme_rw_cb' -D .ftrace.txt; rc=$?
echo "QEMU trace: pci_nvme_rw_cb (one line per command the controller completed): $(grep -c pci_nvme_rw_cb .ftrace.txt) lines" >> forensic.out
rec forensic "f407_main.cc with -append nophase (kernel k407.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k407.elf -append nophase $DEVS_SHOWN -trace pci_nvme_rw_cb -D trace.txt" "$rc" \
    "note:      exit code 3 is expected here: the kernel reports the failure through isa-debug-exit 0x01" "$HW_NOTE"
[ "$rc" = 3 ] || status=1

rm -f k407.elf .kb.txt .ns1.img .root.img .root.expect .t.txt .ntrace.txt .ftrace.txt
exit $status
