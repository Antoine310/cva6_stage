#include <stdint.h>

#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324

#define stringify(x) #x
#define csr_write(csr, val) \
    asm volatile("csrw " stringify(csr) ", %0" :: "r"(val) : "memory")

static inline void uart_putc(char c)
{
    volatile char *uart = (char *)0x10000000;
    uart[0] = c;
    for (volatile int j = 0; j < 1000; j++);
}

void print_str(const char *s)
{
    while (*s) uart_putc(*s++);
}

void print_hex(uint64_t val)
{
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (val >> i) & 0xF;
        uart_putc((digit < 10) ? ('0' + digit) : ('A' + digit - 10));
    }
    uart_putc('\n');
}

static inline uint64_t rdcycle(void)
{
    uint64_t v;
    asm volatile("rdcycle %0" : "=r"(v));
    return v;
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
static inline void fence_rw(void)
{
    asm volatile("fence rw, rw" ::: "memory");
}

static inline uint64_t asm_bench(volatile uint32_t *addr, uint32_t n)
{
    uint64_t cycles;
    uint32_t acc;

    asm volatile(
        // start cycle
        "rdcycle t1\n"

        // acc = 0
        "li %[acc], 0\n"

        // loop
        "1:\n"
        "lw t0, 0(%[addr])\n"
        "add %[acc], %[acc], t0\n"
        "addi %[n], %[n], -1\n"
        "bnez %[n], 1b\n"

        // end cycle
        "rdcycle t2\n"

        // cycles = t2 - t1
        "sub %[cycles], t2, t1\n"

        : [cycles] "=r"(cycles),
          [acc] "=&r"(acc),
          [n] "+r"(n)

        : [addr] "r"(addr)

        : "t0", "t1", "t2", "memory"
    );

    return cycles;
}
int main(void)
{

    set_enclave_id(1);
    set_secure_flag();

    static volatile uint32_t line __attribute__((aligned(64))) = 0x12345678;

    fence_rw();

    uint64_t cycles1 = asm_bench(&line, 100);


    desac_enclave_id(0);
    none_secure_flag();
    
    uint64_t cycles2 = asm_bench(&line, 100);

    print_str("test cycles1 = ");
    print_hex(cycles1);

    print_str("test cycles2 = ");
    print_hex(cycles2);

    return 0;
}