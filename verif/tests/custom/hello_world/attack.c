volatile int array[1] = {1};
volatile int biss[1] = {1};

int main() {

    int t1, t2;

    asm volatile(
        "la t0, array\n"
        "lw x10, 0(t0)\n"
        "csrr %0, cycle\n"

        "lw x10, 0(t0)\n"
        "csrr %1, cycle\n"

        : "=r"(t1), "=r"(t2)
        :
        : "t0", "x10", "memory"
    );

    printf("RESULTAT : delta = %d\n", t2 - t1);
    return 0;
}