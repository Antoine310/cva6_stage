#include <stdint.h>
#include <stdio.h>

#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324
#define CSR_MHPMEVENT5   0x325
#define CSR_HPMCOUNTER5  0xC05

#define stringify(x) #x
#define csr_write(csr, val) \
    asm volatile("csrw " stringify(csr) ", %0" :: "rK"(val))

#define read_csr(csr) ({ \
    unsigned long __tmp; \
    asm volatile("csrr %0, " stringify(csr) : "=r"(__tmp)); \
    __tmp; \
})

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

static inline void fence_rw(void) { asm volatile("fence rw, rw" ::: "memory"); }
static inline void keep_u32(uint32_t x) { asm volatile("" :: "r"(x) : "memory"); }

int main(void)
{
    set_enclave_id(1);
    set_secure_flag();

static volatile uint32_t line __attribute__((aligned(64))) = 0x12345678;    volatile uint32_t sink = 0;

    const uint32_t N = 100;

    sink += line;
    fence_rw();

    set_lecture_hit();

    uint64_t t0 = rdcycle();
    for (uint32_t i = 0; i < N; i++) {
        sink += line;
    }
    uint64_t h1 = lecture_hit();
    uint64_t t1 = rdcycle();
    
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

    printf(" Hit event %llu\n", h1 );

    printf("\nHit 2 : total=%llu cycles, per_load=%llu + %llu/%u cycles, sink=%u (0x%08x)\n",
           (unsigned long long)cycles,
           (unsigned long long)(cycles / N),
           (unsigned long long)(cycles % N),
           (unsigned)N,
           (unsigned)sink, (unsigned)sink);

    printf("\nHit 2 : total=%llu cycles, per_load=%llu + %llu/%u cycles, sink=%u (0x%08x)\n",
           (unsigned long long)cycles2,
           (unsigned long long)(cycles2 / N),
           (unsigned long long)(cycles2 % N),
           (unsigned)N,
           (unsigned)sink, (unsigned)sink);       

    return 0;
}