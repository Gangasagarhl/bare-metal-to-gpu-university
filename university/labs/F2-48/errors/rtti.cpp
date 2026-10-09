// rtti.cpp: run-time type information in code compiled with -fno-rtti.
struct Device {
    virtual ~Device() = default;
};
struct Uart : Device {};

bool isUart(Device* d)
{
    return dynamic_cast<Uart*>(d) != nullptr;
}
