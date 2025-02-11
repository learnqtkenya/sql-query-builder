# Modern C++20 SQL Query Builder

A high-performance, minimal-heap-allocation SQL query builder for modern C++ applications. Designed with Qt6 integration in mind, this builder provides type-safe query construction with minimal runtime overhead.

## Features

- minimal-heap allocations during query construction
- Compile-time type checking
- Modern C++20 features utilization
- Qt6 type integration (QString, QDateTime)
- Fluent interface for intuitive query building
- Protection against SQL injection
- Minimal memory footprint

## Requirements

- C++20 compliant compiler (GCC 10+, Clang 12+, or MSVC 19.29+)
- Qt Core module
- CMake 3.14 or higher
- Ninja build system

## Build

```bash
git clone https://github.com/learnqtkenya/sql-query-builder.git
cd sql-query-builder

mkdir build && cd build

cmake -GNinja \
      -DCMAKE_PREFIX_PATH=<Qt Path> \
      -DCMAKE_BUILD_TYPE=Release \
      ..

ninja
```

## Basic Usage

### SELECT Queries

```cpp
using namespace sql;
using namespace std::string_view_literals;

// Simple SELECT
constexpr std::array columns = {"id"sv, "name"sv, "email"sv};
auto query = QueryBuilder()
    .select(std::span{columns})
    .from("users"sv)
    .where(Column("active"sv) == SqlValue(true))
    .build();

// SELECT with JOIN
auto query = QueryBuilder()
    .select(std::span{columns})
    .from("users"sv)
    .leftJoin("orders"sv, "users.id = orders.user_id"sv)
    .where(Column("orders.total"sv) > SqlValue(1000.0))
    .build();
```

### INSERT Queries

```cpp
auto query = QueryBuilder()
    .insert("users"sv)
    .value("name"sv, "John Doe"sv)
    .value("email"sv, "john@example.com"sv)
    .value("created_at"sv, QDateTime::currentDateTime())
    .build();
```

### UPDATE Queries

```cpp
auto query = QueryBuilder()
    .update("users"sv)
    .set("status"sv, UserStatus::Active)
    .set("updated_at"sv, QDateTime::currentDateTime())
    .where(Column("id"sv) == SqlValue(1))
    .build();
```

### DELETE Queries

```cpp
auto query = QueryBuilder()
    .deleteFrom("users"sv)
    .where(Column("status"sv) == SqlValue(UserStatus::Inactive))
    .build();
```

## Advanced Features

### Complex Conditions

```cpp
auto query = QueryBuilder()
    .select(std::span{columns})
    .from("users"sv)
    .where(
        (Column("age"sv) >= SqlValue(18)) &&
        (
            (Column("status"sv) == SqlValue(UserStatus::Active)) ||
            (Column("role"sv) == SqlValue("admin"sv))
        )
    )
    .build();
```

### Working with Qt Types

```cpp
// DateTime handling
auto startDate = QDateTime::currentDateTime().addDays(-7);
auto query = QueryBuilder()
    .select(std::span{columns})
    .from("logs"sv)
    .where(Column("created_at"sv) >= SqlValue(startDate))
    .build();

// QString with special characters
QString complexTitle = QString("Test's query with \"quotes\"");
auto query = QueryBuilder()
    .select(std::span{columns})
    .from("articles"sv)
    .where(Column("title"sv) == SqlValue(complexTitle))
    .build();
```

### Aggregations and Grouping

```cpp
constexpr std::array columns = {
    "department"sv,
    "COUNT(*) as employee_count"sv,
    "AVG(salary) as avg_salary"sv
};

auto query = QueryBuilder()
    .select(std::span{columns})
    .from("employees"sv)
    .groupBy("department"sv)
    .having("COUNT(*) > 5"sv)
    .orderBy("avg_salary"sv, false)  // DESC
    .build();
```

## Performance Considerations

- All operations are stack-based
- Fixed-size arrays for internal storage
- String views instead of string copies
- Compile-time type checking
- Minimal dynamic allocations

## Memory Layout

| Component | Size |
|-----------|------|
| Maximum Columns | 32 |
| Maximum Conditions | 16 |
| Maximum Joins | 4 |
| Maximum ORDER BY | 8 |
| Maximum GROUP BY | 8 |

## Thread Safety

The builder is designed to be thread-safe for concurrent read operations. Each instance should be used by a single thread for write operations.

## Best Practices

1. Use `string_view` literals with the `sv` suffix
2. Prefer compile-time arrays for column lists
3. Use enums for status and type fields
4. Utilize the builder's type safety features
5. Keep queries scoped to minimize resource lifetime

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Authors

- Erick Kipng'eno (@EricoDeMecha)

## Acknowledgments

- Qt team
- Modern C++ community
