#include <stdint.h>

volatile int array[1] __attribute__((aligned(64))) = {1};
volatile int trash[256] __attribute__((aligned(64)));

void print_hex(uint64_t val) {
    volatile char *uart = (char *)0x10000000;
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (val >> i) & 0xF;
        uart[0] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        for (volatile int j = 0; j < 1000; j++);
    }
    uart[0] = '\n';
}

int main() {
    uint64_t start, end;

    asm volatile(
        "la t0, array\n"

        // warm cache
        "lw t1, 0(t0)\n"

        // petite barrière logique
        "add t1, t1, x0\n"

        "csrr %0, cycle\n"

        // accès mesuré (registre différent)
        "lw t2, 0(t0)\n"
        "add t2, t2, x0\n"

        "fence\n"
        
        "csrr %1, cycle\n"

        : "=r"(start), "=r"(end)
        :
        : "t0", "t1", "t2", "memory"
    );

    print_hex(end - start);

    while (1);
}