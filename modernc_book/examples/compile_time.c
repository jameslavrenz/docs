#include <limits.h>
#include <stdio.h>
#include <string.h>
#define STRINGIZE_RAW(x) #x
#define STRINGIZE(x) STRINGIZE_RAW(x)
#define JOIN_RAW(a, b) a ## b
#define JOIN(a, b) JOIN_RAW(a, b)
#define VERSION_MAJOR 2
static_assert(CHAR_BIT >= 8, "C byte must contain at least eight bits");
struct point { int x; int y; };
static int JOIN(sum, _point)(struct point p) { return p.x + p.y; }
int main(void)
{
    struct point origin = {};
    struct point position = {.x = 2, .y = 3};
    if (origin.x != 0 || origin.y != 0 || sum_point(position) != 5)
        return 1;
    if (strcmp(STRINGIZE(VERSION_MAJOR), "2") != 0) return 1;
    puts("compile_time: OK");
    return 0;
}
