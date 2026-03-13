#include <stdint.h>

__attribute__((noinline))
void foo_mismatch2(const int *a, const int *b, int *c) {
  int x0 = a[0] + b[0];
  int x1 = a[1] - b[1];
  c[0] = x0;
  c[1] = x1;
}

static void ref_mismatch2(const int *a, const int *b, int *c) {
  c[0] = a[0] + b[0];
  c[1] = a[1] - b[1];
}

int main(void) {
  int a[2] = {19, 6};
  int b[2] = {8, 3};
  int out1[2] = {0, 0};
  int out2[2] = {0, 0};

  foo_mismatch2(a, b, out1);
  ref_mismatch2(a, b, out2);

  return (out1[0] != out2[0]) || (out1[1] != out2[1]);
}
