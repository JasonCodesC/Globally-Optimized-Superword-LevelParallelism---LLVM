// SLP_MINIMAL_INT.C
void test_minimal_int(int *a, int *b, int *c) {
    // === Pack #0 === (producer)
    int t0 = a[0] + b[0];
    int t1 = a[1] + b[1];

    // === Pack #1 === (consumer)
    // Use-def chain in swapped order → requires lane permutation
    int u0 = t1 * 2;
    int u1 = t0 * 2;

    // === Final explicit swapped output ===
    c[0] = u1;
    c[1] = u0;
}