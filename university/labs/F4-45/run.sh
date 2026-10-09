#!/usr/bin/env bash
# F4-45 run.sh: record which package versions the survey's evidence comes from.
#   versions : the installed packages that own the headers and files the survey read
set -u -o pipefail
cd "$(dirname "$0")"
{ for f in /usr/include/drm/panfrost_drm.h /usr/include/drm/i915_drm.h /usr/include/linux/kfd_ioctl.h \
           /usr/include/hsa/hsa.h /usr/share/seabios/vgabios-stdvga.bin /lib/firmware/nvidia; do
      pkg="$(dpkg -S "$f" 2>/dev/null | head -n 1 | cut -d: -f1)"
      printf '%-42s %-22s %s\n' "$f" "${pkg:-(no package owns it)}" \
          "$([ -n "$pkg" ] && dpkg-query -W -f='${Version}' "$pkg" 2>/dev/null)"
  done; } > versions.out; rc=$?
{ echo "listing:   (none: dpkg queries)"; echo "toolchain: $(dpkg --version | head -n 1)"
  echo "command:   dpkg -S <file>; dpkg-query -W -f='\${Version}' <package>"
  echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"; echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
  echo "exit code: $rc"; } > versions.log
exit $rc
