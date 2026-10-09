# commit_change.py - F3-34 forensic lab: applies "the commit" under investigation to pipe_model.cpp.
#   python3 commit_change.py pipe_model.cpp <output file> [1|2|3]
# With a third argument, only that one of the three changes is applied (for the answer key).
import sys
s = open(sys.argv[1]).read()
only = int(sys.argv[3]) if len(sys.argv) > 3 else 0
def rep(a, b):
    global s
    assert a in s, a
    s = s.replace(a, b)
if only in (0, 1):
  rep("""            std::size_t k = std::min(n - done, buf_.size() - count_);
""", """            std::size_t k = std::min(n - done, buf_.size() - count_);
            bool was_empty = count_ == 0;
""")
  rep("""            readable_.notify_all();                    // data appeared: wake every sleeping reader
""", """            if (was_empty) readable_.notify_one();     // only an empty pipe can have sleepers
""")
if only in (0, 2):
  rep("""        writable_.notify_all();                        // space appeared: wake every sleeping writer
""", """        if (sleeping_writers_ > 0) writable_.notify_all();   // skip the call when nobody sleeps
""")
if only in (0, 3):
  rep("Pipe jobs(4096);", "Pipe jobs(8192);")
open(sys.argv[2], "w").write(s)
