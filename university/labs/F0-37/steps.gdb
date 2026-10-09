set trace-commands on
set pagination off
set environment ASAN_OPTIONS=detect_leaks=0
break steps.cpp:9
run
print i
print steps.size()
continue
print i
continue
print i
continue
print i
print i <= steps.size()
continue
kill
