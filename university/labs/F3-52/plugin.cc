// plugin.cc - F3-52: a library loaded with dlopen. It has its own thread-local variable.
thread_local int plugin_slot = 0;

extern "C" void plugin_set(int v)
{
    plugin_slot = v;
}

extern "C" int plugin_get()
{
    return plugin_slot;
}
