#include <QCoreApplication>
#include <QDebug>
#include "sqlquerybuilder.hpp"

using namespace sql;
using namespace std::string_view_literals;

// Enum for demonstration
enum class UserStatus : int {
    Active = 1,
    Inactive = 0,
    Pending = 2
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

    // Most complex query example
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

    return a.exec();
}
