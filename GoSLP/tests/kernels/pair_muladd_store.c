#include <stdint.h>

__attribute__((noinline))
void foo_muladd2(const int *a, const int *b, int *c) {
  int x0 = a[0] * 3 + b[0] * 5;
  int x1 = a[1] * 3 + b[1] * 5;
  c[0] = x0;
  c[1] = x1;
}

static void ref_muladd2(const int *a, const int *b, int *c) {
  c[0] = a[0] * 3 + b[0] * 5;
  c[1] = a[1] * 3 + b[1] * 5;
}

int main(void) {
  int a[2] = {4, -9};
  int b[2] = {2, 13};
  int out1[2] = {0, 0};
  int out2[2] = {0, 0};

  foo_muladd2(a, b, out1);
  ref_muladd2(a, b, out2);

  return (out1[0] != out2[0]) || (out1[1] != out2[1]);
}
