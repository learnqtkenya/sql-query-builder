# SQL Query Builder

A lightweight, header-only C++20 SQL query builder with type safety and minimal overhead.

## Features

- Header-only implementation with zero dependencies (optional Qt support)
- Fluent interface with method chaining
- Compile-time type safety via concepts and templates
- Configurable maximum sizes to minimize heap allocations
- Automatic SQL injection protection with proper escaping
- Comprehensive error handling
- Seamless support for enums and custom types

## Basic Usage

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
    
    // Complex query with joins and conditions
    enum class UserStatus { Active = 1, Inactive = 0 };
    
    auto complex = QueryBuilder()
                      .select("u.id"sv, "u.name"sv, "COUNT(o.id) as order_count"sv)
                      .from("users u"sv)
                      .leftJoin("orders o"sv, "u.id = o.user_id"sv)
                      .where(col("u.status") == UserStatus::Active)
                      .groupBy("u.id"sv)
                      .having("COUNT(o.id) > 5"sv)
                      .build();
                      
    // INSERT query
    auto insert = QueryBuilder()
                     .insert("users"sv)
                     .value("name"sv, "John Doe"sv)
                     .value("email"sv, "john@example.com"sv)
                     .value("active"sv, true)
                     .build();
                     
    // UPDATE query
    auto update = QueryBuilder()
                     .update("users"sv)
                     .set("name"sv, "Jane Doe"sv)
                     .where(col("id") == 42)
                     .build();
                     
    // DELETE query
    auto del = QueryBuilder()
                  .deleteFrom("users"sv)
                  .where(col("status") == UserStatus::Inactive)
                  .build();
}
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

// Use custom configuration
auto query = QueryBuilder<MyConfig>()
                .select("id"sv, "name"sv)
                .from("users"sv)
                .build();
```

## Error Handling

```cpp
// With exceptions (when ThrowOnError = true)
try {
    auto query = QueryBuilder<MyConfig>().select("id"sv).build();
} catch (const QueryError& e) {
    std::cerr << "Error: " << e.message << std::endl;
}

// Without exceptions
auto result = builder.select("id"sv).buildResult();
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

// Qt types work seamlessly
QDateTime startDate = QDateTime::currentDateTime().addDays(-7);
QString title = QString("Complex title");

auto query = QueryBuilder()
                .select("id"sv, "title"sv, "created_at"sv)
                .from("tasks"sv)
                .where((col("created_at") >= startDate) &&
                       (col("title") == title))
                .build();
```

## Advanced Features

- Complex nested conditions: `(col("A") == 1 && (col("B") > 2 || col("C") < 3))`
- Raw SQL conditions: `.whereRaw("DATE(created_at) > DATE_SUB(NOW(), INTERVAL 1 MONTH)"sv)`
- EXISTS subqueries: `.whereExists(subQueryString)`
- Null handling: `.whereNull("deleted_at"sv)`
- BETWEEN conditions: `.whereBetween("age"sv, 18, 65)`
- Compound statements: `.where(col("price") > 100).whereNotNull("stock"sv)`
