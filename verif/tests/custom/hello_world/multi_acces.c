#include <stdint.h>

#define N 40
#define STRIDE 4096

volatile uint8_t array[N * STRIDE];
volatile uint64_t sink = 0;

static inline void fence_rw(void)
{
    asm volatile("fence rw, rw" ::: "memory");
}

int main(void)
{
    fence_rw();

    for (int i = 0; i < N; i++) {
        sink += array[i * STRIDE];
    }

    fence_rw();

    asm volatile("" :: "r"(sink) : "memory");
    return 0;
}