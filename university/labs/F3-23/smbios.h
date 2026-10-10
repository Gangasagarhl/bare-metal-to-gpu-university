// smbios.h - F3-23: find the SMBIOS tables and read the strings for the boot banner.
#pragma once

namespace smbios {
void print_banner();   // "Booted on <vendor> <product> (firmware <vendor> <version> <date>)"
}
