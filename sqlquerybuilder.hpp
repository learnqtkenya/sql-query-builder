#pragma once

#include <array>
#include <span>
#include <string_view>
#include <variant>
#include <format>
#include <type_traits>
#include <cassert>
#include <optional>
#include <cstdint>
#include <functional>

// Optional Qt support
#ifdef SQLQUERYBUILDER_USE_QT
#include <QDateTime>
#include <QString>
#endif

namespace sql {
inline namespace v1 {

//=====================
// Static SQL Keywords
//=====================
namespace keywords {
inline constexpr std::string_view SELECT = "SELECT";
inline constexpr std::string_view FROM = "FROM";
inline constexpr std::string_view WHERE = "WHERE";
inline constexpr std::string_view AND = "AND";
inline constexpr std::string_view OR = "OR";
inline constexpr std::string_view JOIN = "JOIN";
inline constexpr std::string_view INNER_JOIN = "INNER JOIN";
inline constexpr std::string_view LEFT_JOIN = "LEFT JOIN";
inline constexpr std::string_view RIGHT_JOIN = "RIGHT JOIN";
inline constexpr std::string_view FULL_JOIN = "FULL JOIN";
inline constexpr std::string_view CROSS_JOIN = "CROSS JOIN";
inline constexpr std::string_view ON = "ON";
inline constexpr std::string_view ORDER_BY = "ORDER BY";
inline constexpr std::string_view GROUP_BY = "GROUP BY";
inline constexpr std::string_view HAVING = "HAVING";
inline constexpr std::string_view LIMIT = "LIMIT";
inline constexpr std::string_view OFFSET = "OFFSET";
inline constexpr std::string_view INSERT = "INSERT";
inline constexpr std::string_view INSERT_OR_REPLACE = "INSERT OR REPLACE";
inline constexpr std::string_view INTO = "INTO";
inline constexpr std::string_view VALUES = "VALUES";
inline constexpr std::string_view UPDATE = "UPDATE";
inline constexpr std::string_view SET = "SET";
inline constexpr std::string_view DELETE = "DELETE";
inline constexpr std::string_view TRUNCATE = "TRUNCATE TABLE";
inline constexpr std::string_view COUNT = "COUNT";
inline constexpr std::string_view SUM = "SUM";
inline constexpr std::string_view AVG = "AVG";
inline constexpr std::string_view MIN = "MIN";
inline constexpr std::string_view MAX = "MAX";
inline constexpr std::string_view BETWEEN = "BETWEEN";
inline constexpr std::string_view IN = "IN";
inline constexpr std::string_view NOT_IN = "NOT IN";
inline constexpr std::string_view LIKE = "LIKE";
inline constexpr std::string_view NOT_LIKE = "NOT LIKE";
inline constexpr std::string_view IS_NULL = "IS NULL";
inline constexpr std::string_view IS_NOT_NULL = "IS NOT NULL";
inline constexpr std::string_view AS = "AS";
inline constexpr std::string_view ASC = "ASC";
inline constexpr std::string_view DESC = "DESC";
inline constexpr std::string_view DISTINCT = "DISTINCT";
inline constexpr std::string_view EXISTS = "EXISTS";
inline constexpr std::string_view NULL_VALUE = "NULL";
inline constexpr std::string_view TRUE_VALUE = "1";
inline constexpr std::string_view FALSE_VALUE = "0";
inline constexpr std::string_view ALL = "*";
inline constexpr std::string_view GROUP_CONCAT = "GROUP_CONCAT";
}

// Configuration
struct DefaultConfig {
    static constexpr size_t MaxColumns = 32;
    static constexpr size_t MaxConditions = 16;
    static constexpr size_t MaxJoins = 4;
    static constexpr size_t MaxOrderBy = 8;
    static constexpr size_t MaxGroupBy = 8;
    static constexpr size_t MaxInValues = 16;
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
        TooManyGroupBy,
        InvalidOperation
    };

    Code code{Code::None};
    std::string_view message;

    QueryError() = default;
    QueryError(Code c, std::string_view msg) : code(c), message(msg) {}

    explicit operator bool() const { return code != Code::None; }
};

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

template<typename T>
struct is_string_literal : std::false_type {};

template<std::size_t N>
struct is_string_literal<const char[N]> : std::true_type {};

template<std::size_t N>
struct is_string_literal<char[N]> : std::true_type {};

// Type traits to check if a value is compatible with SQL
template<typename T>
struct is_sql_compatible : std::false_type {};

template<typename T> requires std::is_integral_v<std::remove_cvref_t<T>>
struct is_sql_compatible<T> : std::true_type {};

template<typename T> requires std::is_floating_point_v<std::remove_cvref_t<T>>
struct is_sql_compatible<T> : std::true_type {};

template<typename T> requires std::is_enum_v<std::remove_cvref_t<T>>
struct is_sql_compatible<T> : std::true_type {};

template<>
struct is_sql_compatible<std::string_view> : std::true_type {};

template<>
struct is_sql_compatible<std::string> : std::true_type {};

template<typename T> requires is_string_literal<std::remove_cvref_t<T>>::value
struct is_sql_compatible<T> : std::true_type {};

template<>
struct is_sql_compatible<bool> : std::true_type {};


// Qt support
#ifdef SQLQUERYBUILDER_USE_QT
template<>
struct is_sql_compatible<QString> : std::true_type {};

template<>
struct is_sql_compatible<QDateTime> : std::true_type {};
#endif

template<typename T>
concept SqlCompatible = is_sql_compatible<std::remove_cvref_t<T>>::value;

// Forward declarations
template<typename Config = DefaultConfig>
class SqlValue;

template<typename Config = DefaultConfig>
class ConditionBase;

template<typename Config = DefaultConfig>
class Condition;

template<typename Config = DefaultConfig>
class Column;

template<typename Config = DefaultConfig>
class AliasedTable;

template<typename Config = DefaultConfig>
class Placeholder;

// Table class with config awareness
template<typename Config = DefaultConfig>
class Table {
private:
    std::string_view name_;

public:
    constexpr explicit Table(std::string_view name) : name_(name) {}

    [[nodiscard]] constexpr std::string_view name() const { return name_; }

    operator std::string_view() const { return name_; }

    [[nodiscard]] AliasedTable<Config> as(std::string_view alias) const;
};

// AliasedTable for better join handling
template<typename Config>
class AliasedTable {
private:
    std::string_view table_;
    std::string_view alias_;

public:
    constexpr AliasedTable(std::string_view table, std::string_view alias)
        : table_(table), alias_(alias) {}

    [[nodiscard]] constexpr std::string_view tableName() const { return table_; }
    [[nodiscard]] constexpr std::string_view alias() const { return alias_; }

    [[nodiscard]] std::string toString() const {
        return std::string(table_) + " " + std::string(alias_);
    }

    operator std::string() const { return toString(); }
};

// Implementation of Table::as
template<typename Config>
AliasedTable<Config> Table<Config>::as(std::string_view alias) const {
    return AliasedTable<Config>(name_, alias);
}

// TypedColumn class with config awareness
template<typename T, typename Config = DefaultConfig>
class TypedColumn {
private:
    std::string_view table_;
    std::string_view name_;
    std::string_view alias_; // Optional column alias

public:
    using value_type = T;
    using config_type = Config;

