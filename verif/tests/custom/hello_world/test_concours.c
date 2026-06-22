#include <stdio.h>
#include <stdint.h>

static inline uint64_t rdcycle(void)
{
    uint64_t c;
    asm volatile ("rdcycle %0" : "=r"(c));
    return c;
}

int main(void)
{
    volatile int x = 0;

    uint64_t t0, t1;
    uint64_t loop1_cycles, loop2_cycles, loop3_cycles, loop4_cycles;
    uint64_t total_cycles;
    uint64_t cycle;

    t0 = rdcycle();

    // Boucle 1 : 255 itérations, 8 addi
    uint64_t start = rdcycle();
    for (int i = 0; i < 10; i++) {
        x++; x++; x++; x++;
        x++; x++; x++; x++;

    }
    uint64_t end = rdcycle();
    loop1_cycles = end - start;

    // Boucle 2 : 64 itérations, 7 addi
    start = rdcycle();
    for (int i = 0; i < 64; i++) {
        x++; x++; x++; x++;
        x++; x++; x++;
    }
    end = rdcycle();
    loop2_cycles = end - start;

    // Boucle 3 : 16 itérations, 42 addi
    start = rdcycle();
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 42; j++) {
            x++;
        }
    }
    end = rdcycle();
    loop3_cycles = end - start;

    // Boucle 4 : 3 itérations, 160 addi
    start = rdcycle();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 160; j++) {
            x++;
        }
    }
    end = rdcycle();
    loop4_cycles = end - start;

    t1 = rdcycle();
    total_cycles = t1 - t0;
    printf("Test    : %llu cycles\n", (unsigned long long)cycle);

    printf("Loop1 (255 x 8 addi)   : %llu cycles\n", (unsigned long long)loop1_cycles);
    printf("Loop2 (64 x 7 addi)    : %llu cycles\n", (unsigned long long)loop2_cycles);
    printf("Loop3 (16 x 42 addi)   : %llu cycles\n", (unsigned long long)loop3_cycles);
    printf("Loop4 (3 x 160 addi)   : %llu cycles\n", (unsigned long long)loop4_cycles);
    printf("Total programme        : %llu cycles\n", (unsigned long long)total_cycles);
    printf("x = %d\n", x);

    return 0;
}