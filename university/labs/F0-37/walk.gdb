set trace-commands on
set pagination off
set environment ASAN_OPTIONS=detect_leaks=0
break main
run
next
print cups
step
print number
next
print answer
next
next
print plates
continue
