#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using clock_t_ = std::chrono::steady_clock;

static constexpr int kBlock = 8;

static void init_data(std::vector<float> &x, std::vector<float> &y,
                      std::vector<float> &w, int n) {
  x.resize(n);
  y.resize(n);
  w.resize(n);
  for (int i = 0; i < n; ++i) {
    x[i] = static_cast<float>(0.01 * (i % 97) + 1.0);
    y[i] = static_cast<float>(0.02 * (i % 89) + 0.5);
    w[i] = static_cast<float>(0.03 * (i % 71) + 1.5);
  }
}

__attribute__((noinline))
static float array_sum_block8(const float *x) {
  float s = 0.0f;
  s += x[0]; s += x[1]; s += x[2]; s += x[3];
  s += x[4]; s += x[5]; s += x[6]; s += x[7];
  return s;
}

__attribute__((noinline))
static float dot_product_block8(const float *x, const float *y) {
  float s = 0.0f;
  s += x[0] * y[0]; s += x[1] * y[1]; s += x[2] * y[2]; s += x[3] * y[3];
  s += x[4] * y[4]; s += x[5] * y[5]; s += x[6] * y[6]; s += x[7] * y[7];
  return s;
}

__attribute__((noinline))
static void rolling_sum_block8(const float *x, float prev, float &last,
                               float &sum_out) {
  float s0 = prev + x[0];
  float s1 = s0 + x[1];
  float s2 = s1 + x[2];
  float s3 = s2 + x[3];
  float s4 = s3 + x[4];
  float s5 = s4 + x[5];
  float s6 = s5 + x[6];
  float s7 = s6 + x[7];
  last = s7;
  sum_out += s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7;
}

__attribute__((noinline))
static void vwap_block8(const float *price, const float *vol, double &num,
                        double &den) {
  float n = 0.0f;
  float d = 0.0f;
  n += price[0] * vol[0]; d += vol[0];
  n += price[1] * vol[1]; d += vol[1];
  n += price[2] * vol[2]; d += vol[2];
  n += price[3] * vol[3]; d += vol[3];
  n += price[4] * vol[4]; d += vol[4];
  n += price[5] * vol[5]; d += vol[5];
  n += price[6] * vol[6]; d += vol[6];
  n += price[7] * vol[7]; d += vol[7];
  num += n;
  den += d;
}

__attribute__((noinline))
static void variance_block8(const float *x, double &sum, double &sumsq) {
  float s = 0.0f;
  float q = 0.0f;
  s += x[0]; q += x[0] * x[0];
  s += x[1]; q += x[1] * x[1];
  s += x[2]; q += x[2] * x[2];
  s += x[3]; q += x[3] * x[3];
  s += x[4]; q += x[4] * x[4];
  s += x[5]; q += x[5] * x[5];
  s += x[6]; q += x[6] * x[6];
  s += x[7]; q += x[7] * x[7];
  sum += s;
  sumsq += q;
}

__attribute__((noinline))
static void covariance_block8(const float *x, const float *y, double &sumx,
                              double &sumy, double &sumxy) {
  float sx = 0.0f, sy = 0.0f, sxy = 0.0f;
  sx += x[0]; sy += y[0]; sxy += x[0] * y[0];
  sx += x[1]; sy += y[1]; sxy += x[1] * y[1];
  sx += x[2]; sy += y[2]; sxy += x[2] * y[2];
  sx += x[3]; sy += y[3]; sxy += x[3] * y[3];
  sx += x[4]; sy += y[4]; sxy += x[4] * y[4];
  sx += x[5]; sy += y[5]; sxy += x[5] * y[5];
  sx += x[6]; sy += y[6]; sxy += x[6] * y[6];
  sx += x[7]; sy += y[7]; sxy += x[7] * y[7];
  sumx += sx;
  sumy += sy;
  sumxy += sxy;
}

static double kernel_array_sum(const std::vector<float> &x, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    float acc = 0.0f;
    for (int i = 0; i < n; i += kBlock)
      acc += array_sum_block8(&x[i]);
    checksum += static_cast<double>(acc);
  }
  return checksum;
}

static double ref_array_sum(const std::vector<float> &x, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    float acc = 0.0f;
    for (int i = 0; i < n; ++i)
      acc += x[i];
    checksum += static_cast<double>(acc);
  }
  return checksum;
}

static double kernel_dot_product(const std::vector<float> &x,
                                 const std::vector<float> &y, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    float acc = 0.0f;
    for (int i = 0; i < n; i += kBlock)
      acc += dot_product_block8(&x[i], &y[i]);
    checksum += static_cast<double>(acc);
  }
  return checksum;
}

static double ref_dot_product(const std::vector<float> &x,
                              const std::vector<float> &y, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    float acc = 0.0f;
    for (int i = 0; i < n; ++i)
      acc += x[i] * y[i];
    checksum += static_cast<double>(acc);
  }
  return checksum;
}

static double kernel_rolling_sum(const std::vector<float> &x, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    float prev = 0.0f;
    float accum = 0.0f;
    for (int i = 0; i < n; i += kBlock)
      rolling_sum_block8(&x[i], prev, prev, accum);
    checksum += static_cast<double>(accum + prev);
  }
  return checksum;
}

