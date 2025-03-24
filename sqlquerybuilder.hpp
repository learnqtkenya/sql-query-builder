#pragma once

#include <array>
#include <span>
#include <string_view>
#include <concepts>
#include <variant>
#include <format>
#include <sstream>
#include <vector>
#include <memory>
#include <optional>
#include <type_traits>
#include <cassert>

// Optional Qt support - can be disabled if not needed
#ifdef SQLQUERYBUILDER_USE_QT
#include <QDateTime>
#include <QString>
#endif

namespace sql {
inline namespace v1 {

// Configuration with compile-time defaults
struct DefaultConfig {
    static constexpr size_t MaxColumns = 32;
    static constexpr size_t MaxConditions = 16;
    static constexpr size_t MaxJoins = 4;
    static constexpr size_t MaxOrderBy = 8;
    static constexpr size_t MaxGroupBy = 8;
    static constexpr bool ThrowOnError = false;
};

// Error handling
struct QueryError {
    enum class Code {
        None,
        TooManyColumns,
        TooManyConditions,
        TooManyJoins,
        EmptyTable,
        InvalidColumn,
        InvalidCondition
    };

    Code code{Code::None};
    std::string message;

    QueryError() = default;
    QueryError(Code c, std::string msg) : code(c), message(std::move(msg)) {}

    explicit operator bool() const { return code != Code::None; }
};

// Result wrapper for error handling
template<typename T>
class Result {
    std::variant<T, QueryError> value_;

public:
    Result(T value) : value_(std::move(value)) {}
    Result(QueryError error) : value_(std::move(error)) {}

    bool hasError() const { return std::holds_alternative<QueryError>(value_); }
    const QueryError& error() const { return std::get<QueryError>(value_); }
    const T& value() const { return std::get<T>(value_); }

    explicit operator bool() const { return !hasError(); }
};

// Small vector implementation for avoiding heap allocations for common cases
template<typename T, size_t StaticSize = 8>
class SmallVector {
    size_t size_{0};
    std::array<T, StaticSize> static_storage_{};
    std::optional<std::vector<T>> dynamic_storage_{};

public:
    void push_back(const T& value) {
        if (size_ < StaticSize) {
            static_storage_[size_++] = value;
        } else {
            if (!dynamic_storage_) {
                dynamic_storage_.emplace();
                for (size_t i = 0; i < StaticSize; ++i) {
                    dynamic_storage_->push_back(std::move(static_storage_[i]));
                }
            }
            dynamic_storage_->push_back(value);
            size_++;
        }
    }

    void push_back(T&& value) {
        if (size_ < StaticSize) {
            static_storage_[size_++] = std::move(value);
        } else {
            if (!dynamic_storage_) {
                dynamic_storage_.emplace();
                for (size_t i = 0; i < StaticSize; ++i) {
                    dynamic_storage_->push_back(std::move(static_storage_[i]));
                }
            }
            dynamic_storage_->push_back(std::move(value));
            size_++;
        }
    }

    T& operator[](size_t index) {
        if (index < StaticSize && !dynamic_storage_) {
            return static_storage_[index];
        } else {
            return (*dynamic_storage_)[index];
        }
    }

    const T& operator[](size_t index) const {
        if (index < StaticSize && !dynamic_storage_) {
            return static_storage_[index];
        } else {
            return (*dynamic_storage_)[index];
        }
    }

    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] bool empty() const { return size_ == 0; }

    using iterator = typename std::conditional<
        StaticSize == 0,
        typename std::vector<T>::iterator,
        T*
        >::type;

    using const_iterator = typename std::conditional<
        StaticSize == 0,
        typename std::vector<T>::const_iterator,
        const T*
        >::type;

    iterator begin() {
        if (dynamic_storage_) {
            return dynamic_storage_->data();
        } else {
            return static_storage_.data();
        }
    }

    iterator end() {
        if (dynamic_storage_) {
            return dynamic_storage_->data() + dynamic_storage_->size();
        } else {
            return static_storage_.data() + size_;
        }
    }

    const_iterator begin() const {
        if (dynamic_storage_) {
            return dynamic_storage_->data();
        } else {
            return static_storage_.data();
        }
    }

    const_iterator end() const {
        if (dynamic_storage_) {
            return dynamic_storage_->data() + dynamic_storage_->size();
        } else {
            return static_storage_.data() + size_;
        }
    }
};

// Type traits for Qt support
#ifdef SQLQUERYBUILDER_USE_QT
template<typename T>
struct is_qt_type : std::false_type {};

template<>
struct is_qt_type<QString> : std::true_type {};

template<>
struct is_qt_type<QDateTime> : std::true_type {};

// Also check for reference types
template<>
struct is_qt_type<QString&> : std::true_type {};

template<>
struct is_qt_type<const QString&> : std::true_type {};

template<>
struct is_qt_type<QDateTime&> : std::true_type {};

template<>
struct is_qt_type<const QDateTime&> : std::true_type {};
#endif

// Concepts for type safety
template<typename T>
concept SqlCompatible =
    std::is_integral_v<std::remove_cvref_t<T>> ||
    std::is_floating_point_v<std::remove_cvref_t<T>> ||
    std::is_enum_v<std::remove_cvref_t<T>> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
#ifdef SQLQUERYBUILDER_USE_QT
    is_qt_type<std::remove_cvref_t<T>>::value ||
#endif
    std::same_as<std::remove_cvref_t<T>, bool>;

// Forward declarations
template<typename Config = DefaultConfig>
class SqlValue;

template<typename Config = DefaultConfig>
class Condition;

template<typename Config = DefaultConfig>
class Column;

// SqlValue class with type-safe storage
template<typename Config>
class SqlValue {
public:
    using StorageType = std::variant<
        std::monostate,  // For NULL
        int64_t,
        double,
        bool,
        std::string
#ifdef SQLQUERYBUILDER_USE_QT
        ,QString
        ,QDateTime
#endif
        >;

private:
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
            storage_ = std::string(value);
        } else if constexpr(std::is_same_v<T, std::string>) {
            storage_ = value;
#ifdef SQLQUERYBUILDER_USE_QT
        } else if constexpr(std::is_same_v<T, QString>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<T, QDateTime>) {
            storage_ = value;
#endif
        } else if constexpr(std::is_enum_v<T>) {
            storage_ = static_cast<int64_t>(value);
        }
    }

    [[nodiscard]] std::string toSqlString() const {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr(std::is_same_v<T, std::monostate>) {
                return "NULL";
            } else if constexpr(std::is_integral_v<T>) {
                return std::to_string(value);
            } else if constexpr(std::is_floating_point_v<T>) {
                return std::to_string(value);
            } else if constexpr(std::is_same_v<T, bool>) {
                return value ? "1" : "0";
            } else if constexpr(std::is_same_v<T, std::string>) {
                std::string escaped = value;
                escapeString(escaped);
                return std::format("'{}'", escaped);
#ifdef SQLQUERYBUILDER_USE_QT
            } else if constexpr(std::is_same_v<T, QString>) {
                std::string str = value.toStdString();
                escapeString(str);
                return std::format("'{}'", str);
            } else if constexpr(std::is_same_v<T, QDateTime>) {
                return std::format("'{}'", value.toString(Qt::ISODate).toStdString());
#endif
            }
            return "NULL"; // Fallback for any unexpected type
        }, storage_);
    }

    [[nodiscard]] bool isNull() const { return std::holds_alternative<std::monostate>(storage_); }

