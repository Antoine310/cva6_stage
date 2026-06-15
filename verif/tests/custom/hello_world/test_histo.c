#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rlibsc.h" 

#define HISTOGRAM_ENTRIES 100000
#define HISTOGRAM_SCALE 10
#define MEASUREMENTS 100


#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324
#define CSR_MHPMEVENT5   0x325
#define CSR_HPMCOUNTER5  0xC05
#define CSR_MHPMEVENT6  0x326
#define CSR_HPMCOUNTER6 0xC06

#define read_csr(csr) ({ \
    unsigned long __tmp; \
    asm volatile("csrr %0, " stringify(csr) : "=r"(__tmp)); \
    __tmp; \
})

#define stringify(x) #x
#define csr_write(csr, val) \
    asm volatile("csrw " stringify(csr) ", %0" :: "rK"(val))


//char __attribute__((aligned(4096))) buffer[64 * 1024];
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

static inline void set_miss_counter(void)
{
    csr_write(CSR_MHPMEVENT6, 2);
}

#define PRIME_N 36
#define PRIME_STRIDE 4096
char __attribute__((aligned(4096))) buffer[(PRIME_N + 2) * PRIME_STRIDE];

static inline void prime(void *addr)
{
    size_t pset = (((size_t)addr) >> 4) & 0xFF;

    for (int k = 1; k <= PRIME_N; k++) {
        maccess(buffer + k * PRIME_STRIDE + (pset << 4));
    }

    asm volatile("fence");
}
/* 
static inline void prime(void *addr)
{
    size_t pset = (((size_t)addr) >> 4) & 0xFF;

    int pn = ((size_t)addr) & 4096;
    int bufpn = ((size_t)buffer) & 4096;

    int i = 1;
    int j = (pn == bufpn) ? 0 : 1;

    REP36(
        maccess(
            buffer +
            i++ * 2 * 4096 +
            j * 4096 +
            (pset << 4)
        );
    )

    asm volatile("fence");
}*/
/*
static inline void prime(void *addr)
{
    size_t pset = (((size_t)addr) >> 4) & 0xFF;

    int pn = ((size_t)addr) & 4096;
    int bufpn = ((size_t)buffer) & 4096;

    int i = 1;
    int j = (pn == bufpn) ? 0 : 1;

    printf("victim=%lx set=%lu\n",
           (unsigned long)addr,
           (unsigned long)(((size_t)addr >> 4) & 0xFF));

    for(int k=1;k<=8;k++)
    {
        uintptr_t a =
            (uintptr_t)(buffer +
                        k * 2 * 4096 +
                        j * 4096 +
                        (pset << 4));

        printf("prime[%d]=%lx set=%lu\n",
               k,
               (unsigned long)a,
               (unsigned long)((a >> 4) & 0xFF));
    }

    REP8(
        maccess(
            buffer +
            i++ * 2 * 4096 +
            j * 4096 +
            (pset << 4)
        );
    )

    asm volatile("fence");
}*/
static inline size_t empty_time(void) {
  uint64_t x = rdcycle();
  uint64_t y = rdcycle();
  return y - x;
}

static inline size_t one_load_time(void *addr) {
  uint64_t x = rdcycle();
  maccess(addr);
  uint64_t y = rdcycle();
  return y - x;
}

size_t measure_access_time(void *address) {
  uint64_t x = rdcycle();
  prime(address);
  uint64_t y = rdcycle();
  return y - x;
}


uint64_t max_hit = 0;
uint64_t max_miss = 0;

void measure_hits(void *address, size_t *histogram, size_t number_of_measurements) {
  

  for (size_t i = 0; i < number_of_measurements; i++) {
    size_t hit = measure_access_time(address);
    if (hit > max_hit) max_hit = hit;
    if (hit < HISTOGRAM_ENTRIES) histogram[hit]++;
  }
  


}

