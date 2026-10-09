# compact.sed - shorten names in profiler output (display only; numbers are untouched)
s/std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >/std::string/g
s/std::allocator<std::string >/A/g
s/std::vector<std::string, A >/std::vector<std::string>/g
s/std::unordered_set<std::string, std::hash<std::string >, std::equal_to<std::string >, A >/std::unordered_set<std::string>/g
s/std::vector<Order, std::allocator<Order> >/std::vector<Order>/g
s/std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul[^>]*>/std::mt19937/g
s/\[abi:cxx11\]//g
s/^==[0-9]+==/==PID==/
s# \[/[^]]*/\.bin_[a-z0-9_]+\]# [program]#g
s# \[/usr/lib/x86_64-linux-gnu/([^]]+)\]# [\1]#g
