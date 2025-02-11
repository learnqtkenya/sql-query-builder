#pragma once

#include <array>
#include <span>
#include <string_view>
#include <concepts>
#include <variant>
#include <QDateTime>
#include <QString>
#include <format>

namespace sql {

// Configuration
constexpr size_t MAX_COLUMNS = 32;
constexpr size_t MAX_CONDITIONS = 16;
constexpr size_t MAX_JOINS = 4;
constexpr size_t MAX_ORDER_BY = 8;
constexpr size_t MAX_GROUP_BY = 8;

// Concepts for type safety
template<typename T>
concept SqlCompatible = std::same_as<T, int> ||
                        std::same_as<T, int64_t> ||
                        std::same_as<T, double> ||
                        std::same_as<T, bool> ||
                        std::same_as<T, std::string_view> ||
                        std::same_as<T, QString> ||
                        std::same_as<T, QDateTime> ||
                        std::is_enum_v<T>;

// Forward declarations
class SqlValue;
class Condition;


// Value type handling
class SqlValue {
    using StorageType = std::variant<
        std::monostate,  // For NULL
        int64_t,
        double,
        bool,
        std::string_view,
        QString,
        QDateTime
        >;

    StorageType storage_;

public:
    SqlValue() = default;

    template<SqlCompatible T>
    explicit SqlValue(T value) {
        if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            storage_ = static_cast<int64_t>(value);
        } else if constexpr(std::is_floating_point_v<T>) {
            storage_ = static_cast<double>(value);
        } else if constexpr(std::is_same_v<T, bool>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<T, std::string_view>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<T, QString>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<T, QDateTime>) {
            storage_ = value;
        } else if constexpr(std::is_enum_v<T>) {
            storage_ = static_cast<int64_t>(value);
        }
    }

    [[nodiscard]] std::string toSqlString() const;
    [[nodiscard]] bool isNull() const { return storage_.index() == 0; }
};

// Condition building
class Condition {
public:
    enum class Op : uint8_t {
        Eq, Ne, Lt, Le, Gt, Ge,
        IsNull, IsNotNull,
        In, NotIn,
        Like, NotLike,
        Between,
        Raw
    };

private:
    std::string_view column_;
    Op op_;
    std::array<SqlValue, 2> values_;
    size_t value_count_{0};
    bool negated_{false};

public:
    // Default constructor for array initialization
    Condition() : column_(""), op_(Op::Raw), value_count_(0) {}

    // Constructor for raw conditions
    explicit Condition(std::string_view raw_condition)
        : column_(raw_condition), op_(Op::Raw), value_count_(0) {}

    // Constructor for regular conditions
    Condition(std::string_view col, Op op, SqlValue value)
        : column_(col), op_(op), value_count_(1) {
        values_[0] = value;
    }

    // Constructor for between conditions
    Condition(std::string_view col, Op op, SqlValue value1, SqlValue value2)
        : column_(col), op_(op), value_count_(2) {
        values_[0] = value1;
        values_[1] = value2;
    }

    [[nodiscard]] std::string toString() const;

    Condition operator!() const {
        auto copy = *this;
        copy.negated_ = !negated_;
        return copy;
    }
    Condition operator&&(const Condition& other) const {
        return Condition(std::string(column_) + " AND " + std::string(other.column_));
    }

    Condition operator||(const Condition& other) const {
        return Condition(std::string(column_) + " OR " + std::string(other.column_));
    }
};

// column
class Column {
    std::string_view name_;

public:
    explicit Column(std::string_view name) : name_(name) {}
    [[nodiscard]] std::string_view name() const { return name_; }

    [[nodiscard]] Condition isNull() const {
        return Condition(name_, Condition::Op::IsNull, SqlValue());
    }

    [[nodiscard]] Condition isNotNull() const {
        return Condition(name_, Condition::Op::IsNotNull, SqlValue());
    }
};

// Join clause
class Join {
public:
    enum class Type : uint8_t { Inner, Left, Right, Full, Cross };

private:
    Type type_;
    std::string_view table_;
    std::string_view condition_;

public:
    // Default constructor for array initialization
    Join() : type_(Type::Inner), table_(""), condition_("") {}

    Join(Type type, std::string_view table, std::string_view condition)
        : type_(type), table_(table), condition_(condition) {}

    [[nodiscard]] std::string toString() const;
};

