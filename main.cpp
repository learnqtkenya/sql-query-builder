#include <QCoreApplication>
#include <QDebug>
#include <QTimeZone>
#include "sqlquerybuilder.hpp"

using namespace sql;
using namespace std::string_view_literals;

// Enum for demonstration
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
enum class TaskStatus : int {
    Pending = 0,
    InProgress = 1,
    Completed = 2,
    Failed = 3
};

void printQuery(const std::string& description, const std::string& query) {
    qDebug() << "\n---" << description.c_str() << "---";
    qDebug().noquote() << QString::fromStdString(query);
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // 1. Basic SELECT query
    {
        constexpr std::array columns = {"id"sv, "name"sv, "email"sv};
        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("users"sv)
                         .build();
        printQuery("Basic SELECT", query);
    }

    // 2. SELECT with WHERE clause
    {
        constexpr std::array columns = {"id"sv, "name"sv};
        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("users"sv)
                         .where(Column("active"sv) == SqlValue(true))
                         .build();
        printQuery("SELECT with WHERE", query);
    }

    // 3. SELECT with multiple conditions
    {
        constexpr std::array columns = {"id"sv, "name"sv};
        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("users"sv)
                         .where((Column("age"sv) >= SqlValue(18)) &&
                                (Column("status"sv) == SqlValue(UserStatus::Active)))
                         .build();
        printQuery("SELECT with multiple conditions", query);
    }

    // 4. SELECT with ORDER BY and LIMIT
    {
        constexpr std::array columns = {"id"sv, "name"sv, "created_at"sv};
        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("users"sv)
                         .orderBy("created_at"sv, false) // DESC
                         .limit(10)
                         .offset(20)
                         .build();
        printQuery("SELECT with ORDER BY and LIMIT", query);
    }

    // 5. Complex JOIN query
    {
        constexpr std::array columns = {
            "users.name"sv,
            "orders.id"sv,
            "orders.total"sv,
            "order_items.product_name"sv
        };

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("users"sv)
                         .leftJoin("orders"sv, "users.id = orders.user_id"sv)
                         .innerJoin("order_items"sv, "orders.id = order_items.order_id"sv)
                         .where(Column("orders.total"sv) > SqlValue(1000.0))
                         .groupBy("users.id"sv)
                         .having("COUNT(orders.id) > 5"sv)
                         .orderBy("users.name"sv)
                         .build();
        printQuery("Complex JOIN", query);
    }

    // 6. Most complex query example
    {
        constexpr std::array subQueryCols = {"user_id"sv};
        auto subqueryStr = QueryBuilder()
                               .select(std::span{subQueryCols})
                               .from("premium_subscriptions"sv)
                               .where(Column("status"sv) == SqlValue(true))
                               .build();

        constexpr std::array columns = {
            "u.id"sv,
            "u.name"sv,
            "COUNT(o.id) as order_count"sv,
            "SUM(o.total) as total_spent"sv
        };

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .distinct()
                         .from("users u"sv)
                         .leftJoin("orders o"sv, "u.id = o.user_id"sv)
                         .innerJoin("user_profiles up"sv, "u.id = up.user_id"sv)
                         .where((Column("u.status"sv) == SqlValue(UserStatus::Active)) &&
                                (Column("u.created_at"sv) >= SqlValue(QDateTime::currentDateTime().addYears(-1))))
                         .whereExists(subqueryStr)
                         .whereNotNull("u.email"sv)
                         .groupBy("u.id"sv)
                         .having("COUNT(o.id) > 10"sv)
                         .orderBy("total_spent"sv, false)
                         .limit(100)
                         .build();

        printQuery("Complex Combined Query", query);
    }

    //************************* With  Qt Types ****************************//

    // Test 1: DateTime ranges with timezone
    {
        auto now = QDateTime::currentDateTime();
        auto utc = QTimeZone(QByteArray("UTC"));
        auto startDate = now.addDays(-7).toTimeZone(utc);
        auto endDate = now.toTimeZone(utc);

        constexpr std::array columns = {"task_id"sv, "title"sv, "created_at"sv};
        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("tasks"sv)
                         .where((Column("created_at"sv) >= SqlValue(startDate)) &&
                                (Column("created_at"sv) <= SqlValue(endDate)))
                         .orderBy("created_at"sv)
                         .build();

        printQuery("DateTime Range Query", query);
    }

    // Test 2: Complex status and enum combinations
    {
        constexpr std::array columns = {
            "t.id"sv,
            "t.title"sv,
            "t.status"sv,
            "t.priority"sv,
            "u.name as assigned_to"sv
        };

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("tasks t"sv)
                         .leftJoin("users u"sv, "t.assigned_user_id = u.id"sv)
                         .where((Column("t.status"sv) == SqlValue(TaskStatus::InProgress)) &&
                                (Column("t.priority"sv) >= SqlValue(Priority::High)))
                         .orderBy("t.priority"sv, false)  // DESC
                         .orderBy("t.created_at"sv, true) // ASC
                         .build();

        printQuery("Enum and Status Query", query);
    }

    // Test 3: Date grouping and aggregation
    {
        constexpr std::array columns = {
            "DATE(created_at) as date"sv,
            "status"sv,
            "COUNT(*) as count"sv,
            "AVG(completion_time) as avg_completion"sv
        };

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("tasks"sv)
                         .where(Column("created_at"sv) >= SqlValue(QDateTime::currentDateTime().addMonths(-1)))
                         .groupBy("DATE(created_at)"sv)
                         .groupBy("status"sv)
                         .having("COUNT(*) > 5"sv)
                         .build();

        printQuery("Date Aggregation Query", query);
    }

    // Test 4: Nested conditions with multiple types
    {
        auto deadline = QDateTime::currentDateTime().addDays(7);
        constexpr std::array columns = {"*"sv};

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("tasks"sv)
                         .where(
                             (Column("status"sv) == SqlValue(TaskStatus::InProgress)) &&
                             (
                                 (Column("priority"sv) == SqlValue(Priority::Critical)) ||
                                 (
                                     (Column("priority"sv) >= SqlValue(Priority::High)) &&
                                     (Column("deadline"sv) <= SqlValue(deadline))
                                     )
                                 )
                             )
                         .build();

        printQuery("Complex Nested Conditions", query);
    }

    // Test 5: Multiple joins with datetime and enum conditions
    {
        constexpr std::array columns = {
            "t.id"sv,
            "t.title"sv,
            "t.status"sv,
            "u.name as assigned_to"sv,
            "d.name as department"sv,
            "COUNT(c.id) as comments_count"sv
        };

        auto lastWeek = QDateTime::currentDateTime().addDays(-7);

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .distinct()
                         .from("tasks t"sv)
                         .innerJoin("users u"sv, "t.assigned_user_id = u.id"sv)
                         .innerJoin("departments d"sv, "u.department_id = d.id"sv)
                         .leftJoin("comments c"sv, "t.id = c.task_id"sv)
                         .where(
                             (Column("t.status"sv) != SqlValue(TaskStatus::Completed)) &&
                             (Column("t.created_at"sv) >= SqlValue(lastWeek)) &&
                             (Column("t.priority"sv) >= SqlValue(Priority::High))
                             )
                         .groupBy("t.id"sv)
                         .groupBy("t.title"sv)
                         .groupBy("t.status"sv)
                         .groupBy("u.name"sv)
                         .groupBy("d.name"sv)
                         .having("COUNT(c.id) > 0"sv)
                         .orderBy("t.priority"sv, false)
                         .orderBy("comments_count"sv, false)
                         .build();

        printQuery("Complex Join with DateTime and Enums", query);
    }

    // Test 6: Test with QString containing special characters
    {
        QString complexTitle = QString("Test's query with \"quotes\" and other's special chars");
        constexpr std::array columns = {"id"sv, "title"sv};

        auto query = QueryBuilder()
                         .select(std::span{columns})
                         .from("tasks"sv)
                         .where(Column("title"sv) == SqlValue(complexTitle))
                         .build();

        printQuery("Special Characters Handling", query);
    }

    return a.exec();
}
