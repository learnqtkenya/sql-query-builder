# SQL Query Builder

A lightweight, header-only C++20 SQL query builder with type safety, stack allocation, and minimal overhead.

## Features

- Header-only implementation with zero dependencies (optional Qt support)
- Stack-allocated with configurable sizes for optimal performance
- Dual API: traditional string-based and modern type-safe column interfaces
- Compile-time type safety via concepts and templates
- Zero heap allocations with proper size configuration
- Automatic SQL injection protection with proper escaping
- Comprehensive error handling
- Support for enums and custom types
- Config-aware typed tables and columns (sqlpp11-like interface)
- Cross-configuration interoperability

## Basic Usage

### String-based API

```cpp
#include "sqlquerybuilder.hpp"
#include <iostream>

int main() {
    using namespace sql;
    using namespace std::string_view_literals;
    
    // Basic SELECT query
    auto query = QueryBuilder()
                    .select("id"sv, "name"sv, "email"sv)
                    .from("users"sv)
                    .where(col("active") == true)
                    .orderBy("created_at"sv, false) // DESC
                    .limit(10)
                    .build();
                    
    std::cout << query << std::endl;
    // Output: SELECT id, name, email FROM users WHERE active = 1 ORDER BY created_at DESC LIMIT 10
}
```

### Type-safe API (sqlpp11-like)

```cpp
// Define tables and typed columns
SQL_DEFINE_TABLE(users)
    SQL_DEFINE_COLUMN(id, int64_t)
    SQL_DEFINE_COLUMN(name, std::string)
    SQL_DEFINE_COLUMN(email, std::string)
    SQL_DEFINE_COLUMN(active, bool)
    SQL_DEFINE_COLUMN(created_at, std::string)
SQL_END_TABLE()

int main() {
    // Create the table instance
    const users_table users;
    
    // Use the typed interface
    auto query = sql::QueryBuilder()
                    .select(users.id, users.name, users.email)
                    .from(users.table)
                    .where(users.active == true)
                    .orderBy(users.created_at, false)
                    .limit(10)
                    .build();
}
```

## Stack Allocation Benefits

The query builder uses stack allocation for all internal data structures, resulting in:

- **Zero heap allocations** during normal operation
- **Better cache locality** for improved performance
- **No memory fragmentation** from frequent allocations/deallocations
- **Predictable memory usage** based on configuration

Use the custom configuration to control stack usage:

```cpp
// Define custom configuration with small limits for minimal stack usage
struct SmallConfig {
    static constexpr size_t MaxColumns = 16;
    static constexpr size_t MaxConditions = 8;
    static constexpr size_t MaxJoins = 4;
    static constexpr size_t MaxOrderBy = 4;
    static constexpr size_t MaxGroupBy = 4;
    static constexpr bool ThrowOnError = false;
};

// Reuse builder for multiple queries
sql::QueryBuilder<SmallConfig> builder;
for (int i = 0; i < 1000; i++) {
    auto query = builder.reset()
        .select(users.id, users.username)
        .from(users.table)
        .where(users.id == i)
        .build();
    // Use query...
}
```

## Type-safe Tables and Columns

Define database schema at compile-time:

```cpp
// Define an enum
enum class UserStatus : int {
    Active = 1,
    Inactive = 0,
    Pending = 2
};

// Define tables with typed columns
SQL_DEFINE_TABLE(users)
    SQL_DEFINE_COLUMN(id, int64_t)
    SQL_DEFINE_COLUMN(name, std::string)
    SQL_DEFINE_COLUMN(email, std::string)
    SQL_DEFINE_COLUMN(status, UserStatus)  // Enum type
    SQL_DEFINE_COLUMN(created_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(orders)
    SQL_DEFINE_COLUMN(id, int64_t)
    SQL_DEFINE_COLUMN(user_id, int64_t)
    SQL_DEFINE_COLUMN(amount, double)
SQL_END_TABLE()

// Use in queries
const users_table users;
const orders_table orders;

// Join with type-safe column comparison
auto query = QueryBuilder()
    .select(users.id, users.name, orders.amount)
    .from(users.table)
    .innerJoin(orders.table, (users.id == orders.user_id).toString())
    .where(users.status == UserStatus::Active)
    .build();
```

## Configuration

Customize memory usage and behavior:

