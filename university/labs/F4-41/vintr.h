// vintr.h - virtual interrupts for the F4-41 guest: an 8259 model and timer injection.
#pragma once
#include "hv.h"

void vintr_attach(Vcpu& v);   // installs the I/O, entry, halt and exit hooks
uint64_t vintr_injected();    // timer interrupts injected so far
