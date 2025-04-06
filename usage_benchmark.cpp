#include "sqlquerybuilder.hpp"
#include <benchmark/benchmark.h>
#include <array>
#include <string>
#include <memory>
#include <vector>

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

// Define a tiny configuration for embedded systems or extremely constrained environments
struct TinyConfig {
    static constexpr size_t MaxColumns = 4;
    static constexpr size_t MaxConditions = 2;
    static constexpr size_t MaxJoins = 1;
    static constexpr size_t MaxOrderBy = 1;
    static constexpr size_t MaxGroupBy = 1;
    static constexpr bool ThrowOnError = false;
};

// Define example enum types
enum class UserStatus : int {
    Active = 1,
    Inactive = 0,
    Pending = 2
};

enum class OrderStatus : int {
    New = 0,
    Processing = 1,
    Shipped = 2,
    Delivered = 3,
    Cancelled = 4
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
SQL_DEFINE_COLUMN(status, UserStatus)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(orders)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(order_number, std::string)
SQL_DEFINE_COLUMN(order_date, std::string)
SQL_DEFINE_COLUMN(total_amount, double)
SQL_DEFINE_COLUMN(status, OrderStatus)
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

SQL_DEFINE_TABLE(categories)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(name, std::string)
SQL_DEFINE_COLUMN(description, std::string)
SQL_DEFINE_COLUMN(parent_id, int64_t)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(reviews)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(product_id, int64_t)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(rating, int)
SQL_DEFINE_COLUMN(comment, std::string)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_END_TABLE()

// Custom table configs
struct users_table_small : public users_table {
    using config_type = SmallConfig;
};

struct users_table_large : public users_table {
    using config_type = LargeConfig;
};

struct users_table_tiny : public users_table {
    using config_type = TinyConfig;
};

// Global table instances for benchmarks
const users_table users;
const users_table_small users_small;
const users_table_large users_large;
const users_table_tiny users_tiny;
const orders_table orders;
const order_items_table order_items;
const products_table products;
const categories_table categories;
const reviews_table reviews;

// Create a list of standard conditions for reuse
std::string getActiveCond() { return (users.active == true).toString(); }
std::string getVerifiedCond() { return (users.verified == true).toString(); }
std::string getJoinCond() { return (users.id == orders.user_id).toString(); }
std::string getOrderJoinCond() { return (orders.id == order_items.order_id).toString(); }
std::string getProductJoinCond() { return (order_items.product_id == products.id).toString(); }

// Benchmark for simple SELECT query
static void BM_SimpleSelect(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .orderBy(users.username)
            .build();

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
            .innerJoin(orders.table, getJoinCond())
            .innerJoin(order_items.table, getOrderJoinCond())
            .innerJoin(products.table, getProductJoinCond())
            .where((users.active == true) &&
                   (orders.status == OrderStatus::Delivered) &&
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

// Benchmark for INSERT OR REPLACE
static void BM_InsertOrReplace(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .insertOrReplace(users.table)
            .value(users.id, 1)
            .value(users.username, "john_doe")
            .value(users.email, "john@example.com")
            .value(users.password, "password123")
            .value(users.active, true)
            .value(users.verified, true)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_InsertOrReplace);

// Benchmark for UPDATE
static void BM_Update(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .update(users.table)
            .set(users.username, "new_username")
            .set(users.email, "new_email@example.com")
            .set(users.updated_at, "2023-01-02 12:00:00")
            .where(users.id == 1)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_Update);

// Benchmark for DELETE
static void BM_Delete(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .deleteFrom(users.table)
            .where(users.active == false)
            .where(users.created_at < "2022-01-01")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_Delete);

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

// Benchmark with different config sizes
static void BM_TinyConfig(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<TinyConfig>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .orderBy(users.username)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_TinyConfig);

static void BM_SmallConfig(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<SmallConfig>()
        .select(users.id, users.username, users.email, users.created_at)
            .from(users.table)
            .where(users.active == true)
            .where(users.verified == true)
            .orderBy(users.username)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_SmallConfig);

static void BM_DefaultConfig(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email, users.created_at, users.updated_at)
            .from(users.table)
            .where(users.active == true)
            .where(users.verified == true)
            .where(users.created_at >= "2023-01-01")
            .orderBy(users.username)
            .orderBy(users.created_at, false)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_DefaultConfig);

static void BM_LargeConfig(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<LargeConfig>()
        .select(users.id, users.username, users.email, users.first_name, users.last_name,
                users.phone, users.address, users.city, users.state, users.country)
            .from(users.table)
            .where(users.active == true)
            .where(users.verified == true)
            .where(users.created_at >= "2023-01-01")
            .where(users.city == "New York")
            .where(users.state == "NY")
            .where(users.country == "USA")
            .orderBy(users.username)
            .orderBy(users.created_at, false)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_LargeConfig);

// Stack vs. Heap Allocation
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

// String type comparison
static void BM_StringView(benchmark::State& state) {
    using namespace std::string_view_literals;

    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select("id"sv, "username"sv, "email"sv)
            .from("users"sv)
            .where(sql::col("active") == true)
            .where(sql::col("verified") == true)
            .where(sql::col("city") == "New York"sv)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_StringView);

static void BM_StdString(benchmark::State& state) {
    std::string id = "id";
    std::string username = "username";
    std::string email = "email";
    std::string usersTable = "users";
    std::string active = "active";
    std::string verified = "verified";
    std::string city = "city";
    std::string newYork = "New York";

    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(id, username, email)
            .from(usersTable)
            .where(sql::col(active) == true)
            .where(sql::col(verified) == true)
            .where(sql::col(city) == newYork)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_StdString);

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

// Cross-config compatibility tests
static void BM_CrossConfig_DefaultWithTinyCol(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users_tiny.id, users_tiny.username)
            .from(users_tiny.table)
            .where(users_tiny.active == true)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_CrossConfig_DefaultWithTinyCol);

static void BM_CrossConfig_LargeWithDefaultCol(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<LargeConfig>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_CrossConfig_LargeWithDefaultCol);

// Aggregate function tests
static void BM_AggregateFunction(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(
            sql::count(users.id).c_str(),
            sql::avg(orders.total_amount).c_str(),
            sql::max(orders.total_amount).c_str(),
            sql::min(orders.total_amount).c_str())
            .from(users.table)
            .leftJoin(orders.table, getJoinCond())
            .where(users.active == true)
            .groupBy(users.id)
            .having("COUNT(orders.id) > 0")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_AggregateFunction);

// Advanced features
static void BM_WhereNull(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username)
            .from(users.table)
            .whereNull(users.email)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_WhereNull);