    constexpr TypedColumn(std::string_view table, std::string_view name)
        : table_(table), name_(name), alias_() {}

    // Constructor that includes column alias
    constexpr TypedColumn(std::string_view table, std::string_view name, std::string_view alias)
        : table_(table), name_(name), alias_(alias) {}

    [[nodiscard]] constexpr std::string_view tableName() const { return table_; }
    [[nodiscard]] constexpr std::string_view name() const { return name_; }
    [[nodiscard]] constexpr std::string_view alias() const { return alias_; }

    [[nodiscard]] std::string qualifiedName() const {
        if (table_.empty()) {
            return std::string(name_);
        }
        return std::string(table_) + "." + std::string(name_);
    }

    [[nodiscard]] TypedColumn<T, Config> as(std::string_view alias) const {
        return TypedColumn<T, Config>(table_, name_, alias);
    }

    operator std::string_view() const { return name_; }

    // Condition generators
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

    template<typename U>
    [[nodiscard]] Condition<Config> in(std::span<const U> values) const;

    template<typename U>
    [[nodiscard]] Condition<Config> notIn(std::span<const U> values) const;

    [[nodiscard]] Condition<Config> eq(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Eq, value);
    }

    [[nodiscard]] Condition<Config> ne(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Ne, value);
    }

    [[nodiscard]] Condition<Config> lt(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Lt, value);
    }

    [[nodiscard]] Condition<Config> le(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Le, value);
    }

    [[nodiscard]] Condition<Config> gt(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Gt, value);
    }

    [[nodiscard]] Condition<Config> ge(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Ge, value);
    }
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
        std::string_view,
        Placeholder<Config>
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
            storage_ = value;
        } else if constexpr(is_string_literal<std::remove_cvref_t<T>>::value) {
            storage_ = std::string_view(value);
#ifdef SQLQUERYBUILDER_USE_QT
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, QString>) {
            storage_ = value;
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, QDateTime>) {
            storage_ = value;
#endif
        } else if constexpr(std::is_enum_v<std::remove_cvref_t<T>>) {
            storage_ = static_cast<int64_t>(value);
        } else if constexpr(std::is_same_v<std::remove_cvref_t<T>, std::string>) {
            storage_ = std::string_view(value.data(), value.size());
        }
    }
    explicit SqlValue(Placeholder<Config> placeholder) : storage_(placeholder) {}


    static std::string escapeString(std::string_view str) {
        // Pre-allocate with a bit of extra space for quotes and potential escapes
        std::string escaped;
        escaped.reserve(str.size() + 10);
        escaped.push_back('\'');

        for (char c : str) {
            if (c == '\'') {
                escaped.append("''");
            } else {
                escaped.push_back(c);
            }
        }

        escaped.push_back('\'');
        return escaped;
    }

    [[nodiscard]] std::string toSqlString() const {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr(std::is_same_v<T, std::monostate>) {
                return std::string(keywords::NULL_VALUE);
            } else if constexpr(std::is_integral_v<T>) {
                return std::to_string(value);
            } else if constexpr(std::is_floating_point_v<T>) {
                return std::to_string(value);
            } else if constexpr(std::is_same_v<T, bool>) {
                return value ? std::string(keywords::TRUE_VALUE) : std::string(keywords::FALSE_VALUE);
            } else if constexpr(std::is_same_v<T, std::string_view>) {
                return escapeString(value);
            } else if constexpr(std::is_same_v<T, Placeholder<Config>>) {
                return value.toString();
#ifdef SQLQUERYBUILDER_USE_QT
            } else if constexpr(std::is_same_v<T, QString>) {
                return escapeString(value.toStdString());
            } else if constexpr(std::is_same_v<T, QDateTime>) {
                return std::format("'{}'", value.toString(Qt::ISODate).toStdString());
#endif
            }
            return std::string(keywords::NULL_VALUE); // Fallback
        }, storage_);
    }

    [[nodiscard]] bool isNull() const { return std::holds_alternative<std::monostate>(storage_); }
    [[nodiscard]] bool isPlaceholder() const {
        return std::holds_alternative<Placeholder<Config>>(storage_);
    }
};

template<typename Config = DefaultConfig>
inline SqlValue<Config> ph(std::string_view name = "") {
    return SqlValue<Config>(Placeholder<Config>(name));
}

template<typename Config>
class Condition;

// Base class for common condition operations
template<typename Config>
class ConditionBase {
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

    enum class Type : uint8_t {
        Invalid,
        SimpleValue,     // column OP value
        ColumnColumn,    // column OP other_column
        Between,         // column BETWEEN value1 AND value2
        IsNull,          // column IS NULL
        IsNotNull,       // column IS NOT NULL
        Raw,             // Raw SQL string
        Compound,        // AND/OR of two conditions
        In               // column IN (values)
    };

    static const char* opToString(Op op) {
        switch (op) {
        case Op::Eq: return "=";
        case Op::Ne: return "!=";
        case Op::Lt: return "<";
        case Op::Le: return "<=";
        case Op::Gt: return ">";
        case Op::Ge: return ">=";
        case Op::Like: return "LIKE";
        case Op::NotLike: return "NOT LIKE";
        case Op::And: return "AND";
        case Op::Or: return "OR";
        default: return "";
        }
    }
};

// Primary Condition class
template<typename Config>
class Condition : public ConditionBase<Config> {
public:
    using typename ConditionBase<Config>::Op;
    using typename ConditionBase<Config>::Type;

private:
    Type type_ = Type::Invalid;
    Op op_ = Op::Eq;
    bool negated_ = false;

    // For column operations
    std::string_view column_;
    std::string_view table_;

    // Storage for various condition types
    struct SimpleConditionData {
        SqlValue<Config> value;
    };

    struct BetweenConditionData {
        SqlValue<Config> start;
        SqlValue<Config> end;
    };

    struct ColumnColumnData {
        std::string_view right_column;
        std::string_view right_table;
    };

    struct RawData {
        std::string raw_sql;
    };

    struct InConditionData {
        std::array<SqlValue<Config>, Config::MaxInValues> values;
        size_t count = 0;
    };

    // CompoundCondition for recursive conditions (AND/OR)
    struct CompoundConditionData {
        std::string left_condition;
        std::string right_condition;
        Op op;
    };

    using ConditionVariant = std::variant<
        std::monostate,            // Invalid or simple conditions
        SimpleConditionData,       // For SimpleValue
        BetweenConditionData,      // For Between
        ColumnColumnData,          // For ColumnColumn
        RawData,                   // For Raw
        InConditionData,           // For In
        CompoundConditionData      // For Compound
        >;

    ConditionVariant data_;

public:
    // Default constructor - creates an invalid condition
    Condition() = default;

    // Constructor for raw SQL conditions
    explicit Condition(std::string_view raw_condition)
        : type_(Type::Raw) {
        data_ = RawData{std::string(raw_condition)};
    }

