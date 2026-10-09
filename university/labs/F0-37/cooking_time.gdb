set trace-commands on
set pagination off
set environment ASAN_OPTIONS=detect_leaks=0
break cooking_time.cpp:11
run
print m
print total
continue
print m
print total
continue
print m
print total
continue