static double ref_rolling_sum(const std::vector<float> &x, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    float prev = 0.0f;
    float accum = 0.0f;
    for (int i = 0; i < n; ++i) {
      prev = prev + x[i];
      accum += prev;
    }
    checksum += static_cast<double>(accum + prev);
  }
  return checksum;
}

static double kernel_vwap(const std::vector<float> &p, const std::vector<float> &v,
                          int iters) {
  const int n = static_cast<int>(p.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    double num = 0.0;
    double den = 0.0;
    for (int i = 0; i < n; i += kBlock)
      vwap_block8(&p[i], &v[i], num, den);
    checksum += num / den;
  }
  return checksum;
}

static double ref_vwap(const std::vector<float> &p, const std::vector<float> &v,
                       int iters) {
  const int n = static_cast<int>(p.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    double num = 0.0;
    double den = 0.0;
    for (int i = 0; i < n; ++i) {
      num += static_cast<double>(p[i]) * static_cast<double>(v[i]);
      den += static_cast<double>(v[i]);
    }
    checksum += num / den;
  }
  return checksum;
}

static double kernel_variance_accum(const std::vector<float> &x, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    double sum = 0.0;
    double sumsq = 0.0;
    for (int i = 0; i < n; i += kBlock)
      variance_block8(&x[i], sum, sumsq);
    checksum += sum + sumsq;
  }
  return checksum;
}

static double ref_variance_accum(const std::vector<float> &x, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    double sum = 0.0;
    double sumsq = 0.0;
    for (int i = 0; i < n; ++i) {
      double v = x[i];
      sum += v;
      sumsq += v * v;
    }
    checksum += sum + sumsq;
  }
  return checksum;
}

static double kernel_covariance_accum(const std::vector<float> &x,
                                      const std::vector<float> &y, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    double sumx = 0.0;
    double sumy = 0.0;
    double sumxy = 0.0;
    for (int i = 0; i < n; i += kBlock)
      covariance_block8(&x[i], &y[i], sumx, sumy, sumxy);
    checksum += sumx + sumy + sumxy;
  }
  return checksum;
}

static double ref_covariance_accum(const std::vector<float> &x,
                                   const std::vector<float> &y, int iters) {
  const int n = static_cast<int>(x.size());
  double checksum = 0.0;
  for (int it = 0; it < iters; ++it) {
    double sumx = 0.0;
    double sumy = 0.0;
    double sumxy = 0.0;
    for (int i = 0; i < n; ++i) {
      double vx = x[i];
      double vy = y[i];
      sumx += vx;
      sumy += vy;
      sumxy += vx * vy;
    }
    checksum += sumx + sumy + sumxy;
  }
  return checksum;
}

int main(int argc, char **argv) {
  const char *kernel = argc > 1 ? argv[1] : "array_sum";
  int iters = argc > 2 ? std::atoi(argv[2]) : 2000;
  int n = argc > 3 ? std::atoi(argv[3]) : 8192;

  if (iters <= 0) iters = 1;
  if (n < kBlock) n = kBlock;
  n = (n / kBlock) * kBlock;

  std::vector<float> x, y, w;
  init_data(x, y, w, n);

  auto start = clock_t_::now();
  double got = 0.0;
  double ref = 0.0;

  if (std::strcmp(kernel, "array_sum") == 0) {
    got = kernel_array_sum(x, iters);
    ref = ref_array_sum(x, iters);
  } else if (std::strcmp(kernel, "dot_product") == 0) {
    got = kernel_dot_product(x, y, iters);
    ref = ref_dot_product(x, y, iters);
  } else if (std::strcmp(kernel, "rolling_sum") == 0) {
    got = kernel_rolling_sum(x, iters);
    ref = ref_rolling_sum(x, iters);
  } else if (std::strcmp(kernel, "vwap") == 0) {
    got = kernel_vwap(x, w, iters);
    ref = ref_vwap(x, w, iters);
  } else if (std::strcmp(kernel, "variance") == 0) {
    got = kernel_variance_accum(x, iters);
    ref = ref_variance_accum(x, iters);
  } else if (std::strcmp(kernel, "covariance") == 0) {
    got = kernel_covariance_accum(x, y, iters);
    ref = ref_covariance_accum(x, y, iters);
  } else {
    std::fprintf(stderr,
                 "Unknown kernel '%s'. Use array_sum|dot_product|rolling_sum|vwap|variance|covariance\n",
                 kernel);
    return 1;
  }

  auto end = clock_t_::now();
  long long time_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  double abs_err = std::fabs(got - ref);
  double tol = 2e-6 * std::fabs(ref) + 1e-2;
  if (abs_err > tol) {
    std::fprintf(stderr,
                 "validation_failed kernel=%s got=%0.9f ref=%0.9f abs_err=%0.9f tol=%0.9f\n",
                 kernel, got, ref, abs_err, tol);
    return 2;
  }

  std::printf("kernel=%s iters=%d n=%d time_us=%lld checksum=%.9f ref=%.9f abs_err=%.9f\n",
              kernel, iters, n, time_us, got, ref, abs_err);
  return 0;
}