    // Constructor for column OP value conditions
    Condition(std::string_view column, Op op, SqlValue<Config> value)
        : type_(Type::SimpleValue), op_(op), column_(column) {
        data_ = SimpleConditionData{value};
    }

    // Column-to-column comparison constructor with explicit tables
    Condition(std::string_view left_table, std::string_view left_column,
              Op op,
              std::string_view right_table, std::string_view right_column)
        : type_(Type::ColumnColumn), op_(op),
        table_(left_table), column_(left_column) {
        data_ = ColumnColumnData{right_column, right_table};
    }

    // Shorthand for column-to-column without explicit tables
    Condition(std::string_view left_column, Op op, std::string_view right_column)
        : type_(Type::ColumnColumn), op_(op),
        column_(left_column) {
        data_ = ColumnColumnData{right_column, ""};
    }

    // Between constructor
    Condition(std::string_view column, Op op, SqlValue<Config> value1, SqlValue<Config> value2)
        : type_(Type::Between), op_(op), column_(column) {
        data_ = BetweenConditionData{value1, value2};
    }

    // IS NULL constructor
    static Condition isNull(std::string_view column) {
        Condition cond;
        cond.type_ = Type::IsNull;
        cond.column_ = column;
        return cond;
    }

    // IS NOT NULL constructor
    static Condition isNotNull(std::string_view column) {
        Condition cond;
        cond.type_ = Type::IsNotNull;
        cond.column_ = column;
        return cond;
    }

    // For IN conditions with a fixed-size array of values
    template<typename T>
    static Condition in(std::string_view column, std::span<const T> values) {
        Condition cond;
        cond.type_ = Type::In;
        cond.op_ = Op::In;
        cond.column_ = column;

        InConditionData inData;
        // Check bounds
        const size_t count = std::min(values.size(), static_cast<size_t>(Config::MaxInValues));

        // Convert each value to SqlValue
        for (size_t i = 0; i < count; ++i) {
            inData.values[i] = SqlValue<Config>(values[i]);
        }
        inData.count = count;
        cond.data_ = std::move(inData);

        return cond;
    }

    // For NOT IN conditions
    template<typename T>
    static Condition notIn(std::string_view column, std::span<const T> values) {
        auto cond = in(column, values);
        cond.op_ = Op::NotIn;
        return cond;
    }

    Condition(const Condition& other)
        : type_(other.type_), op_(other.op_), negated_(other.negated_),
        column_(other.column_), table_(other.table_), data_(other.data_) {
    }

    Condition(Condition&& other) noexcept = default;

    Condition& operator=(const Condition& other) {
        if (this != &other) {
            type_ = other.type_;
            op_ = other.op_;
            negated_ = other.negated_;
            column_ = other.column_;
            table_ = other.table_;
            data_ = other.data_;
        }
        return *this;
    }

    Condition& operator=(Condition&& other) noexcept = default;

    // Cross-config copy constructor
    template<typename OtherConfig>
    Condition(const Condition<OtherConfig>& other) {
        // For cross-config conversion, we'll use the string representation
        type_ = Type::Raw;
        data_ = RawData{other.toString()};
    }

    // Negation operator
    Condition operator!() const {
        Condition result = *this;
        result.negated_ = !negated_;
        return result;
    }
    // Compound AND operator
    Condition operator&&(const Condition& other) const {
        Condition result;
        result.type_ = Type::Compound;
        result.op_ = Op::And;

        // Convert conditions to strings first
        CompoundConditionData compound;
        compound.left_condition = this->toString();
        compound.right_condition = other.toString();
        compound.op = Op::And;
        result.data_ = std::move(compound);

        return result;
    }

    // Compound OR operator
    Condition operator||(const Condition& other) const {
        Condition result;
        result.type_ = Type::Compound;
        result.op_ = Op::Or;

        // Create compound condition using serialized strings
        CompoundConditionData compound;
        compound.left_condition = this->toString();
        compound.right_condition = other.toString();
        compound.op = Op::Or;
        result.data_ = std::move(compound);

        return result;
    }

    // Convert to string for SQL generation
    void toString(std::string& query) const {
        if (type_ == Type::Invalid) {
            query += "INVALID CONDITION";
            return;
        }

        if (negated_) {
            query += "NOT (";
        }

        switch (type_) {
        case Type::Raw:
            query += std::get<RawData>(data_).raw_sql;
            break;

        case Type::IsNull:
            query += column_;
            query += " IS NULL";
            break;

        case Type::IsNotNull:
            query += column_;
            query += " IS NOT NULL";
            break;

        case Type::Between: {
            const auto& betweenData = std::get<BetweenConditionData>(data_);
            query += column_;
            query += " BETWEEN ";
            query += betweenData.start.toSqlString();
            query += " AND ";
            query += betweenData.end.toSqlString();
            break;
        }

        case Type::SimpleValue: {
            const auto& simpleData = std::get<SimpleConditionData>(data_);
            query += column_;
            query += " ";
            query += this->opToString(op_);
            query += " ";
            query += simpleData.value.toSqlString();
            break;
        }

        case Type::ColumnColumn: {
            const auto& colData = std::get<ColumnColumnData>(data_);
            if (!table_.empty() && !colData.right_table.empty()) {
                // Fully qualified names for explicit JOINs
                query += table_;
                query += ".";
                query += column_;
                query += " ";
                query += this->opToString(op_);
                query += " ";
                query += colData.right_table;
                query += ".";
                query += colData.right_column;
            } else {
                // Just column names
                query += column_;
                query += " ";
                query += this->opToString(op_);
                query += " ";
                query += colData.right_column;
            }
            break;
        }

        case Type::Compound: {
            const auto& compoundData = std::get<CompoundConditionData>(data_);
            query += "(";
            query += compoundData.left_condition;
            query += ") ";
            query += this->opToString(compoundData.op);
            query += " (";
            query += compoundData.right_condition;
            query += ")";
            break;
        }

        case Type::In: {
            const auto& inData = std::get<InConditionData>(data_);
            query += column_;
            query += (op_ == Op::In ? " IN (" : " NOT IN (");
            for (size_t i = 0; i < inData.count; ++i) {
                if (i > 0) query += ", ";
                query += inData.values[i].toSqlString();
            }
            query += ")";
            break;
        }

        default:
            query += "UNKNOWN CONDITION TYPE";
            break;
        }

        if (negated_) {
            query += ")";
        }
    }

    std::string toString() const {
        std::string result;
        result.reserve(100); // Reserve a reasonable initial size
        toString(result);
        return result;
    }

    [[nodiscard]] bool isValid() const { return type_ != Type::Invalid; }
    [[nodiscard]] Type getType() const { return type_; }
};

// Expression template for delayed condition evaluation
template<typename Left, typename Right, typename Op, typename Config = DefaultConfig>
class ConditionExpression {
private:
    Left left_;
    Right right_;

public:
    ConditionExpression(Left left, Right right)
        : left_(std::move(left)), right_(std::move(right)) {}

