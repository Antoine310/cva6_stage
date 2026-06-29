#include <stdint.h>
#include <stdio.h>

#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324
#define CSR_MHPMEVENT5   0x325
#define CSR_HPMCOUNTER5  0xC05
#define CSR_MHPMEVENT6  0x326
#define CSR_HPMCOUNTER6 0xC06


#define STRIDE 4

#define stringify(x) #x
#define csr_write(csr, val) \
    asm volatile("csrw " stringify(csr) ", %0" :: "rK"(val))

#define read_csr(csr) ({ \
    unsigned long __tmp; \
    asm volatile("csrr %0, " stringify(csr) : "=r"(__tmp)); \
    __tmp; \
})


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
void print_dec(uint64_t val)
{
    char buf[21]; // max uint64_t = 20 chiffres + '\0'
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        uart_putc('\n');
        return;
    }

    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        uart_putc(buf[--i]);
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

static inline void set_lecture_hit(void)
{
    csr_write(CSR_MHPMEVENT5, 28);
}

static inline uint64_t lecture_hit(void)
{
    return read_csr(CSR_HPMCOUNTER5);
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
static inline void set_miss_counter(void)
{
    csr_write(CSR_MHPMEVENT6, 2);
}
static inline void fence_rw(void) { asm volatile("fence rw, rw" ::: "memory"); }
static inline void keep_u32(uint32_t x) { asm volatile("" :: "r"(x) : "memory"); }

int main(void)
{
    set_enclave_id(1);
    set_secure_flag();

static volatile uint32_t line __attribute__((aligned(64))) = 0x12345678;    volatile uint32_t sink = 0;


    static volatile uint32_t array[128 * STRIDE]
        __attribute__((aligned(64)));

    set_miss_counter();
    set_lecture_hit();

    uint64_t miss0 = read_csr(CSR_HPMCOUNTER6);
    uint64_t a0 = rdcycle();


    for (uint32_t i = 0; i < 99; i++) {
        sink += array[i * STRIDE];
    }
    fence_rw();

    uint64_t a1 = rdcycle();
    uint64_t miss1 = read_csr(CSR_HPMCOUNTER6);

    uint64_t h2 = lecture_hit();
    uint64_t miss3 = read_csr(CSR_HPMCOUNTER6);
    uint64_t a2 = rdcycle();

    for (uint32_t i = 0; i < 99; i++) {
        sink += array[i * STRIDE];
    }
    fence_rw();

    uint64_t a3 = rdcycle();
    uint64_t miss4 = read_csr(CSR_HPMCOUNTER6);
    uint64_t h3 = lecture_hit();
    uint64_t a4 = rdcycle();
    uint64_t missBis = read_csr(CSR_HPMCOUNTER6);


    const uint32_t N = 110;

    sink += line;
    fence_rw();
    uint64_t h0 = lecture_hit();
    uint64_t t0 = rdcycle();
    for (uint32_t i = 0; i < N; i++) {
        sink += line;
    }
    uint64_t t1 = rdcycle();
    uint64_t h1 = lecture_hit();

    fence_rw();

    desac_enclave_id(0);
    none_secure_flag();

    keep_u32(sink);
    
    sink += line;
    fence_rw();

    uint64_t t3 = rdcycle();

    for (uint32_t i = 0; i < N; i++) {
        sink += line;
    }

    uint64_t t4 = rdcycle();

    uint64_t cycles = t1 - t0;

    uint64_t cycles2 = t4 - t3 ;


    uint64_t miss_cycles = a1 - a0;
    uint64_t miss_delta  = miss1 - miss0;
    
    uint64_t miss_cycles2 = a3 - a2;
    uint64_t miss_delta2  = miss4 - miss3;


    print_str(" Hit event 1 ");
    print_hex(h1 - h0);
    print_dec(h1 - h0);

    print_str("Hit event 2 ");
    print_hex(h3 - h2 );
    print_dec(h3 - h2 );

    print_str("Boucle Hit 20 : total=");
    print_hex(cycles );
    print_dec(cycles );

    printf("Boucle Hit desactiver :");
    print_hex(cycles2);
    print_dec(cycles2);


    print_str("miss delta 1 ");
    print_hex(miss_delta);
    print_dec(miss_delta);

    print_str(" miss delta 2 ");
    print_hex(miss_delta2);
    print_dec(miss_delta2);

    print_str(" miss loop 1 : total= ");
    print_hex(miss_cycles);
    print_dec(miss_cycles);

    print_str("miss loop 2 : total= ");
    print_hex(miss_cycles2);
    print_dec(miss_cycles2);

    return 0;
}