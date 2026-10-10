// vtable.cpp: a class with virtual functions, to read its vtable in the object file.
struct Sensor {
    virtual ~Sensor() = default;
    virtual int read() const = 0;
    virtual const char* unit() const { return "raw"; }
};

struct Thermometer : Sensor {
    int read() const override { return 21; }
    const char* unit() const override { return "C"; }
};

Sensor* makeThermometer()
{
    return new Thermometer;
}
