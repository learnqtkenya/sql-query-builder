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
    static constexpr size_t MaxInValues = 32;
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

// Define the tables
SQL_DEFINE_TABLE(users)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(name, std::string)
SQL_DEFINE_COLUMN(email, std::string)
SQL_DEFINE_COLUMN(active, bool)
SQL_DEFINE_COLUMN(status, UserStatus)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(updated_at, std::string)
SQL_DEFINE_COLUMN(last_login, std::string)
SQL_DEFINE_COLUMN(role, std::string)
SQL_DEFINE_COLUMN(department, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(tasks)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(title, std::string)
SQL_DEFINE_COLUMN(description, std::string)
SQL_DEFINE_COLUMN(status, Priority)
SQL_DEFINE_COLUMN(assigned_to, std::string)
SQL_DEFINE_COLUMN(created_at, std::string)
SQL_DEFINE_COLUMN(due_date, std::string)
SQL_DEFINE_COLUMN(completed_at, std::string)
SQL_DEFINE_COLUMN(priority, int)
SQL_DEFINE_COLUMN(category, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(orders)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(order_date, std::string)
SQL_DEFINE_COLUMN(total, double)
SQL_DEFINE_COLUMN(status, std::string)
SQL_DEFINE_COLUMN(shipping_address, std::string)
SQL_DEFINE_COLUMN(payment_method, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(user_profiles)
SQL_DEFINE_COLUMN(user_id, int64_t)
SQL_DEFINE_COLUMN(profile_data, std::string)
SQL_DEFINE_COLUMN(avatar_url, std::string)
SQL_DEFINE_COLUMN(bio, std::string)
SQL_DEFINE_COLUMN(preferences, std::string)
SQL_END_TABLE()

SQL_DEFINE_TABLE(order_items)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(order_id, int64_t)
SQL_DEFINE_COLUMN(product_id, int64_t)
SQL_DEFINE_COLUMN(quantity, int)
SQL_DEFINE_COLUMN(price, double)
SQL_END_TABLE()

SQL_DEFINE_TABLE(products)
SQL_DEFINE_COLUMN(id, int64_t)
SQL_DEFINE_COLUMN(name, std::string)
SQL_DEFINE_COLUMN(description, std::string)
SQL_DEFINE_COLUMN(price, double)
SQL_DEFINE_COLUMN(stock, int)
SQL_DEFINE_COLUMN(category, std::string)
SQL_END_TABLE()

// Create typed tables with custom config
struct users_table_custom : public users_table {
    using config_type = MyConfig;
};

// Helper function to print section headers
void printSection(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

int main() {
    using namespace sql;
    using namespace std::string_view_literals;

    // Create tables
    const users_table users;
    const tasks_table tasks;
    const orders_table orders;
    const user_profiles_table profiles;
    const order_items_table order_items;
    const products_table products;
    const users_table_custom users_custom;

    // Default configuration with the new string_view-based interface
    {
        printSection("Default Configuration (String-based)");
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
        printSection("Default Configuration (Typed)");

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
        printSection("Custom Configuration (String-based)");
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
        printSection("Custom Configuration (Typed)");
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
        printSection("Complex Conditions (String-based)");

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
        printSection("Complex Conditions (Typed)");

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
        printSection("Complex Join Query (String-based)");

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
    {
        printSection("Complex Join Query (Typed)");

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
        printSection("Error Handling");

        auto builder = QueryBuilder();
        // Intentionally omit the table name
        auto result = builder.select(users.id, users.name).buildResult();

        if (result.hasError()) {
            std::cout << "Error: " << result.error().message << "\n";
        }
    }

    // Insert query with string interface
    {
        printSection("Insert Query (String-based)");

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
        printSection("Insert Query (Typed)");

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
        printSection("Update Query (String-based)");

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
        printSection("Update Query (Typed)");

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
        printSection("Delete Query (String-based)");

        auto query = QueryBuilder()
                         .deleteFrom("users"sv)
                         .where(col("status") == UserStatus::Inactive)
                         .build();

        std::cout << query << "\n";
    }

    // Delete query with typed interface
    {
        printSection("Delete Query (Typed)");

        auto query = QueryBuilder()
                         .deleteFrom(users.table)
                         .where(users.status == UserStatus::Inactive)
                         .build();

        std::cout << query << "\n";
    }

    // Special string handling with string interface
    {
        printSection("Special String Handling (String-based)");

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
        printSection("Special String Handling (Typed)");

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
        printSection("Qt Integration (String-based)");

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
        printSection("Qt Integration (Typed)");

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

    // New Examples - Advanced Query Features:

    // SQL Aggregate Functions
    {
        printSection("SQL Aggregate Functions");

        auto query = QueryBuilder()
                         .select(
                             count(orders.id),
                             sum(orders.total),
                             avg(orders.total),
                             max(orders.total),
                             min(orders.total))
                         .from(orders.table)
                         .where(orders.status == "completed")
                         .build();

        std::cout << query << "\n";
    }

    // Between Clause
    {
        printSection("Between Clause");

        auto query = QueryBuilder()
                         .select(orders.id, orders.user_id, orders.total)
                         .from(orders.table)
                         .whereBetween(orders.total, 100.0, 500.0)
                         .where(orders.order_date >= "2023-01-01")
                         .where(orders.order_date <= "2023-12-31")
                         .build();

        std::cout << query << "\n";
    }

    // WhereIn Clause
    {
        printSection("WhereIn Clause");

        std::array<int64_t, 3> userIds = {1, 2, 3};
        std::span<const int64_t> userIdsSpan(userIds);

        auto query = QueryBuilder()
                         .select(orders.id, orders.total)
                         .from(orders.table)
                         .whereIn(orders.user_id, userIdsSpan)
                         .build();

        std::cout << query << "\n";
    }

    // Insert or Replace
    {
        printSection("Insert or Replace");

        auto query = QueryBuilder()
                         .insertOrReplace(users.table)
                         .value(users.id, 1)
                         .value(users.name, "John Doe")
                         .value(users.email, "john@example.com")
                         .build();

        std::cout << query << "\n";
    }

    // Multiple WHERE clauses combined with AND
    {
        printSection("Multiple WHERE Clauses");

        auto query = QueryBuilder()
                         .select(users.id, users.name, users.email)
                         .from(users.table)
                         .where(users.active == true)
                         .where(users.created_at >= "2023-01-01")
                         .where(users.role == "admin")
                         .where(users.department == "IT")
                         .build();

        std::cout << query << "\n";
    }

    // Using LIKE for pattern matching
    {
        printSection("LIKE Pattern Matching");

        auto query = QueryBuilder()
                         .select(users.id, users.name, users.email)
                         .from(users.table)
                         .whereLike(users.email, "%@gmail.com")
                         .build();

        std::cout << query << "\n";
    }

    // Multi-table SELECT with subqueries
    {
        printSection("Subquery as WHERE Condition");

        // First create a subquery that will get the highest order totals
        auto subqueryResult = QueryBuilder()
                                  .select(orders.id)
                                  .from(orders.table)
                                  .where(orders.total > 1000)
                                  .build();

// Convert to std::string if using Qt
#ifdef SQLQUERYBUILDER_USE_QT
        std::string subquery = subqueryResult.toStdString();
#else
        std::string subquery = subqueryResult;
#endif

        // Then use that subquery in a whereExists clause
        auto query = QueryBuilder()
                         .select(users.id, users.name)
                         .from(users.table)
                         .whereExists("SELECT 1 FROM orders WHERE orders.user_id = users.id AND orders.id IN (" + subquery + ")")
                         .build();

        std::cout << query << "\n";
    }

    // Advanced JOIN with multiple tables
    {
        printSection("Advanced JOIN with Multiple Tables");

        auto query = QueryBuilder()
                         .select(
                             users.id, users.name,
                             orders.id, orders.total,
                             order_items.quantity,
                             products.name)
                         .from(users.table)
                         .innerJoin(orders.table, (users.id == orders.user_id).toString())
                         .innerJoin(order_items.table, (orders.id == order_items.order_id).toString())
                         .innerJoin(products.table, (order_items.product_id == products.id).toString())
                         .where(orders.total > 100)
                         .where(products.category == "Electronics")
                         .orderBy(orders.total, false)
                         .build();

        std::cout << query << "\n";
    }

    // Testing std::string integration
    {
        printSection("std::string Integration");

        std::string name = "John Smith";
        std::string email = "john.smith@example.com";
        std::string department = "Marketing";

        auto query = QueryBuilder()
                         .select(users.id, users.email)
                         .from(users.table)
                         .where(users.name == name)
                         .where(users.email == email)
                         .where(users.department == department)
                         .build();

        std::cout << query << "\n";
    }

    // Truncate table
    {
        printSection("Truncate Table");

        auto query = QueryBuilder()
                         .truncate(users.table)
                         .build();

        std::cout << query << "\n";
    }

    // Builder reuse
    {
        printSection("Builder Reuse");

        QueryBuilder builder;

        // First query
        auto query1 = builder
                          .select(users.id, users.name)
                          .from(users.table)
                          .where(users.active == true)
                          .build();
        std::cout << "Query 1: " << query1 << "\n";

        // Reset and build new query
        builder.reset();
        auto query2 = builder
                          .select(orders.id, orders.total)
                          .from(orders.table)
                          .where(orders.user_id == 42)
                          .build();
        std::cout << "Query 2: " << query2 << "\n";
    }

    // Distinct select
    {
        printSection("Distinct Select");

        auto query = QueryBuilder()
                         .distinct()
                         .select(tasks.category)
                         .from(tasks.table)
                         .build();

        std::cout << query << "\n";
    }

    // Using raw SQL in conditions
    {
        printSection("Raw SQL in Conditions");

        auto query = QueryBuilder()
                         .select(users.id, users.name)
                         .from(users.table)
                         .whereRaw("LOWER(name) LIKE '%john%'")
                         .whereRaw("DATE_PART('year', created_at) = 2023")
                         .build();

        std::cout << query << "\n";
    }

    // Using raw SQL in fields
    {
        printSection("Raw SQL in Fields");

        auto query = QueryBuilder()
                         .select(users.id, users.name, "EXTRACT(YEAR FROM created_at) AS year")
                         .from(users.table)
                         .where(users.active == true)
                         .build();

        std::cout << query << "\n";
    }

    // Using placeholders
    {
        printSection("Queries with placeholders");
        auto query = QueryBuilder()
                         .select("id"sv, "name"sv, "email"sv)
                         .from("users"sv)
                         .where(users.name == ph(":ph"))
                         .build();

        std::cout << query << std::endl;

        auto query2 = QueryBuilder()
                          .select("id"sv, "name"sv, "email"sv)
                          .from("users"sv)
                          .where(col("id") == ph("?"))
                          .where(users.email == ph("@status"))
                          .build();

        std::cout << query2 << std::endl;
    }

    return 0;
}
