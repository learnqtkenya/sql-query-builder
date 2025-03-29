#pragma once

#include <array>
#include <span>
#include <string_view>
#include <concepts>
#include <variant>
#include <format>
#include <sstream>
#include <type_traits>
#include <cassert>
#include <optional>
#include <memory>
#include <cstdint>

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
        InvalidCondition,
        TooManyOrderBy,
        TooManyGroupBy
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

// Helper for string literals detection that works properly
template<typename T>
struct is_string_literal : std::false_type {};

template<std::size_t N>
struct is_string_literal<const char[N]> : std::true_type {};

template<std::size_t N>
struct is_string_literal<char[N]> : std::true_type {};

// Qt support
#ifdef SQLQUERYBUILDER_USE_QT
template<typename T>
struct is_qt_type : std::false_type {};

template<>
struct is_qt_type<QString> : std::true_type {};

template<>
struct is_qt_type<QDateTime> : std::true_type {};

template<>
struct is_qt_type<QString&> : std::true_type {};

template<>
struct is_qt_type<const QString&> : std::true_type {};

template<>
struct is_qt_type<QDateTime&> : std::true_type {};

template<>
struct is_qt_type<const QDateTime&> : std::true_type {};
#endif

// Updated SqlCompatible concept for better string literal handling
template<typename T>
concept SqlCompatible =
    std::is_integral_v<std::remove_cvref_t<T>> ||
    std::is_floating_point_v<std::remove_cvref_t<T>> ||
    std::is_enum_v<std::remove_cvref_t<T>> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view> ||
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    is_string_literal<std::remove_cvref_t<T>>::value ||
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

// Table class with config awareness
template<typename Config = DefaultConfig>
class Table {
private:
    std::string_view name_;

public:
    constexpr explicit Table(std::string_view name) : name_(name) {}

    [[nodiscard]] constexpr std::string_view name() const { return name_; }

    // Implicit conversion to string_view
    operator std::string_view() const { return name_; }
};

// TypedColumn class with config awareness
template<typename T, typename Config = DefaultConfig>
class TypedColumn {
private:
    std::string_view table_;
    std::string_view name_;

public:
    using value_type = T;
    using config_type = Config;

    constexpr TypedColumn(std::string_view table, std::string_view name)
        : table_(table), name_(name) {}

    [[nodiscard]] constexpr std::string_view tableName() const { return table_; }
    [[nodiscard]] constexpr std::string_view name() const { return name_; }

    // Generate qualified name (table.column)
    [[nodiscard]] std::string qualifiedName() const {
        return std::string(table_) + "." + std::string(name_);
    }

    // Implicit conversion to string_view
    operator std::string_view() const { return name_; }

