#include "sqlquerybuilder.hpp"
#include <benchmark/benchmark.h>
#include <array>
#include <string>
#include <memory>

// Define our custom configuration with larger limits for stress testing
struct LargeConfig {
    static constexpr size_t MaxColumns = 100;
    static constexpr size_t MaxConditions = 50;
    static constexpr size_t MaxJoins = 10;
    static constexpr size_t MaxOrderBy = 20;
    static constexpr size_t MaxGroupBy = 20;
    static constexpr bool ThrowOnError = false;
};

// Define smaller configuration for the stack-allocated version
struct SmallConfig {
    static constexpr size_t MaxColumns = 16;
    static constexpr size_t MaxConditions = 8;
    static constexpr size_t MaxJoins = 4;
    static constexpr size_t MaxOrderBy = 4;
    static constexpr size_t MaxGroupBy = 4;
    static constexpr bool ThrowOnError = false;
};

// Define tables using the macros
SQL_DEFINE_TABLE(users)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(username, std::string)
SQL_DEFINE_COLUMN(email, std::string)
SQL_DEFINE_COLUMN(password, std::string)
SQL_DEFINE_COLUMN(first_name, std::string)
SQL_DEFINE_COLUMN(last_name, std::string)
SQL_DEFINE_COLUMN(phone, std::string)
SQL_DEFINE_COLUMN(address, std::string)
SQL_DEFINE_COLUMN(city, std::string)
SQL_DEFINE_COLUMN(state, std::string)
SQL_DEFINE_COLUMN(country, std::string)
SQL_DEFINE_COLUMN(zip_code, std::string)
SQL_DEFINE_COLUMN(active, bool)
SQL_DEFINE_COLUMN(verified, bool)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(orders)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(order_number, std::string)
SQL_DEFINE_COLUMN(order_date, std::string)
SQL_DEFINE_COLUMN(total_amount, double)
SQL_DEFINE_COLUMN(status, std::string)
SQL_DEFINE_COLUMN(shipping_address, std::string)
SQL_DEFINE_COLUMN(shipping_city, std::string)
SQL_DEFINE_COLUMN(shipping_state, std::string)
SQL_DEFINE_COLUMN(shipping_country, std::string)
SQL_DEFINE_COLUMN(shipping_zip_code, std::string)
SQL_DEFINE_COLUMN(billing_address, std::string)
SQL_DEFINE_COLUMN(billing_city, std::string)
SQL_DEFINE_COLUMN(billing_state, std::string)
SQL_DEFINE_COLUMN(billing_country, std::string)
SQL_DEFINE_COLUMN(billing_zip_code, std::string)
SQL_DEFINE_COLUMN(payment_method, std::string)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(order_items)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(order_id, int64_t)
SQL_DEFINE_COLUMN(product_id, int64_t)
SQL_DEFINE_COLUMN(quantity, int)
SQL_DEFINE_COLUMN(unit_price, double)
SQL_DEFINE_COLUMN(total_price, double)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(products)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(name, std::string)
SQL_DEFINE_COLUMN(description, std::string)
SQL_DEFINE_COLUMN(sku, std::string)
SQL_DEFINE_COLUMN(category_id, int64_t)
SQL_DEFINE_COLUMN(price, double)
SQL_DEFINE_COLUMN(cost, double)
SQL_DEFINE_COLUMN(stock_quantity, int)
SQL_DEFINE_COLUMN(active, bool)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_END_TABLE()

// Global table instances for benchmarks
const users_table users;
const orders_table orders;
const order_items_table order_items;
const products_table products;

// Benchmark for simple SELECT query
static void BM_SimpleSelect(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .orderBy(users.username)
            .build();

        // Prevent the compiler from optimizing away the query construction
        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_SimpleSelect);

// Benchmark for complex JOIN query
static void BM_ComplexJoin(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(
            users.id, users.username,
            orders.order_number, orders.total_amount,
            products.name, products.price,
            order_items.quantity
            )
            .from(users.table)
            .innerJoin(orders.table,
                       (users.id == orders.user_id).toString())
            .innerJoin(order_items.table,
                       (orders.id == order_items.order_id).toString())
            .innerJoin(products.table,
                       (order_items.product_id == products.id).toString())
            .where((users.active == true) &&
                   (orders.status == "completed") &&
                   (products.active == true))
            .orderBy(orders.created_at, false)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_ComplexJoin);

