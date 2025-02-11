#include "sqlquerybuilder.hpp"
#include <algorithm>
#include <utility>

namespace sql {

std::string SqlValue::toSqlString() const {
    return std::visit([]<typename T>(const T& value) -> std::string {
        if constexpr(std::is_same_v<T, std::monostate>) {
            return "NULL";
        } else if constexpr(std::is_integral_v<T>) {
            return std::to_string(value);
        } else if constexpr(std::is_floating_point_v<T>) {
            return std::to_string(value);
        } else if constexpr(std::is_same_v<T, bool>) {
            return value ? "1" : "0";
        } else if constexpr(std::is_same_v<T, std::string_view>) {
            std::string str(value);
            size_t pos = 0;
            while((pos = str.find('\'', pos)) != std::string::npos) {
                str.replace(pos, 1, "''");
                pos += 2;
            }
            return std::format("'{}'", str);
        } else if constexpr(std::is_same_v<T, QString>) {
            std::string str = value.toStdString();
            size_t pos = 0;
            while((pos = str.find('\'', pos)) != std::string::npos) {
                str.replace(pos, 1, "''");
                pos += 2;
            }
            return std::format("'{}'", str);
        } else if constexpr(std::is_same_v<T, QDateTime>) {
            return std::format("'{}'", value.toString(Qt::ISODate).toStdString());
        }
    }, storage_);
}

std::string Condition::toString() const {
    if (op_ == Op::Raw) {
        return condition_str_;
    }

    if (op_ == Op::And || op_ == Op::Or) {
        assert(left_ && right_);
        return std::format("({}) {} ({})",
                           left_->toString(),
                           op_ == Op::And ? "AND" : "OR",
                           right_->toString());
    }

    std::string result;
    if (negated_) {
        result += "NOT (";
    }

    switch (op_) {
    case Op::IsNull:
        result += std::format("{} IS NULL", column_);
        break;
    case Op::IsNotNull:
        result += std::format("{} IS NOT NULL", column_);
        break;
    case Op::Between:
        if (value_count_ == 2) {
            result += std::format("{} BETWEEN {} AND {}",
                                  column_,
                                  values_[0].toSqlString(),
                                  values_[1].toSqlString());
        }
        break;
    default:
        const char* op_str = "";
        switch (op_) {
        case Op::Eq: op_str = "="; break;
        case Op::Ne: op_str = "!="; break;
        case Op::Lt: op_str = "<"; break;
        case Op::Le: op_str = "<="; break;
        case Op::Gt: op_str = ">"; break;
        case Op::Ge: op_str = ">="; break;
        case Op::Like: op_str = "LIKE"; break;
        case Op::NotLike: op_str = "NOT LIKE"; break;
        default: break;
        }
        result += std::format("{} {} {}",
                              column_, op_str, values_[0].toSqlString());
    }

    if (negated_) {
        result += ")";
    }

    return result;
}

std::string Join::toString() const {
    const char* type_str = "";
    switch (type_) {
    case Type::Inner: type_str = "INNER JOIN"; break;
    case Type::Left:  type_str = "LEFT JOIN"; break;
    case Type::Right: type_str = "RIGHT JOIN"; break;
    case Type::Full:  type_str = "FULL JOIN"; break;
    case Type::Cross: type_str = "CROSS JOIN"; break;
    }

    return std::format("{} {} ON {}", type_str, table_, condition_);
}

QueryBuilder& QueryBuilder::select(std::span<const std::string_view> cols) {
    type_ = QueryType::Select;
    select_columns_.count = std::min(cols.size(), MAX_COLUMNS);
    std::copy_n(cols.begin(), select_columns_.count, select_columns_.columns.begin());
    return *this;
}

QueryBuilder& QueryBuilder::from(std::string_view table) {
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::where(const Condition& condition) {
    if (where_.count < MAX_CONDITIONS) {
        where_.conditions[where_.count++] = condition;
    }
    return *this;
}

QueryBuilder& QueryBuilder::orderBy(std::string_view column, bool asc) {
    if (order_by_.count < MAX_ORDER_BY) {
        order_by_.columns[order_by_.count++] = {column, asc};
    }
    return *this;
}

QueryBuilder& QueryBuilder::limit(int32_t limit) {
    limit_ = limit;
    return *this;
}

QueryBuilder& QueryBuilder::offset(int32_t offset) {
    offset_ = offset;
    return *this;
}

QueryBuilder& QueryBuilder::distinct() {
    distinct_ = true;
    return *this;
}

QueryBuilder& QueryBuilder::insert(std::string_view table) {
    type_ = QueryType::Insert;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::update(std::string_view table) {
    type_ = QueryType::Update;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::deleteFrom(std::string_view table) {
    type_ = QueryType::Delete;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::truncate(std::string_view table) {
    type_ = QueryType::Truncate;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::innerJoin(std::string_view table, std::string_view condition) {
    if (joins_.count < MAX_JOINS) {
        joins_.joins[joins_.count++] = Join(Join::Type::Inner, table, condition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::leftJoin(std::string_view table, std::string_view condition) {
    if (joins_.count < MAX_JOINS) {
        joins_.joins[joins_.count++] = Join(Join::Type::Left, table, condition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::rightJoin(std::string_view table, std::string_view condition) {
    if (joins_.count < MAX_JOINS) {
        joins_.joins[joins_.count++] = Join(Join::Type::Right, table, condition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::fullJoin(std::string_view table, std::string_view condition) {
    if (joins_.count < MAX_JOINS) {
        joins_.joins[joins_.count++] = Join(Join::Type::Full, table, condition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereIn(std::string_view column, std::span<const SqlValue> values) {
    if (where_.count < MAX_CONDITIONS) {
        std::string condition = std::string(column) + " IN (";
        bool first = true;
        for (const auto& value : values) {
            if (!first) condition += ", ";
            condition += value.toSqlString();
            first = false;
        }
        condition += ")";
        where_.conditions[where_.count++] = Condition(condition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereBetween(std::string_view column, const SqlValue& start, const SqlValue& end) {
    if (where_.count < MAX_CONDITIONS) {
        where_.conditions[where_.count++] = Condition(column, Condition::Op::Between, start, end);
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereLike(std::string_view column, std::string_view pattern) {
    if (where_.count < MAX_CONDITIONS) {
        where_.conditions[where_.count++] = Condition(column, Condition::Op::Like, SqlValue(pattern));
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereNull(std::string_view column) {
    if (where_.count < MAX_CONDITIONS) {
        where_.conditions[where_.count++] = Condition(column, Condition::Op::IsNull, SqlValue());
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereNotNull(std::string_view column) {
    if (where_.count < MAX_CONDITIONS) {
        where_.conditions[where_.count++] = Condition(column, Condition::Op::IsNotNull, SqlValue());
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereExists(std::string_view subquery) {
    if (where_.count < MAX_CONDITIONS) {
        std::string condition = std::format("EXISTS ({})", subquery);
        where_.conditions[where_.count++] = Condition(condition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereRaw(std::string_view rawCondition) {
    if (where_.count < MAX_CONDITIONS) {
        where_.conditions[where_.count++] = Condition(rawCondition);
    }
    return *this;
}

QueryBuilder& QueryBuilder::groupBy(std::string_view column) {
    if (group_by_.count < MAX_GROUP_BY) {
        group_by_.columns[group_by_.count++] = column;
    }
    return *this;
}

QueryBuilder& QueryBuilder::having(std::string_view condition) {
    having_ = condition;
    return *this;
}

std::string QueryBuilder::build() const {
    std::string query;
    query.reserve(1024); // Reasonable initial size

    switch (type_) {
    case QueryType::Select:
        buildSelect(query);
        break;
    case QueryType::Insert:
        buildInsert(query);
        break;
    case QueryType::Update:
        buildUpdate(query);
        break;
    case QueryType::Delete:
        buildDelete(query);
        break;
    case QueryType::Truncate:
        buildTruncate(query);
        break;
    }

    return query;
}

void QueryBuilder::buildSelect(std::string& query) const {
    query += "SELECT ";
    if (distinct_) query += "DISTINCT ";

    if (select_columns_.count == 0) {
        query += "*";
    } else {
        bool first = true;
        for (size_t i = 0; i < select_columns_.count; ++i) {
            if (!first) query += ", ";
            query += std::string(select_columns_.columns[i]);
            first = false;
        }
    }

    query += " FROM ";
    query += table_;

    // Joins
    for (size_t i = 0; i < joins_.count; ++i) {
        query += ' ';
        query += joins_.joins[i].toString();
    }

    // Where
    if (where_.count > 0) {
        query += " WHERE ";
        for (size_t i = 0; i < where_.count; ++i) {
            if (i > 0) query += " AND ";
            query += where_.conditions[i].toString();
        }
    }

    // Group By
    if (group_by_.count > 0) {
        query += " GROUP BY ";
        bool first = true;
        for (size_t i = 0; i < group_by_.count; ++i) {
            if (!first) query += ", ";
            query += group_by_.columns[i];
            first = false;
        }

        if (!having_.empty()) {
            query += " HAVING ";
            query += having_;
        }
    }

    // Order By
    if (order_by_.count > 0) {
        query += " ORDER BY ";
        bool first = true;
        for (size_t i = 0; i < order_by_.count; ++i) {
            if (!first) query += ", ";
            query += order_by_.columns[i].first;
            query += order_by_.columns[i].second ? " ASC" : " DESC";
            first = false;
        }
    }

    if (limit_ >= 0) {
        query += std::format(" LIMIT {}", limit_);
    }

    if (offset_ >= 0) {
        query += std::format(" OFFSET {}", offset_);
    }
}

void QueryBuilder::buildInsert(std::string& query) const {
    query += "INSERT INTO ";
    query += table_;
    query += " (";

    bool first = true;
    for (size_t i = 0; i < values_.count; ++i) {
        if (!first) query += ", ";
        query += values_.values[i].first;
        first = false;
    }

    query += ") VALUES (";

    first = true;
    for (size_t i = 0; i < values_.count; ++i) {
        if (!first) query += ", ";
        query += values_.values[i].second.toSqlString();
        first = false;
    }

    query += ")";
}

void QueryBuilder::buildUpdate(std::string& query) const {
    query += "UPDATE ";
    query += table_;
    query += " SET ";

    bool first = true;
    for (size_t i = 0; i < values_.count; ++i) {
        if (!first) query += ", ";
        query += values_.values[i].first;
        query += " = ";
        query += values_.values[i].second.toSqlString();
        first = false;
    }

    if (where_.count > 0) {
        query += " WHERE ";
        for (size_t i = 0; i < where_.count; ++i) {
            if (i > 0) query += " AND ";
            query += where_.conditions[i].toString();
        }
    }
}

void QueryBuilder::buildDelete(std::string& query) const {
    query += "DELETE FROM ";
    query += table_;

    if (where_.count > 0) {
        query += " WHERE ";
        for (size_t i = 0; i < where_.count; ++i) {
            if (i > 0) query += " AND ";
            query += where_.conditions[i].toString();
        }
    }
}

void QueryBuilder::buildTruncate(std::string& query) const {
    query += "TRUNCATE TABLE ";
    query += table_;
}

} // namespace sql
