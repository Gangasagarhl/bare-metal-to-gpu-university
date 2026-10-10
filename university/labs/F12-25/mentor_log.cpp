// mentor_log.cpp - read a semester of mentoring-session records and point at patterns.
// Input lines:
//   session <n> <minutes> <mentee keyboard %> <questions by mentee> <questions by mentor>
//           <goal set by: mentee|mentor|both> <reflection written: y|n>
//           <specific feedback points> <vague feedback points>
//   cancel  <n> <cancelled by: mentee|mentor>
// The numbers are crude signals that the mentor records honestly after each session;
// they start a conversation in the reflection, they do not grade anybody.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Session
{
    int n = 0, minutes = 0, keyboard = 0, qMentee = 0, qMentor = 0;
    std::string goalBy;
    char reflection = 'n';
    int specific = 0, vague = 0;
    bool cancelled = false;
    std::string cancelledBy;
};

int main()
{
    std::vector<Session> log;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind) || kind[0] == '#') {
            continue;
        }
        Session s;
        if (kind == "session" && in >> s.n >> s.minutes >> s.keyboard >> s.qMentee >> s.qMentor
                                       >> s.goalBy >> s.reflection >> s.specific >> s.vague) {
            log.push_back(s);
        } else if (kind == "cancel" && in >> s.n >> s.cancelledBy) {
            s.cancelled = true;
            log.push_back(s);
        } else {
            std::cout << "cannot read: " << line << '\n';
            return 2;
        }
    }

    enum { Drove, Told, MentorGoal, NoReflection, Vague, Rules };
    const char* ruleText[Rules] = {
        "mentor held the keyboard most of the session",
        "mentor asked no questions",
        "the goal was set by the mentor alone",
        "no written reflection",
        "more vague than specific feedback",
    };
    int count[Rules] = {};
    int held = 0, cancelledMentee = 0, cancelledMentor = 0, streak = 0, longestStreak = 0;

    std::cout << " #  min  mentee-kbd  q(mentee)  q(mentor)  goal    refl  feedback s/v  flags\n";
    for (const Session& s : log) {
        if (s.cancelled) {
            (s.cancelledBy == "mentee" ? cancelledMentee : cancelledMentor)++;
            ++streak;
            longestStreak = std::max(longestStreak, streak);
            std::cout << std::setw(2) << s.n << "  cancelled by " << s.cancelledBy << '\n';
            continue;
        }
        streak = 0;
        ++held;
        bool flag[Rules] = {s.keyboard < 50, s.qMentor == 0, s.goalBy == "mentor",
                            s.reflection != 'y', s.vague > s.specific};
        std::cout << std::setw(2) << s.n << std::setw(5) << s.minutes << std::setw(10)
                  << s.keyboard << " %" << std::setw(11) << s.qMentee << std::setw(11) << s.qMentor
                  << "  " << std::left << std::setw(8) << s.goalBy << std::right << "  "
                  << s.reflection << std::setw(10) << s.specific << '/' << s.vague << "    ";
        for (int r = 0; r < Rules; ++r) {
            if (flag[r]) {
                ++count[r];
                std::cout << 'F' << r + 1 << ' ';
            }
        }
        std::cout << '\n';
    }
    std::cout << "\nsessions held " << held << ", cancelled by mentee " << cancelledMentee
              << ", by mentor " << cancelledMentor << ", longest run of cancellations "
              << longestStreak << '\n';

    // Compare the first and the second half of the held sessions.
    std::vector<const Session*> h;
    for (const Session& s : log) {
        if (!s.cancelled) {
            h.push_back(&s);
        }
    }
    if (h.size() >= 2) {
        const std::size_t half = h.size() / 2;
        auto avg = [&](std::size_t from, std::size_t to, auto field) {
            double sum = 0;
            for (std::size_t i = from; i < to; ++i) {
                sum += field(*h[i]);
            }
            return sum / static_cast<double>(to - from);
        };
        auto kb = [](const Session& s) { return s.keyboard; };
        auto qm = [](const Session& s) { return s.qMentee; };
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "mentee keyboard share: first half " << avg(0, half, kb) << " %, second half "
                  << avg(half, h.size(), kb) << " %\n";
        std::cout << "questions by mentee:   first half " << avg(0, half, qm)
                  << " per session, second half " << avg(half, h.size(), qm) << '\n';
    }
    std::cout << "\nflags (a pattern is a flag in 3 or more held sessions):\n";
    for (int r = 0; r < Rules; ++r) {
        std::cout << "  F" << r + 1 << " " << std::left << std::setw(46) << ruleText[r]
                  << std::right << std::setw(3) << count[r] << (count[r] >= 3 ? "  PATTERN" : "")
                  << '\n';
    }
    if (cancelledMentee >= 2) {
        std::cout << "  the mentee cancelled " << cancelledMentee
                  << " sessions: ask, kindly, what the sessions are not giving them\n";
    }
    return 0;
}