// Main query builder
class QueryBuilder {
public:
    enum class QueryType : uint8_t {
        Select, Insert, Update, Delete, Truncate
    };

private:
    QueryType type_{QueryType::Select};
    std::string_view table_;

    struct {
        std::array<std::string_view, MAX_COLUMNS> columns{};
        size_t count{0};
    } select_columns_;

    struct {
        std::array<std::pair<std::string_view, SqlValue>, MAX_COLUMNS> values{};
        size_t count{0};
    } values_;

    struct {
        std::array<Condition, MAX_CONDITIONS> conditions{};
        size_t count{0};
    } where_;

    struct {
        std::array<Join, MAX_JOINS> joins{};
        size_t count{0};
    } joins_;

    struct {
        std::array<std::pair<std::string_view, bool>, MAX_ORDER_BY> columns{};
        size_t count{0};
    } order_by_;

    struct {
        std::array<std::string_view, MAX_GROUP_BY> columns{};
        size_t count{0};
    } group_by_;

    std::string_view having_;
    int32_t limit_{-1};
    int32_t offset_{-1};
    bool distinct_{false};

public:
    // Builder methods
    QueryBuilder& select(std::span<const std::string_view> cols);
    QueryBuilder& from(std::string_view table);
    QueryBuilder& where(const Condition& condition);
    QueryBuilder& orderBy(std::string_view column, bool asc = true);
    QueryBuilder& limit(int32_t limit);
    QueryBuilder& offset(int32_t offset);
    QueryBuilder& distinct();

    QueryBuilder& insert(std::string_view table);
    template<SqlCompatible T>
    QueryBuilder& value(std::string_view column, T&& val) {
        if (values_.count < MAX_COLUMNS) {
            values_.values[values_.count++] = {
                column,
                SqlValue(std::forward<T>(val))
            };
        }
        return *this;
    }

    QueryBuilder& update(std::string_view table);
    template<SqlCompatible T>
    QueryBuilder& set(std::string_view column, T&& val) {
        if (values_.count < MAX_COLUMNS) {
            values_.values[values_.count++] = {
                column,
                SqlValue(std::forward<T>(val))
            };
        }
        return *this;
    }

    QueryBuilder& deleteFrom(std::string_view table);
    QueryBuilder& truncate(std::string_view table);

    // Join methods
    QueryBuilder& innerJoin(std::string_view table, std::string_view condition);
    QueryBuilder& leftJoin(std::string_view table, std::string_view condition);
    QueryBuilder& rightJoin(std::string_view table, std::string_view condition);
    QueryBuilder& fullJoin(std::string_view table, std::string_view condition);

    // Where clause variations
    QueryBuilder& whereIn(std::string_view column, std::span<const SqlValue> values);
    QueryBuilder& whereBetween(std::string_view column, const SqlValue& start, const SqlValue& end);
    QueryBuilder& whereLike(std::string_view column, std::string_view pattern);
    QueryBuilder& whereNull(std::string_view column);
    QueryBuilder& whereNotNull(std::string_view column);
    QueryBuilder& whereExists(std::string_view subquery);
    QueryBuilder& whereRaw(std::string_view rawCondition);

    // Additional features
    QueryBuilder& groupBy(std::string_view column);
    QueryBuilder& having(std::string_view condition);
    QueryBuilder& with(std::string_view name, const QueryBuilder& subquery);

    [[nodiscard]] std::string build() const;

private:
    void buildSelect(std::string& query) const;
    void buildInsert(std::string& query) const;
    void buildUpdate(std::string& query) const;
    void buildDelete(std::string& query) const;
    void buildTruncate(std::string& query) const;
};

// Operator overloads for conditions
inline Condition operator==(const Column& col, const SqlValue& val) {
    return Condition(col.name(), Condition::Op::Eq, val);
}

inline Condition operator!=(const Column& col, const SqlValue& val) {
    return Condition(col.name(), Condition::Op::Ne, val);
}

inline Condition operator<(const Column& col, const SqlValue& val) {
    return Condition(col.name(), Condition::Op::Lt, val);
}

inline Condition operator<=(const Column& col, const SqlValue& val) {
    return Condition(col.name(), Condition::Op::Le, val);
}

inline Condition operator>(const Column& col, const SqlValue& val) {
    return Condition(col.name(), Condition::Op::Gt, val);
}

inline Condition operator>=(const Column& col, const SqlValue& val) {
    return Condition(col.name(), Condition::Op::Ge, val);
}

} // namespace sql