void measure_misses(void *address, size_t *histogram, size_t number_of_measurements) {
  

  for (size_t i = 0; i < number_of_measurements; i++) {
    maccess(address);
    size_t miss = measure_access_time(address);
    if (miss > max_miss) max_miss = miss;
    if (miss < HISTOGRAM_ENTRIES) histogram[miss]++;
  }

}

/* 
void measure_hits(void *address, size_t *histogram,
                  size_t number_of_measurements) {
  for (size_t i = 0; i < number_of_measurements; i++) {
    size_t hit = measure_access_time(address);
    if (hit < HISTOGRAM_ENTRIES)
      histogram[hit]++;
  }
}

void measure_misses(void *address, size_t *histogram,
                    size_t number_of_measurements) {
  for (size_t i = 0; i < number_of_measurements; i++) {
    maccess(address);
    size_t miss = measure_access_time(address);
    if (miss < HISTOGRAM_ENTRIES)
      histogram[miss]++;
  }
}*/

static inline size_t probe_time(void *addr)
{
    uint64_t x = rdcycle();
    maccess(addr);
    uint64_t y = rdcycle();
    return y - x;
}
static inline void set_secure_flag(void)
{
    csr_write(CSR_MHPMEVENT3, (1u << 23));
}


size_t hit_histogram[HISTOGRAM_ENTRIES], miss_histogram[HISTOGRAM_ENTRIES];
char __attribute__((aligned(4096))) address[4096];

int main(int argc, char *argv[]) {
  
  set_secure_flag();

  set_miss_counter();

  memset(address, 1, 4096);
  memset(buffer, 2, sizeof(buffer));

  memset(hit_histogram, 0, sizeof(hit_histogram));
  memset(miss_histogram, 0, sizeof(miss_histogram));
  /*
    
  int evicted = 0;

  for (int i = 0; i < 100; i++) {
      maccess(address);

      uint64_t m0 = read_csr(CSR_HPMCOUNTER6);
      prime(address);
      uint64_t m1 = read_csr(CSR_HPMCOUNTER6);
      maccess(address);
      uint64_t m2 = read_csr(CSR_HPMCOUNTER6);

      if ((m2 - m1) > 0)
          evicted++;
  }
  printf("evicted=%d/40\n", evicted);

    
  uint64_t m0 = read_csr(CSR_HPMCOUNTER6);

  maccess(address); // miss    
  
  asm volatile("fence");

  uint64_t r1 = probe_time(address) ;
    
  uint64_t m1 = read_csr(CSR_HPMCOUNTER6);
  
  prime(address);           // éviction

  uint64_t m2 = read_csr(CSR_HPMCOUNTER6);

  uint64_t r2 = probe_time(address) ;

  uint64_t m3 = read_csr(CSR_HPMCOUNTER6);
 

  printf("hit test=%lu\n", r1); // hit 
  printf("miss=%lu\n", r2);  

  printf("verif 1 nb miss (=1 normalement)=%lu\n", m1-m0); // hit 
  printf("verif 2 nb miss (=36 normalement)=%lu\n", m2-m1); // hit 
  printf("verif 3 nb miss (=0 normalement)=%lu\n", m3-m2); // hit 
  */

    max_hit = 0;
    max_miss = 0;
    measure_hits(address, hit_histogram, MEASUREMENTS);
    measure_misses(address, miss_histogram, MEASUREMENTS);

    print_str("max_hit = ");
    print_dec(max_hit);
    print_str("max_miss = ");
    print_dec(max_miss);

  for (size_t i = 0; i < HISTOGRAM_ENTRIES; i += HISTOGRAM_SCALE) {
    size_t hit = 0, miss = 0;
    for (size_t scale = 0; scale < HISTOGRAM_SCALE; scale++) {
      hit += hit_histogram[i + scale];
      miss += miss_histogram[i + scale];
    }
        if (hit || miss) {
            print_str("Sortie = ");
            print_dec(i);
            print_dec(hit);
            print_dec(miss);
        }
    }

  return 0;
}