```cpp
// Define custom configuration
struct MyConfig {
    static constexpr size_t MaxColumns = 64;    // Max columns in SELECT
    static constexpr size_t MaxConditions = 32; // Max WHERE conditions
    static constexpr size_t MaxJoins = 8;       // Max JOIN clauses
    static constexpr size_t MaxOrderBy = 16;    // Max ORDER BY clauses
    static constexpr size_t MaxGroupBy = 16;    // Max GROUP BY clauses
    static constexpr bool ThrowOnError = true;  // Throw exceptions on errors
};

// Use custom configuration with string interface
auto query = QueryBuilder<MyConfig>()
    .select("id"sv, "name"sv)
    .from("users"sv)
    .build();

// Use custom configuration with typed interface
auto typedQuery = QueryBuilder<MyConfig>()
    .select(users.id, users.name)
    .from(users.table)
    .build();
```

## Cross-Configuration Interoperability

Tables defined with one configuration can be used with builders of another:

```cpp
// Define custom configuration
struct MyConfig {
    static constexpr size_t MaxColumns = 64;
    static constexpr size_t MaxConditions = 32;
    // ... other settings
};

// Use default config table with custom config builder
const users_table users;  // Uses DefaultConfig

auto query = QueryBuilder<MyConfig>()
    .select(users.id, users.name)  // DefaultConfig columns with MyConfig builder
    .from(users.table)
    .where(users.active == true)
    .build();
```

## Complex Queries

### Joins and Aggregates

```cpp
// Complex query with joins and conditions
auto query = QueryBuilder()
    .select(users.id, users.name, "COUNT(orders.id) as order_count"sv)
    .from(users.table)
    .leftJoin(orders.table, (users.id == orders.user_id).toString())
    .where(users.status == UserStatus::Active)
    .groupBy(users.id)
    .groupBy(users.name)
    .having("COUNT(orders.id) > 5"sv)
    .orderBy("order_count"sv, false)
    .limit(100)
    .build();
```

### Insert, Update and Delete

```cpp
// INSERT query
auto insert = QueryBuilder()
    .insert(users.table)
    .value(users.name, "John Doe"sv)
    .value(users.email, "john@example.com"sv)
    .value(users.active, true)
    .value(users.status, UserStatus::Active)
    .build();
    
// UPDATE query
auto update = QueryBuilder()
    .update(users.table)
    .set(users.name, "Jane Doe"sv)
    .set(users.status, UserStatus::Active)
    .where(users.id == 42)
    .build();
    
// DELETE query
auto del = QueryBuilder()
    .deleteFrom(users.table)
    .where(users.status == UserStatus::Inactive)
    .build();
```

## Error Handling

```cpp
// With exceptions (when ThrowOnError = true)
try {
    auto query = QueryBuilder<MyConfig>().select(users.id).build();
} catch (const QueryError& e) {
    std::cerr << "Error: " << e.message << std::endl;
}

// Without exceptions
auto result = builder.select(users.id).buildResult();
if (result.hasError()) {
    std::cerr << "Error: " << result.error().message << std::endl;
} else {
    std::cout << result.value() << std::endl;
}
```

## Qt Integration

```cpp
#define SQLQUERYBUILDER_USE_QT
#include "sqlquerybuilder.hpp"

// Qt types work seamlessly with both interfaces
QDateTime startDate = QDateTime::currentDateTime().addDays(-7);
QString title = QString("Complex title");

// String-based interface
auto query1 = QueryBuilder()
    .select("id"sv, "title"sv, "created_at"sv)
    .from("tasks"sv)
    .where((col("created_at") >= startDate) &&
           (col("title") == title))
    .build();

// Type-safe interface
SQL_DEFINE_TABLE(tasks)
    SQL_DEFINE_COLUMN(id, int64_t)
    SQL_DEFINE_COLUMN(title, std::string)
    SQL_DEFINE_COLUMN(created_at, std::string)
SQL_END_TABLE()

const tasks_table tasks;

auto query2 = QueryBuilder()
    .select(tasks.id, tasks.title, tasks.created_at)
    .from(tasks.table)
    .where((tasks.created_at >= startDate) &&
           (tasks.title == title))
    .build();
```

## Advanced Features

- Complex nested conditions: `(users.active == true && (users.role == "admin" || users.permissions > 10))`
- Raw SQL conditions: `.whereRaw("DATE(created_at) > DATE_SUB(NOW(), INTERVAL 1 MONTH)"sv)`
- EXISTS subqueries: `.whereExists(subQueryString)`
- Null handling: `.whereNull(users.deleted_at)`
- BETWEEN conditions: `.whereBetween(users.age, 18, 65)`
- Compound statements: `.where(users.price > 100).whereNotNull(users.stock)`
- String literal support: `.where(users.email == "user@example.com")`