private:
    static void escapeString(std::string& str) {
        size_t pos = 0;
        while ((pos = str.find('\'', pos)) != std::string::npos) {
            str.replace(pos, 1, "''");
            pos += 2;
        }
    }
};

// Condition class for SQL conditions
template<typename Config>
class Condition {
public:
    enum class Op : uint8_t {
        Eq, Ne, Lt, Le, Gt, Ge,
        IsNull, IsNotNull,
        In, NotIn,
        Like, NotLike,
        Between,
        Raw,
        And,
        Or
    };

private:
    std::string_view column_;
    Op op_;
    std::array<SqlValue<Config>, 2> values_{};
    size_t value_count_{0};
    bool negated_{false};
    std::string condition_str_;
    std::shared_ptr<Condition> left_;
    std::shared_ptr<Condition> right_;

public:
    // Default constructor
    Condition() = default;

    // Constructor for raw conditions
    explicit Condition(std::string_view raw_condition)
        : column_(""), op_(Op::Raw), condition_str_(raw_condition) {}

    // Constructor for regular conditions
    Condition(std::string_view col, Op op, SqlValue<Config> value)
        : column_(col), op_(op), value_count_(1) {
        values_[0] = value;
    }

    // Constructor for between conditions
    Condition(std::string_view col, Op op, SqlValue<Config> value1, SqlValue<Config> value2)
        : column_(col), op_(op), value_count_(2) {
        values_[0] = value1;
        values_[1] = value2;
    }

