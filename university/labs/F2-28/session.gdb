# session.gdb: a GDB session for streak.cc, run with: gdb -q -batch -x session.gdb ./streak
set pagination off
break longest_rise
run
info args
print temps.size()
# stop each time 'best' changes value
watch best
continue
continue
info locals
# remove the watchpoint (number 2), stop on the return line instead
delete 2
break streak.cc:21
continue
print run
print best
backtrace
continue
