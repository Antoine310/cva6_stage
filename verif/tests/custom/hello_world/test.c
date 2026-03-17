
volatile int array[1] = {1};

int main() {

    int a, b;
    int t;

    asm volatile(
        "la t0, array\n"
        //"lw %0, 0(t0)\n"
        //"lw %1, 0(t0)\n"
        "csrr %2, cycle\n"
        : "=r"(a), "=r"(b), "=r"(t)
        :
        : "t0", "memory"
    );

    return 0;
}