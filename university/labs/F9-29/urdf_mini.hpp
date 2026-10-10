// F9-29 Listing 2: a deliberately tiny reader for the subset of URDF used in this course
// (robot, link, inertial/mass, joint, parent, child, origin, axis, limit), plus checks
// of the tree and forward kinematics. Not a general XML parser: no entities, no CDATA.
#pragma once
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Element {
    std::string tag;
    std::map<std::string, std::string> attr;
    std::vector<std::unique_ptr<Element>> kids;
    const Element* child(const std::string& t) const
    {
        for (const auto& k : kids)
            if (k->tag == t) return k.get();
        return nullptr;
    }
};

// parse "<tag a="1" b="2"> ... </tag>", "<tag/>", comments and the <?xml ...?> line
inline std::unique_ptr<Element> parseXml(const std::string& s)
{
    auto root = std::make_unique<Element>();
    std::vector<Element*> stack{root.get()};
    std::size_t i = 0;
    while ((i = s.find('<', i)) != std::string::npos) {
        if (s.compare(i, 4, "<!--") == 0) {
            i = s.find("-->", i) + 3;
            continue;
        }
        if (s.compare(i, 2, "<?") == 0) {
            i = s.find("?>", i) + 2;
            continue;
        }
        const std::size_t end = s.find('>', i);
        if (end == std::string::npos) throw std::runtime_error("unclosed tag");
        std::string body = s.substr(i + 1, end - i - 1);
        i = end + 1;
        if (body[0] == '/') { // closing tag
            if (stack.size() < 2 || stack.back()->tag != body.substr(1))
                throw std::runtime_error("bad </" + body.substr(1) + ">");
            stack.pop_back();
            continue;
        }
        const bool selfClosing = body.back() == '/';
        if (selfClosing) body.pop_back();
        auto e = std::make_unique<Element>();
        std::istringstream in(body);
        in >> e->tag;
        std::string rest((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        static const std::regex attrRe(
            R"re(([A-Za-z_][\w:.-]*)\s*=\s*"([^"]*)")re"); // name="value"
        for (std::sregex_iterator it(rest.begin(), rest.end(), attrRe), stop; it != stop; ++it)
            e->attr[(*it)[1]] = (*it)[2];
        Element* raw = e.get();
        stack.back()->kids.push_back(std::move(e));
        if (!selfClosing) stack.push_back(raw);
    }
    if (stack.size() != 1) throw std::runtime_error("unclosed element <" + stack.back()->tag + ">");
    return root;
}

using V3 = std::array<double, 3>;
using M3 = std::array<V3, 3>;
struct Tf {
    M3 R{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    V3 t{0, 0, 0};
};
inline Tf operator*(const Tf& a, const Tf& b)
{
    Tf c;
    for (int i = 0; i < 3; ++i) {
        c.t[i] = a.t[i];
        for (int j = 0; j < 3; ++j) {
            c.R[i][j] = a.R[i][0] * b.R[0][j] + a.R[i][1] * b.R[1][j] + a.R[i][2] * b.R[2][j];
            c.t[i] += a.R[i][j] * b.t[j];
        }
    }
    return c;
}
inline M3 rotAxis(const V3& u, double q) // Rodrigues
{
    const double c = std::cos(q), s = std::sin(q), v = 1 - c;
    return {{{c + u[0] * u[0] * v, u[0] * u[1] * v - u[2] * s, u[0] * u[2] * v + u[1] * s},
             {u[1] * u[0] * v + u[2] * s, c + u[1] * u[1] * v, u[1] * u[2] * v - u[0] * s},
             {u[2] * u[0] * v - u[1] * s, u[2] * u[1] * v + u[0] * s, c + u[2] * u[2] * v}}};
}
inline V3 parse3(const std::string& s)
{
    std::istringstream in(s);
    V3 v{};
    if (!(in >> v[0] >> v[1] >> v[2]))
        throw std::runtime_error("expected three numbers: \"" + s + "\"");
    return v;
}

struct JointModel {
    std::string name, type, parent, child;
    Tf origin; // parent link frame <- joint frame (at zero motion)
    V3 rpy{}, axis{1, 0, 0};
    double lower = 0, upper = 0;
};

struct Model {
    std::string name, root;
    std::map<std::string, double> mass;        // link -> mass (0 if none given)
    std::map<std::string, JointModel> byChild; // child link -> the joint that carries it

    static Model load(const std::string& file)
    {
        std::ifstream f(file);
        if (!f) throw std::runtime_error("cannot open " + file);
        std::stringstream ss;
        ss << f.rdbuf();
        const auto doc = parseXml(ss.str());
        const Element* robot = doc->child("robot");
        if (!robot) throw std::runtime_error("no <robot> element");
        Model m;
        m.name = robot->attr.at("name");
        for (const auto& k : robot->kids) {
            if (k->tag == "link") {
                const Element* in = k->child("inertial");
                m.mass[k->attr.at("name")] =
                    in && in->child("mass") ? std::stod(in->child("mass")->attr.at("value")) : 0.0;
            } else if (k->tag == "joint") {
                JointModel j;
                j.name = k->attr.at("name");
                j.type = k->attr.at("type");
                j.parent = k->child("parent")->attr.at("link");
                j.child = k->child("child")->attr.at("link");
                if (const Element* o = k->child("origin")) {
                    j.origin.t = parse3(o->attr.at("xyz"));
                    j.rpy = parse3(
                        o->attr.at("rpy")); // radians: roll about x, pitch about y, yaw about z
                    j.origin.R = (Tf{rotAxis({0, 0, 1}, j.rpy[2]), {}} *
                                  Tf{rotAxis({0, 1, 0}, j.rpy[1]), {}} *
                                  Tf{rotAxis({1, 0, 0}, j.rpy[0]), {}})
                                     .R;
                }
                if (const Element* a = k->child("axis")) j.axis = parse3(a->attr.at("xyz"));
                if (const Element* l = k->child("limit")) {
                    j.lower = std::stod(l->attr.at("lower"));
                    j.upper = std::stod(l->attr.at("upper"));
                }
                if (m.byChild.count(j.child))
                    throw std::runtime_error("link " + j.child + " has two parents");
                m.byChild[j.child] = j;
            }
        }
        std::vector<std::string> roots; // checks: links exist, exactly one root
        for (const auto& [child, j] : m.byChild)
            if (!m.mass.count(j.parent) || !m.mass.count(child))
                throw std::runtime_error("joint " + j.name + " names an unknown link");
        for (const auto& [link, mass] : m.mass)
            if (!m.byChild.count(link)) roots.push_back(link);
        if (roots.size() != 1)
            throw std::runtime_error("expected exactly one root link, found " +
                                     std::to_string(roots.size()));
        m.root = roots[0];
        return m;
    }

    // T_root_link for the joint values q (radians, by joint name; missing = 0)
    Tf pose(const std::string& link, const std::map<std::string, double>& q) const
    {
        if (link == root) return {};
        const JointModel& j = byChild.at(link);
        Tf T = pose(j.parent, q) * j.origin;
        if (j.type == "revolute" || j.type == "continuous") {
            const auto it = q.find(j.name);
            T = T * Tf{rotAxis(j.axis, it == q.end() ? 0.0 : it->second), {}};
        }
        return T;
    }
};
