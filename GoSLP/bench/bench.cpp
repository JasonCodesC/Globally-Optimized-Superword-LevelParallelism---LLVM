#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// Kernels (mirrors tests/vec4_add.c and a heavy quad_loops kernel)
static inline void vec4_add_mul(const int *a, const int *b, int *c, int *d) {
    int c0 = a[0] + b[0];
    int c1 = a[1] + b[1];
    int c2 = a[2] + b[2];
    int c3 = a[3] + b[3];

    c[0] = c0;
    c[1] = c1;
    c[2] = c2;
    c[3] = c3;

    d[0] = c0 * 3;
    d[1] = c1 * 3;
    d[2] = c2 * 3;
    d[3] = c3 * 3;
}

static inline void saxpy3(const float *x, const float *y, float *z) {
    // N = 16, unrolled by 4
    for (int i = 0; i < 16; i += 4) {
        float x0 = x[i + 0];
        float x1 = x[i + 1];
        float x2 = x[i + 2];
        float x3 = x[i + 3];

        float y0 = y[i + 0];
        float y1 = y[i + 1];
        float y2 = y[i + 2];
        float y3 = y[i + 3];

        z[i + 0] = x0 * 2.0f + y0;
        z[i + 1] = x1 * 2.0f + y1;
        z[i + 2] = x2 * 2.0f + y2;
        z[i + 3] = x3 * 2.0f + y3;
    }
}

static inline float dot64(const float *x, const float *y) {
    // Dot over 64 floats, unrolled to expose SLP opportunities.
    float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
    float acc4 = 0.0f, acc5 = 0.0f, acc6 = 0.0f, acc7 = 0.0f;
    for (int i = 0; i < 64; i += 8) {
        acc0 += x[i + 0] * y[i + 0];
        acc1 += x[i + 1] * y[i + 1];
        acc2 += x[i + 2] * y[i + 2];
        acc3 += x[i + 3] * y[i + 3];
        acc4 += x[i + 4] * y[i + 4];
        acc5 += x[i + 5] * y[i + 5];
        acc6 += x[i + 6] * y[i + 6];
        acc7 += x[i + 7] * y[i + 7];
    }
    return ((acc0 + acc1) + (acc2 + acc3)) + ((acc4 + acc5) + (acc6 + acc7));
}

static void nested(uint64_t seed, uint64_t &acc_int, double &acc_float) {

    constexpr int N = 4096;
    alignas(64) static int a[N];
    alignas(64) static int b[N];
    alignas(64) static int c[N];
    alignas(64) static int d[N];
    alignas(64) static float xf[N];
    alignas(64) static float yf[N];
    alignas(64) static float zf[N];
    alignas(64) static float wf[N];


    for (int i = 0; i < N; ++i) {
        a[i] = b[i] = 0;
    }

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N - 1; ++j) {
            a[i] = b[j] + b[j+1];
            b[i] = a[j] + a[j+1];
        }
    }

    for (int i = 0; i < N; ++i) {
        acc_int += a[i];
        acc_int += b[i];
    }
    
}

static inline void contrived_ex_1(const int *a, const int *b, int *c, int *d) {
    int x0 = a[0] + b[0];
    int y0 = x0 * 3;
    c[0] = y0 + 5;

    int x1 = a[1] + b[1];
    int y1 = x1 * 3;
    c[1] = y1 + 5;

    int x2 = a[2] + b[2];
    int y2 = x2 * 3;
    d[0] = y2 + 5;

    int x3 = a[3] + b[3];
    int y3 = x3 * 3;
    d[1] = y3 + 5;
}


static uint64_t checksum_int(const int *data, size_t len) {
    uint64_t acc = 0;
    for (size_t i = 0; i < len; ++i)
        acc += static_cast<uint64_t>(data[i]) * (i + 1);
    return acc;
}

static double checksum_float(const float *data, size_t len) {
    double acc = 0.0;
    for (size_t i = 0; i < len; ++i)
        acc += static_cast<double>(data[i]) * static_cast<double>(i + 1);
    return acc;
}

int main(int argc, char **argv) {
    const char *kernel = "vec4_add";
    if (argc > 1)
        kernel = argv[1];

    uint64_t iters = 10'000'000ULL;
    if (argc > 2)
        iters = std::strtoull(argv[2], nullptr, 10);
    if (iters == 0)
        iters = 1;

    using clock = std::chrono::steady_clock;
    auto start = clock::now();

    uint64_t acc_int = 0;
    double acc_float = 0.0;

    if (std::strcmp(kernel, "vec4_add") == 0) {
        for (uint64_t i = 0; i < iters; ++i) {
            int a[4] = {1 + static_cast<int>(i & 7),
                        2 + static_cast<int>(i & 7),
                        3 + static_cast<int>(i & 7),
                        4 + static_cast<int>(i & 7)};
            int b[4] = {5 + static_cast<int>((i >> 2) & 7),
                        6 + static_cast<int>((i >> 2) & 7),
                        7 + static_cast<int>((i >> 2) & 7),
                        8 + static_cast<int>((i >> 2) & 7)};
            int c[4];
            int d[4];
            vec4_add_mul(a, b, c, d);
            acc_int += checksum_int(c, 4);
            acc_int += checksum_int(d, 4);
        }
    } else if (std::strcmp(kernel, "saxpy3") == 0) {
        for (uint64_t i = 0; i < iters; ++i) {
            float x[16];
            float y[16];
            for (int j = 0; j < 16; ++j) {
                x[j] = static_cast<float>(j + 1 + (i & 3));
                y[j] = static_cast<float>(j * 3 + ((i >> 2) & 3));
            }
            float z[16];
            saxpy3(x, y, z);
            acc_float += checksum_float(z, 16);
        }
    } else if (std::strcmp(kernel, "dot64") == 0) {
        for (uint64_t i = 0; i < iters; ++i) {
            float x[64];
            float y[64];
            for (int j = 0; j < 64; ++j) {
                x[j] = static_cast<float>(1.0 + j + (i & 7));
                y[j] = static_cast<float>(0.5 + 2 * j + ((i >> 3) & 7));
            }
            float dot = dot64(x, y);
            acc_float += static_cast<double>(dot);
        }
    } else if (std::strcmp(kernel, "quad_loops") == 0) {
        for (uint64_t i = 0; i < iters; ++i) {
            nested(i, acc_int, acc_float);
        }
    } else if (std::strcmp(kernel, "contrived_ex_1") == 0) {
        int a[2] = {3,7};
        int b[2] = {4,5};
        int c[2];
        int d[2];
        contrived_ex_1(a,b,c,d);
    } else {
        std::fprintf(stderr, "Unknown kernel '%s'. Use vec4_add, saxpy3, dot64, or quad_loops (or contrived_ex_<x>).\n", kernel);
        return 1;
    }

    auto end = clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::printf("kernel=%s iters=%llu time_us=%lld checksum_int=%llu checksum_float=%f\n",
                kernel,
                static_cast<unsigned long long>(iters),
                static_cast<long long>(micros),
                static_cast<unsigned long long>(acc_int),
                acc_float);
    return 0;
}
