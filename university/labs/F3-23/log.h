// log.h - F3-23: leveled kernel logging to every console, with an output lock.
#pragma once
#include "kformat.h"

enum class Level { Debug, Info, Warn, Error };

void log_init(bool with_framebuffer);    // serial always; framebuffer console when available
void klog(Level level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void log_set_min_level(Level level);
void log_set_clock(uint64_t (*now_ns)()); // F3-25 adds timestamps
void log_lock_acquire();                  // exposed for the deadlock test
void log_lock_release();
