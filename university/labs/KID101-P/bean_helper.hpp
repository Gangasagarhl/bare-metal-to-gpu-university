// The "literal bean helper": a tiny simulator written for the KID101 practical exam.
// It is the course's own teaching model of a computer that follows written steps,
// like the literal cook of F0-01. It is not a model of any real machine.
//
// Two cups stand on the table: a LEFT cup with some beans and a RIGHT cup. The helper
// has one hand. It reads one step per line and does exactly what the line says.
// The steps it understands (exact words, capital letters):
//   PICK UP ONE BEAN FROM THE LEFT CUP
//   PUT THE BEAN IN THE RIGHT CUP
//   IF THE RIGHT CUP HAS n BEANS, STOP      (n is a whole number; LEFT CUP also works)
//   GO BACK TO STEP n                        (n is a step number)
//   STOP
// Any other line is "not understood" and skipped. A step that cannot be done
// (for example PUT with an empty hand) does nothing and says so. The helper gives up
// after 30 steps so that a loop that never ends still produces a short trace.
#include <iostream>
#include <string>
#include <vector>

struct Table
{
    int leftCup = 10;
    int rightCup = 0;
    bool handHoldsBean = false;
};

inline bool startsWith(const std::string& line, const std::string& prefix)
{
    return line.size() >= prefix.size() && line.compare(0, prefix.size(), prefix) == 0;
}

inline bool endsWith(const std::string& line, const std::string& suffix)
{
    return line.size() >= suffix.size()
        && line.compare(line.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Reads the number between a prefix and a suffix; -1 if the line does not fit.
inline int numberBetween(const std::string& line, const std::string& prefix, const std::string& suffix)
{
    if (!startsWith(line, prefix) || !endsWith(line, suffix) || line.size() <= prefix.size() + suffix.size()) {
        return -1;
    }
    const std::string middle = line.substr(prefix.size(), line.size() - prefix.size() - suffix.size());
    if (middle.empty()) {
        return -1;
    }
    for (const char c : middle) {
        if (c < '0' || c > '9') {
            return -1;
        }
    }
    return std::stoi(middle);
}

inline std::string describe(const Table& t)
{
    return "left cup " + std::to_string(t.leftCup) + ", right cup " + std::to_string(t.rightCup)
         + ", hand: " + (t.handHoldsBean ? "one bean" : "empty");
}

// Runs the steps read from `in` on a table with `leftStart` beans in the left cup and
// `rightStart` in the right cup. The task passes when the helper reaches STOP with
// exactly `target` beans in the right cup and nothing left in its hand. Returns 0 on
// a pass and 1 otherwise, so the run record shows the result.
inline int runHelper(std::istream& in, int leftStart, int rightStart, int target)
{
    const int stepLimit = 30;
    Table t;
    t.leftCup = leftStart;
    t.rightCup = rightStart;

    std::vector<std::string> steps;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (!line.empty()) {
            steps.push_back(line);
        }
    }

    std::cout << "The task: end with exactly " << target << " beans in the right cup, then STOP.\n";
    std::cout << "At the start: " << describe(t) << '\n';
    std::cout << "The helper read " << steps.size() << " steps.\n\n";

    int done = 0;
    int notUnderstood = 0;
    int nothingHappened = 0;
    bool stopped = false;
    std::size_t next = 0;
    while (next < steps.size() && !stopped && done < stepLimit) {
        const std::size_t number = next + 1;
        const std::string& step = steps[next];
        next = next + 1;
        done = done + 1;
        std::cout << "step " << number << ": " << step << " -> ";
        if (step == "PICK UP ONE BEAN FROM THE LEFT CUP") {
            if (t.handHoldsBean) {
                nothingHappened = nothingHappened + 1;
                std::cout << "nothing happened: the hand already holds a bean";
            } else if (t.leftCup == 0) {
                nothingHappened = nothingHappened + 1;
                std::cout << "nothing happened: the left cup is empty";
            } else {
                t.leftCup = t.leftCup - 1;
                t.handHoldsBean = true;
                std::cout << describe(t);
            }
        } else if (step == "PUT THE BEAN IN THE RIGHT CUP") {
            if (!t.handHoldsBean) {
                nothingHappened = nothingHappened + 1;
                std::cout << "nothing happened: the hand is empty";
            } else {
                t.rightCup = t.rightCup + 1;
                t.handHoldsBean = false;
                std::cout << describe(t);
            }
        } else if (step == "STOP") {
            stopped = true;
            std::cout << "the helper stops";
        } else if (numberBetween(step, "IF THE RIGHT CUP HAS ", " BEANS, STOP") >= 0) {
            const int n = numberBetween(step, "IF THE RIGHT CUP HAS ", " BEANS, STOP");
            if (t.rightCup == n) {
                stopped = true;
                std::cout << "yes, the right cup has " << t.rightCup << ": the helper stops";
            } else {
                std::cout << "no, the right cup has " << t.rightCup;
            }
        } else if (numberBetween(step, "IF THE LEFT CUP HAS ", " BEANS, STOP") >= 0) {
            const int n = numberBetween(step, "IF THE LEFT CUP HAS ", " BEANS, STOP");
            if (t.leftCup == n) {
                stopped = true;
                std::cout << "yes, the left cup has " << t.leftCup << ": the helper stops";
            } else {
                std::cout << "no, the left cup has " << t.leftCup;
            }
        } else if (numberBetween(step, "GO BACK TO STEP ", "") >= 0) {
            const int n = numberBetween(step, "GO BACK TO STEP ", "");
            if (n >= 1 && static_cast<std::size_t>(n) <= steps.size()) {
                next = static_cast<std::size_t>(n - 1);
                std::cout << "going to step " << n;
            } else {
                nothingHappened = nothingHappened + 1;
                std::cout << "nothing happened: there is no step " << n;
            }
        } else {
            notUnderstood = notUnderstood + 1;
            std::cout << "I do not understand this step; skipped";
        }
        std::cout << '\n';
    }

    std::cout << "\nWhat happened at the end:\n";
    std::cout << "  beans in the left cup:         " << t.leftCup << '\n';
    std::cout << "  beans in the right cup:        " << t.rightCup << '\n';
    std::cout << "  bean still in the hand:        " << (t.handHoldsBean ? "yes" : "no") << '\n';
    std::cout << "  the helper reached STOP:       " << (stopped ? "yes" : "no") << '\n';
    if (!stopped && done >= stepLimit) {
        std::cout << "  gave up after " << stepLimit << " steps:        yes (the steps never reached STOP)\n";
    } else if (!stopped) {
        std::cout << "  ran out of steps before STOP:  yes\n";
    }
    std::cout << "  steps the helper did:          " << done << '\n';
    std::cout << "  steps it did not understand:   " << notUnderstood << '\n';
    std::cout << "  steps where nothing happened:  " << nothingHappened << '\n';
    const bool pass = stopped && t.rightCup == target && !t.handHoldsBean;
    std::cout << "  task passed:                   " << (pass ? "yes" : "no") << '\n';
    return pass ? 0 : 1;
}