// Benchmark for queries with many conditions
static void BM_ManyConditions(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<LargeConfig>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .where(users.verified == true)
            .where(users.city == "New York")
            .where(users.state == "NY")
            .where(users.country == "USA")
            .where(users.created_at >= "2023-01-01")
            .where(users.created_at <= "2023-12-31")
            .orderBy(users.username)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_ManyConditions);

// Benchmark for INSERT with many values
static void BM_Insert(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .insert(users.table)
            .value(users.username, "john_doe")
            .value(users.email, "john@example.com")
            .value(users.password, "password123")
            .value(users.first_name, "John")
            .value(users.last_name, "Doe")
            .value(users.phone, "555-1234")
            .value(users.address, "123 Main St")
            .value(users.city, "New York")
            .value(users.state, "NY")
            .value(users.country, "USA")
            .value(users.zip_code, "10001")
            .value(users.active, true)
            .value(users.verified, true)
            .value(users.created_at, "2023-01-01 12:00:00")
            .value(users.updated_at, "2023-01-01 12:00:00")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_Insert);

// Benchmark for builder reuse
static void BM_QueryReuse(benchmark::State& state) {
    sql::QueryBuilder<> builder;

    for (auto _ : state) {
        state.PauseTiming();
        builder.reset();
        state.ResumeTiming();

        for (int i = 0; i < 5; i++) {
            auto query = builder
                             .select(users.id, users.username, users.email)
                             .from(users.table)
                             .where(users.id == i)
                             .build();

            benchmark::DoNotOptimize(query);

            // Reset for next iteration
            builder.reset();
        }
    }
}
BENCHMARK(BM_QueryReuse);

// Benchmark for stack-allocated queries with SmallConfig
static void BM_StackAllocated(benchmark::State& state) {
    sql::QueryBuilder<SmallConfig> builder;

    for (auto _ : state) {
        state.PauseTiming();
        builder.reset();
        state.ResumeTiming();

        for (int i = 0; i < 10; i++) {
            auto query = builder
                             .select(users.id, users.username, users.email,
                                     users.first_name, users.last_name, users.phone)
                             .from(users.table)
                             .where(users.active == true)
                             .where(users.verified == true)
                             .where(users.city == "New York")
                             .where(users.created_at >= "2023-01-01")
                             .build();

            benchmark::DoNotOptimize(query);

            // Reset for next iteration
            builder.reset();
        }
    }
}
BENCHMARK(BM_StackAllocated);

// Benchmark for heap-allocated queries with SmallConfig
static void BM_HeapAllocated(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 10; i++) {
            auto builder = std::make_unique<sql::QueryBuilder<SmallConfig>>();
            auto query = builder->select(users.id, users.username, users.email,
                                         users.first_name, users.last_name, users.phone)
                             .from(users.table)
                             .where(users.active == true)
                             .where(users.verified == true)
                             .where(users.city == "New York")
                             .where(users.created_at >= "2023-01-01")
                             .build();

            benchmark::DoNotOptimize(query);
        }
    }
}
BENCHMARK(BM_HeapAllocated);

// Comparing string-based vs typed column interface
static void BM_StringBasedInterface(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select("id", "username", "email")
            .from("users")
            .where(sql::col("active") == true)
            .where(sql::col("verified") == true)
            .where(sql::col("city") == "New York")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_StringBasedInterface);

static void BM_TypedInterface(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .where(users.verified == true)
            .where(users.city == "New York")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_TypedInterface);

// Test cross-config interoperability
static void BM_CrossConfigInteroperability(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<LargeConfig>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)  // DefaultConfig column with LargeConfig builder
            .orderBy(users.username)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_CrossConfigInteroperability);

// Test string literal handling performance
static void BM_StringLiteralHandling(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username)
            .from(users.table)
            .where(users.email == "example@mail.com")  // String literal
            .where(users.city == "New York")  // String literal
            .where(users.username == "johndoe")  // String literal
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_StringLiteralHandling);

// Memory usage benchmarks
static void BM_SmallConfigMemoryFootprint(benchmark::State& state) {
    // This benchmark measures the construction time as a proxy for measuring
    // memory allocation overhead
    for (auto _ : state) {
        sql::QueryBuilder<SmallConfig> builder;
        benchmark::DoNotOptimize(builder);
    }
}
BENCHMARK(BM_SmallConfigMemoryFootprint);

static void BM_LargeConfigMemoryFootprint(benchmark::State& state) {
    for (auto _ : state) {
        sql::QueryBuilder<LargeConfig> builder;
        benchmark::DoNotOptimize(builder);
    }
}
BENCHMARK(BM_LargeConfigMemoryFootprint);

BENCHMARK_MAIN();
