#include <stdint.h>

__attribute__((noinline))
void foo_add4(const int *a, const int *b, int *c) {
  int x0 = a[0] + b[0];
  int x1 = a[1] + b[1];
  int x2 = a[2] + b[2];
  int x3 = a[3] + b[3];
  c[0] = x0;
  c[1] = x1;
  c[2] = x2;
  c[3] = x3;
}

static void ref_add4(const int *a, const int *b, int *c) {
  c[0] = a[0] + b[0];
  c[1] = a[1] + b[1];
  c[2] = a[2] + b[2];
  c[3] = a[3] + b[3];
}

int main(void) {
  int a[4] = {7, -3, 19, 4};
  int b[4] = {5, 11, -2, 9};
  int out1[4] = {0, 0, 0, 0};
  int out2[4] = {0, 0, 0, 0};

  foo_add4(a, b, out1);
  ref_add4(a, b, out2);

  return (out1[0] != out2[0]) || (out1[1] != out2[1]) ||
         (out1[2] != out2[2]) || (out1[3] != out2[3]);
}
