#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rlibsc.h" 

#define HISTOGRAM_ENTRIES 10000
#define HISTOGRAM_SCALE 10
#define MEASUREMENTS 5


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

static inline void set_miss_counter(void)
{
    csr_write(CSR_MHPMEVENT6, 2);
}

#define PRIME_N 36
#define PRIME_STRIDE 4096
char __attribute__((aligned(4096))) buffer[(PRIME_N  + 2) * PRIME_STRIDE];


static inline void prime_probe(void *addr) // set le cache pour avoir la bonne mesure prime probe 
{
    size_t pset = (((size_t)addr) >> 4) & 0xFF; // Init le set pour le cache 

    for (int k = 1; k <= PRIME_N; k++) {
        maccess(buffer + k * PRIME_STRIDE + (pset << 4)); // Calcul adresse buffet + index dans buffer + bon set 
    }

    asm volatile("fence");
}

static inline void victime(void *addr)
{
    size_t pset = (((size_t)addr) >> 4) & 0xFF; // Init le set pour le cache 

    maccess(buffer + 0 * PRIME_STRIDE + (pset << 4)); // Calcul adresse buffet + index dans buffer + bon set 
    
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
  prime_probe(address);
  uint64_t y = rdcycle();

  return y - x;
}


uint64_t max_ref = 0;
uint64_t cumul_ref = 0;
uint64_t max_probe = 0;
uint64_t cumul_prime = 0;

uint64_t check_miss_ref = 0;
uint64_t check_miss_prime_probe = 0;
uint64_t check_miss_victime = 0;


void measure_prime_ref(void *address, size_t *histogram, size_t number_of_measurements) {

  for (size_t i = 0; i < number_of_measurements; i++) {
    prime_probe(address);
    uint64_t m0 = read_csr(CSR_HPMCOUNTER6);
    size_t prime = measure_access_time(address);
    uint64_t m1 = read_csr(CSR_HPMCOUNTER6);
    check_miss_ref = check_miss_ref + (m1-m0); 
    cumul_ref = cumul_ref + prime;
    if (prime > max_ref) max_ref = prime;
    if (prime < HISTOGRAM_ENTRIES) histogram[prime]++;
  }
}

void measure_prime_probe(void *address, size_t *histogram, size_t number_of_measurements) {

  for (size_t i = 0; i < number_of_measurements; i++) {
    prime_probe(address);
    uint64_t m0 = read_csr(CSR_HPMCOUNTER6);
    victime(address);
    uint64_t m1 = read_csr(CSR_HPMCOUNTER6);
    size_t probe = measure_access_time(address);
    uint64_t m2 = read_csr(CSR_HPMCOUNTER6);
    check_miss_prime_probe = check_miss_prime_probe + (m2-m1); 
    check_miss_victime = check_miss_victime + (m1-m0); 
    cumul_prime = cumul_prime + probe ; 
    if (probe > max_probe) max_probe = probe;
    if (probe < HISTOGRAM_ENTRIES) histogram[probe]++;
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
 
  printf("START\n");

  memset(address, 1, 4096);
  memset(buffer, 2, sizeof(buffer));
  
  measure_prime_ref(address, hit_histogram, MEASUREMENTS);
  measure_prime_probe(address, miss_histogram, MEASUREMENTS);

  printf("max_ref=%lu\n", (unsigned long)max_ref);
  printf("moyenne cumul ref =%lu\n", (unsigned long)cumul_ref/MEASUREMENTS);
  printf("moyenne check_miss_ref =%lu\n", (unsigned long)check_miss_ref/MEASUREMENTS ); // Normalement 0

  printf("max_probe=%lu\n", (unsigned long)max_probe);
  printf("moyenne cumul prime_probe =%lu\n", (unsigned long)cumul_prime/MEASUREMENTS);
  printf("moyenne check_miss_victime =%lu\n", (unsigned long)check_miss_victime/MEASUREMENTS ); // normalement 1 ici  
  printf("moyenne check_miss_prime_probe =%lu\n", (unsigned long)check_miss_prime_probe/MEASUREMENTS );  // nombre de lignes de l'attaquant evinc par la victime


  for (size_t i = 0; i < HISTOGRAM_ENTRIES; i += HISTOGRAM_SCALE) {
    size_t hit = 0, miss = 0;
    for (size_t scale = 0; scale < HISTOGRAM_SCALE; scale++) {
      hit += hit_histogram[i + scale];
      miss += miss_histogram[i + scale];
    }
    if (hit || miss)
      printf("sortie b %lu: %lu %lu\n", (unsigned long)i, (unsigned long)hit, (unsigned long)miss);  
    }
   
  return 0;
}
