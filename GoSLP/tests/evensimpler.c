// SLP_MINIMAL_INT.C
void test_minimal_int(int *a) {
    // === Pack #0 === (producer)
    // int t0 = a[0];
    // int t1 = a[1];

    // === Pack #1 === (consumer)
    // Use-def chain in swapped order → requires lane permutation
    int u0 = a[0] * 2;
    int u1 = a[1] * 2;
}