    operator Condition<Config>() const {
        // Construct the actual condition only when needed
        if constexpr (std::is_same_v<Op, typename ConditionBase<Config>::Op>) {
            // Handle direct operations
            return Condition<Config>(left_, Op::value, right_);
        } else {
            // Handle compound conditions (AND/OR)
            Condition<Config> leftCond = left_;
            Condition<Config> rightCond = right_;
            if constexpr (std::is_same_v<Op, std::logical_and<>>) {
                return leftCond && rightCond;
            } else if constexpr (std::is_same_v<Op, std::logical_or<>>) {
                return leftCond || rightCond;
            }
        }
    }

    // Allow combining with other expressions
    template<typename OtherRight, typename OtherOp>
    auto operator&&(const ConditionExpression<Right, OtherRight, OtherOp, Config>& other) const {
        return ConditionExpression<ConditionExpression, ConditionExpression<Right, OtherRight, OtherOp, Config>, std::logical_and<>, Config>(*this, other);
    }

    template<typename OtherRight, typename OtherOp>
    auto operator||(const ConditionExpression<Right, OtherRight, OtherOp, Config>& other) const {
        return ConditionExpression<ConditionExpression, ConditionExpression<Right, OtherRight, OtherOp, Config>, std::logical_or<>, Config>(*this, other);
    }
};

// Column class with non-template base implementation
template<typename Config>
class Column {
    std::string_view name_;

public:
    explicit Column(std::string_view name) : name_(name) {}
    [[nodiscard]] std::string_view name() const { return name_; }

    [[nodiscard]] Condition<Config> isNull() const {
        return Condition<Config>::isNull(name_);
    }

    [[nodiscard]] Condition<Config> isNotNull() const {
        return Condition<Config>::isNotNull(name_);
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> eq(T&& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Eq, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> ne(T&& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Ne, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> lt(T&& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Lt, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> le(T&& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Le, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> gt(T&& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Gt, SqlValue<Config>(std::forward<T>(value)));
    }

    template<SqlCompatible T>
    [[nodiscard]] Condition<Config> ge(T&& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Ge, SqlValue<Config>(std::forward<T>(value)));
    }

    [[nodiscard]] Condition<Config> like(std::string_view pattern) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Like, SqlValue<Config>(pattern));
    }

    [[nodiscard]] Condition<Config> notLike(std::string_view pattern) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::NotLike, SqlValue<Config>(pattern));
    }

    template<SqlCompatible T, SqlCompatible U>
    [[nodiscard]] Condition<Config> between(T&& start, U&& end) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(
            name_,
            Op::Between,
            SqlValue<Config>(std::forward<T>(start)),
            SqlValue<Config>(std::forward<U>(end))
            );
    }

    template<typename U>
    [[nodiscard]] Condition<Config> in(std::span<const U> values) const {
        return Condition<Config>::in(name_, values);
    }

    template<typename U>
    [[nodiscard]] Condition<Config> notIn(std::span<const U> values) const {
        return Condition<Config>::notIn(name_, values);
    }

    [[nodiscard]] Condition<Config> eq(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Eq, value);
    }

    [[nodiscard]] Condition<Config> ne(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Ne, value);
    }

    [[nodiscard]] Condition<Config> lt(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Lt, value);
    }

    [[nodiscard]] Condition<Config> le(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Le, value);
    }

    [[nodiscard]] Condition<Config> gt(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Gt, value);
    }

    [[nodiscard]] Condition<Config> ge(const SqlValue<Config>& value) const {
        using Op = typename ConditionBase<Config>::Op;
        return Condition<Config>(name_, Op::Ge, value);
    }
};

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::isNull() const {
    return Condition<Config>::isNull(name_);
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::isNotNull() const {
    return Condition<Config>::isNotNull(name_);
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::eq(U&& value) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Eq, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::eq(const TypedColumn<OtherType, OtherConfig>& other) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(table_, name_, Op::Eq, other.tableName(), other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::ne(U&& value) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Ne, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::ne(const TypedColumn<OtherType, OtherConfig>& other) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(table_, name_, Op::Ne, other.tableName(), other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::lt(U&& value) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Lt, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::lt(const TypedColumn<OtherType, OtherConfig>& other) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(table_, name_, Op::Lt, other.tableName(), other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::le(U&& value) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Le, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::le(const TypedColumn<OtherType, OtherConfig>& other) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(table_, name_, Op::Le, other.tableName(), other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::gt(U&& value) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Gt, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::gt(const TypedColumn<OtherType, OtherConfig>& other) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(table_, name_, Op::Gt, other.tableName(), other.name());
}

template<typename T, typename Config>
template<SqlCompatible U>
Condition<Config> TypedColumn<T, Config>::ge(U&& value) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Ge, SqlValue<Config>(std::forward<U>(value)));
}

template<typename T, typename Config>
template<typename OtherType, typename OtherConfig>
Condition<Config> TypedColumn<T, Config>::ge(const TypedColumn<OtherType, OtherConfig>& other) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(table_, name_, Op::Ge, other.tableName(), other.name());
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::like(std::string_view pattern) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::Like, SqlValue<Config>(pattern));
}

template<typename T, typename Config>
Condition<Config> TypedColumn<T, Config>::notLike(std::string_view pattern) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(name_, Op::NotLike, SqlValue<Config>(pattern));
}

template<typename T, typename Config>
template<SqlCompatible T1, SqlCompatible T2>
Condition<Config> TypedColumn<T, Config>::between(T1&& start, T2&& end) const {
    using Op = typename ConditionBase<Config>::Op;
    return Condition<Config>(
        name_,
        Op::Between,
        SqlValue<Config>(std::forward<T1>(start)),
        SqlValue<Config>(std::forward<T2>(end))
        );
}

template<typename T, typename Config>
template<typename U>
Condition<Config> TypedColumn<T, Config>::in(std::span<const U> values) const {
    return Condition<Config>::in(name_, values);
}

template<typename T, typename Config>
template<typename U>
Condition<Config> TypedColumn<T, Config>::notIn(std::span<const U> values) const {
    return Condition<Config>::notIn(name_, values);
}

// Operator overloads for TypedColumn
template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator==(const TypedColumn<T, Config>& col, U&& val) {
    return col.eq(std::forward<U>(val));
}

