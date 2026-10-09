// counter.cpp: the translation unit that DEFINES the order counter and the id function.
int g_orders = 100;      // global and initialised: a GLOBAL symbol in .data
static int s_calls;      // file-local and zero: a LOCAL symbol in .bss

int next_order_id()
{
    ++s_calls;
    return ++g_orders;
}

int calls_so_far()
{
    return s_calls;
}
