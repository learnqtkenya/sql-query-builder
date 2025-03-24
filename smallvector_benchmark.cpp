#include "sqlquerybuilder.hpp"
#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <random>

// Structure to simulate a typical element in our SQL builder
struct TestElement {
    std::string name;
    int value;
    bool flag;

    // Default constructor required for std::vector
    TestElement() : name("default"), value(0), flag(false) {}

    explicit TestElement(int i) : name("element_" + std::to_string(i)), value(i), flag(i % 2 == 0) {}

    // Make sure copying/moving is measurable
    TestElement(const TestElement&) = default;
    TestElement& operator=(const TestElement&) = default;
    TestElement(TestElement&&) = default;
    TestElement& operator=(TestElement&&) = default;
};

// Benchmark: Push back - small size (below SmallVector static capacity)
static void BM_PushBack_Small_StdVector(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<TestElement> vec;
        for (int i = 0; i < 8; ++i) {
            vec.push_back(TestElement(i));
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PushBack_Small_StdVector);

static void BM_PushBack_Small_SmallVector(benchmark::State& state) {
    for (auto _ : state) {
        sql::SmallVector<TestElement, 8> vec;
        for (int i = 0; i < 8; ++i) {
            vec.push_back(TestElement(i));
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PushBack_Small_SmallVector);

// Benchmark: Push back - medium size (exceeding SmallVector static capacity)
static void BM_PushBack_Medium_StdVector(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<TestElement> vec;
        for (int i = 0; i < 16; ++i) {
            vec.push_back(TestElement(i));
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PushBack_Medium_StdVector);

static void BM_PushBack_Medium_SmallVector(benchmark::State& state) {
    for (auto _ : state) {
        sql::SmallVector<TestElement, 8> vec;
        for (int i = 0; i < 16; ++i) {
            vec.push_back(TestElement(i));
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PushBack_Medium_SmallVector);

// Benchmark: Push back - large size (much bigger than static capacity)
static void BM_PushBack_Large_StdVector(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<TestElement> vec;
        for (int i = 0; i < 100; ++i) {
            vec.push_back(TestElement(i));
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PushBack_Large_StdVector);

static void BM_PushBack_Large_SmallVector(benchmark::State& state) {
    for (auto _ : state) {
        sql::SmallVector<TestElement, 8> vec;
        for (int i = 0; i < 100; ++i) {
            vec.push_back(TestElement(i));
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_PushBack_Large_SmallVector);

// Benchmark: Random access - small size
static void BM_RandomAccess_Small_StdVector(benchmark::State& state) {
    std::vector<TestElement> vec;
    for (int i = 0; i < 8; ++i) {
        vec.push_back(TestElement(i));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 7);

    for (auto _ : state) {
        int idx = distrib(gen);
        benchmark::DoNotOptimize(vec[idx]);
    }
}
BENCHMARK(BM_RandomAccess_Small_StdVector);

static void BM_RandomAccess_Small_SmallVector(benchmark::State& state) {
    sql::SmallVector<TestElement, 8> vec;
    for (int i = 0; i < 8; ++i) {
        vec.push_back(TestElement(i));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 7);

    for (auto _ : state) {
        int idx = distrib(gen);
        benchmark::DoNotOptimize(vec[idx]);
    }
}
BENCHMARK(BM_RandomAccess_Small_SmallVector);

// Benchmark: Random access - medium size
static void BM_RandomAccess_Medium_StdVector(benchmark::State& state) {
    std::vector<TestElement> vec;
    for (int i = 0; i < 16; ++i) {
        vec.push_back(TestElement(i));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 15);

    for (auto _ : state) {
        int idx = distrib(gen);
        benchmark::DoNotOptimize(vec[idx]);
    }
}
BENCHMARK(BM_RandomAccess_Medium_StdVector);

static void BM_RandomAccess_Medium_SmallVector(benchmark::State& state) {
    sql::SmallVector<TestElement, 8> vec;
    for (int i = 0; i < 16; ++i) {
        vec.push_back(TestElement(i));
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 15);

    for (auto _ : state) {
        int idx = distrib(gen);
        benchmark::DoNotOptimize(vec[idx]);
    }
}
BENCHMARK(BM_RandomAccess_Medium_SmallVector);

// Benchmark: Iteration - small size
static void BM_Iteration_Small_StdVector(benchmark::State& state) {
    std::vector<TestElement> vec;
    for (int i = 0; i < 8; ++i) {
        vec.push_back(TestElement(i));
    }

    for (auto _ : state) {
        int sum = 0;
        for (const auto& elem : vec) {
            sum += elem.value;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_Iteration_Small_StdVector);

static void BM_Iteration_Small_SmallVector(benchmark::State& state) {
    sql::SmallVector<TestElement, 8> vec;
    for (int i = 0; i < 8; ++i) {
        vec.push_back(TestElement(i));
    }

    for (auto _ : state) {
        int sum = 0;
        for (const auto& elem : vec) {
            sum += elem.value;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_Iteration_Small_SmallVector);

// Benchmark: Iteration - medium size
static void BM_Iteration_Medium_StdVector(benchmark::State& state) {
    std::vector<TestElement> vec;
    for (int i = 0; i < 16; ++i) {
        vec.push_back(TestElement(i));
    }

    for (auto _ : state) {
        int sum = 0;
        for (const auto& elem : vec) {
            sum += elem.value;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_Iteration_Medium_StdVector);

static void BM_Iteration_Medium_SmallVector(benchmark::State& state) {
    sql::SmallVector<TestElement, 8> vec;
    for (int i = 0; i < 16; ++i) {
        vec.push_back(TestElement(i));
    }

    for (auto _ : state) {
        int sum = 0;
        for (const auto& elem : vec) {
            sum += elem.value;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_Iteration_Medium_SmallVector);

// Benchmark: Construction/Destruction cycle
static void BM_ConstructDestruct_Small_StdVector(benchmark::State& state) {
    for (auto _ : state) {
        {
            std::vector<TestElement> vec;
            for (int i = 0; i < 8; ++i) {
                vec.push_back(TestElement(i));
            }
        } // Destruction happens here
    }
}
BENCHMARK(BM_ConstructDestruct_Small_StdVector);

static void BM_ConstructDestruct_Small_SmallVector(benchmark::State& state) {
    for (auto _ : state) {
        {
            sql::SmallVector<TestElement, 8> vec;
            for (int i = 0; i < 8; ++i) {
                vec.push_back(TestElement(i));
            }
        } // Destruction happens here
    }
}
BENCHMARK(BM_ConstructDestruct_Small_SmallVector);

// Benchmark: Construction/Destruction cycle - medium size
static void BM_ConstructDestruct_Medium_StdVector(benchmark::State& state) {
    for (auto _ : state) {
        {
            std::vector<TestElement> vec;
            for (int i = 0; i < 16; ++i) {
                vec.push_back(TestElement(i));
            }
        } // Destruction happens here
    }
}
BENCHMARK(BM_ConstructDestruct_Medium_StdVector);

static void BM_ConstructDestruct_Medium_SmallVector(benchmark::State& state) {
    for (auto _ : state) {
        {
            sql::SmallVector<TestElement, 8> vec;
            for (int i = 0; i < 16; ++i) {
                vec.push_back(TestElement(i));
            }
        } // Destruction happens here
    }
}
BENCHMARK(BM_ConstructDestruct_Medium_SmallVector);

// Memory allocation count benchmark
#ifdef _MSC_VER
// Windows version using CRT debug heap
#include <crtdbg.h>

class AllocationCounter {
    _CrtMemState state_start_;
public:
    AllocationCounter() {
        _CrtMemCheckpoint(&state_start_);
    }
    ~AllocationCounter() = default;

    int getAllocationCount() {
        _CrtMemState state_end, state_diff;
        _CrtMemCheckpoint(&state_end);
        _CrtMemDifference(&state_diff, &state_start_, &state_end);
        return state_diff.lAllocsCurPtr;
    }
};

#else
// Unix version using mallinfo2 (Linux) or custom allocator
#include <malloc.h>

class AllocationCounter {
    struct mallinfo2 start_info_;
public:
    AllocationCounter() {
        start_info_ = mallinfo2();
    }
    ~AllocationCounter() = default;

    int getAllocationCount() {
        struct mallinfo2 current = mallinfo2();
        return current.uordblks - start_info_.uordblks;
    }
};
#endif

static void BM_AllocationCount_StdVector(benchmark::State& state) {
    const int size = state.range(0);

    AllocationCounter counter;
    std::vector<TestElement> vec;

    for (int i = 0; i < size; ++i) {
        vec.push_back(TestElement(i));
    }

    state.counters["allocations"] = counter.getAllocationCount();
}

static void BM_AllocationCount_SmallVector(benchmark::State& state) {
    const int size = state.range(0);

    AllocationCounter counter;
    sql::SmallVector<TestElement, 8> vec;

    for (int i = 0; i < size; ++i) {
        vec.push_back(TestElement(i));
    }

    state.counters["allocations"] = counter.getAllocationCount();
}

BENCHMARK(BM_AllocationCount_StdVector)->Arg(4)->Arg(8)->Arg(16)->Arg(32)->Arg(64);
BENCHMARK(BM_AllocationCount_SmallVector)->Arg(4)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

// Run the benchmark
BENCHMARK_MAIN();
