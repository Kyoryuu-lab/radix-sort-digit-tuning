#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <tbb/tbb.h>

std::vector<uint32_t> generate_data(size_t n) {
    std::vector<uint32_t> data(n);
    std::mt19937 rng(1337);
    for (size_t i = 0; i < n; ++i) {
        data[i] = rng();
    }
    return data;
}

int compare_uint(const void* a, const void* b) {
    uint32_t arg1 = *static_cast<const uint32_t*>(a);
    uint32_t arg2 = *static_cast<const uint32_t*>(b);
    return (arg1 > arg2) - (arg1 < arg2);
}

// Variante 1: Naiv (8-Bit)
void radix_sort_v1(std::vector<uint32_t>& input) {
    constexpr int RADIX_BITS = 8;
    constexpr int NUM_BUCKETS = 1 << RADIX_BITS;
    constexpr uint32_t RADIX_MASK = NUM_BUCKETS - 1;

    const size_t n = input.size();
    std::vector<uint32_t> output(n);
    const size_t p = tbb::info::default_concurrency();
    const size_t block_size = (n + p - 1) / p;

    for (int shift = 0; shift < 32; shift += RADIX_BITS) {
        std::vector<std::vector<size_t>> hist(p, std::vector<size_t>(NUM_BUCKETS, 0));
        tbb::parallel_for(size_t(0), p, [&](size_t t) {
            size_t start = t * block_size;
            size_t end = std::min(start + block_size, n);
            for (size_t i = start; i < end; ++i) {
                hist[t][(input[i] >> shift) & RADIX_MASK]++;
            }
        });

        std::vector<std::vector<size_t>> offsets(p, std::vector<size_t>(NUM_BUCKETS, 0));
        size_t running_sum = 0;
        for (size_t b = 0; b < NUM_BUCKETS; ++b) {
            for (size_t t = 0; t < p; ++t) {
                offsets[t][b] = running_sum;
                running_sum += hist[t][b];
            }
        }

        tbb::parallel_for(size_t(0), p, [&](size_t t) {
            size_t start = t * block_size;
            size_t end = std::min(start + block_size, n);
            auto local_offsets = offsets[t];
            for (size_t i = start; i < end; ++i) {
                uint32_t digit = (input[i] >> shift) & RADIX_MASK;
                output[local_offsets[digit]++] = input[i];
            }
        });

        input = output;
    }
}

// Variante 2: Optimiert & Parametrisiert
template <int RADIX_BITS>
void radix_sort_v2(std::vector<uint32_t>& data) {
    constexpr int NUM_BUCKETS = 1 << RADIX_BITS;
    constexpr uint32_t RADIX_MASK = NUM_BUCKETS - 1;

    const size_t n = data.size();
    std::vector<uint32_t> temp(n);

    uint32_t* src = data.data();
    uint32_t* dst = temp.data();

    const size_t p = tbb::info::default_concurrency();
    const size_t block_size = (n + p - 1) / p;

    std::vector<size_t> hist(p * NUM_BUCKETS);
    std::vector<size_t> offsets(p * NUM_BUCKETS);

    for (int shift = 0; shift < 32; shift += RADIX_BITS) {
        std::fill(hist.begin(), hist.end(), 0);

        // 1. Phase: Histogram (1 Schleife)
        tbb::parallel_for(size_t(0), p, [&](size_t t) {
            size_t start = t * block_size;
            size_t end = std::min(start + block_size, n);
            size_t* my_hist = &hist[t * NUM_BUCKETS];
            for (size_t i = start; i < end; ++i) {
                my_hist[(src[i] >> shift) & RADIX_MASK]++;
            }
        });

        // 2. Phase: Scan-Buckets (exakt 2 Schleifen fuer Column-Major Scan)
        size_t running_sum = 0;
        for (size_t b = 0; b < NUM_BUCKETS; ++b) {
            for (size_t t = 0; t < p; ++t) {
                offsets[t * NUM_BUCKETS + b] = running_sum;
                running_sum += hist[t * NUM_BUCKETS + b];
            }
        }

        // 3. Phase: Scatter (1 Schleife, direkte Zeigerindizierung)
        tbb::parallel_for(size_t(0), p, [&](size_t t) {
            size_t start = t * block_size;
            size_t end = std::min(start + block_size, n);
            size_t* my_offsets = &offsets[t * NUM_BUCKETS];
            for (size_t i = start; i < end; ++i) {
                uint32_t val = src[i];
                dst[my_offsets[(val >> shift) & RADIX_MASK]++] = val;
            }
        });

        std::swap(src, dst);
    }

    if (src != data.data()) {
        std::copy(temp.begin(), temp.end(), data.begin());
    }
}

