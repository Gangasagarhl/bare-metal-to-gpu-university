// libb.cc - F3-52: the middle library. Linked against liba.so, so liba.so is its DT_NEEDED.
extern "C" int base_value();
extern "C" const char* who();

extern "C" int answer()
{
    return base_value() + 2;
}

extern "C" const char* who_does_libb_see()
{
    return who();                       // which who()? the dynamic linker decides at run time
}
