// bmc_sim.cpp - F3-42 Listing 3: a model of a server's Baseboard Management Controller
// (BMC). The BMC has its own processor and firmware and runs on standby power, so it answers
// requests while the host is off. Requests are IPMI-style messages: network function (netfn),
// command, data; responses carry a completion code. NetFn and command numbers marked "header"
// come from the Linux UAPI header <linux/ipmi_msgdefs.h> installed in the build container;
// the ones marked "UNVERIFIED" are recalled from the IPMI v2.0 specification (see the chapter).
#include <linux/ipmi_msgdefs.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr uint8_t kNetFnChassis = 0x00;          // UNVERIFIED
constexpr uint8_t kCmdChassisControl = 0x02;     // UNVERIFIED (data 0x01 = power up)
constexpr uint8_t kCmdGetSensorReading = 0x2D;   // UNVERIFIED (netfn sensor/event)
constexpr uint8_t kCcSensorNotPresent = 0xCB;    // UNVERIFIED completion code

using Bytes = std::vector<uint8_t>;

struct Response {
    uint8_t netfn;
    uint8_t cmd;
    uint8_t completion;
    Bytes data;
};

class Bmc {
public:
    bool hostPowered = false;
    int cpuTempC = 30;
    int fanRpm = 1200;

    // One control-loop pass of the BMC firmware: read sensors, set the fan.
    void step()
    {
        cpuTempC += hostPowered ? 3 : -1;
        if (cpuTempC < 30) { cpuTempC = 30; }
        fanRpm = cpuTempC < 50 ? 1200 : 1200 + (cpuTempC - 50) * 150;   // simple fan curve
    }

    Response handle(uint8_t netfn, uint8_t cmd, const Bytes& req)
    {
        Response r{static_cast<uint8_t>(netfn + 1), cmd, IPMI_CC_NO_ERROR, {}};   // response netfn = request + 1
        if (netfn == IPMI_NETFN_APP_REQUEST && cmd == IPMI_GET_DEVICE_ID_CMD) {
            r.data = {0x20, 0x01, 0x03, 0x05};   // model's device id, revision, firmware 3.05
        } else if (netfn == kNetFnChassis && cmd == kCmdChassisControl && !req.empty()) {
            hostPowered = req[0] == 0x01;
        } else if (netfn == IPMI_NETFN_SENSOR_EVENT_REQUEST && cmd == kCmdGetSensorReading && !req.empty()) {
            if (req[0] == 1) { r.data = {static_cast<uint8_t>(cpuTempC)}; }
            else if (req[0] == 2) { r.data = {static_cast<uint8_t>(fanRpm / 100)}; }   // x100 rpm
            else { r.completion = kCcSensorNotPresent; }
        } else {
            r.completion = IPMI_INVALID_COMMAND_ERR;
        }
        return r;
    }
};

void show(const char* via, uint8_t netfn, uint8_t cmd, const Bytes& req, const Response& r, const char* what)
{
    std::printf("%-5s request  netfn 0x%02x cmd 0x%02x data [", via, netfn, cmd);
    for (uint8_t b : req) { std::printf(" %02x", b); }
    std::printf(" ]   %s\n      response netfn 0x%02x cmd 0x%02x cc 0x%02x data [", what, r.netfn, r.cmd, r.completion);
    for (uint8_t b : r.data) { std::printf(" %02x", b); }
    std::printf(" ]\n");
}

}  // namespace

int main()
{
    Bmc bmc;
    std::printf("host power: off; the BMC runs on standby power\n");
    Bytes none;
    auto r = bmc.handle(IPMI_NETFN_APP_REQUEST, IPMI_GET_DEVICE_ID_CMD, none);
    show("LAN", IPMI_NETFN_APP_REQUEST, IPMI_GET_DEVICE_ID_CMD, none, r, "Get Device ID, out of band");
    const bool idOk = r.completion == IPMI_CC_NO_ERROR && r.data.size() == 4;

    const Bytes powerUp = {0x01};
    r = bmc.handle(kNetFnChassis, kCmdChassisControl, powerUp);
    show("LAN", kNetFnChassis, kCmdChassisControl, powerUp, r, "Chassis Control: power up");
    for (int i = 0; i < 10; ++i) { bmc.step(); }

    const Bytes cpu = {1}, fan = {2}, bogus = {9};
    r = bmc.handle(IPMI_NETFN_SENSOR_EVENT_REQUEST, kCmdGetSensorReading, cpu);
    show("KCS", IPMI_NETFN_SENSOR_EVENT_REQUEST, kCmdGetSensorReading, cpu, r, "CPU temperature, in band from the host OS");
    const int temp = r.data.empty() ? -1 : r.data[0];
    r = bmc.handle(IPMI_NETFN_SENSOR_EVENT_REQUEST, kCmdGetSensorReading, fan);
    show("KCS", IPMI_NETFN_SENSOR_EVENT_REQUEST, kCmdGetSensorReading, fan, r, "fan speed (x100 rpm)");
    r = bmc.handle(IPMI_NETFN_SENSOR_EVENT_REQUEST, kCmdGetSensorReading, bogus);
    show("KCS", IPMI_NETFN_SENSOR_EVENT_REQUEST, kCmdGetSensorReading, bogus, r, "a sensor that does not exist");
    const bool errOk = r.completion == kCcSensorNotPresent;
    r = bmc.handle(IPMI_NETFN_APP_REQUEST, 0x7F, none);
    show("KCS", IPMI_NETFN_APP_REQUEST, 0x7F, none, r, "a command this BMC does not implement");
    const bool unknownOk = r.completion == IPMI_INVALID_COMMAND_ERR;

    std::printf("after 10 control-loop passes with the host on: CPU %d C, fan %d rpm (BMC-controlled)\n",
                bmc.cpuTempC, bmc.fanRpm);
    return idOk && errOk && unknownOk && temp == bmc.cpuTempC ? 0 : 1;
}