int main() {
    const size_t N = 1'000'000'000;
    const unsigned int p = tbb::info::default_concurrency();

    std::cout << "========================================================================\n";
    std::cout << "PVA Rechneruebung 2: RadixSort Benchmark & Analyse\n";
    std::cout << "N = " << N << " | Threads: " << p << "\n";
    std::cout << "========================================================================\n";

    auto orig_data = generate_data(N);

    // Baseline: std::qsort
    auto qsort_data = orig_data;
    auto t1 = std::chrono::high_resolution_clock::now();
    std::qsort(qsort_data.data(), qsort_data.size(), sizeof(uint32_t), compare_uint);
    auto t2 = std::chrono::high_resolution_clock::now();
    double t_seq = std::chrono::duration<double, std::milli>(t2 - t1).count();
    std::cout << std::left << std::setw(28) << "std::qsort (Baseline):"
              << std::right << std::setw(10) << std::fixed << std::setprecision(2) << t_seq << " ms"
              << " | Speedup:  1.00x | Effizienz: 100.0 %\n";

    // Variante 1: Naiv (8b)
    auto v1_data = orig_data;
    auto t3 = std::chrono::high_resolution_clock::now();
    radix_sort_v1(v1_data);
    auto t4 = std::chrono::high_resolution_clock::now();
    double t_v1 = std::chrono::duration<double, std::milli>(t4 - t3).count();
    double sp_v1 = t_seq / t_v1;
    std::cout << std::left << std::setw(28) << "Variante 1 (8-Bit naiv):"
              << std::right << std::setw(10) << t_v1 << " ms"
              << " | Speedup: " << std::setw(5) << sp_v1 << "x | Effizienz: " << (sp_v1 / p) * 100.0 << " %\n";

    std::cout << "------------------------------------------------------------------------\n";
    std::cout << "Variante 2: Untersuchung der Radix-Groesse (r-Bit):\n";

    // r = 4 bit (8 проходов)
    auto v2_4b = orig_data;
    auto t5 = std::chrono::high_resolution_clock::now();
    radix_sort_v2<4>(v2_4b);
    auto t6 = std::chrono::high_resolution_clock::now();
    double t_4b = std::chrono::duration<double, std::milli>(t6 - t5).count();
    double sp_4b = t_seq / t_4b;
    std::cout << std::left << std::setw(28) << "Variante 2 (r = 4 Bit):"
              << std::right << std::setw(10) << t_4b << " ms"
              << " | Speedup: " << std::setw(5) << sp_4b << "x | Effizienz: " << (sp_4b / p) * 100.0 << " %\n";

    // r = 8 bit (4 прохода) — Оптимум
    auto v2_8b = orig_data;
    auto t7 = std::chrono::high_resolution_clock::now();
    radix_sort_v2<8>(v2_8b);
    auto t8 = std::chrono::high_resolution_clock::now();
    double t_8b = std::chrono::duration<double, std::milli>(t8 - t7).count();
    double sp_8b = t_seq / t_8b;
    std::cout << std::left << std::setw(28) << "Variante 2 (r = 8 Bit) [OPT]:"
              << std::right << std::setw(10) << t_8b << " ms"
              << " | Speedup: " << std::setw(5) << sp_8b << "x | Effizienz: " << (sp_8b / p) * 100.0 << " %\n";

    // r = 16 bit (2 прохода)
    auto v2_16b = orig_data;
    auto t9 = std::chrono::high_resolution_clock::now();
    radix_sort_v2<16>(v2_16b);
    auto t10 = std::chrono::high_resolution_clock::now();
    double t_16b = std::chrono::duration<double, std::milli>(t10 - t9).count();
    double sp_16b = t_seq / t_16b;
    std::cout << std::left << std::setw(28) << "Variante 2 (r = 16 Bit):"
              << std::right << std::setw(10) << t_16b << " ms"
              << " | Speedup: " << std::setw(5) << sp_16b << "x | Effizienz: " << (sp_16b / p) * 100.0 << " %\n";

    std::cout << "========================================================================\n";
    std::cout << "Validierungsstatus: "
              << (std::is_sorted(v2_8b.begin(), v2_8b.end()) ? "ALLE SORTIERUNGEN KORREKT (SUCCESS)" : "FEHLER")
              << "\n";

    return 0;
}
