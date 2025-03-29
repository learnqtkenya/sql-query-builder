#define SQLQUERYBUILDER_USE_QT
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

SQL_DEFINE_TABLE(users)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(name, std::string)
SQL_DEFINE_COLUMN(email, std::string)
SQL_DEFINE_COLUMN(active, bool)
SQL_DEFINE_COLUMN(status, UserStatus)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(tasks)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(title, std::string)
SQL_DEFINE_COLUMN(status, Priority)
SQL_DEFINE_COLUMN(assigned_to, std::string)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(priority, int)
SQL_END_TABLE()

SQL_DEFINE_TABLE(orders)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(total, double)
SQL_END_TABLE()

SQL_DEFINE_TABLE(user_profiles)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(profile_data, std::string)
SQL_END_TABLE()

// Create typed tables with custom config
struct users_table_custom : public users_table {
    using config_type = MyConfig;
};

int main() {
    using namespace sql;
    using namespace std::string_view_literals;

    // Create tables
    const users_table users;
    const tasks_table tasks;
    const orders_table orders;
    const user_profiles_table profiles;
    const users_table_custom users_custom;

    // Default configuration with the new string_view-based interface
    {
        std::cout << "\n=== Default Configuration (String-based) ===\n";
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

    // Default configuration with the new typed column interface
    {
        std::cout << "\n=== Default Configuration (Typed) ===\n";

        auto query = QueryBuilder()
                         .select(users.id, users.name, users.email)
                         .from(users.table)
                         .where(users.active == true)
                         .orderBy(users.created_at, false)
                         .limit(10)
                         .build();

        std::cout << query << "\n";
    }

    // Custom configuration with string interface
    {
        std::cout << "\n=== Custom Configuration (String-based) ===\n";
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

    // Custom configuration with typed interface
    {
        std::cout << "\n=== Custom Configuration (Typed) ===\n";
        using CustomBuilder = QueryBuilder<MyConfig>;

        auto query = CustomBuilder()
                         .select(users.id, users.name, users.email)
                         .from(users.table)
                         .where(users.status == UserStatus::Active)
                         .orderBy(users.created_at, false)
                         .limit(10)
                         .build();

        std::cout << query << "\n";
    }

    // Complex conditions with string interface
    {
        std::cout << "\n=== Complex Conditions (String-based) ===\n";

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

    // Complex conditions with typed interface
    {
        std::cout << "\n=== Complex Conditions (Typed) ===\n";

        auto query = QueryBuilder()
                         .select(all_of(tasks.table))
                         .from(tasks.table)
                         .where(
                             (tasks.status == Priority::High) &&
                             (
                                 (tasks.assigned_to == "admin"sv) ||
                                 (tasks.created_at >= "2023-01-01"sv)
                                 )
                             )
                         .orderBy(tasks.priority, false)
                         .build();

        std::cout << query << "\n";
    }

    // Complex join with string interface
    {
        std::cout << "\n=== Complex Join Query (String-based) ===\n";

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

    // Complex join with typed interface
    // Note: In a real setup you'd use aliased tables, but for simplicity we'll use the basic table names
    {
        std::cout << "\n=== Complex Join Query (Typed) ===\n";

        // We need to manually build the condition strings since we don't have aliased tables
        std::string join_condition1 = (users.id == orders.user_id).toString();
        std::string join_condition2 = (users.id == profiles.user_id).toString();
        std::string having_condition = "COUNT(orders.id) > 5";

        auto query = QueryBuilder()
                         .select(users.id, users.name, "COUNT(orders.id) as order_count"sv)
                         .from(users.table)
                         .leftJoin(orders.table, join_condition1)
                         .innerJoin(profiles.table, join_condition2)
                         .where(users.status == UserStatus::Active)
                         .whereNotNull(users.email)
                         .groupBy(users.id)
                         .groupBy(users.name)
                         .having(having_condition)
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
        auto result = builder.select(users.id, users.name).buildResult();

        if (result.hasError()) {
            std::cout << "Error: " << result.error().message << "\n";
        }
    }

    // Insert query with string interface
    {
        std::cout << "\n=== Insert Query (String-based) ===\n";

        auto query = QueryBuilder()
                         .insert("users"sv)
                         .value("name"sv, "John Doe"sv)
                         .value("email"sv, "john@example.com"sv)
                         .value("active"sv, true)
                         .value("status"sv, UserStatus::Active)
                         .build();

        std::cout << query << "\n";
    }

    // Insert query with typed interface
    {
        std::cout << "\n=== Insert Query (Typed) ===\n";

        auto query = QueryBuilder()
                         .insert(users.table)
                         .value(users.name, "John Doe"sv)
                         .value(users.email, "john@example.com"sv)
                         .value(users.active, true)
                         .value(users.status, UserStatus::Active)
                         .build();

        std::cout << query << "\n";
    }

    // Update query with string interface
    {
        std::cout << "\n=== Update Query (String-based) ===\n";

        auto query = QueryBuilder()
                         .update("users"sv)
                         .set("name"sv, "Jane Doe"sv)
                         .set("status"sv, UserStatus::Active)
                         .where(col("id") == 42)
                         .build();

        std::cout << query << "\n";
    }

    // Update query with typed interface
    {
        std::cout << "\n=== Update Query (Typed) ===\n";

        auto query = QueryBuilder()
                         .update(users.table)
                         .set(users.name, "Jane Doe"sv)
                         .set(users.status, UserStatus::Active)
                         .where(users.id == 42)
                         .build();

        std::cout << query << "\n";
    }

    // Delete query with string interface
    {
        std::cout << "\n=== Delete Query (String-based) ===\n";

        auto query = QueryBuilder()
                         .deleteFrom("users"sv)
                         .where(col("status") == UserStatus::Inactive)
                         .build();

        std::cout << query << "\n";
    }

    // Delete query with typed interface
    {
        std::cout << "\n=== Delete Query (Typed) ===\n";

        auto query = QueryBuilder()
                         .deleteFrom(users.table)
                         .where(users.status == UserStatus::Inactive)
                         .build();

        std::cout << query << "\n";
    }

    // Special string handling with string interface
    {
        std::cout << "\n=== Special String Handling (String-based) ===\n";

        std::string title = "Test's query with \"quotes\" and other's special chars";

        auto query = QueryBuilder()
                         .select("id"sv, "title"sv)
                         .from("tasks"sv)
                         .where(col("title") == title)
                         .build();

        std::cout << query << "\n";
    }

    // Special string handling with typed interface
    {
        std::cout << "\n=== Special String Handling (Typed) ===\n";

        std::string title = "Test's query with \"quotes\" and other's special chars";

        auto query = QueryBuilder()
                         .select(tasks.id, tasks.title)
                         .from(tasks.table)
                         .where(tasks.title == title)
                         .build();

        std::cout << query << "\n";
    }

#ifdef SQLQUERYBUILDER_USE_QT
    // Qt integration with string interface
    {
        std::cout << "\n=== Qt Integration (String-based) ===\n";

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

    // Qt integration with typed interface
    {
        std::cout << "\n=== Qt Integration (Typed) ===\n";

        QDateTime startDate = QDateTime::currentDateTime().addDays(-7);
        QString complexTitle = QString("Test's query with \"quotes\" and other's special chars");

        auto query = QueryBuilder()
                         .select(tasks.id, tasks.title, tasks.created_at)
                         .from(tasks.table)
                         .where((tasks.created_at >= startDate) &&
                                (tasks.title == complexTitle))
                         .orderBy(tasks.created_at)
                         .build();

        std::cout << query << "\n";
    }
#endif

    return 0;
}
