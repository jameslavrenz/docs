#include <stdio.h>

struct point { int x, y; };

int main(void)
{
    auto flags = 0b1010'0101u;
    typeof(flags) another = 0b0000'0011u;
    struct point origin = {};
    int *missing = nullptr;
    bool good = (flags & another) == 1u;

    static_assert(sizeof(flags) == sizeof(unsigned));
    if (!good || origin.x != 0 || origin.y != 0 ||
        missing != nullptr)
        return 1;
    puts("c23_basics: OK");
    return 0;
}
