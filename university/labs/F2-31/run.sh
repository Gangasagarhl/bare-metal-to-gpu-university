#!/usr/bin/env bash
# F2-31 lab: put the university template under git, make a short history, find the commit
# that broke a test with git bisect, revert it, resolve a merge conflict, tag the result,
# and check that a clean clone builds and tests with one command (curriculum P1).
# Author names and dates are fixed so that the commit hashes are the same on every run.
set -u
cd "$(dirname "$0")" || exit 2
LAB="$(pwd)"
T="${LAB}/.tmp"; rm -rf "$T"; mkdir -p "$T/home"
export HOME="$T/home" GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME="Amara Okafor" GIT_AUTHOR_EMAIL="amara@uni.invalid"
export GIT_COMMITTER_NAME="Amara Okafor" GIT_COMMITTER_EMAIL="amara@uni.invalid"
day=1
when() { export GIT_AUTHOR_DATE="2026-10-0${day}T10:00:00+00:00" GIT_COMMITTER_DATE="2026-10-0${day}T10:00:00+00:00"; day=$((day + 1)); }
status=0
begin() {
    OUT="${LAB}/$1.out"; LOG="${LAB}/$1.log"; : > "$OUT"
    {
        echo "listing:   $2"
        echo "toolchain: $(git --version); $(cmake --version | head -n 1); $(g++ --version | head -n 1)"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    } > "$LOG"
}
c() {
    echo "\$ $1" >> "$OUT"
    (cd "$HERE" && bash -c "$1") >> "$OUT" 2>&1
    LAST=$?
}
end() {
    sed -i -E "s#${T}/#./#g; s#${LAB}/#./#g" "$OUT"
    echo "exit code: ${LAST}" >> "$LOG"
    if [ $# -ge 2 ]; then echo "note:      $2" >> "$LOG"; fi
    if [ "$LAST" != "$1" ]; then echo "result:    UNEXPECTED (expected exit code $1)" >> "$LOG"; status=1; fi
}

# ---- 1. a repository with the template as its first commit ------------------------------
cp -r ../F2-26/template "$T/uni"
HERE="$T/uni"
begin first_commit "../F2-26/template (copied)" "git init; git add; git commit"
c "git init -q -b main && git status --short | head -n 4"
c "git add . && git status --short"
when; c "git commit -q -m 'P1: template with CMake, stats library, tests and ci.sh' && git log --oneline"
end 0

# ---- 2. four more commits; the third of them breaks a test ------------------------------
begin history "history/stats_nth.cpp, history/check.sh" "four commits, then git log, git show, git diff"
c "printf '# uni_template\n\nRun ./ci.sh to configure, build and test.\n' > README.md && git add README.md"
when; c "git commit -q -m 'docs: add README'"
when; c "cp $LAB/history/stats_nth.cpp src/stats.cpp && git commit -q -am 'stats: faster median with nth_element'"
when; c "sed -i 's/\"count %zu mean/\"n=%zu mean/' app/main.cpp && git commit -q -am 'app: shorter output'"
when; c "sed -i 's/--parallel 2/--parallel 4/' ci.sh && git commit -q -am 'ci: build with 4 jobs'"
c "git log --oneline"
c "git show --stat --format='%h %an %ad%n    %s' --date=short HEAD~2"
c "git diff HEAD~3 HEAD~2 -- src/stats.cpp"
end 0

# ---- 3. forensic: the tests fail on main; find the first bad commit -------------------
begin ci_fails "src/stats.cpp at HEAD" "./ci.sh (on the latest commit)"
c "./ci.sh 2>&1 | sed -n '/Start 1/,/tests failed/p'; echo \"ci.sh exit status: \${PIPESTATUS[0]}\""
c "git status --short"
end 0 "the CI script stops at the first failing configuration; the transcript keeps the test part"
begin bisect "history/check.sh" "git bisect start HEAD <first commit>; git bisect run check.sh"
c "cp $LAB/history/check.sh ../check.sh"
c "git bisect start HEAD \$(git rev-list --max-parents=0 HEAD)"
c "git bisect run ../check.sh"
c "git bisect log | grep -E '^(git bisect|# first bad)'"
c "git bisect reset"
end 0

# ---- 4. the fix: revert the bad commit, check again ------------------------------------
begin revert "src/stats.cpp" "git revert --no-edit <bad commit>; ./ci.sh"
when; c "git revert --no-edit \$(git log --format=%h --grep='nth_element') && git log --oneline -n 2"
c "./ci.sh | tail -n 1"
end 0

# ---- 5. a branch, a merge conflict and its resolution -----------------------------------
begin merge "app/main.cpp" "git switch -c feature/units; commit; git switch main; commit; git merge"
c "git switch -q -c feature/units && sed -i 's/median %g\\\\n/median %g (ms)\\\\n/' app/main.cpp && git diff --stat"
when; c "git commit -q -am 'app: say that times are in ms'"
c "git switch -q main && sed -i 's/\"n=%zu mean/\"runs=%zu mean/' app/main.cpp"
when; c "git commit -q -am 'app: call the count runs'"
when; c "git merge feature/units"
c "git status --short"
c "grep -n -A6 '<<<<<<<' app/main.cpp"
c "sed -i '/^<<<<<<< /,/^>>>>>>> /c\\    std::printf(\"runs=%zu mean %g median %g (ms)\\\\n\", xs.size(), *m, *md);' app/main.cpp && grep -n 'printf(\"runs' app/main.cpp"
c "git add app/main.cpp && git commit -q --no-edit && git log --oneline --graph -n 4"
end 0

# ---- 6. tag the milestone; a clean clone builds and tests with one command (P1) -------
begin clone "the repository above" "git tag -a P1; git clone; ./ci.sh in the clone"
when; c "git tag -a P1 -m 'P1: reproducible build and host tests' && git describe --tags"
HERE="$T"
c "git clone -q uni clone && cd clone && git log --oneline -n 1 && ./ci.sh | tail -n 1"
c "cd clone && git status --short && echo 'working tree clean (build/ is ignored)'"
end 0 "a second folder in the same container, not a second machine"

# ---- 7. inside git: objects and their hashes --------------------------------------------
HERE="$T/uni"
begin objects "the repository above" "git cat-file; git hash-object"
c "git cat-file -t HEAD && git cat-file -p HEAD"
c "git cat-file -p 'HEAD^{tree}'"
c "git hash-object README.md && git rev-parse HEAD:README.md"
c "printf 'hello\\n' | git hash-object --stdin && printf 'blob 6\\0hello\\n' | sha1sum"
end 0
rm -rf "$T"
exit $status
