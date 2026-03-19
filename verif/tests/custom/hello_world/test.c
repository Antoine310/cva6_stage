
volatile int array[1] = {1};
volatile int biss[1] = {1};

int main() {

    int a, b;
    int t;

    asm volatile(
        "la t0, array\n"
        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "csrr x12, cycle\n"

        "lw x10, 0(t0)\n"
        "csrr x12, cycle\n"
        "csrr x12, cycle\n"
        "csrr x12, cycle\n"
        "csrr x12, cycle\n"

        "la t0, biss\n"
        "lw x10, 0(t0)\n"
        "lw x11, 0(t0)\n"
        "csrr x12, cycle\n"
        :
        :
        : "t0", "x10", "x11", "x12", "memory"
    );
    return 0;
}
// v2 : 6415 cycles! v1 :  6446 cycles! value=0x1106