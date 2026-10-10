// endpoints.cpp - which pods does a service send traffic to? A pod is an endpoint of a
// service when ALL of the service's selector labels match the pod's labels exactly and
// the pod is ready. The university's model of the rule F5-25 describes (DS303).
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using Labels = std::map<std::string, std::string>;

Labels parseLabels(const std::string& text)        // "app=web,tier=frontend"
{
    Labels labels;
    std::istringstream in(text);
    std::string pair;
    while (std::getline(in, pair, ',')) {
        const auto eq = pair.find('=');
        if (eq != std::string::npos) {
            labels[pair.substr(0, eq)] = pair.substr(eq + 1);
        }
    }
    return labels;
}

bool selects(const Labels& selector, const Labels& podLabels)
{
    for (const auto& [key, value] : selector) {
        const auto it = podLabels.find(key);
        if (it == podLabels.end() || it->second != value) {
            return false;
        }
    }
    return true;
}

int main()
{
    struct Pod { std::string name, ip; Labels labels; bool ready; };
    std::vector<Pod> pods;
    std::vector<std::pair<std::string, Labels>> services;
    std::string kind, name, a, b;
    while (std::cin >> kind) {
        if (kind == "pod") {
            std::string ready;
            std::cin >> name >> a >> b >> ready;   // name ip labels ready
            pods.push_back({name, a, parseLabels(b), ready == "true"});
        } else if (kind == "service") {
            std::cin >> name >> a;                 // name selector
            services.emplace_back(name, parseLabels(a));
        }
    }
    std::cout << "pods:\n";
    for (const Pod& p : pods) {
        std::cout << "  " << p.name << "  " << p.ip << "  ready=" << (p.ready ? "true " : "false")
                  << "  labels:";
        for (const auto& [k, v] : p.labels) {
            std::cout << ' ' << k << '=' << v;
        }
        std::cout << '\n';
    }
    for (const auto& [svc, selector] : services) {
        std::cout << "service " << svc << "  selector:";
        for (const auto& [k, v] : selector) {
            std::cout << ' ' << k << '=' << v;
        }
        std::cout << "\n  endpoints:";
        int count = 0;
        for (const Pod& p : pods) {
            if (p.ready && selects(selector, p.labels)) {
                std::cout << ' ' << p.ip;
                ++count;
            }
        }
        std::cout << (count == 0 ? " (none)" : "") << "\n";
    }
    return 0;
}
