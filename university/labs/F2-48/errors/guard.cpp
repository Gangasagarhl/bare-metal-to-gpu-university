// guard.cpp: a function-local static whose constructor runs on first use.
int readSensor();

struct Calibration {
    int offset;
    Calibration() : offset(readSensor()) {}
};

int calibrated(int raw)
{
    static Calibration c;   // initialised the first time this line is reached
    return raw - c.offset;
}
