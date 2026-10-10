// units_typed.cpp - F12-19 Listing 5 (expected to fail to compile): passing pound-force
// seconds where newton seconds are expected is now a compile error, not a wrong number.
struct NewtonSeconds
{
    double value;
};

struct PoundForceSeconds
{
    double value;
};

double velocity_change(NewtonSeconds impulse, double mass_kg)
{
    return impulse.value / mass_kg;
}

int main()
{
    const PoundForceSeconds p{100.0};
    return static_cast<int>(velocity_change(p, 50.0));  // error: no conversion
}
