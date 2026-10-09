// redfish.cpp - the university's own miniature model of a Redfish-style management service:
// resources are JSON documents at paths, linked to each other, all reachable from one service
// root; a client discovers what exists by following links instead of knowing addresses in
// advance, and changes state by posting an action. NOT a Redfish implementation: no HTTP, no
// TLS, no authentication, and the property names below were written from memory of the DMTF
// Redfish specification and schemas (see the unverified box in F5-29).
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Resource
{
    std::map<std::string, std::string> props;       // property -> value (already JSON text)
    std::vector<std::string> links;                 // paths of linked resources
    std::string link_name = "Members";              // how the links are presented
};

class Service
{
public:
    Service()
    {
        add("/redfish/v1", {{"RedfishVersion", "\"(model)\""}}, {"/redfish/v1/Systems", "/redfish/v1/Chassis"}, "Links");
        add("/redfish/v1/Systems", {{"Name", "\"Computer System Collection\""}}, {"/redfish/v1/Systems/1"});
        add("/redfish/v1/Systems/1", {{"Manufacturer", "\"DS304-Lab\""}, {"SerialNumber", "\"R07-0001\""},
                                      {"PowerState", "\"On\""}}, {"/redfish/v1/Systems/1/Actions/ComputerSystem.Reset"}, "Actions");
        add("/redfish/v1/Chassis", {{"Name", "\"Chassis Collection\""}}, {"/redfish/v1/Chassis/1"});
        add("/redfish/v1/Chassis/1", {{"ChassisType", "\"RackMount\""}}, {"/redfish/v1/Chassis/1/Thermal"}, "Links");
        add("/redfish/v1/Chassis/1/Thermal", {{"CPU1 Temp", "{\"ReadingCelsius\": 48}"}, {"Fan1", "{\"Reading\": 5200}"}}, {});
    }

    // GET: print the resource as JSON; return its links so the client can follow them.
    std::vector<std::string> get(const std::string& path) const
    {
        const auto it = tree_.find(path);
        std::cout << "GET " << path << '\n';
        if (it == tree_.end()) {
            std::cout << "  404 not found\n";
            return {};
        }
        const Resource& r = it->second;
        std::cout << "  { \"@odata.id\": \"" << path << "\"";
        for (const auto& [k, v] : r.props) {
            std::cout << ",\n    \"" << k << "\": " << v;
        }
        if (!r.links.empty()) {
            std::cout << ",\n    \"" << r.link_name << "\": [";
            for (std::size_t i = 0; i < r.links.size(); ++i) {
                std::cout << (i ? ", " : "") << "{\"@odata.id\": \"" << r.links[i] << "\"}";
            }
            std::cout << "]";
        }
        std::cout << " }\n";
        return r.links;
    }

    // POST to an action: the only way state changes in this model.
    void post_reset(const std::string& path, const std::string& reset_type)
    {
        std::cout << "POST " << path << "  {\"ResetType\": \"" << reset_type << "\"}\n";
        Resource& system = tree_.at("/redfish/v1/Systems/1");
        if (reset_type == "ForceOff") {
            system.props["PowerState"] = "\"Off\"";
        } else if (reset_type == "On") {
            system.props["PowerState"] = "\"On\"";
        } else {
            std::cout << "  400 bad request: unknown ResetType\n";
            return;
        }
        log_.push_back("reset " + reset_type + " -> PowerState " + system.props["PowerState"]);
        std::cout << "  204 accepted\n";
    }

    const std::vector<std::string>& log() const { return log_; }

private:
    void add(const std::string& path, std::map<std::string, std::string> props,
             std::vector<std::string> links, std::string link_name = "Members")
    {
        tree_[path] = Resource{std::move(props), std::move(links), std::move(link_name)};
    }

    std::map<std::string, Resource> tree_;
    std::vector<std::string> log_;
};

int main()
{
    Service bmc;
    // 1. discovery: start at the root and follow every link once (breadth first)
    std::vector<std::string> queue{"/redfish/v1"};
    std::map<std::string, bool> seen;
    for (std::size_t i = 0; i < queue.size(); ++i) {
        if (seen.count(queue[i]) != 0 || queue[i].find("/Actions/") != std::string::npos) {
            continue;                               // already read, or an action (not a GET)
        }
        seen[queue[i]] = true;
        for (const std::string& next : bmc.get(queue[i])) {
            queue.push_back(next);
        }
    }
    std::cout << "discovered " << seen.size() << " resources from one known address\n\n";
    // 2. change state through an action, then read the state back
    const std::string action = "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset";
    bmc.post_reset(action, "ForceOff");
    bmc.get("/redfish/v1/Systems/1");
    bmc.post_reset(action, "PowerCycleTwice");
    bmc.post_reset(action, "On");
    std::cout << "\nservice log:\n";
    for (const std::string& line : bmc.log()) {
        std::cout << "  " << line << '\n';
    }
    return 0;
}