template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator==(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.eq(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator!=(const TypedColumn<T, Config>& col, U&& val) {
    return col.ne(std::forward<U>(val));
}

template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator!=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.ne(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator<(const TypedColumn<T, Config>& col, U&& val) {
    return col.lt(std::forward<U>(val));
}

template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator<(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.lt(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator<=(const TypedColumn<T, Config>& col, U&& val) {
    return col.le(std::forward<U>(val));
}

template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator<=(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.le(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator>(const TypedColumn<T, Config>& col, U&& val) {
    return col.gt(std::forward<U>(val));
}

template<typename T, typename U, typename ConfigT, typename ConfigU>
inline Condition<ConfigT> operator>(const TypedColumn<T, ConfigT>& col1, const TypedColumn<U, ConfigU>& col2) {
    return col1.gt(col2);
}

template<typename T, typename Config, SqlCompatible U>
inline Condition<Config> operator>=(const TypedColumn<T, Config>& col, U&& val) {
    return col.ge(std::forward<U>(val));
}

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
// new
template<typename T, typename Config>
inline Condition<Config> operator==(const TypedColumn<T, Config>& col, const SqlValue<Config>& val) {
    return col.eq(val);
}

template<typename T, typename Config>
inline Condition<Config> operator!=(const TypedColumn<T, Config>& col, const SqlValue<Config>& val) {
    return col.ne(val);
}

template<typename T, typename Config>
inline Condition<Config> operator<(const TypedColumn<T, Config>& col, const SqlValue<Config>& val) {
    return col.lt(val);
}

template<typename T, typename Config>
inline Condition<Config> operator<=(const TypedColumn<T, Config>& col, const SqlValue<Config>& val) {
    return col.le(val);
}

template<typename T, typename Config>
inline Condition<Config> operator>(const TypedColumn<T, Config>& col, const SqlValue<Config>& val) {
    return col.gt(val);
}

template<typename T, typename Config>
inline Condition<Config> operator>=(const TypedColumn<T, Config>& col, const SqlValue<Config>& val) {
    return col.ge(val);
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

inline std::ostream& operator<<(std::ostream& os, const QString& str) {
    return os << str.toStdString();
}
#endif

// SQL function types
enum class SqlFunction {
    None,
    Count,
    Sum,
    Avg,
    Min,
    Max,
    GroupConcat
};

// Column reference
template<typename Config = DefaultConfig>
class ColumnRef {
private:
    std::string_view column_;
    SqlFunction function_ = SqlFunction::None;
    std::string_view alias_; // Optional column alias

public:
    ColumnRef() = default;

    explicit ColumnRef(std::string_view column, SqlFunction function = SqlFunction::None, std::string_view alias = "")
        : column_(column), function_(function), alias_(alias) {}

    std::string_view column() const { return column_; }
    SqlFunction function() const { return function_; }
    std::string_view alias() const { return alias_; }

    // Create an aliased column
    ColumnRef as(std::string_view alias) const {
        return ColumnRef(column_, function_, alias);
    }

    void toSql(std::string& query) const {
        switch (function_) {
        case SqlFunction::Count:
            query += "COUNT(";
            query += column_;
            query += ")";
            break;
        case SqlFunction::Sum:
            query += "SUM(";
            query += column_;
            query += ")";
            break;
        case SqlFunction::Avg:
            query += "AVG(";
            query += column_;
            query += ")";
            break;
        case SqlFunction::Min:
            query += "MIN(";
            query += column_;
            query += ")";
            break;
        case SqlFunction::Max:
            query += "MAX(";
            query += column_;
            query += ")";
            break;
        case SqlFunction::GroupConcat:
            query += "GROUP_CONCAT(";
            query += column_;
            query += ")";
            break;
        case SqlFunction::None:
        default:
            query += column_;
            break;
        }

        if (!alias_.empty()) {
            query += " AS ";
            query += alias_;
        }
    }
};

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

    void toString(std::string& query) const {
        const char* type_str = "";
        switch (type_) {
        case Type::Inner: type_str = "INNER JOIN"; break;
        case Type::Left:  type_str = "LEFT JOIN"; break;
        case Type::Right: type_str = "RIGHT JOIN"; break;
        case Type::Full:  type_str = "FULL JOIN"; break;
        case Type::Cross: type_str = "CROSS JOIN"; break;
        }

        query += type_str;
        query += " ";
        query += table_;
        query += " ON ";
        query += condition_;
    }

    [[nodiscard]] std::string toString() const {
        std::string result;
        result.reserve(table_.size() + condition_.size() + 20);
        toString(result);
        return result;
    }
};

// Placeholder class to represent a SQL parameter placeholder
template <typename Config>
class Placeholder {
private:
    std::string name_;
    enum class Style {
        QuestionMark,  // ?
        Dollar,        // $1, $2, etc.
        Colon,         // :name
        At             // @name
    };
    Style style_;

public:
    explicit Placeholder(std::string_view name = "")
        : name_(name) {
        if (name.empty()) {
            style_ = Style::QuestionMark;
        } else if (name[0] == ':') {
            style_ = Style::Colon;
        } else if (name[0] == '@') {
            style_ = Style::At;
        } else if (name[0] == '$') {
            style_ = Style::Dollar;
        } else {
            style_ = Style::Colon;
            name_ = ":" + std::string(name);
        }
    }

    [[nodiscard]] std::string toString() const {
        switch (style_) {
        case Style::QuestionMark:
            return "?";
        case Style::Dollar:
        case Style::Colon:
        case Style::At:
            return name_;
        default:
            return "?";
        }
    }

    [[nodiscard]] constexpr bool isPlaceholder() const { return true; }
};

// A class that holds a fluent where builder for complex conditions
template<typename Config>
class WhereBuilder {
private:
    using ConditionFn = std::function<void(WhereBuilder<Config>&)>;
    Condition<Config> condition_;

public:
    WhereBuilder() = default;

    WhereBuilder& condition(const Condition<Config>& cond) {
        condition_ = cond;
        return *this;
    }

    WhereBuilder& and_(const Condition<Config>& cond) {
        if (condition_.isValid()) {
            condition_ = condition_ && cond;
        } else {
            condition_ = cond;
        }
        return *this;
    }

    WhereBuilder& or_(const Condition<Config>& cond) {
        if (condition_.isValid()) {
            condition_ = condition_ || cond;
        } else {
            condition_ = cond;
        }
        return *this;
    }

    WhereBuilder& and_(ConditionFn builder) {
        WhereBuilder<Config> subBuilder;
        builder(subBuilder);
        return and_(subBuilder.build());
    }

    WhereBuilder& or_(ConditionFn builder) {
        WhereBuilder<Config> subBuilder;
        builder(subBuilder);
        return or_(subBuilder.build());
    }

    Condition<Config> build() const {
        return condition_;
    }

    operator Condition<Config>() const {
        return build();
    }
};


template<typename Config = DefaultConfig>
class QueryBuilder {
public:
    enum class QueryType : uint8_t { Select, Insert, InsertOrReplace, Update, Delete, Truncate };

private:
    // Core frequently accessed fields
    struct {
        QueryType type{QueryType::Select};
        std::string_view table;
        bool distinct{false};
    } core_;

    // Columns and values (high-frequency access)
    struct {
        std::array<ColumnRef<Config>, Config::MaxColumns> select_columns{};
        size_t select_columns_count{0};
        std::array<std::pair<std::string_view, SqlValue<Config>>, Config::MaxColumns> values{};
        size_t values_count{0};
    } columns_;

    // Conditions and joins (medium-frequency access)
    struct {
        std::array<Condition<Config>, Config::MaxConditions> where_conditions{};
        size_t where_conditions_count{0};
        std::array<Join<Config>, Config::MaxJoins> joins{};
        size_t joins_count{0};
    } filters_;

    // Ordering, grouping, and limits (low-frequency access)
    struct {
        std::array<std::pair<std::string_view, bool>, Config::MaxOrderBy> order_by{};
        size_t order_by_count{0};
        std::array<std::string_view, Config::MaxGroupBy> group_by{};
        size_t group_by_count{0};
        std::string_view having;
        int32_t limit{-1};
        int32_t offset{-1};
    } ordering_;

    // Error handling
    mutable std::optional<QueryError> last_error_;

    [[nodiscard]] size_t estimateSize() const {
        size_t size = 64; // Base size
        size += core_.table.size();
        size += columns_.select_columns_count * 20; // Average column name size + potential function/alias
        size += filters_.where_conditions_count * 50; // Average condition size
        size += filters_.joins_count * 60; // Average join size
        size += ordering_.order_by_count * 25; // Column name + ASC/DESC
        size += ordering_.group_by_count * 15; // Column name
        size += ordering_.having.size();
        if (ordering_.limit >= 0) size += 15;
        if (ordering_.offset >= 0) size += 15;
        return size;
    }

public:
    QueryBuilder() = default;

    QueryBuilder& reset() {
        core_.type = QueryType::Select;
        core_.table = "";
        core_.distinct = false;

        columns_.select_columns_count = 0;
        columns_.values_count = 0;

        filters_.where_conditions_count = 0;
        filters_.joins_count = 0;

        ordering_.order_by_count = 0;
        ordering_.group_by_count = 0;
        ordering_.having = "";
        ordering_.limit = -1;
        ordering_.offset = -1;

        last_error_ = std::nullopt;
        return *this;
    }

    [[nodiscard]] std::optional<QueryError> lastError() const {
        return last_error_;
    }

    template<typename... Cols>
    QueryBuilder& select(Cols&&... cols) {
        static_assert((QueryType::Select == QueryType::Select), "SELECT can only be used with SELECT queries");
        core_.type = QueryType::Select;
        const size_t additional = sizeof...(cols);

        if (columns_.select_columns_count + additional > Config::MaxColumns) {
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

    template<typename T, size_t Extent>
    QueryBuilder& select(std::span<const T, Extent> cols) {
        static_assert((QueryType::Select == QueryType::Select), "SELECT can only be used with SELECT queries");
        core_.type = QueryType::Select;

        if (columns_.select_columns_count + cols.size() > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        for (const auto& col : cols) {
            if constexpr (std::is_convertible_v<T, std::string_view>) {
                columns_.select_columns[columns_.select_columns_count++] = ColumnRef<Config>(static_cast<std::string_view>(col));
            }
        }
        return *this;
    }

    template<typename T>
    void addSelectColumn(const T& col) {
        if constexpr (std::is_same_v<std::remove_cvref_t<T>, ColumnRef<Config>>) {
            columns_.select_columns[columns_.select_columns_count++] = col;
        } else if constexpr (std::is_convertible_v<T, std::string_view>) {
            columns_.select_columns[columns_.select_columns_count++] = ColumnRef<Config>(static_cast<std::string_view>(col));
        } else {
            static_assert(std::is_convertible_v<T, std::string_view> ||
                              std::is_same_v<std::remove_cvref_t<T>, ColumnRef<Config>>,
                          "Column type not supported for select");
        }
    }

    // Select from span
    QueryBuilder& select(std::span<const std::string_view> cols) {
        static_assert((QueryType::Select == QueryType::Select), "SELECT can only be used with SELECT queries");
        core_.type = QueryType::Select;

        if (columns_.select_columns_count + cols.size() > Config::MaxColumns) {
            auto error = QueryError(QueryError::Code::TooManyColumns,
                                    std::format("Too many columns: limit is {}", Config::MaxColumns));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        for (const auto& col : cols) {
            columns_.select_columns[columns_.select_columns_count++] = ColumnRef<Config>(col);
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
            core_.table = static_cast<std::string_view>(table);
        } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>) {
            core_.table = table.toString();
        } else {
            static_assert(std::is_convertible_v<T, std::string_view> ||
                              std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>,
                          "Table type not supported for from");
        }
        return *this;
    }

    template<typename Fn>
    QueryBuilder& where(Fn builderFn) {
        WhereBuilder<Config> builder;
        builderFn(builder);
        return where(builder.build());
    }

    template<typename OtherConfig>
    QueryBuilder& where(const Condition<OtherConfig>& condition) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        // Convert condition from OtherConfig to our Config
        filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>(condition);
        return *this;
    }

    template<SqlCompatible T>
    QueryBuilder& whereOp(std::string_view column, typename ConditionBase<Config>::Op op, T&& value) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>(
            column, op, SqlValue<Config>(std::forward<T>(value))
            );
        return *this;
    }

    // OrderBy with bounds checking and compile-time validation
    template<typename T>
    QueryBuilder& orderBy(const T& column, bool asc = true) {
        static_assert((QueryType::Select == QueryType::Select), "ORDER BY can only be used with SELECT queries");

        if (ordering_.order_by_count >= Config::MaxOrderBy) {
            auto error = QueryError(QueryError::Code::TooManyOrderBy,
                                    std::format("Too many order by clauses: limit is {}", Config::MaxOrderBy));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            ordering_.order_by[ordering_.order_by_count++] = {static_cast<std::string_view>(column), asc};
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Column type not supported for orderBy");
        }
        return *this;
    }

    QueryBuilder& limit(int32_t limit) {
        static_assert((QueryType::Select == QueryType::Select), "LIMIT can only be used with SELECT queries");
        ordering_.limit = limit;
        return *this;
    }

    QueryBuilder& offset(int32_t offset) {
        static_assert((QueryType::Select == QueryType::Select), "OFFSET can only be used with SELECT queries");
        ordering_.offset = offset;
        return *this;
    }

    QueryBuilder& distinct() {
        static_assert((QueryType::Select == QueryType::Select), "DISTINCT can only be used with SELECT queries");
        core_.distinct = true;
        return *this;
    }

    // Insert with bounds checking
    template<typename T>
    QueryBuilder& insert(const T& table) {
        core_.type = QueryType::Insert;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            core_.table = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for insert");
        }
        return *this;
    }

    // Insert OR REPLACE with bounds checking
    template<typename T>
    QueryBuilder& insertOrReplace(const T& table) {
        core_.type = QueryType::InsertOrReplace;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            core_.table = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for insertOrReplace");
        }
        return *this;
    }

    template<typename Col, SqlCompatible T>
    QueryBuilder& value(const Col& column, T&& val) {
        if (columns_.values_count >= Config::MaxColumns) {
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
            columns_.values[columns_.values_count].first = static_cast<std::string_view>(column);
            columns_.values[columns_.values_count].second = SqlValue<Config>(std::forward<T>(val));
            columns_.values_count++;
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for value");
        }
        return *this;
    }

    // Update methods
    template<typename T>
    QueryBuilder& update(const T& table) {
        core_.type = QueryType::Update;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            core_.table = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for update");
        }
        return *this;
    }

    template<typename Col, SqlCompatible T>
    QueryBuilder& set(const Col& column, T&& val) {
        if (columns_.values_count >= Config::MaxColumns) {
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
            columns_.values[columns_.values_count].first = static_cast<std::string_view>(column);
            columns_.values[columns_.values_count].second = SqlValue<Config>(std::forward<T>(val));
            columns_.values_count++;
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for set");
        }
        return *this;
    }

    // Delete methods
    template<typename T>
    QueryBuilder& deleteFrom(const T& table) {
        core_.type = QueryType::Delete;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            core_.table = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for deleteFrom");
        }
        return *this;
    }

    // Truncate methods
    template<typename T>
    QueryBuilder& truncate(const T& table) {
        core_.type = QueryType::Truncate;
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            core_.table = static_cast<std::string_view>(table);
        } else {
            static_assert(std::is_convertible_v<T, std::string_view>,
                          "Table type not supported for truncate");
        }
        return *this;
    }

    // Join methods with bounds checking
    template<typename T>
    QueryBuilder& innerJoin(const T& table, std::string_view condition) {
        static_assert((QueryType::Select == QueryType::Select), "JOIN can only be used with SELECT queries");
        if (filters_.joins_count >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Inner,
                static_cast<std::string_view>(table),
                condition
            };
        } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Inner,
                table.toString(),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view> ||
                              std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>,
                          "Table type not supported for innerJoin");
        }
        return *this;
    }

    template<typename T, typename L, typename R>
    QueryBuilder& innerJoin(const T& table, const Condition<Config>& condition) {
        return innerJoin(table, condition.toString());
    }

    template<typename T>
    QueryBuilder& leftJoin(const T& table, std::string_view condition) {
        static_assert((QueryType::Select == QueryType::Select), "JOIN can only be used with SELECT queries");
        if (filters_.joins_count >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Left,
                static_cast<std::string_view>(table),
                condition
            };
        } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Left,
                table.toString(),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view> ||
                              std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>,
                          "Table type not supported for leftJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& leftJoin(const T& table, const Condition<Config>& condition) {
        return leftJoin(table, condition.toString());
    }

    template<typename T>
    QueryBuilder& rightJoin(const T& table, std::string_view condition) {
        static_assert((QueryType::Select == QueryType::Select), "JOIN can only be used with SELECT queries");
        if (filters_.joins_count >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Right,
                static_cast<std::string_view>(table),
                condition
            };
        } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Right,
                table.toString(),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view> ||
                              std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>,
                          "Table type not supported for rightJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& rightJoin(const T& table, const Condition<Config>& condition) {
        return rightJoin(table, condition.toString());
    }

    template<typename T>
    QueryBuilder& fullJoin(const T& table, std::string_view condition) {
        static_assert((QueryType::Select == QueryType::Select), "JOIN can only be used with SELECT queries");
        if (filters_.joins_count >= Config::MaxJoins) {
            auto error = QueryError(QueryError::Code::TooManyJoins,
                                    std::format("Too many joins: limit is {}", Config::MaxJoins));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<T, std::string_view>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Full,
                static_cast<std::string_view>(table),
                condition
            };
        } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>) {
            filters_.joins[filters_.joins_count++] = Join<Config>{
                Join<Config>::Type::Full,
                table.toString(),
                condition
            };
        } else {
            static_assert(std::is_convertible_v<T, std::string_view> ||
                              std::is_same_v<std::remove_cvref_t<T>, AliasedTable<Config>>,
                          "Table type not supported for fullJoin");
        }
        return *this;
    }

    template<typename T>
    QueryBuilder& fullJoin(const T& table, const Condition<Config>& condition) {
        return fullJoin(table, condition.toString());
    }

    // Where variations
    template<typename Col, typename T>
    QueryBuilder& whereIn(const Col& column, std::span<const T> values) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            auto condition = Condition<Config>::in(static_cast<std::string_view>(column), values);
            filters_.where_conditions[filters_.where_conditions_count++] = std::move(condition);
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereIn");
        }
        return *this;
    }

    template<typename Col, typename T>
    QueryBuilder& whereNotIn(const Col& column, std::span<const T> values) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            auto condition = Condition<Config>::notIn(static_cast<std::string_view>(column), values);
            filters_.where_conditions[filters_.where_conditions_count++] = std::move(condition);
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNotIn");
        }
        return *this;
    }

    template<typename Col, typename T, typename U>
    QueryBuilder& whereBetween(const Col& column, T&& start, U&& end) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            using Op = typename ConditionBase<Config>::Op;
            filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Op::Between,
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
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            using Op = typename ConditionBase<Config>::Op;
            filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>(
                static_cast<std::string_view>(column),
                Op::Like,
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
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>::isNull(
                static_cast<std::string_view>(column)
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNull");
        }
        return *this;
    }

    template<typename Col>
    QueryBuilder& whereNotNull(const Col& column) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>::isNotNull(
                static_cast<std::string_view>(column)
                );
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for whereNotNull");
        }
        return *this;
    }

    QueryBuilder& whereExists(std::string_view subquery) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        std::string condition = "EXISTS (" + std::string(subquery) + ")";
        filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>(condition);
        return *this;
    }

    QueryBuilder& whereRaw(std::string_view rawCondition) {
        if (filters_.where_conditions_count >= Config::MaxConditions) {
            auto error = QueryError(QueryError::Code::TooManyConditions,
                                    std::format("Too many conditions: limit is {}", Config::MaxConditions));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        filters_.where_conditions[filters_.where_conditions_count++] = Condition<Config>(rawCondition);
        return *this;
    }

    template<typename Col>
    QueryBuilder& groupBy(const Col& column) {
        static_assert((QueryType::Select == QueryType::Select), "GROUP BY can only be used with SELECT queries");
        if (ordering_.group_by_count >= Config::MaxGroupBy) {
            auto error = QueryError(QueryError::Code::TooManyGroupBy,
                                    std::format("Too many group by clauses: limit is {}", Config::MaxGroupBy));
            last_error_ = error;
            if constexpr(Config::ThrowOnError) {
                throw error;
            }
            return *this;
        }

        if constexpr (std::is_convertible_v<Col, std::string_view>) {
            ordering_.group_by[ordering_.group_by_count++] = static_cast<std::string_view>(column);
        } else {
            static_assert(std::is_convertible_v<Col, std::string_view>,
                          "Column type not supported for groupBy");
        }
        return *this;
    }

    QueryBuilder& having(std::string_view condition) {
        static_assert((QueryType::Select == QueryType::Select), "HAVING can only be used with SELECT queries");
        ordering_.having = condition;
        return *this;
    }

    // Build with error handling
    [[nodiscard]] Result<std::string> buildResult() const {
        if (core_.table.empty() && core_.type != QueryType::Select) {
            QueryError error(QueryError::Code::EmptyTable, "Table name is required");
            last_error_ = error;
            return error;
        }

        try {
            std::string query;
            query.reserve(estimateSize());

            switch (core_.type) {
            case QueryType::Select:
                buildSelect(query);
                break;
            case QueryType::Insert:
                buildInsert(query, false);
                break;
            case QueryType::InsertOrReplace:
                buildInsert(query, true);
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

#ifdef SQLQUERYBUILDER_USE_QT
    [[nodiscard]] QString build() const {
        auto result = buildResult();
        if (result.hasError()) {
            if constexpr(Config::ThrowOnError) {
                throw result.error();
            }
            return QString("/* ERROR: %1 */").arg(QString::fromUtf8(result.error().message.data(),
                                                                    static_cast<int>(result.error().message.size())));
        }
        return QString::fromStdString(result.value());
    }
#else
    [[nodiscard]] std::string build() const {
        auto result = buildResult();
        if (result.hasError()) {
            if constexpr(Config::ThrowOnError) {
                throw result.error();
            }
            return "/* ERROR: " + std::string(result.error().message) + " */";
        }
        return result.value();
    }
#endif

private:
    void buildSelect(std::string& query) const {
        query += keywords::SELECT;
        query += " ";

        if (core_.distinct) {
            query += keywords::DISTINCT;
            query += " ";
        }

        if (columns_.select_columns_count == 0) {
            query += keywords::ALL;
        } else {
            bool first = true;
            for (size_t i = 0; i < columns_.select_columns_count; ++i) {
                if (!first) query += ", ";
                columns_.select_columns[i].toSql(query);
                first = false;
            }
        }

        query += " ";
        query += keywords::FROM;
        query += " ";
        query += core_.table;

        // Joins
        for (size_t i = 0; i < filters_.joins_count; ++i) {
            query += " ";
            filters_.joins[i].toString(query);
        }

        // Where
        if (filters_.where_conditions_count > 0) {
            query += " ";
            query += keywords::WHERE;
            query += " ";
            bool first = true;
            for (size_t i = 0; i < filters_.where_conditions_count; ++i) {
                if (!first) {
                    query += " ";
                    query += keywords::AND;
                    query += " ";
                }
                filters_.where_conditions[i].toString(query);
                first = false;
            }
        }

        // Group By
        if (ordering_.group_by_count > 0) {
            query += " ";
            query += keywords::GROUP_BY;
            query += " ";
            bool first = true;
            for (size_t i = 0; i < ordering_.group_by_count; ++i) {
                if (!first) query += ", ";
                query += ordering_.group_by[i];
                first = false;
            }

            if (!ordering_.having.empty()) {
                query += " ";
                query += keywords::HAVING;
                query += " ";
                query += ordering_.having;
            }
        }

        // Order By
        if (ordering_.order_by_count > 0) {
            query += " ";
            query += keywords::ORDER_BY;
            query += " ";
            bool first = true;
            for (size_t i = 0; i < ordering_.order_by_count; ++i) {
                if (!first) query += ", ";
                query += ordering_.order_by[i].first;
                query += " ";
                query += (ordering_.order_by[i].second ? keywords::ASC : keywords::DESC);
                first = false;
            }
        }

        if (ordering_.limit >= 0) {
            query += " ";
            query += keywords::LIMIT;
            query += " ";
            query += std::to_string(ordering_.limit);
        }

        if (ordering_.offset >= 0) {
            query += " ";
            query += keywords::OFFSET;
            query += " ";
            query += std::to_string(ordering_.offset);
        }
    }

    void buildInsert(std::string& query, bool orReplace) const {
        if (columns_.values_count == 0) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for INSERT");
        }

        query += keywords::INSERT;

        if (orReplace) {
            query += " OR REPLACE";
        }

        query += " ";
        query += keywords::INTO;
        query += " ";
        query += core_.table;
        query += " (";

        bool first = true;
        for (size_t i = 0; i < columns_.values_count; ++i) {
            if (!first) query += ", ";
            query += columns_.values[i].first;
            first = false;
        }

        query += ") ";
        query += keywords::VALUES;
        query += " (";

        first = true;
        for (size_t i = 0; i < columns_.values_count; ++i) {
            if (!first) query += ", ";
            query += columns_.values[i].second.toSqlString();
            first = false;
        }

        query += ")";
    }

    void buildUpdate(std::string& query) const {
        if (columns_.values_count == 0) {
            throw QueryError(QueryError::Code::InvalidCondition, "No values specified for UPDATE");
        }

        query += keywords::UPDATE;
        query += " ";
        query += core_.table;
        query += " ";
        query += keywords::SET;
        query += " ";

        bool first = true;
        for (size_t i = 0; i < columns_.values_count; ++i) {
            if (!first) query += ", ";
            query += columns_.values[i].first;
            query += " = ";
            query += columns_.values[i].second.toSqlString();
            first = false;
        }

        if (filters_.where_conditions_count > 0) {
            query += " ";
            query += keywords::WHERE;
            query += " ";
            first = true;
            for (size_t i = 0; i < filters_.where_conditions_count; ++i) {
                if (!first) {
                    query += " ";
                    query += keywords::AND;
                    query += " ";
                }
                filters_.where_conditions[i].toString(query);
                first = false;
            }
        }
    }

    void buildDelete(std::string& query) const {
        query += keywords::DELETE;
        query += " ";
        query += keywords::FROM;
        query += " ";
        query += core_.table;

        if (filters_.where_conditions_count > 0) {
            query += " ";
            query += keywords::WHERE;
            query += " ";
            bool first = true;
            for (size_t i = 0; i < filters_.where_conditions_count; ++i) {
                if (!first) {
                    query += " ";
                    query += keywords::AND;
                    query += " ";
                }
                filters_.where_conditions[i].toString(query);
                first = false;
            }
        }
    }

    void buildTruncate(std::string& query) const {
        query += keywords::TRUNCATE;
        query += " ";
        query += core_.table;
    }
};

//=====================
// SQL Aggregate Functions
//=====================

inline ColumnRef<> count(std::string_view column) {
    return ColumnRef<>(column, SqlFunction::Count);
}

inline ColumnRef<> sum(std::string_view column) {
    return ColumnRef<>(column, SqlFunction::Sum);
}

inline ColumnRef<> avg(std::string_view column) {
    return ColumnRef<>(column, SqlFunction::Avg);
}

inline ColumnRef<> min(std::string_view column) {
    return ColumnRef<>(column, SqlFunction::Min);
}

inline ColumnRef<> max(std::string_view column) {
    return ColumnRef<>(column, SqlFunction::Max);
}

inline ColumnRef<> groupConcat(std::string_view column) {
    return ColumnRef<>(column, SqlFunction::GroupConcat);
}

// Function to create an alias
inline std::string as(std::string_view expr, std::string_view alias) {
    return std::string(expr) + " AS " + std::string(alias);
}

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

template<SqlCompatible T, typename Config = DefaultConfig>
inline SqlValue<Config> val(T&& value) {
    return SqlValue<Config>(std::forward<T>(value));
}

// All columns helper
template<typename Config = DefaultConfig>
inline std::string_view all_of(const Table<Config>&) {
    return keywords::ALL;
}

// Raw condition helper
template<typename Config = DefaultConfig>
inline Condition<Config> raw(std::string_view condition) {
    return Condition<Config>(condition);
}

// Macros for easy table definition
#define SQL_DEFINE_TABLE(name) \
struct name##_table { \
        const sql::Table<> table = sql::table(#name);

#define SQL_DEFINE_COLUMN(name, type) \
    const sql::TypedColumn<type> name = sql::column<type>(table, #name);

#define SQL_END_TABLE() };

} // namespace v1
} // namespace sql
