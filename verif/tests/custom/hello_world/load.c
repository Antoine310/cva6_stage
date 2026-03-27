
volatile int array[4096];

int main() {

    int a, b;
    int t;

    asm volatile(
        "la t0, array\n"

        "lw x10, 0(t0)\n"
        "add x12, x10, x11\n"
        "add x12, x10, x11\n"

        "csrr x12, cycle\n"

        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "lw x12, 0(t0)\n"
        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "lw x12, 0(t0)\n"
        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "lw x12, 0(t0)\n"
        "lw x10, 0(t0)\n"

        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "lw x12, 0(t0)\n"
        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "lw x12, 0(t0)\n"
        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "lw x12, 0(t0)\n"
        "lw x10, 0(t0)\n"

        "csrr x12, cycle\n"
        :
        :
        : "t0", "x10", "x11", "x12", "memory"
    );
    return 0;
} 