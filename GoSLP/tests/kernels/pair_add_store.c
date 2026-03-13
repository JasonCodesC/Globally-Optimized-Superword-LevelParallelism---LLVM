#include <stdint.h>

__attribute__((noinline))
void foo_add2(const int *a, const int *b, int *c) {
  int x0 = a[0] + b[0];
  int x1 = a[1] + b[1];
  c[0] = x0;
  c[1] = x1;
}

static void ref_add2(const int *a, const int *b, int *c) {
  c[0] = a[0] + b[0];
  c[1] = a[1] + b[1];
}

int main(void) {
  int a[2] = {7, -3};
  int b[2] = {5, 11};
  int out1[2] = {0, 0};
  int out2[2] = {0, 0};

  foo_add2(a, b, out1);
  ref_add2(a, b, out2);

  return (out1[0] != out2[0]) || (out1[1] != out2[1]);
}
