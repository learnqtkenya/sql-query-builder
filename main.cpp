#include "sqlquerybuilder.hpp"
#include <iostream>
#include <string>
#include <array>

// Optional: Define custom config
struct MyConfig {
    static constexpr size_t MaxColumns = 64;
    static constexpr size_t MaxConditions = 32;
    static constexpr size_t MaxJoins = 8;
    static constexpr size_t MaxOrderBy = 16;
    static constexpr size_t MaxGroupBy = 16;
    static constexpr bool ThrowOnError = true;
};

// Example enum types
enum class UserStatus : int {
    Active = 1,
    Inactive = 0,
    Pending = 2
};

enum class Priority : int {
    Low = 0,
    Medium = 1,
    High = 2,
    Critical = 3
};

int main() {
    using namespace sql;
    using namespace std::string_view_literals;

    // Default configuration
    {
        std::cout << "\n=== Default Configuration ===\n";
        constexpr std::array columns = {"id"sv, "name"sv, "email"sv};

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("users"sv)
                         .where(col("active") == true)
                         .orderBy("created_at"sv, false)
                         .limit(10)
                         .build();

        std::cout << query << "\n";
    }

    // Custom configuration
    {
        std::cout << "\n=== Custom Configuration ===\n";
        using CustomBuilder = QueryBuilder<MyConfig>;

        auto query = CustomBuilder()
                         .select("id"sv, "name"sv, "email"sv)
                         .from("users"sv)
                         .where(col<MyConfig>("status") == UserStatus::Active)
                         .orderBy("created_at"sv, false)
                         .limit(10)
                         .build();

        std::cout << query << "\n";
    }

    // Complex conditions
    {
        std::cout << "\n=== Complex Conditions ===\n";

        auto query = QueryBuilder()
                         .select("*"sv)
                         .from("tasks"sv)
                         .where(
                             (col("status") == Priority::High) &&
                             (
                                 (col("assigned_to") == "admin"sv) ||
                                 (col("created_at") >= "2023-01-01"sv)
                                 )
                             )
                         .orderBy("priority"sv, false)
                         .build();

        std::cout << query << "\n";
    }

    // Complex join with multiple conditions
    {
        std::cout << "\n=== Complex Join Query ===\n";

        auto query = QueryBuilder()
                         .select("u.id"sv, "u.name"sv, "COUNT(o.id) as order_count"sv)
                         .from("users u"sv)
                         .leftJoin("orders o"sv, "u.id = o.user_id"sv)
                         .innerJoin("user_profiles up"sv, "u.id = up.user_id"sv)
                         .where(col("u.status") == UserStatus::Active)
                         .whereNotNull("u.email"sv)
                         .groupBy("u.id"sv)
                         .groupBy("u.name"sv)
                         .having("COUNT(o.id) > 5"sv)
                         .orderBy("order_count"sv, false)
                         .limit(100)
                         .build();

        std::cout << query << "\n";
    }

    // Error handling
    {
        std::cout << "\n=== Error Handling ===\n";

        auto builder = QueryBuilder();
        // Intentionally omit the table name
        auto result = builder.select("id"sv, "name"sv).buildResult();

        if (result.hasError()) {
            std::cout << "Error: " << result.error().message << "\n";
        }
    }

    // Insert query
    {
        std::cout << "\n=== Insert Query ===\n";

        auto query = QueryBuilder()
                         .insert("users"sv)
                         .value("name"sv, "John Doe"sv)
                         .value("email"sv, "john@example.com"sv)
                         .value("active"sv, true)
                         .value("status"sv, UserStatus::Active)
                         .build();

        std::cout << query << "\n";
    }

    // Update query
    {
        std::cout << "\n=== Update Query ===\n";

        auto query = QueryBuilder()
                         .update("users"sv)
                         .set("name"sv, "Jane Doe"sv)
                         .set("status"sv, UserStatus::Active)
                         .where(col("id") == 42)
                         .build();

        std::cout << query << "\n";
    }

    // Delete query
    {
        std::cout << "\n=== Delete Query ===\n";

        auto query = QueryBuilder()
                         .deleteFrom("users"sv)
                         .where(col("status") == UserStatus::Inactive)
                         .build();

        std::cout << query << "\n";
    }

    // Special string handling
    {
        std::cout << "\n=== Special String Handling ===\n";

        std::string title = "Test's query with \"quotes\" and other's special chars";

        auto query = QueryBuilder()
                         .select("id"sv, "title"sv)
                         .from("tasks"sv)
                         .where(col("title") == title)
                         .build();

        std::cout << query << "\n";
    }

#ifdef SQLQUERYBUILDER_USE_QT
    // Qt integration (only if Qt support is enabled)
    {
        std::cout << "\n=== Qt Integration ===\n";

        QDateTime startDate = QDateTime::currentDateTime().addDays(-7);
        QString complexTitle = QString("Test's query with \"quotes\" and other's special chars");

        auto query = QueryBuilder()
                         .select("id"sv, "title"sv, "created_at"sv)
                         .from("tasks"sv)
                         .where((col("created_at") >= startDate) &&
                                (col("title") == complexTitle))
                         .orderBy("created_at"sv)
                         .build();

        std::cout << query << "\n";
    }
#endif

    return 0;
}
