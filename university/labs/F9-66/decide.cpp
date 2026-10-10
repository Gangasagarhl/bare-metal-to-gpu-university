// decide.cpp - F9-66: a weighted decision matrix for the robot computer's operating system,
// with hard requirements and a sensitivity check.
// Input (stdin):
//   option <name>                          one line per candidate architecture
//   must <requirement> <option> <yes|no>   hard requirements: an option with "no" is out
//   criterion <name> <weight> <score per option, in option order, 1..5>
// The scores and weights are the design team's judgements, written down so they can be argued
// with; the program only does the arithmetic and shows how fragile the result is.
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Criterion {
    std::string name;
    double weight = 0;
    std::vector<int> score;
};

int winner(const std::vector<std::string>& options, const std::set<size_t>& out,
           const std::vector<Criterion>& crit, std::vector<double>* totals)
{
    int best = -1;
    double best_total = -1;
    for (size_t o = 0; o < options.size(); ++o) {
        double total = 0;
        for (const Criterion& c : crit) {
            total += c.weight * c.score[o];
        }
        if (totals) {
            totals->push_back(total);
        }
        if (!out.count(o) && total > best_total) {
            best_total = total;
            best = static_cast<int>(o);
        }
    }
    return best;
}

int main()
{
    std::vector<std::string> options;
    std::vector<Criterion> crit;
    std::set<size_t> out;
    std::map<size_t, std::string> why_out;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "option") {
            std::string n;
            in >> n;
            options.push_back(n);
        } else if (kind == "must") {
            std::string req, opt, ok;
            in >> req >> opt >> ok;
            for (size_t o = 0; o < options.size(); ++o) {
                if (options[o] == opt && ok == "no") {
                    out.insert(o);
                    why_out[o] += " " + req;
                }
            }
        } else if (kind == "criterion") {
            Criterion c;
            in >> c.name >> c.weight;
            int s = 0;
            while (in >> s) {
                c.score.push_back(s);
            }
            if (c.score.size() != options.size()) {
                std::cerr << "criterion " << c.name << ": need one score per option\n";
                return 2;
            }
            crit.push_back(c);
        }
    }

    std::vector<double> totals;
    int best = winner(options, out, crit, &totals);
    std::cout << std::left << std::setw(26) << "criterion (weight)";
    for (const auto& o : options) {
        std::cout << std::setw(14) << o;
    }
    std::cout << '\n';
    for (const Criterion& c : crit) {
        std::ostringstream label;
        label << c.name << " (" << c.weight << ")";
        std::cout << std::setw(26) << label.str();
        for (int s : c.score) {
            std::cout << std::setw(14) << s;
        }
        std::cout << '\n';
    }
    std::cout << std::setw(26) << "weighted total";
    for (double t : totals) {
        std::cout << std::setw(14) << t;
    }
    std::cout << '\n';
    for (const auto& [o, why] : why_out) {
        std::cout << "excluded by a hard requirement: " << options[o] << " (fails:" << why << ")\n";
    }
    if (best < 0) {
        std::cout << "no option meets the hard requirements\n";
        return 1;
    }
    std::cout << "choice: " << options[static_cast<size_t>(best)] << "\n";

    // sensitivity: halve or double each weight in turn; does the choice change?
    int flips = 0;
    for (size_t i = 0; i < crit.size(); ++i) {
        for (double f : {0.5, 2.0}) {
            std::vector<Criterion> c2 = crit;
            c2[i].weight *= f;
            int w = winner(options, out, c2, nullptr);
            if (w != best) {
                ++flips;
                std::cout << "  sensitive: weight of " << crit[i].name << " x" << f << " -> "
                          << options[static_cast<size_t>(w)] << '\n';
            }
        }
    }
    std::cout << "weight changes that flip the choice: " << flips << " of " << 2 * crit.size() << '\n';
    return 0;
}
