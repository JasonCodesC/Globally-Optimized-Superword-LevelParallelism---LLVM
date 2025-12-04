// SLP_MINIMAL_INT.C

#include <stdio.h>

void test_minimal_int(int *a, int *b, int *c) {
    // === Pack #0 === (producer)
    int t0 = a[0] * b[0];
    int t1 = a[1] * b[1];

    c[0] = t0;
    c[1] = t1;

    printf("z[0] = %d, z[1] = %d\n", c[0], c[1]);

    // === Pack #1 === (consumer)
    // Use-def chain in swapped order → requires lane permutation
    int u0 = t0 * 2;
    int u1 = t1 * 2;

    // // === Final explicit swapped output ===
    c[0] = u0;
    c[1] = u1;
}

int main() {
    int x[2] = {1, 2};
    int y[2] = {3, 4};
    int z[2];

    // Call the function
    test_minimal_int(x, y, z);

    // Print the results
    printf("z[0] = %d, z[1] = %d\n", z[0], z[1]);

    return 0;
}
