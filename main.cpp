#include "sqlquerybuilder.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <array>
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

// Instantiate table objects
const users_table users;
const orders_table orders;
const order_items_table order_items;
const products_table products;

// Benchmark class for measuring performance
class Benchmark {
public:
    template<typename Func>
    static void run(const std::string& name, Func func, int iterations = 10000) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; i++) {
            auto result = func();
            // Use the result to prevent optimization from removing the function call
            if (result.empty()) {
                std::cerr << "Empty result, this should not happen" << std::endl;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << name << ": " << duration << "ms for " << iterations
                  << " iterations (" << (duration / static_cast<double>(iterations))
                  << "ms per iteration)" << std::endl;
    }
};

int main() {
    std::cout << "Running SQL Query Builder benchmark...\n" << std::endl;

    // Test 1: Simple SELECT query
    Benchmark::run("Stack-allocated Simple SELECT", []() {
        auto query = sql::QueryBuilder<>()
        .select(users.id, users.username, users.email)
            .from(users.table)
            .where(users.active == true)
            .orderBy(users.username)
            .build();
        return query;
    });

    // Test 2: Complex JOIN query
    Benchmark::run("Stack-allocated Complex JOIN", []() {
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
        return query;
    });

    // Test 3: Many conditions with LargeConfig
    Benchmark::run("Stack-allocated Many Conditions with LargeConfig", []() {
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
        return query;
    });

    // Test 4: INSERT with many values
    Benchmark::run("Stack-allocated INSERT", []() {
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
        return query;
    });

    // Test 5: Reuse builder for many queries
    Benchmark::run("Stack-allocated Query Reuse", []() {
        sql::QueryBuilder<> builder;
        std::string result;

        for (int i = 0; i < 5; i++) {
            auto query = builder.reset()
            .select(users.id, users.username, users.email)
                .from(users.table)
                .where(users.id == i)
                .build();
            result += query; // Accumulate to ensure the loop doesn't get optimized away
        }
        return result;
    });

    // Test 6: Compare with different config types
    std::cout << "\nComparing stack vs heap allocation under stress...\n" << std::endl;

    // First use small config with stack allocation
    Benchmark::run("Stack-allocated - Stress Test with SmallConfig", []() {
        // Create a builder with small limits that fits on stack
        sql::QueryBuilder<SmallConfig> builder;
        std::string result;

        for (int i = 0; i < 10; i++) {
            auto query = builder.reset()
            .select(users.id, users.username, users.email,
                    users.first_name, users.last_name, users.phone)
                .from(users.table)
                .where(users.active == true)
                .where(users.verified == true)
                .where(users.city == "New York")
                .where(users.created_at >= "2023-01-01")
                .build();
            result += query;
        }
        return result;
    });

    // Now allocate builders on heap to simulate the old way
    Benchmark::run("Heap-allocated - Stress Test with SmallConfig", []() {
        std::string result;
        for (int i = 0; i < 10; i++) {
            // Create a new builder on heap each time (simulate dynamic allocation)
            auto builder = std::make_unique<sql::QueryBuilder<SmallConfig>>();
            auto query = builder->select(users.id, users.username, users.email,
                                         users.first_name, users.last_name, users.phone)
                             .from(users.table)
                             .where(users.active == true)
                             .where(users.verified == true)
                             .where(users.city == "New York")
                             .where(users.created_at >= "2023-01-01")
                             .build();
            result += query;
        }
        return result;
    });

    return 0;
}
