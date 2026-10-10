// liba.cc - F3-52: the bottom library. libb.so depends on it.
extern "C" int base_value()
{
    return 40;
}

extern "C" const char* who()            // also defined by the executable: interposition test
{
    return "liba.so";
}
