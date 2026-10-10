// dangling.cc - F11-08, demo: a pointer that outlives what it points to.
// A very common real bug: keep a pointer (or reference, or iterator, or
// std::string_view) into a container, then make the container reallocate. The
// old storage is freed; the pointer now points into freed memory. Built with
// -fsanitize=address by run.sh: ASan reports heap-use-after-free and shows both
// the use and the free.
#include <cstdio>
#include <vector>

int main()
{
    std::vector<int> readings;
    readings.push_back(10);
    readings.push_back(20);

    // Grab a pointer to the first reading to "watch" it.
    int* watch = &readings[0];
    std::printf("watch sees %d\n", *watch);

    // The vector grows. When it runs out of capacity it allocates new storage,
    // copies the elements, and frees the old block -- the block `watch` is in.
    for (int i = 0; i < 1000; ++i) readings.push_back(i);

    // Use the stale pointer: it points into memory the vector has freed.
    std::printf("watch now sees %d\n", *watch);   // heap-use-after-free
    return 0;
}
