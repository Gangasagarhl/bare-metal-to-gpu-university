// namespace.cpp - DS403 F5-45: Plan 9 style per-process name spaces, as a model.
// Every process has its own mount table: "bind new old" makes `old` show what `new` names;
// with -a or -b the two directories are unioned (new after or before old).
// File servers here are maps from paths to contents; in Plan 9 each would speak 9P.
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace {
using Server = std::map<std::string, std::string>;   // path inside the server -> contents

struct Mount { std::string at; const Server* srv; std::string root; };

class NameSpace {
public:
    // bind: `where` now shows `root` of `srv`; mode 'r' replace, 'a' after, 'b' before.
    void bind(const std::string& where, const Server* srv, const std::string& root, char mode)
    {
        if (mode != 'r' && table_.find(where) == table_.end()) {
            // A union starts from what `where` shows now (for example the local /bin).
            std::string best = cover(where);
            std::vector<Mount> now;
            if (!best.empty())
                for (const Mount& m : table_.at(best)) now.push_back(Mount{where, m.srv, m.root + rest(where, best)});
            table_[where] = now;
        }
        auto& list = table_[where];
        if (mode == 'r') list.clear();
        if (mode == 'b') list.insert(list.begin(), Mount{where, srv, root});
        else list.push_back(Mount{where, srv, root});
    }
    // Resolve a path: the longest mount point that is a prefix wins; union members in order.
    std::string open(const std::string& path) const
    {
        std::string best = cover(path);
        if (best.empty()) return "(no such file)";
        for (const Mount& m : table_.at(best)) {
            auto it = m.srv->find(m.root + rest(path, best));
            if (it != m.srv->end()) return it->second;
        }
        return "(no such file)";
    }

private:
    // The longest mount point that contains `path` ("/" contains everything).
    std::string cover(const std::string& path) const
    {
        std::string best;
        for (const auto& kv : table_) {
            const std::string& at = kv.first;
            bool inside = at == "/" || path == at || path.compare(0, at.size() + 1, at + "/") == 0;
            if (inside && at.size() > best.size()) best = at;
        }
        return best;
    }
    static std::string rest(const std::string& path, const std::string& at)
    {
        return at == "/" ? path : path.substr(at.size());
    }

    std::map<std::string, std::vector<Mount>> table_;
};
}  // namespace

int main()
{
    Server local = {{"/bin/ls", "ls for this terminal"}, {"/dev/cons", "this terminal's console"},
                    {"/net/tcp", "this terminal's TCP stack"}};
    Server fileserver = {{"/bin/cc", "the compiler kept on the file server"},
                         {"/usr/ana/notes", "Ana's notes"}};
    Server gateway = {{"/net/tcp", "the gateway's TCP stack (it reaches the outside network)"}};

    NameSpace shell;                                   // process 1
    shell.bind("/", &local, "", 'r');
    shell.bind("/bin", &fileserver, "/bin", 'a');      // union: local /bin, then the server's
    shell.bind("/usr", &fileserver, "/usr", 'r');
    NameSpace browser = shell;                         // process 2 starts as a copy...
    browser.bind("/net", &gateway, "/net", 'r');       // ...then imports the gateway's /net

    const char* paths[] = {"/bin/ls", "/bin/cc", "/usr/ana/notes", "/dev/cons", "/net/tcp"};
    std::printf("%-16s %-48s %s\n", "path", "process 1 (shell)", "process 2 (browser)");
    for (const char* p : paths)
        std::printf("%-16s %-48s %s\n", p, shell.open(p).c_str(), browser.open(p).c_str());
    return 0;
}