static void BM_WhereNotNull(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username)
            .from(users.table)
            .whereNotNull(users.email)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_WhereNotNull);

static void BM_WhereBetween(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(orders.id, orders.total_amount)
            .from(orders.table)
            .whereBetween(orders.total_amount, 100.0, 500.0)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_WhereBetween);

static void BM_WhereIn(benchmark::State& state) {
    // Create a list of IDs
    std::array<int64_t, 5> ids = {1, 2, 3, 4, 5};

    std::span<const int64_t> idsSpan(ids);

    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username)
            .from(users.table)
            .whereIn(users.id, idsSpan)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_WhereIn);

static void BM_WhereLike(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .whereLike(users.email, "%@gmail.com")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_WhereLike);

// Complex operator tests
static void BM_ComplexOperators(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username)
            .from(users.table)
            .where(
                (users.active == true && users.verified == true) ||
                (users.created_at >= "2023-01-01" && users.country == "USA")
                )
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_ComplexOperators);

// Multiple joins with different join types
static void BM_MultipleJoinTypes(benchmark::State& state) {
    std::string productsJoin = (categories.id == products.category_id).toString();
    std::string reviewsJoin = (products.id == reviews.product_id).toString();

    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(
            products.id,
            products.name,
            categories.name,
            "AVG(reviews.rating) as avg_rating"
            )
            .from(products.table)
            .innerJoin(categories.table, productsJoin)
            .leftJoin(reviews.table, reviewsJoin)
            .where(products.active == true)
            .groupBy(products.id)
            .groupBy(products.name)
            .groupBy(categories.name)
            .having("COUNT(reviews.id) > 0")
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_MultipleJoinTypes);

// Simulate common query patterns
static void BM_LoginQuery(benchmark::State& state) {
    std::string email = "user@example.com";
    std::string password = "hashedpassword123";

    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.email == email)
            .where(users.password == password)
            .where(users.active == true)
            .limit(1)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_LoginQuery);

static void BM_ProductListingQuery(benchmark::State& state) {
    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(
            products.id,
            products.name,
            products.price,
            products.description,
            categories.name
            )
            .from(products.table)
            .leftJoin(categories.table, (categories.id == products.category_id).toString())
            .where(products.active == true)
            .where(products.stock_quantity > 0)
            .orderBy(products.price)
            .limit(20)
            .offset(0)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_ProductListingQuery);

static void BM_OrderHistoryQuery(benchmark::State& state) {
    int64_t userId = 42;

    for (auto _ : state) {
        auto query = sql::QueryBuilder<>()
        .select(
            orders.id,
            orders.order_number,
            orders.order_date,
            orders.total_amount,
            orders.status,
            "COUNT(order_items.id) as item_count"
            )
            .from(orders.table)
            .leftJoin(order_items.table, (orders.id == order_items.order_id).toString())
            .where(orders.user_id == userId)
            .groupBy(orders.id)
            .groupBy(orders.order_number)
            .groupBy(orders.order_date)
            .groupBy(orders.total_amount)
            .groupBy(orders.status)
            .orderBy(orders.order_date, false)
            .build();

        benchmark::DoNotOptimize(query);
    }
}
BENCHMARK(BM_OrderHistoryQuery);

// Memory footprint benchmarks
static void BM_TinyConfigMemoryFootprint(benchmark::State& state) {
    for (auto _ : state) {
        sql::QueryBuilder<TinyConfig> builder;
        benchmark::DoNotOptimize(builder);
    }
}
BENCHMARK(BM_TinyConfigMemoryFootprint);

static void BM_SmallConfigMemoryFootprint(benchmark::State& state) {
    for (auto _ : state) {
        sql::QueryBuilder<SmallConfig> builder;
        benchmark::DoNotOptimize(builder);
    }
}
BENCHMARK(BM_SmallConfigMemoryFootprint);

static void BM_DefaultConfigMemoryFootprint(benchmark::State& state) {
    for (auto _ : state) {
        sql::QueryBuilder<> builder;
        benchmark::DoNotOptimize(builder);
    }
}
BENCHMARK(BM_DefaultConfigMemoryFootprint);

static void BM_LargeConfigMemoryFootprint(benchmark::State& state) {
    for (auto _ : state) {
        sql::QueryBuilder<LargeConfig> builder;
        benchmark::DoNotOptimize(builder);
    }
}
BENCHMARK(BM_LargeConfigMemoryFootprint);

BENCHMARK_MAIN();