    // Condition generation methods
    [[nodiscard]] Condition<Config> isNull() const;
    [[nodiscard]] Condition<Config> isNotNull() const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> eq(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> eq(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> ne(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> ne(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> lt(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> lt(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> le(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> le(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> gt(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> gt(const TypedColumn<OtherType, OtherConfig>& other) const;

    template<SqlCompatible U>
    [[nodiscard]] Condition<Config> ge(U&& value) const;

    template<typename OtherType, typename OtherConfig>
    [[nodiscard]] Condition<Config> ge(const TypedColumn<OtherType, OtherConfig>& other) const;

    [[nodiscard]] Condition<Config> like(std::string_view pattern) const;
    [[nodiscard]] Condition<Config> notLike(std::string_view pattern) const;

    template<SqlCompatible T1, SqlCompatible T2>
    [[nodiscard]] Condition<Config> between(T1&& start, T2&& end) const;
};

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
    explicit SqlValue(T&& value) {
        if constexpr(std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, bool>) {
            storage_ = static_cast<int64_t>(value);
        } else if constexpr(std::is_floating_point_v<std::remove_cvref_t<T>>) {
            storage_ = static_cast<double>(value);
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, bool>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, std::string_view>) {
            storage_ = std::string(value);
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, std::string>) {
            storage_ = value;
        } else if constexpr(is_string_literal<std::remove_cvref_t<T>>::value) {
            storage_ = std::string(value);
#ifdef SQLQUERYBUILDER_USE_QT
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, QString>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, QDateTime>) {
            storage_ = value;
#endif
        } else if constexpr(std::is_enum_v<std::remove_cvref_t<T>>) {
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
    // For column-to-column comparisons
    std::string_view right_column_;
    bool is_column_comparison_{false};

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

    // Constructor for column-to-column comparison
    Condition(std::string_view left_col, Op op, std::string_view right_col)
        : column_(left_col), op_(op), right_column_(right_col), is_column_comparison_(true) {}

    // Constructor for between conditions
    Condition(std::string_view col, Op op, SqlValue<Config> value1, SqlValue<Config> value2)
        : column_(col), op_(op), value_count_(2) {
        values_[0] = value1;
        values_[1] = value2;
    }

    // Constructor for compound conditions
    Condition(const Condition& lhs, Op op, const Condition& rhs)
        : op_(op), left_(std::make_shared<Condition>(lhs)), right_(std::make_shared<Condition>(rhs)) {}

    // Template constructor for different Config types
    template<typename OtherConfig>
    Condition(const Condition<OtherConfig>& other)
        : column_(other.getColumn()),
        op_(static_cast<Op>(other.getOp())),
        value_count_(other.getValueCount()),
        negated_(other.isNegated()),
        condition_str_(other.getRawCondition()),
        is_column_comparison_(other.isColumnComparison()),
        right_column_(other.getRightColumn()) {

        // Copy values if there are any
        if (value_count_ > 0) {
            values_[0] = SqlValue<Config>(other.getValueString(0));
            if (value_count_ > 1) {
                values_[1] = SqlValue<Config>(other.getValueString(1));
            }
        }

        // Copy compound conditions if they exist
        if (other.getLeft()) {
            left_ = std::make_shared<Condition<Config>>(*other.getLeft());
        }
        if (other.getRight()) {
            right_ = std::make_shared<Condition<Config>>(*other.getRight());
        }
    }

    // Accessors for conversion between different Config types
    std::string_view getColumn() const { return column_; }
    Op getOp() const { return op_; }
    size_t getValueCount() const { return value_count_; }
    bool isNegated() const { return negated_; }
    std::string getRawCondition() const { return condition_str_; }
    bool isColumnComparison() const { return is_column_comparison_; }
    std::string_view getRightColumn() const { return right_column_; }
    std::string getValueString(size_t idx) const { return values_[idx].toSqlString(); }
    std::shared_ptr<Condition> getLeft() const { return left_; }
    std::shared_ptr<Condition> getRight() const { return right_; }

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

            oss << column_ << " " << op_str << " ";
            if (is_column_comparison_) {
                oss << right_column_;
            } else {
                oss << values_[0].toSqlString();
            }
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

// TypedColumn method implementations
template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::isNull() const {
    return Condition<Config>(name_, Condition<Config>::Op::IsNull, SqlValue<Config>());
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::isNotNull() const {
    return Condition<Config>(name_, Condition<Config>::Op::IsNotNull, SqlValue<Config>());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::eq(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Eq, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::eq(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Eq, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::ne(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ne, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::ne(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ne, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::lt(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Lt, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::lt(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Lt, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::le(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Le, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::le(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Le, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::gt(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Gt, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::gt(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Gt, other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::ge(U&& value) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ge, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::ge(const TypedColumn<OtherType, OtherConfig>& other) const {
    return Condition<Config>(name_, Condition<Config>::Op::Ge, other.name());
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::like(std::string_view pattern) const {
    return Condition<Config>(name_, Condition<Config>::Op::Like, SqlValue<Config>(pattern));
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::notLike(std::string_view pattern) const {
    return Condition<Config>(name_, Condition<Config>::Op::NotLike, SqlValue<Config>(pattern));
}

template<typename T, typename Config>
template<SqlCompatible T1, SqlCompatible T2>
Condition<Config> TypedColumn<T, Config>::between(T1&& start, T2&& end) const {
    return Condition<Config>(
        name_,
        Condition<Config>::Op::Between,
        SqlValue<Config>(std::forward<T1>(start)),
        SqlValue<Config>(std::forward<T2>(end))
        );
}

// Column class
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

// Operator overloads for TypedColumn
template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator==(const TypedColumn<T, Config>& col, U&& val) {
    return col.eq(std::forward<U>(val));
}

// Modified operator for TypedColumn == TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator==(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.eq(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator!=(const TypedColumn<T, Config>& col, U&& val) {
    return col.ne(std::forward<U>(val));
}

// Modified operator for TypedColumn != TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator!=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.ne(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator<(const TypedColumn<T, Config>& col, U&& val) {
    return col.lt(std::forward<U>(val));
}

// Modified operator for TypedColumn < TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator<(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.lt(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator<=(const TypedColumn<T, Config>& col, U&& val) {
    return col.le(std::forward<U>(val));
}

// Modified operator for TypedColumn <= TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator<=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.le(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator>(const TypedColumn<T, Config>& col, U&& val) {
    return col.gt(std::forward<U>(val));
}

// Modified operator for TypedColumn > TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator>(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.gt(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator>=(const TypedColumn<T, Config>& col, U&& val) {
    return col.ge(std::forward<U>(val));
}

// Modified operator for TypedColumn >= TypedColumn comparisons with different configs
template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator>=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.ge(col2);
}

// Operator overloads for Column
template<typename Config>
inline Condition<Config> operator==(const Column<Config>& col, const SqlValue<Config>& val) {
    return col.eq(val);
}

template<typename Config, SqlCompatible T>
inline Condition<Config> operator==(const Column<Config>& col, T&& val) {
    return col.eq(std::forward<T>(val));
}

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

// Qt type operator overloads
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

// Main QueryBuilder class with stack-allocated storage
template<typename Config = DefaultConfig>
class QueryBuilder {
public:
    enum class QueryType : uint8_t { Select, Insert, Update, Delete, Truncate };

private:
    QueryType type_{QueryType::Select};
    std::string_view table_;

    // Use fixed-size arrays with explicit tracking of the used size
    std::array<std::string_view, Config::MaxColumns> select_columns_{};
    size_t select_columns_count_{0};

    std::array<std::pair<std::string_view, SqlValue<Config>>, Config::MaxColumns> values_{};
    size_t values_count_{0};

    std::array<Condition<Config>, Config::MaxConditions> where_conditions_{};
    size_t where_conditions_count_{0};

    std::array<Join<Config>, Config::MaxJoins> joins_{};
    size_t joins_count_{0};

    std::array<std::pair<std::string_view, bool>, Config::MaxOrderBy> order_by_{};
    size_t order_by_count_{0};

    std::array<std::string_view, Config::MaxGroupBy> group_by_{};
    size_t group_by_count_{0};

    std::string_view having_;
    int32_t limit_{-1};
    int32_t offset_{-1};
    bool distinct_{false};
    mutable std::optional<QueryError> last_error_;

    // Helper method to estimate query size for string reservation
    [[nodiscard]] size_t estimateSize() const {
        size_t size = 64; // Base size
        size += table_.size();
        size += select_columns_count_ * 10;
        size += where_conditions_count_ * 30;
        size += joins_count_ * 40;
        size += order_by_count_ * 15;
        size += group_by_count_ * 15;
        size += having_.size();
        if (limit_ >= 0) size += 15;
        if (offset_ >= 0) size += 15;
        return size;
    }

public:
    QueryBuilder() = default;

    // Reset builder state - Fixed to return reference to this
    QueryBuilder& reset() {
        type_ = QueryType::Select;
        table_ = "";
        select_columns_count_ = 0;
        values_count_ = 0;
        where_conditions_count_ = 0;
        joins_count_ = 0;
        order_by_count_ = 0;
        group_by_count_ = 0;
        having_ = "";
        limit_ = -1;
        offset_ = -1;
        distinct_ = false;
        last_error_ = std::nullopt;
        return *this;
    }

    // Get last error
    [[nodiscard]] std::optional<QueryError> lastError() const {
        return last_error_;
    }

    // Select methods with fixed-size array bounds checking
    template<typename... Cols>
    QueryBuilder& select(Cols&&... cols) {
        type_ = QueryType::Select;
        const size_t additional = sizeof...(cols);

        if (select_columns_count_ + additional > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        (addSelectColumn(std::forward<Cols>(cols)), ...);
        return *this;
    }

    // Helper to add columns to select
    template<typename T>
    void addSelectColumn(const T& col) {
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            select_columns_[select_columns_count_++] = static_cast<std::string_view>(col);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Column type not supported for select");
        }
    }

    // Select from span
    QueryBuilder& select(std::span<const std::string_view> cols) {
        type_ = QueryType::Select;

        if (select_columns_count_ + cols.size() > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        for (const auto& col : cols) {
            select_columns_[select_columns_count_++] = col;
        }
        return *this;
    }

    // Select from array
    template<size_t N>
    QueryBuilder& select(const std::array<std::string_view, N>& cols) {
        return select(std::span<const std::string_view>(cols));
    }

    // From method
    template<typename T>
    QueryBuilder& from(const T& table) {
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for from");
        }
        return *this;
    }

    // Where method with bounds checking and conversion between configs
    template<typename OtherConfig>
    QueryBuilder& where(const Condition<OtherConfig>& condition) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        // Convert condition from OtherConfig to our Config
        where_conditions_[where_conditions_count_++] = Condition<Config>(condition);
        return *this;
    }

    template<SqlCompatible T>
    QueryBuilder& whereOp(std::string_view column, typename Condition<Config>::Op op, T&& value) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        where_conditions_[where_conditions_count_++] = Condition<Config>(
            column, op, SqlValue<Config>(std::forward<T>(value))
            );
        return *this;
    }

    // OrderBy with bounds checking
    template<typename T>
    QueryBuilder& orderBy(const T& column, bool asc = true) {
        if (order_by_count_ >= Config::MaxOrderBy) {
            auto error = QueryError(QueryError::Code::TooManyOrderBy,
                                    std::format("Too many order by clauses: limit is {}", Config::MaxOrderBy));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            order_by_[order_by_count_++] = {static_cast<std::string_view>(column), asc};
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Column type not supported for orderBy");
        }
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

    // Insert with bounds checking
    template<typename T>
    QueryBuilder& insert(const T& table) {
        type_ = QueryType::Insert;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for insert");
        }
        return *this;
    }

    template<typename Col, SqlCompatible T>
    QueryBuilder& value(const Col& column, T&& val) {
        if (values_count_ >= Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many values: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            // Direct assignment of the pair at the current index
            values_[values_count_].first = static_cast<std::string_view>(column);
            values_[values_count_].second = SqlValue<Config>(std::forward<T>(val));
            values_count_++;
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for value");
        }
        return *this;
    }

    // Update methods
    template<typename T>
    QueryBuilder& update(const T& table) {
        type_ = QueryType::Update;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for update");
        }
        return *this;
    }

    template<typename Col, SqlCompatible T>
    QueryBuilder& set(const Col& column, T&& val) {
        if (values_count_ >= Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many values: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            // Direct assignment of the pair at the current index
            values_[values_count_].first = static_cast<std::string_view>(column);
            values_[values_count_].second = SqlValue<Config>(std::forward<T>(val));
            values_count_++;
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for set");
        }
        return *this;
    }

    // Delete methods
    template<typename T>
    QueryBuilder& deleteFrom(const T& table) {
        type_ = QueryType::Delete;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for deleteFrom");
        }
        return *this;
    }

    // Truncate methods
    template<typename T>
    QueryBuilder& truncate(const T& table) {
        type_ = QueryType::Truncate;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            table_ = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for truncate");
        }
        return *this;
    }

    // Join methods with bounds checking
    template<typename T>
    QueryBuilder& innerJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Inner,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for innerJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& leftJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Left,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for leftJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& rightJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Right,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for rightJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& fullJoin(const T& table, std::string_view condition) {
        if (joins_count_ >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            joins_[joins_count_++] = Join<Config>{
                Join<Config>::Type::Full,
                static_cast<std::string_view>(table),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for fullJoin");
        }
        return *this;
    }

    // Where variations
    template<typename Col, typename T>
    QueryBuilder& whereIn(const Col& column, std::span<const T> values) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        std::ostringstream condition;
        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            condition << static_cast<std::string_view>(column) << " IN (";
            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0) condition << ", ";
                condition << SqlValue<Config>(values[i]).toSqlString();
            }
            condition << ")";
            where_conditions_[where_conditions_count_++] = Condition<Config>(condition.str());
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereIn");
        }
        return *this;
    }

    template<typename Col, typename T, typename U>
    QueryBuilder& whereBetween(const Col& column, T&& start, U&& end) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::Between,
                SqlValue<Config>(std::forward<T>(start)),
                SqlValue<Config>(std::forward<U>(end))
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereBetween");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereLike(const Col& column, std::string_view pattern) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::Like,
                SqlValue<Config>(pattern)
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereLike");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereNull(const Col& column) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::IsNull,
                SqlValue<Config>()
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNull");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereNotNull(const Col& column) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            where_conditions_[where_conditions_count_++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Condition<Config>::Op::IsNotNull,
                SqlValue<Config>()
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNotNull");
        }
        return *this;
    }

    QueryBuilder& whereExists(std::string_view subquery) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        std::string condition = std::string("EXISTS (") + std::string(subquery) + ")";
        where_conditions_[where_conditions_count_++] = Condition<Config>(condition);
        return *this;
    }

    QueryBuilder& whereRaw(std::string_view rawCondition) {
        if (where_conditions_count_ >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        where_conditions_[where_conditions_count_++] = Condition<Config>(rawCondition);
        return *this;
    }

    template<typename Col>
    QueryBuilder& groupBy(const Col& column) {
        if (group_by_count_ >= Config::MaxGroupBy) {
            auto error = QueryError(QueryError::Code::TooManyGroupBy,
                                    std::format("Too many group by clauses: limit is {}", Config::MaxGroupBy));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            group_by_[group_by_count_++] = static_cast<std::string_view>(column);
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for groupBy");
        }
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

        if (select_columns_count_ == 0) {
            oss << "*";
        } else {
            bool first = true;
            for (size_t i = 0; i < select_columns_count_; ++i) {
                if (!first) oss << ", ";
                oss << select_columns_[i];
                first = false;
            }
        }

        oss << " FROM " << table_;

        // Joins
        for (size_t i = 0; i < joins_count_; ++i) {
            oss << " ";
            joins_[i].toString(oss);
        }

        // Where
        if (where_conditions_count_ > 0) {
            oss << " WHERE ";
            bool first = true;
            for (size_t i = 0; i < where_conditions_count_; ++i) {
                if (!first) oss << " AND ";
                where_conditions_[i].toString(oss);
                first = false;
            }
        }

        // Group By
        if (group_by_count_ > 0) {
            oss << " GROUP BY ";
            bool first = true;
            for (size_t i = 0; i < group_by_count_; ++i) {
                if (!first) oss << ", ";
                oss << group_by_[i];
                first = false;
            }

            if (!having_.empty()) {
                oss << " HAVING " << having_;
            }
        }

        // Order By
        if (order_by_count_ > 0) {
            oss << " ORDER BY ";
            bool first = true;
            for (size_t i = 0; i < order_by_count_; ++i) {
                if (!first) oss << ", ";
                oss << order_by_[i].first << (order_by_[i].second ? " ASC" : " DESC");
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
        if (values_count_ == 0) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for INSERT");
        }

        std::ostringstream oss;
        oss << "INSERT INTO " << table_ << " (";

        bool first = true;
        for (size_t i = 0; i < values_count_; ++i) {
            if (!first) oss << ", ";
            oss << values_[i].first;
            first = false;
        }

        oss << ") VALUES (";

        first = true;
        for (size_t i = 0; i < values_count_; ++i) {
            if (!first) oss << ", ";
            oss << values_[i].second.toSqlString();
            first = false;
        }

        oss << ")";
        query = oss.str();
    }

    void buildUpdate(std::string& query) const {
        if (values_count_ == 0) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for UPDATE");
        }

        std::ostringstream oss;
        oss << "UPDATE " << table_ << " SET ";

        bool first = true;
        for (size_t i = 0; i < values_count_; ++i) {
            if (!first) oss << ", ";
            oss << values_[i].first << " = " << values_[i].second.toSqlString();
            first = false;
        }

        if (where_conditions_count_ > 0) {
            oss << " WHERE ";
            first = true;
            for (size_t i = 0; i < where_conditions_count_; ++i) {
                if (!first) oss << " AND ";
                where_conditions_[i].toString(oss);
                first = false;
            }
        }

        query = oss.str();
    }

    void buildDelete(std::string& query) const {
        std::ostringstream oss;
        oss << "DELETE FROM " << table_;

        if (where_conditions_count_ > 0) {
            oss << " WHERE ";
            bool first = true;
            for (size_t i = 0; i < where_conditions_count_; ++i) {
                if (!first) oss << " AND ";
                where_conditions_[i].toString(oss);
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
[[nodiscard]] constexpr Table<Config> table(std::string_view name) {
    return Table<Config>(name);
}

template<typename T, typename Config = DefaultConfig>
[[nodiscard]] constexpr TypedColumn<T, Config> column(const Table<Config>& table, std::string_view name) {
    return TypedColumn<T, Config>(table.name(), name);
}

template<typename Config = DefaultConfig>
inline Column<Config> col(std::string_view name) {
    return Column<Config>(name);
}

template<typename T, typename Config = DefaultConfig>
inline SqlValue<Config> val(T&& value) {
    return SqlValue<Config>(std::forward<T>(value));
}

// SQL function helpers
template<typename T, typename Config = DefaultConfig>
inline std::string count(const T& col) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string("COUNT(") + std::string(static_cast<std::string_view>(col)) + ")";
    } else {
        static_assert(std::is_convertible_v<T, std::string_view>,
                      "Column type not supported for count");
        return "";
    }
}

template<typename T, typename Config = DefaultConfig>
inline std::string sum(const T& col) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string("SUM(") + std::string(static_cast<std::string_view>(col)) + ")";
    } else {
        static_assert(std::is_convertible_v<T, std::string_view>,
                      "Column type not supported for sum");
        return "";
    }
}

template<typename T, typename Config = DefaultConfig>
inline std::string avg(const T& col) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string("AVG(") + std::string(static_cast<std::string_view>(col)) + ")";
    } else {
        static_assert(std::is_convertible_v<T, std::string_view>,
                      "Column type not supported for avg");
        return "";
    }
}

template<typename T, typename Config = DefaultConfig>
inline std::string min(const T& col) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string("MIN(") + std::string(static_cast<std::string_view>(col)) + ")";
    } else {
        static_assert(std::is_convertible_v<T, std::string_view>,
                      "Column type not supported for min");
        return "";
    }
}

template<typename T, typename Config = DefaultConfig>
inline std::string max(const T& col) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string("MAX(") + std::string(static_cast<std::string_view>(col)) + ")";
    } else {
        static_assert(std::is_convertible_v<T, std::string_view>,
                      "Column type not supported for max");
        return "";
    }
}

// Helper to select all columns from a table
template<typename T, typename Config = DefaultConfig>
inline std::string_view all_of(const T& t) {
    return "*";
}

// Macro for defining table structures
#define SQL_DEFINE_TABLE(name) \
struct name##_table { \
        const sql::Table<> table = sql::table(#name);

// Macro for column definition in a table
#define SQL_DEFINE_COLUMN(name, type) \
    const sql::TypedColumn<type> name = sql::column<type>(table, #name);

// Macro to end table definition
#define SQL_END_TABLE() };

} // namespace v1
} // namespace sql