    // Constructor for compound conditions
    Condition(const Condition& lhs, Op op, const Condition& rhs)
        : op_(op), left_(std::make_shared<Condition>(lhs)), right_(std::make_shared<Condition>(rhs)) {}

    // Logical operators
    Condition operator!() const {
        Condition result = *this;
        result.negated_ = !result.negated_;
        return result;
    }

    Condition operator&&(const Condition& other) const {
        return Condition(*this, Op::And, other);
    }

    Condition operator||(const Condition& other) const {
        return Condition(*this, Op::Or, other);
    }

    // String conversion
    void toString(std::ostringstream& oss) const {
        if (op_ == Op::Raw) {
            oss << condition_str_;
            return;
        }

        if (op_ == Op::And || op_ == Op::Or) {
            assert(left_ && right_);
            if (negated_) oss << "NOT (";
            oss << "(";
            left_->toString(oss);
            oss << ") " << (op_ == Op::And ? "AND" : "OR") << " (";
            right_->toString(oss);
            oss << ")";
            if (negated_) oss << ")";
            return;
        }

        if (negated_) {
            oss << "NOT (";
        }

        switch (op_) {
        case Op::IsNull:
            oss << column_ << " IS NULL";
            break;
        case Op::IsNotNull:
            oss << column_ << " IS NOT NULL";
            break;
        case Op::Between:
            if (value_count_ == 2) {
                oss << column_ << " BETWEEN " << values_[0].toSqlString()
                << " AND " << values_[1].toSqlString();
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
            oss << column_ << " " << op_str << " " << values_[0].toSqlString();
        }

        if (negated_) {
            oss << ")";
        }
    }

    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        toString(oss);
        return oss.str();
    }
};

// Column class for type-safe column references
template<typename Config>
class Column {
    std::string_view name_;

public:
    explicit Column(std::string_view name) : name_(name) {}
    [[nodiscard]] std::string_view name() const { return name_; }

    [[nodiscard]] Condition<Config> isNull() const {
        return Condition<Config>(name_, Condition<Config>::Op::IsNull, SqlValue<Config>());
    }

