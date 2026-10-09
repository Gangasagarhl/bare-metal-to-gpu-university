#include "ring_buffer.h"

int main()
{
    RingBuffer<int, 0> nothing;   // a buffer with no room breaks the invariant
    return static_cast<int>(nothing.size());
}
