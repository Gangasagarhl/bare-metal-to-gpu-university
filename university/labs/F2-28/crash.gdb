# crash.gdb: run the program on the new machine's file and look at the crash
set pagination off
run < config_new.in
backtrace
frame 3
info locals
print settings.size()
print settings[1].key