    [[nodiscard]] Condition<Config> isNotNull() const {
        return Condition<Config>(name_, Condition<Config>::Op::IsNotNull, SqlValue<Config>());
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> eq(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Eq, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> ne(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Ne, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> lt(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Lt, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> le(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Le, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> gt(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Gt, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> ge(T&& value) const {
        return Condition<Config>(name_, Condition<Config>::Op::Ge, SqlValue<Config>(std::forward<T>(value)));
    }

    [[nodiscard]] Condition<Config> like(std::string_view pattern) const {
        return Condition<Config>(name_, Condition<Config>::Op::Like, SqlValue<Config>(pattern));
    }

    [[nodiscard]] Condition<Config> notLike(std::string_view pattern) const {
        return Condition<Config>(name_, Condition<Config>::Op::NotLike, SqlValue<Config>(pattern));
    }

    template<SqlCompatible T, SqlCompatible U>
    [[nodiscard]] Condition<Config> between(T&& start, U&& end) const {
        return Condition<Config>(
            name_,
            Condition<Config>::Op::Between,
            SqlValue<Config>(std::forward<T>(start)),
            SqlValue<Config>(std::forward<U>(end))
            );
    }
};

// Operator overloads for Conditions
template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.eq(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator==(const Column<Config>& col, T&& val) {
    return col.eq(std::forward<T>(val));
}

// Special case for std::string
template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const std::string& val) {
    return col.eq(std::string_view(val));
}

template<typename Config>
inline Condition<Config> operator!=(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.ne(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator!=(const Column<Config>& col, T&& val) {
    return col.ne(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator<(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.lt(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator<(const Column<Config>& col, T&& val) {
    return col.lt(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator<=(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.le(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator<=(const Column<Config>& col, T&& val) {
    return col.le(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator>(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.gt(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator>(const Column<Config>& col, T&& val) {
    return col.gt(std::forward<T>(val));
}

template<typename Config>
inline Condition<Config> operator>=(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.ge(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator>=(const Column<Config>& col, T&& val) {
    return col.ge(std::forward<T>(val));
}

// Special cases for Qt types
#ifdef SQLQUERYBUILDER_USE_QT
template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const QString& val) {
    return col.eq(std::string_view(val.toStdString()));
}

template<typename Config>
inline Condition<Config> operator!=(const Column<Config>& col, const QString& val) {
    return col.ne(std::string_view(val.toStdString()));
}

template<typename Config>
inline Condition<Config> operator<(const Column<Config>& col, const QDateTime& val) {
    return col.lt(val);
}

template<typename Config>
inline Condition<Config> operator<=(const Column<Config>& col, const QDateTime& val) {
    return col.le(val);
}

template<typename Config>
inline Condition<Config> operator>(const Column<Config>& col, const QDateTime& val) {
    return col.gt(val);
}

template<typename Config>
inline Condition<Config> operator>=(const Column<Config>& col, const QDateTime& val) {
    return col.ge(val);
}
#endif

// Join class
template<typename Config>
class Join {
public:
    enum class Type : uint8_t { Inner, Left, Right, Full, Cross };

private:
    Type type_;
    std::string_view table_;
    std::string_view condition_;

public:
    Join() = default;

    Join(Type type, std::string_view table, std::string_view condition)
        : type_(type), table_(table), condition_(condition) {}

    void toString(std::ostringstream& oss) const {
        const char* type_str = "";
        switch (type_) {
        case Type::Inner: type_str = "INNER JOIN"; break;
        case Type::Left:  type_str = "LEFT JOIN"; break;
        case Type::Right: type_str = "RIGHT JOIN"; break;
        case Type::Full:  type_str = "FULL JOIN"; break;
        case Type::Cross: type_str = "CROSS JOIN"; break;
        }

        oss << type_str << " " << table_ << " ON " << condition_;
    }

    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        toString(oss);
        return oss.str();
    }
};

// Main QueryBuilder class
template<typename Config = DefaultConfig>
class QueryBuilder {
public:
    enum class QueryType : uint8_t { Select, Insert, Update, Delete, Truncate };

private:
    QueryType type_{QueryType::Select};
    std::string_view table_;

    SmallVector<std::string_view, Config::MaxColumns> select_columns_;
    SmallVector<std::pair<std::string_view, SqlValue<Config>>, Config::MaxColumns> values_;
    SmallVector<Condition<Config>, Config::MaxConditions> where_conditions_;
    SmallVector<Join<Config>, Config::MaxJoins> joins_;
    SmallVector<std::pair<std::string_view, bool>, Config::MaxOrderBy> order_by_;
    SmallVector<std::string_view, Config::MaxGroupBy> group_by_;

    std::string_view having_;
    int32_t limit_{-1};
    int32_t offset_{-1};
    bool distinct_{false};
    mutable std::optional<QueryError> last_error_;

    // Helper method to estimate query size for string reservation
    [[nodiscard]] size_t estimateSize() const {
        size_t size = 64; // Base size
        size += table_.size();
        size += select_columns_.size() * 10;
        size += where_conditions_.size() * 30;
        size += joins_.size() * 40;
        size += order_by_.size() * 15;
        size += group_by_.size() * 15;
        size += having_.size();
        if (limit_ >= 0) size += 15;
        if (offset_ >= 0) size += 15;
        return size;
    }

public:
    QueryBuilder() = default;

    // Reset builder state
    void reset() {
        type_ = QueryType::Select;
        table_ = "";
        select_columns_ = {};
        values_ = {};
        where_conditions_ = {};
        joins_ = {};
        order_by_ = {};
        group_by_ = {};
        having_ = "";
        limit_ = -1;
        offset_ = -1;
        distinct_ = false;
        last_error_ = std::nullopt;
    }

    // Get last error
    [[nodiscard]] std::optional<QueryError> lastError() const {
        return last_error_;
    }

    // Select statements
    template<typename... Cols>
        requires (std::convertible_to<Cols, std::string_view> && ...)
    QueryBuilder& select(Cols&&... cols) {
        type_ = QueryType::Select;
        (select_columns_.push_back(std::string_view(std::forward<Cols>(cols))), ...);
        return *this;
    }

    QueryBuilder& select(std::span<const std::string_view> cols) {
        type_ = QueryType::Select;
        for (const auto& col : cols) {
            select_columns_.push_back(col);
        }
        return *this;
    }

    template<size_t N>
    QueryBuilder& select(const std::array<std::string_view, N>& cols) {
        return select(std::span<const std::string_view>(cols));
    }

    QueryBuilder& from(std::string_view table) {
        table_ = table;
        return *this;
    }

    QueryBuilder& where(const Condition<Config>& condition) {
        where_conditions_.push_back(condition);
        return *this;
    }

    template<SqlCompatible T>
    QueryBuilder& whereOp(std::string_view column, typename Condition<Config>::Op op, T&& value) {
        where_conditions_.push_back(
            Condition<Config>(column, op, SqlValue<Config>(std::forward<T>(value)))
            );
        return *this;
    }

    QueryBuilder& orderBy(std::string_view column, bool asc = true) {
        order_by_.push_back({column, asc});
        return *this;
    }

    QueryBuilder& limit(int32_t limit) {
        limit_ = limit;
        return *this;
    }

    QueryBuilder& offset(int32_t offset) {
        offset_ = offset;
        return *this;
    }

    QueryBuilder& distinct() {
        distinct_ = true;
        return *this;
    }

    // Insert statements
    QueryBuilder& insert(std::string_view table) {
        type_ = QueryType::Insert;
        table_ = table;
        return *this;
    }

    template<SqlCompatible T>
    QueryBuilder& value(std::string_view column, T&& val) {
        values_.push_back({
            column,
            SqlValue<Config>(std::forward<T>(val))
        });
        return *this;
    }

    // Update statements
    QueryBuilder& update(std::string_view table) {
        type_ = QueryType::Update;
        table_ = table;
        return *this;
    }

    template<SqlCompatible T>
    QueryBuilder& set(std::string_view column, T&& val) {
        values_.push_back({
            column,
            SqlValue<Config>(std::forward<T>(val))
        });
        return *this;
    }

    // Delete statements
    QueryBuilder& deleteFrom(std::string_view table) {
        type_ = QueryType::Delete;
        table_ = table;
        return *this;
    }

    // Truncate statements
    QueryBuilder& truncate(std::string_view table) {
        type_ = QueryType::Truncate;
        table_ = table;
        return *this;
    }

    // Join methods
    QueryBuilder& innerJoin(std::string_view table, std::string_view condition) {
        joins_.push_back(Join<Config>{Join<Config>::Type::Inner, table, condition});
        return *this;
    }

    QueryBuilder& leftJoin(std::string_view table, std::string_view condition) {
        joins_.push_back(Join<Config>{Join<Config>::Type::Left, table, condition});
        return *this;
    }

    QueryBuilder& rightJoin(std::string_view table, std::string_view condition) {
        joins_.push_back(Join<Config>{Join<Config>::Type::Right, table, condition});
        return *this;
    }

    QueryBuilder& fullJoin(std::string_view table, std::string_view condition) {
        joins_.push_back(Join<Config>{Join<Config>::Type::Full, table, condition});
        return *this;
    }

    // Where variations
    template<typename T>
    QueryBuilder& whereIn(std::string_view column, std::span<const T> values) {
        std::ostringstream condition;
        condition << column << " IN (";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) condition << ", ";
            condition << SqlValue<Config>(values[i]).toSqlString();
        }
        condition << ")";
        where_conditions_.push_back(Condition<Config>(condition.str()));
        return *this;
    }

    template<typename T, typename U>
    QueryBuilder& whereBetween(std::string_view column, T&& start, U&& end) {
        where_conditions_.push_back(
            Condition<Config>(
                column,
                Condition<Config>::Op::Between,
                SqlValue<Config>(std::forward<T>(start)),
                SqlValue<Config>(std::forward<U>(end))
                )
            );
        return *this;
    }

    QueryBuilder& whereLike(std::string_view column, std::string_view pattern) {
        where_conditions_.push_back(
            Condition<Config>(
                column,
                Condition<Config>::Op::Like,
                SqlValue<Config>(pattern)
                )
            );
        return *this;
    }

    QueryBuilder& whereNull(std::string_view column) {
        where_conditions_.push_back(
            Condition<Config>(
                column,
                Condition<Config>::Op::IsNull,
                SqlValue<Config>()
                )
            );
        return *this;
    }

    QueryBuilder& whereNotNull(std::string_view column) {
        where_conditions_.push_back(
            Condition<Config>(
                column,
                Condition<Config>::Op::IsNotNull,
                SqlValue<Config>()
                )
            );
        return *this;
    }

    QueryBuilder& whereExists(std::string_view subquery) {
        std::string condition = std::string("EXISTS (") + std::string(subquery) + ")";
        where_conditions_.push_back(Condition<Config>(condition));
        return *this;
    }

    QueryBuilder& whereRaw(std::string_view rawCondition) {
        where_conditions_.push_back(Condition<Config>(rawCondition));
        return *this;
    }

    QueryBuilder& groupBy(std::string_view column) {
        group_by_.push_back(column);
        return *this;
    }

    QueryBuilder& having(std::string_view condition) {
        having_ = condition;
        return *this;
    }

    // Build with error handling
    [[nodiscard]] Result<std::string> buildResult() const {
        if (table_.empty()) {
            QueryError error(QueryError::Code::EmptyTable, "Table name is required");
            last_error_ = error;
            return error;
        }

        try {
            std::string query;
            query.reserve(estimateSize());

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

            return Result<std::string>(query);
        } catch (const std::exception& e) {
            QueryError error(QueryError::Code::InvalidCondition, e.what());
            last_error_ = error;
            return error;
        }
    }

    // Simplified build method for backward compatibility
    [[nodiscard]] std::string build() const {
        auto result = buildResult();
        if (result.hasError()) {
            if constexpr(Config::ThrowOnError) {
                throw result.error();
            }
            return "/* ERROR: " + result.error().message + " */";
        }
        return result.value();
    }

private:
    void buildSelect(std::string& query) const {
        std::ostringstream oss;
        oss << "SELECT ";
        if (distinct_) oss << "DISTINCT ";

        if (select_columns_.empty()) {
            oss << "*";
        } else {
            bool first = true;
            for (const auto& col : select_columns_) {
                if (!first) oss << ", ";
                oss << col;
                first = false;
            }
        }

        oss << " FROM " << table_;

        // Joins
        for (const auto& join : joins_) {
            oss << " ";
            join.toString(oss);
        }

        // Where
        if (!where_conditions_.empty()) {
            oss << " WHERE ";
            bool first = true;
            for (const auto& condition : where_conditions_) {
                if (!first) oss << " AND ";
                condition.toString(oss);
                first = false;
            }
        }

        // Group By
        if (!group_by_.empty()) {
            oss << " GROUP BY ";
            bool first = true;
            for (const auto& col : group_by_) {
                if (!first) oss << ", ";
                oss << col;
                first = false;
            }

            if (!having_.empty()) {
                oss << " HAVING " << having_;
            }
        }

        // Order By
        if (!order_by_.empty()) {
            oss << " ORDER BY ";
            bool first = true;
            for (const auto& [col, asc] : order_by_) {
                if (!first) oss << ", ";
                oss << col << (asc ? " ASC" : " DESC");
                first = false;
            }
        }

        if (limit_ >= 0) {
            oss << " LIMIT " << limit_;
        }

        if (offset_ >= 0) {
            oss << " OFFSET " << offset_;
        }

        query = oss.str();
    }

    void buildInsert(std::string& query) const {
        if (values_.empty()) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for INSERT");
        }

        std::ostringstream oss;
        oss << "INSERT INTO " << table_ << " (";

        bool first = true;
        for (const auto& [col, _] : values_) {
            if (!first) oss << ", ";
            oss << col;
            first = false;
        }

        oss << ") VALUES (";

        first = true;
        for (const auto& [_, value] : values_) {
            if (!first) oss << ", ";
            oss << value.toSqlString();
            first = false;
        }

        oss << ")";
        query = oss.str();
    }

    void buildUpdate(std::string& query) const {
        if (values_.empty()) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for UPDATE");
        }

        std::ostringstream oss;
        oss << "UPDATE " << table_ << " SET ";

        bool first = true;
        for (const auto& [col, value] : values_) {
            if (!first) oss << ", ";
            oss << col << " = " << value.toSqlString();
            first = false;
        }

        if (!where_conditions_.empty()) {
            oss << " WHERE ";
            first = true;
            for (const auto& condition : where_conditions_) {
                if (!first) oss << " AND ";
                condition.toString(oss);
                first = false;
            }
        }

        query = oss.str();
    }

    void buildDelete(std::string& query) const {
        std::ostringstream oss;
        oss << "DELETE FROM " << table_;

        if (!where_conditions_.empty()) {
            oss << " WHERE ";
            bool first = true;
            for (const auto& condition : where_conditions_) {
                if (!first) oss << " AND ";
                condition.toString(oss);
                first = false;
            }
        }

        query = oss.str();
    }

    void buildTruncate(std::string& query) const {
        query = "TRUNCATE TABLE " + std::string(table_);
    }
};

// Helper functions
template<typename Config = DefaultConfig>
inline Column<Config> col(std::string_view name) {
    return Column<Config>(name);
}

template<typename T, typename Config = DefaultConfig>
inline SqlValue<Config> val(T&& value) {
    return SqlValue<Config>(std::forward<T>(value));
}

} // namespace v1
} // namespace sql
