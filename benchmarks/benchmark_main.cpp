#include <benchmark/benchmark.h>

static void BM_Example(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark code here
        int x = 0;
        for (int i = 0; i < 1000; ++i) {
            x += i;
        }
    }
}
BENCHMARK(BM_Example);

BENCHMARK_MAIN();