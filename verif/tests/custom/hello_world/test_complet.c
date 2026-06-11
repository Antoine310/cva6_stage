#include <stdint.h>
#include <stdio.h>

#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324

#define stringify(x) #x
#define csr_write(csr, val) \
    asm volatile("csrw " stringify(csr) ", %0" :: "rK"(val))


    static inline void uart_putc(char c)
{
    volatile char *uart = (char *)0x10000000;
    uart[0] = c;

    for (volatile int j = 0; j < 1000; j++);
}

void print_str(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

void print_hex(uint64_t val)
{
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (val >> i) & 0xF;

        uart_putc(
            (digit < 10)
            ? ('0' + digit)
            : ('A' + digit - 10)
        );
    }

    uart_putc('\n');
}

static inline void set_enclave_id(uint8_t id)
{
    uint32_t v = ((uint32_t)(id & 0xF)) << 23;
    csr_write(CSR_MHPMEVENT4, v);
}

static inline void desac_enclave_id(uint8_t id)
{
    uint32_t v = ((uint32_t)(id & 0xF)) << 23;
    csr_write(CSR_MHPMEVENT4, v);
}

static inline void set_secure_flag(void)
{
    csr_write(CSR_MHPMEVENT3, (1u << 23));
}

static inline void none_secure_flag(void)
{
    csr_write(CSR_MHPMEVENT3, (0u << 23 ));
}

static inline uint64_t rdcycle(void)
{
    uint64_t v;
    asm volatile("rdcycle %0" : "=r"(v));
    return v;
}

static inline void fence_rw(void) { asm volatile("fence rw, rw" ::: "memory"); }
static inline void keep_u32(uint32_t x) { asm volatile("" :: "r"(x) : "memory"); }

int main(void)
{
    static volatile uint32_t line __attribute__((aligned(64))) = 0x12345678;    volatile uint32_t sink = 0;

    set_enclave_id(1);
    set_secure_flag();
    for (volatile int i = 0; i < 10000; i++);

    const uint32_t N = 100;

    sink += line;
    //fence_rw();

    uint64_t t0 = rdcycle();
    for (uint32_t i = 0; i < N; i++) {
        sink += line;
    }
    uint64_t t1 = rdcycle();
    
    //fence_rw();

    desac_enclave_id(0);
    none_secure_flag();

    //keep_u32(sink);
    
    sink += line;
    fence_rw();

    uint64_t t3 = rdcycle();

    for (uint32_t i = 0; i < N; i++) {
        sink += line;
    }

    uint64_t t4 = rdcycle();

    uint64_t cycles = t1 - t0;

    uint64_t cycles2 = t4 - t3 ;


    printf("\n Boucle Hit 1 : total=%llu cycles\n", (unsigned long long)cycles);
    printf("\n Boucle Hit 2 : total=%llu cycles\n", (unsigned long long)cycles2);


    return 0;
} 
