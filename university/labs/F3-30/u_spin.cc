// u_spin.cc - loops forever in ring 3 until the kernel kills it.
int main()
{
    for (;;) {
        asm volatile("pause");
    }
}
