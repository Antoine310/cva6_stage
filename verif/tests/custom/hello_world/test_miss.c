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
        "lw x10, 0(t0)\n"          // warmup : array en cache

        // pollution plus forte
        "la t0, trash\n"
        "lw t1,   0(t0)\n"
        "lw t1,  64(t0)\n"
        "lw t1, 128(t0)\n"
        "lw t1, 192(t0)\n"
        "lw t1, 256(t0)\n"
        "lw t1, 320(t0)\n"
        "lw t1, 384(t0)\n"
        "lw t1, 448(t0)\n"

        "csrr %0, cycle\n"

        "la t0, array\n"
        "lw x10, 0(t0)\n"          // accès à mesurer

        "csrr %1, cycle\n"
        : "=r"(start), "=r"(end)
        :
        : "t0", "t1", "x10", "memory"
    );

    print_hex(end - start);

    while (1);
}